/*
 * Eliot_MQTT_Task.c
 *
 *  Created on: Feb 13, 2019
 *      Author: Legrand
 */

#include "Eliot_MQTT_Task.h"
#include "Eliot_REST_Task.h"

#include "Eliot_Common.h"
#include "Mqtt.h"
#include "jansson_private.h"

#include "../MQX/rtcs/source/include/rtcspcb.h"
#include "../MQX/rtcs/source/include/tcp.h"
#include "../MQX/rtcs/source/include/rtcs_prv.h"


#include <stdlib.h>

#define ELIOT_MQTT_SERVICE              "Eliot MQTT"
#define ELIOT_INITIAL_TASK_DELAY_MS     2000



char eliot_device_name[sizeof(ELIOT_HOST"/devices/")+ELIOT_DEVICE_ID_SIZE];
char eliot_username[sizeof(ELIOT_HOST"/")+ELIOT_DEVICE_ID_SIZE];
char eliot_msgs_topic_name[sizeof("devices//messages/devicebound/#")+ELIOT_DEVICE_ID_SIZE];
char eliot_event_topic[sizeof("devices//messages/events/")+ELIOT_DEVICE_ID_SIZE];

uint8_t eliot_firmware_update_state_last = UPDATE_STATE_NONE;
bool_t eliot_firmware_version_trigger = false;
uint32_t eliot_firmware_version_sent_last = 0;
uint32_t eliot_keepalive_last = 0;
uint32_t eliot_mqtt_loop_counter = 0;
uint32_t eliot_event_last_checksum = 0;
uint32_t eliot_mqtt_event_last = 0;          // Timestamp when a non-repeated event was received
uint32_t eliot_mqtt_tcp_rx_count_last = 0;   // The last observed value of tcb->rx_count
uint32_t eliot_mqtt_tcp_rx_time_last = 0;    // Timestamp when tcb->rx_count last changed


zone_properties last_metrics_zone;
bool_t last_metrics_scene_running = false;
SOCKET_STRUCT *mqtt_socket = 0;

extern char * UpdateStateStrings[4];    // declared in Socket_Task.c

// Returns true if the MQTT TCP connection is active.
bool_t Eliot_MqttIsConnected()
{
	SOCKET_STRUCT* socket = mqtt_socket;
	TCB_STRUCT* tcb = 0;	// Transmission control block

	if (!cloudState)
	{
		return false;
	}

	if(socket)
	{
		if(socket->VALID == SOCKET_VALID)
		{
			tcb = (TCB_STRUCT*)socket->TCB_PTR;
			if(tcb)
			{
				if(tcb->VALID == TCB_VALID_ID)
				{
					if(tcb->rx_count != eliot_mqtt_tcp_rx_count_last)
					{
						eliot_mqtt_tcp_rx_time_last = Eliot_TimeNow();
						eliot_mqtt_tcp_rx_count_last = tcb->rx_count;
					}
				}
				else
				{
					// Transmission control block is invalid
					return false;
				}
			}
			else
			{
				// Transmission control block is null
				return false;
			}
		}
		else
		{
			// Socket struct is invalid
			return false;
		}
	}
	else
	{
		// Socket struct is null
		return false;
	}

	return (Eliot_TimeNow() < (eliot_mqtt_tcp_rx_time_last + ELIOT_MQTT_RECONNECT_SEC));
}


// Prints the AES keys used by TLS 1.2 to encrypt/decrypt MQTT traffic.
void Eliot_MqttPrintAESKeys(MQTTCtx* ctx)
{
	// To decrypt TLS traffic into the raw MQTT header and message:
	// 1. In Wireshark, find "Encrypted Application Data" in the packet under "Transport Layer Security".
	// 2. Right click, "Copy" -> "...as a Hex Stream".
	// 3. Paste into an AES decryption tool such as https://cryptii.com/pipes/aes-encryption.
	// 4. Choose CBC / Cipher block chaining.
	// 5. Provide the _WRITE_KEY and _WRITE_IV as printed below, depending on traffic direction.
	// 6. Decrypt the data and discard the first 16 bytes of the result (TLS explicit IV value).
	//      The first byte of the remainder will be the MQTT message type and flags.
	//      The message body can be viewed with a hex stream to ASCII converter such as:
	//      https://www.rapidtables.com/convert/number/hex-to-ascii.html

	int x=0;
	char output[2*AES_256_KEY_SIZE*+1] = {0};

	// If DBGPRINT_INTSTRSTR() is disabled, bail out early.
	DBGPRINT_INTSTRSTR(__LINE__, "Eliot_MqttPrintAESKeys() - Printing keys...", ""); else return;

	for(x=0; x<AES_256_KEY_SIZE; x++)
	{
		sprintf(output + x*2, "%02X", ctx->client.tls.ssl->keys.server_write_key[x]);
	}
	DBGPRINT_INTSTRSTR(__LINE__, "server_write_key", output);

	for(x=0; x<AES_IV_SIZE; x++)
	{
		sprintf(output + x*2, "%02X", ctx->client.tls.ssl->keys.server_write_IV[x]);
	}
	DBGPRINT_INTSTRSTR(__LINE__, "server_write_IV", output);

	for(x=0; x<AES_256_KEY_SIZE; x++)
	{
		sprintf(output + x*2, "%02X", ctx->client.tls.ssl->keys.client_write_key[x]);
	}
	DBGPRINT_INTSTRSTR(__LINE__, "client_write_key", output);

	for(x=0; x<AES_IV_SIZE; x++)
	{
		sprintf(output + x*2, "%02X", ctx->client.tls.ssl->keys.client_write_IV[x]);
	}
	DBGPRINT_INTSTRSTR(__LINE__, "client_write_IV", output);
}


int32_t Eliot_BuildMqttStrings()
{
	if(!eliot_lcm_module_id[0])
	{
		eliot_device_name[0] = 0;
		eliot_username[0] = 0;
		eliot_msgs_topic_name[0] = 0;
		eliot_event_topic[0] = 0;
	}
	else
	{
		snprintf(eliot_device_name, sizeof(eliot_device_name), ELIOT_HOST"/devices/%s", eliot_lcm_module_id);
		snprintf(eliot_username, sizeof(eliot_username), ELIOT_HOST"/%s", eliot_lcm_module_id);
		snprintf(eliot_msgs_topic_name, sizeof(eliot_msgs_topic_name), "devices/%s/messages/devicebound/#", eliot_lcm_module_id);
		snprintf(eliot_event_topic, sizeof(eliot_event_topic), "devices/%s/messages/events/", eliot_lcm_module_id);
	}
	
	DBGPRINT_INTSTRSTR(sizeof(eliot_device_name), "Eliot_BuildMqttStrings() eliot_device_name:", eliot_device_name);
	DBGPRINT_INTSTRSTR(sizeof(eliot_username), "Eliot_BuildMqttStrings() eliot_username:", eliot_username);
	DBGPRINT_INTSTRSTR(sizeof(eliot_msgs_topic_name), "Eliot_BuildMqttStrings() eliot_msgs_topic_name", eliot_msgs_topic_name);
	DBGPRINT_INTSTRSTR(sizeof(eliot_event_topic), "Eliot_BuildMqttStrings() eliot_event_topic", eliot_event_topic);
	
	return ELIOT_RET_OK;
}


