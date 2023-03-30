/*
 * Eliot_MQTT_Task.h
 *
 *  Created on: Feb 13, 2019
 *      Author: Legrand
 */

#ifndef ELIOT_MQTT_TASK_H_
#define ELIOT_MQTT_TASK_H_

#include "defines.h"
#include <stdint.h>
#include "onq_standard_types.h"
#include "jansson_private.h"
#include "String_Sanitizer.h"
#include "../WolfMQTT/wolfmqtt/mqtt_socket.h"           // Required for AES key debug
#include "../WolfSSL/wolfssl-3.11.0-stable/wolfssl/internal.h"  // Required for AES key debug

#define ELIOT_HOST              "eliot-iothub.azure-devices.net"

#define ELIOT_QOS               MQTT_QOS_0
#define ELIOT_KEEP_ALIVE_SEC    60 * 60 * 12 // 12 hours so we don't re-connect constantly
#define ELIOT_CMD_TIMEOUT_MS    30000
#define ELIOT_SAS_TOKEN_SIZE    400
#define ELIOT_MQTT_RECONNECT_MS 500
#define ELIOT_MQTT_TCP_KEEPALIVE_MS     24000
#define ELIOT_REPEAT_TIME_EXPIRE_SEC    2 //Seconds in which to ignore repeat commands

// Update LCM module firmware metrics no more often than every 3 seconds
#define ELIOT_FIRMWARE_INFO_INTERVAL_MIN_SEC	3
// Update LCM module firmware metrics at least daily
#define ELIOT_FIRMWARE_INFO_INTERVAL_MAX_SEC	86400
// Send an MQTT keepalive message every 17 minutes
#define ELIOT_KEEPALIVE_INTERVAL_SEC			1020

// Send diagnostic error counts to Eliot at most every ~1.2 days
#define ELIOT_DIAGNOSTIC_INFO_INTERVAL_MASK     0x1FFFFF

#define ELIOT_MQTT_ROUTE_MAX_LENGTH             32
#define ELIOT_MQTT_ROUTE_DIAGNOSTIC             "eliot-route=diagnostic"
#define ELIOT_MQTT_ROUTE_SYSTEM                 "eliot-route=system"
#define ELIOT_MQTT_ROUTE_DEFAULT                0


#define Mqtt_QueueZoneChanged(index) Mqtt_SendTaskMessage(ELIOT_COMMAND_ZONE_CHANGED, index, NULL);
#define Mqtt_QueueSceneStarted(index) Mqtt_SendTaskMessage(ELIOT_COMMAND_SCENE_STARTED, index, NULL);
#define Mqtt_QueueSceneFinished(index) Mqtt_SendTaskMessage(ELIOT_COMMAND_SCENE_FINISHED, index, NULL);


extern char eliot_device_name[sizeof(ELIOT_HOST"/devices/")+ELIOT_DEVICE_ID_SIZE];
extern char eliot_username[sizeof(ELIOT_HOST"/")+ELIOT_DEVICE_ID_SIZE];
extern char eliot_msgs_topic_name[sizeof("devices//messages/devicebound/#")+ELIOT_DEVICE_ID_SIZE];
extern char eliot_event_topic[sizeof("devices//messages/events/")+ELIOT_DEVICE_ID_SIZE];

int32_t Eliot_BuildMqttStrings(void);

void Eliot_SendFirmwareInfo(uint32_t interval_sec);

bool_t Mqtt_SendTaskMessage(byte_t, byte_t, json_t*);

void Eliot_MQTT_Task();

#endif /* ELIOT_MQTT_TASK_H_ */
