/*
 * Eliot_Common.h
 *
 *  Created on: Mar 8, 2019
 *      Author: Legrand
 */

#ifndef ELIOT_COMMON_H_
#define ELIOT_COMMON_H_

#include <stdint.h>
#include "defines.h"
#include "onq_standard_types.h"

#define ELIOT_RET_UNDEFINED              0x0000
#define ELIOT_RET_OK                     0x0001
#define ELIOT_RET_RESOURCE_NOT_FOUND     0x0002
#define ELIOT_RET_CONNECT_FAILURE        0x0004
#define ELIOT_RET_RESPONSE_TIMEOUT       0x0008
#define ELIOT_RET_CLOUD_ERROR            0x0010
#define ELIOT_RET_INPUT_ERROR            0x0020
#define ELIOT_RET_QUOTA_EXCEEDED         0x0040
#define ELIOT_RET_CONFLICT               0x0080

//List Of Connection states
#define COMSTATE_INIT                    0	//Initial
#define COMSTATE_GET_IP                  1	//Getting IP Address
#define COMSTATE_GOT_IP                  2	//Address Resolved
#define COMSTATE_GET_SOCKET              3	//Connecting Socket
#define COMSTATE_GET_SSL                 4	//Connecting to SSL
#define COMSTATE_GOT_SSL                 5	//SSL Connected
#define COMSTATE_START_REST              6	//Starting REST
#define COMSTATE_TOKEN_EXPIRED           7	//User Token Expired (Refreshing)
#define COMSTATE_NO_TOKEN                8	//No User Token (Refreshing)
#define COMSTATE_MQTT_INIT               20	//MQTT Initializing
#define COMSTATE_MQTT_CONN_ELIOT         21	//MQTT Connecting To Eliot
#define COMSTATE_MQTT_DISCONN_ELIOT      22	//MQTT Disconnecting from Eliot
#define COMSTATE_MQTT_CONNECTED          23	//MQTT Connected
#define COMSTATE_MQTT_CONN_ISSUES        24	//MQTT Issues During Connection


#define ELIOT_HTTP_OK                200
#define ELIOT_HTTP_NO_CONTENT        204
#define ELIOT_HTTP_QUOTA_EXCEEDED    403
#define ELIOT_HTTP_QUOTA_EXCEEDED2   429
#define ELIOT_HTTP_CONFLICT          409

#define DEBUG_ELIOT_ANY              0
#define DEBUG_ELIOT                  (1 && DEBUG_ELIOT_ANY)
#define DEBUG_ELIOT_ERRORS           (1 && DEBUG_ELIOT_ANY)
#define DEBUG_ELIOT_SECURE           (1 && DEBUG_ELIOT_ANY)

#define ELIOT_LED_ACTIVITY_DURATION  8   // Network activity flicker duration
#define ELIOT_LED_ERROR_SPACING      8   // Dead time between error sequences

// If two keepalives have been missed, the LCM is disconnected.
#define ELIOT_MQTT_RECONNECT_SEC     (((ELIOT_MQTT_TCP_KEEPALIVE_MS/1000) * 2) + 6)

#define DBGPRINT_INTSTRSTR if (DEBUG_ELIOT || DEBUG_JUMPER) broadcastDebug
#define DBGPRINT_ERRORS_INTSTRSTR if (DEBUG_ELIOT_ERRORS) broadcastDebug
#define DBGPRINT_SECURE_INTSTRSTR if (DEBUG_ELIOT_SECURE) broadcastDebug
#define DBGPRINT_INSECURE_INTSTRSTR if (DEBUG_ELIOT && !(DEBUG_ELIOT_SECURE)) broadcastDebug
#define DBGSNPRINTF if (DEBUG_ELIOT) snprintf
#define DEBUGBUF debugBuf
#define SIZEOF_DEBUGBUF sizeof(debugBuf)

#define SERIAL_NUMBER_FORMAT_0          0
#define SERIAL_NUMBER_FORMAT_1          1
#define SERIAL_NUMBER_FORMAT_CURRENT    SERIAL_NUMBER_FORMAT_1

// These labels make it possible to identify the error type in the log output.
// Each member of the ELIOT_ERROR_COUNT struct requires a corresponding label.
#define ELIOT_ERROR_LABELS {"EliotConnect","Connect","WsslNew","WsslSetFd","WsslContext","OpenSockets","Socket","Timeout","DNS","REST","SendBytes","RefreshToken","UserToken","LcmPlantId","LcmModId","PrimaryKey","SyncConn","AddConn","DeleteConn","RenameConn","SyncRead","DuplEvent"}

typedef struct ELIOT_ERROR_COUNT
{
	uint32_t eliot_connect;         // 0
	uint32_t connect;               // 1
	uint32_t wolfssl_new;           // 2
	uint32_t wolfssl_set_fd;        // 3
	uint32_t wolfssl_ctx;           // 4
	uint32_t open_sockets;          // 5
	uint32_t socket_err;            // 6
	uint32_t response_timeouts;     // 7
	uint32_t dns;                   // 8
	uint32_t rest;                  // 9
	uint32_t send_bytes;            // 10
	uint32_t refresh_token;         // 11
	uint32_t user_token;            // 12
	uint32_t lcm_plant_id;          // 13
	uint32_t lcm_module_id;         // 14
	uint32_t primary_key;           // 15
	uint32_t sync_conn;             // 16
	uint32_t add_conn;              // 17
	uint32_t delete_conn;           // 18
	uint32_t rename_conn;           // 19
	uint32_t sync_read;             // 20
	uint32_t duplicate_event;       // 21
} ELIOT_ERROR_COUNT;

typedef struct ELIOT_LED_STATE
{
	uint32_t error_flags;         // L2 LED will flash red b times for every bit b2:b31 that is set
	int8_t active_code;           // The error code / bit that is currently being flashed
	int8_t countdown;             // Flash countdown
	int8_t clock_last;            // Tracks flash rate
	int8_t activity_holdoff;      // Temporarily inhibits Eliot_LedSet() for a more noticeable flicker 
} ELIOT_LED_STATE;

extern const char eliot_empty_string[];
extern ELIOT_ERROR_COUNT eliot_error_count;
extern uint32_t eliot_mqtt_event_last;
extern ELIOT_LED_STATE eliot_led_state;
extern char eliot_user_token[ELIOT_TOKEN_SIZE];
extern char *eliot_refresh_token;
extern const char *err_label[];
extern int last_metrics_scene_index;
extern int last_metrics_zone_index;

void Eliot_ResetErrorCounters(void);

void Eliot_PrintErrorCounters(void);

void AdjustDeviceLevel(const char* deviceId, short delta);

void SetDeviceState(const char* deviceId, short state, short level);

int FindZoneIndexById(const char* deviceId);

int FindSceneIndexById(const char* deviceId);

void SetSceneState(const char* deviceId, const bool_t state);

int32_t Eliot_LedIndicateActivity(void);

int32_t Eliot_LedSet(void);

void logConnectionSequence(int SequenceNum, const char *dataStr);

#endif /* ELIOT_COMMON_H_ */