static int Eliot_Connect_CB(void *context, const char* host, word16 port, int timeout_ms)
{
    SocketContext *sock = (SocketContext*)context;
    int type = SOCK_STREAM;
    int rc = -1;
    int so_error = 0;
    struct sockaddr_in result;
    MQTTCtx* mqttCtx = sock->mqttCtx;

    _ip_address hostaddr = 0;
    char ipName[64];
    RTCS_resolve_ip_address(host, &hostaddr, ipName, 64);
    if (hostaddr == 0)
    {
        return MQTT_CODE_ERROR_NETWORK;
    }

    sock->addr.sin_family = PF_INET;
    sock->fd = socket(sock->addr.sin_family, type, 0);
    mqtt_socket = sock->fd;

    if (sock->fd == RTCS_SOCKET_ERROR)
        return MQTT_CODE_ERROR_NETWORK;

    sock->stat = SOCK_CONN;

    memset((char *)&result, 0, sizeof(struct sockaddr_in));
    result.sin_family = PF_INET;
    result.sin_addr.s_addr = hostaddr;
    result.sin_port = port;

    uint32_t optValue = true;
    setsockopt(sock->fd, SOL_TCP, OPT_NOWAIT, &optValue, sizeof(optValue));
    rc = connect(sock->fd, (struct sockaddr*)&result, sizeof(struct sockaddr_in));

    if (rc != RTCS_OK)
    {
        return MQTT_CODE_ERROR_NETWORK;
    }

    // setsockopt OPT_KEEPALIVE granularity is one minute.
    // Directly setting the TCB struct allows for finer control:
    if(mqtt_socket)
    {
        if(mqtt_socket->TCB_PTR)
        {
            if(mqtt_socket->TCB_PTR->VALID == TCB_VALID_ID)
            {
                mqtt_socket->TCB_PTR->keepaliveto = ELIOT_MQTT_TCP_KEEPALIVE_MS;
            }
        }
    }

    return MQTT_CODE_SUCCESS;
}

static int Eliot_Write_CB(void *context, const byte* buf, int buf_len, int timeout_ms)
{
    SocketContext *sock = (SocketContext*)context;
    int rc;
    int so_error = 0;

    if (context == NULL || buf == NULL || buf_len <= 0)
    {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    rc = (int)send(sock->fd, buf, buf_len, 0);
    if (rc == RTCS_ERROR)
    {
        rc = MQTT_CODE_ERROR_NETWORK;
    }

    (void)timeout_ms;
    Eliot_LedIndicateActivity();
    return rc;
}

static int Eliot_Read_CB(void *context, byte* buf, int buf_len, int timeout_ms)
{
    byte peek = 0;
    SocketContext *sock = (SocketContext*)context;
    int rc = -1;
    int so_error = 0;
    int bytes = 0;
    int flags = 0;

    uint32_t sockets[1];
    sockets[0] = sock->fd;

    if(buf_len > MAX_BUFFER_SIZE)
    {
        DBGPRINT_ERRORS_INTSTRSTR(buf_len, "Eliot_Read_CB()","WARNING: Caught buffer overrun.  Attempted write size:");
        buf_len = MAX_BUFFER_SIZE>>1;
    }

    if (context == NULL || buf == NULL || buf_len <= 0)
    {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    if (peek == 1)
    {
        flags |= RTCS_MSG_PEEK;
    }

    while (bytes < buf_len)
    {
        rc = RTCS_selectset(sockets, 1, -1);
        if (rc != RTCS_SOCKET_ERROR)
        {
            // This will be interpreted as a MQTT_CODE_CONTINUE
            // by consumers (Subscribe, and WaitMessage)
            if (rc == 0) return 0;

            rc = (int)recv(sock->fd,
                           &buf[bytes],
                           buf_len - bytes,
                           flags);
            if (rc <= 0)
            {
                return MQTT_CODE_ERROR_NETWORK;
            }
            else
            {
                bytes += rc;
            }
        }
        else
        {
            return MQTT_CODE_ERROR_TIMEOUT;
        }
    }
    Eliot_LedIndicateActivity();
    return bytes;
}



// Sends an MQTT packet
int32_t Eliot_SendMqttPacket(MQTTCtx* mqttCtx, char *data, uint32_t length, char* route)
{
	mqttCtx->stat = WMQ_PUB;
	memset(&mqttCtx->publish, 0, sizeof(MqttPublish));
	mqttCtx->publish.retain = 0;
	mqttCtx->publish.qos = mqttCtx->qos;
	mqttCtx->publish.duplicate = 0;
	mqttCtx->publish.packet_id = Mqtt_Get_PacketID();
	mqttCtx->publish.buffer = (byte*) data;
	mqttCtx->publish.total_len = (word16) length;

	if(route)
	{
		char route_topic[sizeof(eliot_event_topic)+ELIOT_MQTT_ROUTE_MAX_LENGTH];
		snprintf(route_topic, sizeof(route_topic), "%s%s", eliot_event_topic, route);
		mqttCtx->publish.topic_name = route_topic;
		DBGPRINT_INTSTRSTR(strlen(route_topic), "Eliot_SendMqttPacket() using route", route_topic);
		return MqttClient_Publish(&mqttCtx->client, &mqttCtx->publish);
	}
	else
	{
		mqttCtx->publish.topic_name = eliot_event_topic;
		return MqttClient_Publish(&mqttCtx->client, &mqttCtx->publish);
	}
}


// This function is used to break out of the Eliot_GetEvent() loop if the TCP
// connection has dropped.
int32_t Eliot_MqttHeartbeatMonitor(MQTTCtx* mqttCtx)
{
	uint32_t now = Eliot_TimeNow();

	// eliot_mqtt_tcp_rx_time_last is zero until there is some TCP traffic.
	if(eliot_mqtt_tcp_rx_time_last && !Eliot_MqttIsConnected())
	{
		DBGPRINT_ERRORS_INTSTRSTR(now - eliot_mqtt_tcp_rx_time_last, "Eliot_MqttHeartbeatMonitor()", "Triggering reconnect...");
		return ELIOT_RET_CONNECT_FAILURE;
	}

	return ELIOT_RET_OK;
}


// Updates the LCM module's firmwareVersion attribute in Eliot.
uint32_t Eliot_PublishFirmwareVersion(MQTTCtx* mqttCtx)
{
	char output[156 + 2*ELIOT_DEVICE_ID_SIZE + sizeof(SYSTEM_INFO_FIRMWARE_VERSION)];

	if(!Eliot_MqttIsConnected())
	{
		return ELIOT_RET_CONNECT_FAILURE;
	}

	if(!eliot_lcm_module_id[0])
	{
		return ELIOT_RET_INPUT_ERROR;
	}

	snprintf(output, sizeof(output),
	"{"
		"\"jsonrpc\": \"2.0\","
		"\"method\": \"module.setProperties\","
		"\"params\": ["
		"{"
			"\"version\": \"1.0\","
			"\"firmwareVersion\": \""SYSTEM_INFO_FIRMWARE_VERSION"\","
			"\"sender\": {"
				"\"plant\": {"
					"\"coal\": {"
						"\"id\": \"%s\""
						"}"
					"}"
				"}"
			"}"
		"],"
		"\"id\": \"%s\""
	"}"
	, eliot_lcm_module_id, eliot_lcm_module_id);

	DBGPRINT_INTSTRSTR(strlen(output), "Eliot_PublishFirmwareVersion() Sending:", output);
	return Eliot_SendMqttPacket(mqttCtx, output, strlen(output), ELIOT_MQTT_ROUTE_SYSTEM);
}


uint32_t Eliot_KeepAliveTask(MQTTCtx* mqttCtx)
{
	char output[208];

	if(!Eliot_MqttIsConnected())
	{
		return ELIOT_RET_CONNECT_FAILURE;
	}

	if(!eliot_lcm_module_id[0])
	{
		return ELIOT_RET_INPUT_ERROR;
	}

	if((Eliot_TimeNow()-ELIOT_KEEPALIVE_INTERVAL_SEC) < eliot_keepalive_last)
	{
		return ELIOT_RET_OK;
	}

	snprintf(output, sizeof(output),
	"{"
		"\"message\": {"
			"\"id\": \"%s\""
		"}"
	"}"
	, eliot_lcm_module_id);

	DBGPRINT_INTSTRSTR(strlen(output), "Eliot_KeepAliveTask() Sending:", output);
	eliot_keepalive_last = Eliot_TimeNow();

	return Eliot_SendMqttPacket(mqttCtx, output, strlen(output), ELIOT_MQTT_ROUTE_DIAGNOSTIC);
}

// Set one or more metrics for a single module.  'metrics_string' is JSON syntax, no trailing comma:
// 		"key0":"value0", "key1":"value1", "key2":"value2"
uint32_t Eliot_PublishModuleMetrics(MQTTCtx* mqttCtx, char* module_id, char* metrics_string)
{
	char output[512];

	DBGPRINT_INTSTRSTR(strlen(metrics_string), "Eliot_PublishModuleMetrics() module ID:", module_id);

	snprintf(output, sizeof(output),
	"{"
		"\"message\": {"
			"\"systemProperties\": {"
				"\"contentType\": \"application/json\","
				"\"contentEncoding\": \"utf-8\""
			"},"
			"\"appProperties\": {"
				"\"eliot-route\": \"telemetry\""
			"},"
			"\"body\": {"
				"\"jsonrpc\": \"2.0\","
				"\"params\": [{"
					"\"modules\": [{"
					"%s"
					",\"sender\": {"
						"\"plant\": {"
							"\"coal\": {"
								"\"id\": \"%s\","
								"}"
							"}"
						"}"
					"}]"
				"}]"
			"}"
		"}"
	"}"
	, metrics_string, module_id);

	return Eliot_SendMqttPacket(mqttCtx, output, strlen(output), ELIOT_MQTT_ROUTE_DEFAULT);
}


// Send diagnostic data to Eliot under the "mDiagnostics" metric.
uint32_t Eliot_DiagnosticInfoTask(MQTTCtx* mqttCtx)
{
	char buffer[300];
	uint32_t idx=0;		// Output string buffer offset
	uint32_t err=0;		// Error counter iterator
	uint32_t *count_ptr=(uint32_t*)&eliot_error_count;
	buffer[sizeof(buffer)-2]=0;	// For detecting a full buffer

	if(eliot_mqtt_loop_counter & ELIOT_DIAGNOSTIC_INFO_INTERVAL_MASK)
	{
		return ELIOT_RET_OK;
	}

	if(!eliot_lcm_module_id[0])
	{
		DBGPRINT_ERRORS_INTSTRSTR(__LINE__, "Eliot_DiagnosticInfoTask()", "No LCM module ID");
		return ELIOT_RET_INPUT_ERROR;
	}

	idx+=snprintf(buffer, sizeof(buffer), "\"mDiagnostics\":\"");

	for(err=0; err<(sizeof(ELIOT_ERROR_COUNT)>>2); err++)
	{
		if(idx >= sizeof(buffer)-1)
		{
			break;
		}

		if(*count_ptr)
		{
			idx+=snprintf(buffer+idx, sizeof(buffer)-idx, "%s:%d ", err_label[err], *count_ptr);
		}
		count_ptr++;
	}

	idx+=snprintf(buffer+idx, sizeof(buffer)-idx, "\"");
	// Add end quotes if snprintf runs out of space
	if(buffer[sizeof(buffer)-2])	// If the buffer is full
	{
		buffer[sizeof(buffer)-2]='"';
	}

	DBGPRINT_INTSTRSTR(idx, "Eliot_DiagnosticInfoTask() Sending metric: ", buffer);

	Eliot_PublishModuleMetrics(mqttCtx, eliot_lcm_module_id, buffer);
	return ELIOT_RET_OK;
}

// This function will send firmware information to Eliot if firmwareUpdateState
// changes, if eliot_firmware_version_trigger is set or if no data has been sent
// in over ELIOT_FIRMWARE_INFO_INTERVAL_MAX_SEC seconds.
uint32_t Eliot_FirmwareInfoTask(MQTTCtx* mqttCtx)
{
	char buffer[300];
	uint32_t percentage_done=0;
	uint32_t idx=0;
	uint32_t now = Eliot_TimeNow();

	if(!Eliot_MqttIsConnected())
	{
		return ELIOT_RET_CONNECT_FAILURE;
	}

	if(!eliot_firmware_version_trigger &&
		(firmwareUpdateState==eliot_firmware_update_state_last) &&
		(now < (eliot_firmware_version_sent_last + ELIOT_FIRMWARE_INFO_INTERVAL_MAX_SEC)))
	{
		return ELIOT_RET_OK;
	}

	// Don't use the string if its length runs beyond the space reserved for it.
	if(_strnlen(updateFirmwareInfo.versionHash, sizeof(updateFirmwareInfo.versionHash)+1) >= sizeof(updateFirmwareInfo.versionHash))
	{
		return ELIOT_RET_INPUT_ERROR;
	}

	if(updateFirmwareInfo.S19LineCount)
	{
		percentage_done = firmwareUpdateLineNumber;
		percentage_done *= 100;
		percentage_done /= updateFirmwareInfo.S19LineCount;
	}

	if(percentage_done < 0)
	{
		percentage_done = 0;
	}
	else
	{
		if(percentage_done > 100)
		{
			percentage_done = 100;
		}
	}

	DBGPRINT_INTSTRSTR(percentage_done, "Eliot_FirmwareInfoTask(): Updating firmware info: versionHash/percentage_done", updateFirmwareInfo.versionHash);

    if (firmwareUpdateState >= (sizeof(UpdateStateStrings)/sizeof(UpdateStateStrings[0])))
    { // should not be able to happen, but make sure we bounds-check the UpdateStateStrings array
       firmwareUpdateState = UPDATE_STATE_NONE;
    }

    snprintf(buffer, sizeof(buffer),
		"\"mVersion\":\"%d:%d:%d-%d-%s\", "
		"\"mStatus\":\"%s\", "
		"\"mProgress\":\"%d\" ",
		updateFirmwareInfo.firmwareVersion[0],
		updateFirmwareInfo.firmwareVersion[1],
		updateFirmwareInfo.firmwareVersion[2],
		updateFirmwareInfo.firmwareVersion[3],
		updateFirmwareInfo.versionHash,
		UpdateStateStrings[firmwareUpdateState],
		percentage_done);

    Eliot_PublishModuleMetrics(mqttCtx, eliot_lcm_module_id, buffer);

    // Send the signed update information separately to reduce memory requirements
    if (updateFirmwareInfo.reportSha256Flag)
    {
       snprintf(buffer, sizeof(buffer),
           "\"mUpdateHash\":\"%s\", "            // 18 + 64   subtotal: 82 bytes
           "\"mUpdateHost\":\"%s\",  "           // 18 + 64   subtotal: 164 bytes
           "\"mUpdatePath\":\"%s\" ",            // 17 + 64   subtotal: 245 bytes
           updateFirmwareInfo.sha256HashString,
           updateFirmwareInfo.updateHost,
           updateFirmwareInfo.updatePath);

       Eliot_PublishModuleMetrics(mqttCtx, eliot_lcm_module_id, buffer);
    }

	// Send release notes separately to reduce memory requirements
	idx = snprintf(buffer, sizeof(buffer), "\"mReleaseNotes\":\"");
	SanitizeString(&buffer[idx], updateFirmwareInfo.releaseNotes, sizeof(buffer)-3);
	idx = strlen(buffer);
	buffer[idx++]='"';
	buffer[idx++]=0;

	Eliot_PublishModuleMetrics(mqttCtx, eliot_lcm_module_id, buffer);

	eliot_firmware_version_trigger = false;
	eliot_firmware_update_state_last = firmwareUpdateState;
	eliot_firmware_version_sent_last = now;

	return ELIOT_RET_OK;
}



// Send firmware information conditionally based on a rate limit.  Data will
// be sent if it has not been in 'interval_sec' seconds.  The effective time interval
// is the greater of 'interval_sec' and 'ELIOT_FIRMWARE_INFO_INTERVAL_MIN_SEC'.
void Eliot_SendFirmwareInfo(uint32_t interval_sec)
{
	uint32_t now = Eliot_TimeNow();

	if(interval_sec < ELIOT_FIRMWARE_INFO_INTERVAL_MIN_SEC)
	{
		interval_sec = ELIOT_FIRMWARE_INFO_INTERVAL_MIN_SEC;
	}

	// Set a flag which sends firmware info, if the update rate has not been exceeded.
	if(now > (eliot_firmware_version_sent_last + interval_sec))
	{
		eliot_firmware_version_trigger = true;
	}
}



void Eliot_WriteMetrics(MQTTCtx* mqttCtx, int index, bool_t scene, bool_t sceneFinished)
{
	DBGPRINT_INTSTRSTR(index, "Eliot_WriteMetrics() index", "index");
    char deviceId[ELIOT_DEVICE_ID_SIZE];
    json_t* device_metrics = json_object();

    if (scene)
    {
        scene_properties properties;
        GetSceneProperties(index, &properties);

        DBGPRINT_INTSTRSTR(0, "Eliot_WriteMetrics() scene", properties.EliotDeviceId);

        int sceneRunning = (sceneFinished) ? 0 : 1;

        DBGPRINT_INTSTRSTR(sceneRunning, "Eliot_WriteMetrics() running", properties.EliotDeviceId);
        json_object_set_new(device_metrics, "running", json_integer(sceneRunning));

        strcpy(deviceId, properties.EliotDeviceId);
    }
    else
    {
        zone_properties properties;
        GetZoneProperties(index, &properties);

        DBGPRINT_INTSTRSTR(properties.deviceType, "Eliot_WriteMetrics() deviceType", properties.EliotDeviceId);

        int powerState = properties.targetState;

        if (properties.deviceType == SHADE_TYPE
         || properties.deviceType == SHADE_GROUP_TYPE)
        {
            powerState = 1;
        }

        if (properties.deviceType == SWITCH_TYPE
         || properties.deviceType == DIMMER_TYPE
         || properties.deviceType == FAN_CONTROLLER_TYPE
         || properties.deviceType == SHADE_TYPE
         || properties.deviceType == SHADE_GROUP_TYPE)
        {
            DBGPRINT_INTSTRSTR(powerState, "Eliot_WriteMetrics() powerState", properties.EliotDeviceId);
            json_object_set_new(device_metrics, "powerState", json_integer(powerState));
        }

        if (properties.deviceType == DIMMER_TYPE
         || properties.deviceType == FAN_CONTROLLER_TYPE
         || properties.deviceType == SHADE_TYPE
         || properties.deviceType == SHADE_GROUP_TYPE)
        {
            DBGPRINT_INTSTRSTR(properties.targetPowerLevel, "Eliot_WriteMetrics() level", properties.EliotDeviceId);
            json_object_set_new(device_metrics, "level", json_integer(properties.targetPowerLevel));
        }

        strcpy(deviceId, properties.EliotDeviceId);
    }

    json_t* modules = json_pack("{ss,s{s{s{ss}}}}",
        "status", "on",
        "sender",
          "plant",
            "coal",
              "id", deviceId
    );

    // Only adds new keys to modules object, tack on our device metrics
    json_object_update_missing(modules, device_metrics);

    json_t* telemetry = json_pack("{s{s{ss,ss},s{ss},s{ss,s[{s[o]}]}}}",
        "message",
          "systemProperties",
            "contentType", "application/json",
            "contentEncoding", "utf-8",
          "appProperties",
            "eliot-route", "telemetry",
          "body",
            "jsonrpc", "2.0",
            "params",
              "modules", modules
    );

    char* telemetry_string = json_dumps(telemetry, (JSON_COMPACT | JSON_PRESERVE_ORDER));
    if (telemetry_string)
    {
        mqttCtx->publish.buffer = (byte*) telemetry_string;
        mqttCtx->publish.total_len = (word16) strlen(telemetry_string);

        MqttClient_Publish(&mqttCtx->client, &mqttCtx->publish);

        jsonp_free(telemetry_string);
        mqttCtx->publish.buffer = NULL;
    }

    json_delete(telemetry);
    json_delete(device_metrics);
    json_delete(modules);
}

static int Eliot_Disconnect_CB(void *context)
{
    SocketContext *sock = (SocketContext*)context;
    if (sock)
    {
        shutdown(sock->fd, FLAG_ABORT_CONNECTION);
        sock->fd = RTCS_SOCKET_ERROR;
        sock->stat = SOCK_BEGIN;
    }

    mqtt_socket = 0;
    free(sock);
    return 0;
}

bool_t VerifySceneChanged(int index)
{
    return true; // Until scene fading is fully implemented

    scene_properties properties;
    GetSceneProperties(index, &properties);

    if (last_metrics_scene_index == index &&
        properties.running == last_metrics_scene_running)
    {
        return false;
    }

    last_metrics_scene_running = properties.running;
    last_metrics_scene_index = index;

    return true;
}

bool_t VerifyZoneChanged(int index)
{
    zone_properties properties;
    GetZoneProperties(index, &properties);

    if (last_metrics_zone_index == index &&
        properties.state == last_metrics_zone.state &&
        properties.targetPowerLevel == last_metrics_zone.targetPowerLevel &&
        properties.powerLevel == last_metrics_zone.powerLevel)
    {
        return false;
    }

    last_metrics_zone = properties;
    last_metrics_zone_index = index;

    return true;
}

void Eliot_PublishMetrics(MQTTCtx* mqttCtx, int index, bool_t scene, bool_t sceneFinished)
{
    bool_t send_metrics = scene? VerifySceneChanged(index) : VerifyZoneChanged(index);

    if (send_metrics)
    {
        mqttCtx->stat = WMQ_PUB;
        memset(&mqttCtx->publish, 0, sizeof(MqttPublish));
        mqttCtx->publish.retain = 0;
        mqttCtx->publish.qos = mqttCtx->qos;
        mqttCtx->publish.duplicate = 0;
        mqttCtx->publish.topic_name = eliot_event_topic;
        mqttCtx->publish.packet_id = Mqtt_Get_PacketID();

        // Force publishing of state to connector
        Eliot_WriteMetrics(mqttCtx, index, scene, sceneFinished);
    }
}



// This function returns true if the same message has already
// been received by Eliot_Event_CB().  It compares and stores
// a checksum of the message in eliot_event_last_checksum.

static bool_t Eliot_EventIsDuplicate(MqttMessage *msg)
{
	uint32_t acc=0;
	uint32_t x=0;
	uint32_t now = Eliot_TimeNow();

	if(!msg->buffer_len)
	{
		return true;
	}

	for(x=0; x<msg->buffer_len-1; x+=2)
	{
		acc+=*(uint16_t*)(msg->buffer+x);
	}

	if ((acc==eliot_event_last_checksum) && ((now - eliot_mqtt_event_last) <= ELIOT_REPEAT_TIME_EXPIRE_SEC))
	{
		DBGPRINT_INTSTRSTR((int)(now - eliot_mqtt_event_last), ELIOT_MQTT_SERVICE, "Repeat Detected!");
		eliot_error_count.duplicate_event++;
		return true;
	}
	else
	{
		eliot_mqtt_event_last = now;
		eliot_event_last_checksum = acc;
		return false;
	}
}



static int Eliot_Event_CB(MqttClient *client, MqttMessage *msg, byte msg_new, byte msg_done)
{
    DBGPRINT_INTSTRSTR(msg_new, "Eliot_Event_CB()", "");
    MQTTCtx* mqttCtx = (MQTTCtx*)client->ctx;
    byte buf[MQTT_MESSAGE_SIZE+1];
    word32 len;

    (void)mqttCtx;

    // Don't process the same event twice.
    if(Eliot_EventIsDuplicate(msg))
    {
        return MQTT_CODE_SUCCESS;
    }

    if (msg_new)
    {
        len = msg->topic_name_len;
        if (len > MQTT_MESSAGE_SIZE)
        {
            len = MQTT_MESSAGE_SIZE;
        }
        memcpy(buf, msg->topic_name, len);
        buf[len] = '\0';
    }

    len = msg->buffer_len;
    if (len > MQTT_MESSAGE_SIZE)
    {
        len = MQTT_MESSAGE_SIZE;
    }

    memcpy(buf, msg->buffer, len);
    buf[len] = '\0';
    DBGPRINT_INTSTRSTR(0, ELIOT_MQTT_SERVICE, (char*) buf);

    json_error_t jsonError;
    json_t* payload = json_loads((char*) buf, 0, &jsonError);
    json_t* json_event = json_object_get(payload, "event");
    json_t* json_device_id = json_object_get(payload, "id");
    bool_t scene = false;

    if (json_event && json_device_id)
    {
        const char* deviceId = json_string_value(json_device_id);
        const char* event = json_string_value(json_event);

        if (strcmp("on", event) == 0)
        {
            SetDeviceState(deviceId, true, -1);
        }
        else if (strcmp("off", event) == 0)
        {
            SetDeviceState(deviceId, false, -1);
        }
        else if (strcmp("setLevel", event) == 0) {
            json_t* json_level = json_object_get(payload, "level");

            if (json_level) {
                const int level = json_integer_value(json_level);

                SetDeviceState(deviceId, -1, level);
                json_delete(json_level);
            }
        }
        else if (strcmp("adjustLevel", event) == 0) {
            json_t* json_delta = json_object_get(payload, "delta");

            if (json_delta) {
                const int delta = json_integer_value(json_delta);

                AdjustDeviceLevel(deviceId, delta);
                json_delete(json_delta);
            }
        }
        else if (strcmp("execute", event) == 0) {
            SetSceneState(deviceId, true);
            scene = true;
        }

        int index = FindZoneIndexById(deviceId);
        if (index != -1)
        {
            Eliot_PublishMetrics(mqttCtx, index, scene, false);
        }
    }
    else if (json_event)
    {
        const char* event = json_string_value(json_event);

        if (strcmp("updateFirmware", event) == 0) {
            SendUpdateCommand(UPDATE_COMMAND_INSTALL_FIRMWARE, NULL, NULL);
        }
        else if (strcmp("discardUpdate", event) == 0) {
           SendUpdateCommand(UPDATE_COMMAND_DISCARD_UPDATE, NULL, NULL);
        }

    }

    json_delete(payload);

    return MQTT_CODE_SUCCESS;
}

static int Eliot_Init(MQTTCtx *mqttCtx)
{
    int rc = MQTT_CODE_SUCCESS, i;

    switch (mqttCtx->stat)
    {
        case WMQ_BEGIN:
        {
            if (!mqttCtx->use_tls)
            {
                return MQTT_CODE_ERROR_BAD_ARG;
            }

            FALL_THROUGH;
        }

        case WMQ_NET_INIT:
        {
            mqttCtx->stat = WMQ_NET_INIT;

            rc = MqttClientNet_Init(&mqttCtx->net, mqttCtx);
            if (rc == MQTT_CODE_CONTINUE)
            {
                return rc;
            }

            mqttCtx->net.connect_cb = Eliot_Connect_CB;
            mqttCtx->net.read_cb = Eliot_Read_CB;
            mqttCtx->net.write_cb = Eliot_Write_CB;
            mqttCtx->net.disconnect_cb = Eliot_Disconnect_CB;

            if (rc != MQTT_CODE_SUCCESS)
            {
                DBGPRINT_ERRORS_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "MqttClientNet_Init failure!");
                goto exit;
            }

            mqttCtx->tx_buf = (byte*)malloc(MAX_BUFFER_SIZE);
            mqttCtx->rx_buf = (byte*)malloc(MAX_BUFFER_SIZE);

            Url_Encoder_Init();

            rc = SasTokenCreate((char*)mqttCtx->app_ctx, ELIOT_SAS_TOKEN_SIZE, eliot_primary_key, eliot_device_name);
            if (rc < 0)
            {
                DBGPRINT_ERRORS_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "Bad SasToken!");
                goto exit;
            }

            FALL_THROUGH;
        }

        case WMQ_INIT:
        {
            mqttCtx->stat = WMQ_INIT;

            rc = MqttClient_Init(&mqttCtx->client, &mqttCtx->net, Eliot_Event_CB,
                mqttCtx->tx_buf, MAX_BUFFER_SIZE, mqttCtx->rx_buf, MAX_BUFFER_SIZE,
                mqttCtx->cmd_timeout_ms);
            if (rc == MQTT_CODE_CONTINUE)
            {
                return rc;
            }

            if (rc != MQTT_CODE_SUCCESS)
            {
                DBGPRINT_ERRORS_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "MqttClient_Init failed!");
                goto exit;
            }
            mqttCtx->client.ctx = mqttCtx;

            FALL_THROUGH;
        }

        case WMQ_TCP_CONN:
        {
            mqttCtx->stat = WMQ_TCP_CONN;

            rc = MqttClient_NetConnect(&mqttCtx->client, mqttCtx->host, mqttCtx->port,
                DEFAULT_CON_TIMEOUT_MS, mqttCtx->use_tls, 0);

            if (rc == MQTT_CODE_CONTINUE)
            {
                return rc;
            }

            if (rc != MQTT_CODE_SUCCESS)
            {
                DBGPRINT_ERRORS_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "MqttClient_NetConnect failed!");
                goto exit;
            }

            memset(&mqttCtx->connect, 0, sizeof(MqttConnect));
            mqttCtx->connect.keep_alive_sec = mqttCtx->keep_alive_sec;
            mqttCtx->connect.clean_session = mqttCtx->clean_session;
            mqttCtx->connect.client_id = mqttCtx->client_id;

            mqttCtx->connect.username = eliot_username;
            mqttCtx->connect.password = (const char *)mqttCtx->app_ctx;

            FALL_THROUGH;
        }

        case WMQ_MQTT_CONN:
        {
            mqttCtx->stat = WMQ_MQTT_CONN;

            rc = MqttClient_Connect(&mqttCtx->client, &mqttCtx->connect);
            if (rc == MQTT_CODE_CONTINUE)
            {
                return rc;
            }

            if (rc != MQTT_CODE_SUCCESS)
            {
                DBGPRINT_ERRORS_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "MqttClient_Connect failed");
                goto disconn;
            }

            mqttCtx->topics[0].topic_filter = mqttCtx->topic_name;
            mqttCtx->topics[0].qos = mqttCtx->qos;

            memset(&mqttCtx->subscribe, 0, sizeof(MqttSubscribe));
            mqttCtx->subscribe.stat = MQTT_MSG_BEGIN;
            mqttCtx->subscribe.packet_id = Mqtt_Get_PacketID();
            mqttCtx->subscribe.topic_count = sizeof(mqttCtx->topics)/sizeof(MqttTopic);
            mqttCtx->subscribe.topics = mqttCtx->topics;

            FALL_THROUGH;
        }

        case WMQ_SUB:
        {
            mqttCtx->stat = WMQ_SUB;

            rc = MqttClient_Subscribe(&mqttCtx->client, &mqttCtx->subscribe);
            if (rc == MQTT_CODE_CONTINUE)
            {
                DBGPRINT_INTSTRSTR(0, ELIOT_MQTT_SERVICE, "Continuing from Subscribe!");
                return rc;
            }

            if (rc == MQTT_CODE_ERROR_TIMEOUT)
            {
                return rc;
            }
            else if (rc != MQTT_CODE_SUCCESS)
            {
                DBGPRINT_ERRORS_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "MqttClient_Subscribe failed!");
                mqttCtx->stat = WMQ_NET_DISCONNECT;
                mqttCtx->return_code = rc;
                rc = MQTT_CODE_CONTINUE;
                return rc;
            }

            for (i = 0; i < mqttCtx->subscribe.topic_count; i++)
            {
                mqttCtx->topic = &mqttCtx->subscribe.topics[i];
            }

            memset(&mqttCtx->publish, 0, sizeof(MqttPublish));
            mqttCtx->publish.retain = 0;
            mqttCtx->publish.qos = mqttCtx->qos;
            mqttCtx->publish.duplicate = 0;
            mqttCtx->publish.topic_name = eliot_event_topic;
            mqttCtx->publish.packet_id = Mqtt_Get_PacketID();
            mqttCtx->publish.buffer = NULL;
            mqttCtx->publish.total_len = 0;

            FALL_THROUGH;
        }

        case WMQ_PUB:
        {
            mqttCtx->stat = WMQ_PUB;

            rc = MqttClient_Publish(&mqttCtx->client, &mqttCtx->publish);
            if (rc == MQTT_CODE_CONTINUE)
            {
                return rc;
            }

            if (rc != MQTT_CODE_SUCCESS)
            {
                DBGPRINT_ERRORS_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "MqttClient_Publish failed!");
                goto disconn;
            }

            return rc;
        }

        case WMQ_DISCONNECT:
        {
            rc = MqttClient_Disconnect(&mqttCtx->client);
            if (rc == MQTT_CODE_CONTINUE)
            {
                return rc;
            }

            if (rc != MQTT_CODE_SUCCESS)
            {
                DBGPRINT_ERRORS_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "MqttClient_Disconnect failed!");
                goto disconn;
            }

            FALL_THROUGH;
        }

        case WMQ_NET_DISCONNECT:
        {
            mqttCtx->stat = WMQ_NET_DISCONNECT;

            rc = MqttClient_NetDisconnect(&mqttCtx->client);
            if (rc == MQTT_CODE_CONTINUE)
            {
                return rc;
            }

            FALL_THROUGH;
        }

        case WMQ_DONE:
        {
            mqttCtx->stat = WMQ_DONE;
            rc = mqttCtx->return_code;
            goto exit;
        }

        case WMQ_UNSUB:
        default:
            rc = MQTT_CODE_ERROR_STAT;
            goto exit;
    }

disconn:
    mqttCtx->stat = WMQ_NET_DISCONNECT;
    mqttCtx->return_code = rc;
    rc = MQTT_CODE_CONTINUE;

exit:

    if (rc != MQTT_CODE_CONTINUE)
    {
        if (mqttCtx->tx_buf)
        {
            free(mqttCtx->tx_buf);
            mqttCtx->tx_buf = 0;
        }

        if (mqttCtx->rx_buf)
        {
            free(mqttCtx->rx_buf);
            mqttCtx->rx_buf = 0;
        }

        MqttClientNet_DeInit(&mqttCtx->net);
    }

    return rc;
}

static void Mqtt_HandleTaskMessage(ELIOT_TASK_MESSAGE_PTR EliotTaskMessagePtr, MQTTCtx* mqttCtx)
{
    if (EliotTaskMessagePtr)
    {
        switch (EliotTaskMessagePtr->command)
        {
            case ELIOT_COMMAND_ZONE_CHANGED:
            {
                Eliot_PublishMetrics(mqttCtx, EliotTaskMessagePtr->index, false, false);
                break;
            }
            case ELIOT_COMMAND_SCENE_STARTED:
            {
                Eliot_PublishMetrics(mqttCtx, EliotTaskMessagePtr->index, true, false);
                break;
            }
            case ELIOT_COMMAND_SCENE_FINISHED:
            {
                Eliot_PublishMetrics(mqttCtx, EliotTaskMessagePtr->index, true, true);
                break;
            }
        }

        _msg_free(EliotTaskMessagePtr);
    }
}

static int Eliot_GetEvent(MQTTCtx* mqttCtx, _queue_id queueId)
{
    ELIOT_TASK_MESSAGE_PTR EliotTaskMessagePtr;
	int rc = 0;
	mqttCtx->stat = WMQ_WAIT_MSG;
	DBGPRINT_INTSTRSTR(0, ELIOT_MQTT_SERVICE, "Waiting for message...");
	Eliot_MqttPrintAESKeys(mqttCtx);
	Eliot_PublishFirmwareVersion(mqttCtx);

	// Prevent Eliot_MqttHeartbeatMonitor from breaking
	// out of the the do/while loop prematurely.
	eliot_mqtt_tcp_rx_time_last = 0;

	do
	{
        // Increment this task's watchdog counter
        watchdog_task_counters[ELIOT_MQTT_TASK]++;

        _time_delay(50);
        eliot_mqtt_loop_counter++;

        if (mStopRead || mqttCtx->test_mode)
        {
            rc = MQTT_CODE_SUCCESS;
            break;
        }

        rc = MqttClient_WaitMessage(&mqttCtx->client, mqttCtx->cmd_timeout_ms);

        // Break out since our MQTT strings were cleared.
        if (!eliot_event_topic[0])
        {
            rc = MQTT_CODE_SUCCESS;
            break;
        }

        Eliot_DiagnosticInfoTask(mqttCtx);
        Eliot_FirmwareInfoTask(mqttCtx);
        Eliot_KeepAliveTask(mqttCtx);

        if(Eliot_MqttHeartbeatMonitor(mqttCtx) != ELIOT_RET_OK)
        {
            rc = MQTT_CODE_ERROR_NETWORK;
            break;
        }

        EliotTaskMessagePtr = _msgq_poll(queueId);
        if (EliotTaskMessagePtr)
        {
            Mqtt_HandleTaskMessage(EliotTaskMessagePtr, mqttCtx);
        }

        if (rc == MQTT_CODE_CONTINUE)
        {
            continue;
        }
        else if (rc != MQTT_CODE_SUCCESS)
        {
            DBGPRINT_ERRORS_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "MqttClient_WaitMessage failed");
            break;
        }

        EliotTaskMessagePtr = _msgq_poll(queueId);
        if (EliotTaskMessagePtr)
        {
            Mqtt_HandleTaskMessage(EliotTaskMessagePtr, mqttCtx);
        }
	} while (1);

	if (rc != MQTT_CODE_SUCCESS)
	{
		mqttCtx->stat = WMQ_NET_DISCONNECT;
		mqttCtx->return_code = rc;
		rc = MQTT_CODE_CONTINUE;
	}

	return rc;
}


static int Eliot_Connect(MQTTCtx* mqttCtx)
{
	logConnectionSequence(COMSTATE_MQTT_CONN_ELIOT, "MQTT: Connecting to Eliot");
	int rc = 0;
	do
	{
		rc = Eliot_Init(mqttCtx);

		if (rc == MQTT_CODE_ERROR_NETWORK || rc == MQTT_CODE_ERROR_TIMEOUT)
		{
			rc = MQTT_CODE_CONTINUE;
			mqttCtx->stat = WMQ_BEGIN;
			_time_delay(ELIOT_MQTT_RECONNECT_MS);
		}
	} while (rc == MQTT_CODE_CONTINUE);

	return rc;
}

static int Eliot_Disconnect(MQTTCtx* mqttCtx)
{
    logConnectionSequence(COMSTATE_MQTT_DISCONN_ELIOT, "MQTT: Disconnecting from Eliot");
    int rc = 0;

    DBGPRINT_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "Calling MqttClient_Disconnect");
    rc = MqttClient_Disconnect(&mqttCtx->client);

    if (rc != MQTT_CODE_SUCCESS)
    {
        DBGPRINT_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "MqttClient_Disconnect failed!");
    }
    else
    {

        DBGPRINT_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "Calling MqttClient_NetDisconnect");
        rc = MqttClient_NetDisconnect(&mqttCtx->client);
        if (rc != MQTT_CODE_SUCCESS)
        {
            DBGPRINT_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "MqttClient_NetDisconnect failed!");
        }
        else
        {
            if (mqttCtx->tx_buf)
            {
                free(mqttCtx->tx_buf);
                mqttCtx->tx_buf = 0;
            }

            if (mqttCtx->rx_buf)
            {
                free(mqttCtx->rx_buf);
                mqttCtx->rx_buf = 0;
            }

            DBGPRINT_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "Calling MqttClientNet_DeInit");
            rc = MqttClientNet_DeInit(&mqttCtx->net);
            if (rc != MQTT_CODE_SUCCESS)
            {
                DBGPRINT_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "MqttClientNet_DeInit failed!");
            }
        }
    }

    if (rc == MQTT_CODE_SUCCESS)
    {
        DBGPRINT_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "Eliot_Disconnect returning success!");
    }
    else
    {
        DBGPRINT_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "Eliot_Disconnect returning failure!");
    }

	return rc;
}

bool_t Mqtt_SendTaskMessage(byte_t command, byte_t index, json_t* messageObj)
{
    ELIOT_TASK_MESSAGE_PTR EliotTaskMessagePtr;
    bool_t errorFlag = true;

    if (MqttTaskMessagePoolID)
    {
        EliotTaskMessagePtr = (ELIOT_TASK_MESSAGE_PTR) _msg_alloc(MqttTaskMessagePoolID);
        if (EliotTaskMessagePtr)
        {
            EliotTaskMessagePtr->HEADER.SOURCE_QID = 0;  //not relevant
            EliotTaskMessagePtr->HEADER.TARGET_QID = _msgq_get_id(0, MQTT_TASK_MESSAGE_QUEUE);
            EliotTaskMessagePtr->HEADER.SIZE = sizeof(ELIOT_TASK_MESSAGE);

            EliotTaskMessagePtr->command = command;
            EliotTaskMessagePtr->index = index;
            EliotTaskMessagePtr->jsonObj = messageObj;

            errorFlag = !_msgq_send(EliotTaskMessagePtr);
        }
    }

    return errorFlag;
}

void Eliot_InitMqttContext(MQTTCtx* mqttCtx, char* sasToken)
{
    Mqtt_Init_Ctx(mqttCtx);
    mqttCtx->app_name = "LCM";
    mqttCtx->host = ELIOT_HOST;
    mqttCtx->qos = ELIOT_QOS;
    mqttCtx->port = 8883;
    mqttCtx->keep_alive_sec = ELIOT_KEEP_ALIVE_SEC;
    mqttCtx->client_id = eliot_lcm_module_id;
    mqttCtx->topic_name = eliot_msgs_topic_name;
    mqttCtx->cmd_timeout_ms = ELIOT_CMD_TIMEOUT_MS;
    mqttCtx->use_tls = 1;
    mqttCtx->app_ctx = (void*)sasToken;
}

void Eliot_MQTT_Task()
{
    int rc;
    MQTTCtx mqttCtx;
    char sasToken[ELIOT_SAS_TOKEN_SIZE] = {0};
    bool firstTime = true;
    // Open a message queue
    _queue_id queueId = _msgq_open(MQTT_TASK_MESSAGE_QUEUE, 0);
    bool_t connected = false;

    // Create a message pool
    MqttTaskMessagePoolID = _msgpool_create(sizeof(ELIOT_TASK_MESSAGE), 8, 0, 0);

	// String members below are built by:
	// Eliot_MQTT_Task() -> Eliot_Connect() -> Eliot_Init() -> Eliot_LcmPlantAndModuleInit() -> Eliot_BuildMqttStrings()

    Eliot_InitMqttContext(&mqttCtx, sasToken);

    eliot_device_name[0] = 0;
    eliot_username[0] = 0;
    eliot_msgs_topic_name[0] = 0;
    eliot_event_topic[0] = 0;

    _time_delay(ELIOT_INITIAL_TASK_DELAY_MS);

    while(1)
    {
        // Increment this task's watchdog counter
        watchdog_task_counters[ELIOT_MQTT_TASK]++;

        // If cloud operations disabled, continue
        if (!cloudState)
        {
            if (connected)
            {
                connected = false;
                Eliot_Disconnect(&mqttCtx);
                Eliot_InitMqttContext(&mqttCtx, sasToken);
            }

            _time_delay(50);
            continue;
        }

        // Don't do anything if we don't have MQTT strings setup yet
        if (eliot_lcm_module_id[0] && eliot_event_topic[0])
        {
            if (firstTime || !connected)
            {
                Eliot_Connect(&mqttCtx);
                firstTime = false;
                connected = true;
            }

            rc = Eliot_GetEvent(&mqttCtx, queueId);
            if (rc == MQTT_CODE_CONTINUE)
            {
                DBGPRINT_INTSTRSTR(0, ELIOT_MQTT_SERVICE, "Trying to reestablish connection ...");
                rc = Eliot_Connect(&mqttCtx);
                if (rc != MQTT_CODE_SUCCESS)
                {
                    logConnectionSequence(COMSTATE_MQTT_CONN_ISSUES, "MQTT: Issues During Reconnection");
                    DBGPRINT_ERRORS_INTSTRSTR(0, ELIOT_MQTT_SERVICE, "Issues during reconnection ...");
                }
                else
                {
                    DBGPRINT_INTSTRSTR(0, ELIOT_MQTT_SERVICE, "Reconnected ...");
                    logConnectionSequence(COMSTATE_MQTT_CONNECTED, "MQTT: Connected");
                }
            }
        }
        else if (!firstTime)
        {
            // MQTT strings were cleared after we were already connected.  Disconnect in that case.
            DBGPRINT_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "Calling Eliot_Disconnect");
            rc = Eliot_Disconnect(&mqttCtx);

            if (rc == MQTT_CODE_SUCCESS)
            {
                DBGPRINT_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "Eliot_Disconnect succeeded!");

                // Re-init
                firstTime = true;
                Eliot_InitMqttContext(&mqttCtx, sasToken);
            }
            else
            {
                DBGPRINT_INTSTRSTR(rc, ELIOT_MQTT_SERVICE, "Eliot_Disconnect failed!");
            }
        }
        _time_delay(50);
    }
}
