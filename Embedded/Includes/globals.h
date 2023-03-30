/*
 * globals.h
 *
 *  Created on: Jun 17, 2013
 *      Author: Legrand
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "typedefs.h"

//-----------------------------------------------------------------------------

#ifdef MAIN
#define PUBEXT
#else
#define PUBEXT extern
#endif

#if (__cplusplus)
extern "C"
{
#endif

//-----------------------------------------------------------------------------

PUBEXT system_properties                  systemProperties;
PUBEXT TIME_STRUCT                        lastAddALightTime;
PUBEXT system_shade_support_properties    systemShadeSupportProperties;
PUBEXT extended_system_properties         extendedSystemProperties;
PUBEXT boolean                            inDST;
PUBEXT bool_t                             systemTimeValid;
PUBEXT firmware_info                      firmwareInfo;


PUBEXT word_t                             global_Error;

PUBEXT json_socket         jsonSockets[MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS];
PUBEXT onqtimer_t          jsonSocketTimer[MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS];
PUBEXT large_timer_t       timeSincePingReceived; // must be long to support > 65 seconds
PUBEXT byte_t              updateAlertPingCounter[MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS];

PUBEXT LWGPIO_STRUCT       jsonActivityLED;
PUBEXT LWGPIO_STRUCT       IN0Input;
PUBEXT LWGPIO_STRUCT       IN1Input;
PUBEXT LWGPIO_STRUCT       OUT0Output;
PUBEXT LWGPIO_STRUCT       OUT1Output;
PUBEXT LWGPIO_STRUCT       factoryResetInput;
PUBEXT LWGPIO_STRUCT       spareS3Input;
PUBEXT LWGPIO_STRUCT       spareS4Input;
PUBEXT LWGPIO_STRUCT       led1Green;
PUBEXT LWGPIO_STRUCT       led1Red;
PUBEXT LWGPIO_STRUCT       led2Green;
PUBEXT LWGPIO_STRUCT       led2Red;
PUBEXT LWGPIO_STRUCT       ledD7Blue;
PUBEXT LWGPIO_STRUCT       debugJumperInput;


PUBEXT onqtimer_t             jsonActivityTimer;
PUBEXT onqtimer_t             restartSystemTimer;
PUBEXT onqtimer_t             flashSaveTimer;
PUBEXT large_timer_t          checkForUpdatesTimer;
PUBEXT large_timer_t          refreshTokenTimer;
PUBEXT large_timer_t          checkWatchdogCountersTimer;
PUBEXT volatile large_timer_t          clearQubeAddressTimer;

PUBEXT byte_t              firmwareUpdateState;

PUBEXT word_t              currentFirmwareVersion[4];
PUBEXT word_t              firmwareUpdateLineNumber;

PUBEXT _enet_address       myMACAddress
#ifdef MAIN
   = DEFAULT_MAC_ADDRESS
#endif
;

PUBEXT byte_t     flashBlockIndexForNextWrite;

// Array of watchdog counters - one for each task
PUBEXT unsigned long watchdog_task_counters[NUM_TASKS_PLUS_ONE]
#ifdef MAIN
   = {}
#endif
;

// global socketMonitor send message pool
PUBEXT _pool_id   socketMonitorMessagePoolID
#ifdef MAIN
   = 0
#endif
;

// global RS232 Receive Task message pool
PUBEXT _pool_id   RS232ReceiveTaskMessagePoolID
#ifdef MAIN
   = 0
#endif
;

// global RS232 Transmit Ack message pool
PUBEXT _pool_id   RFM100TransmitAckMessagePoolID
#ifdef MAIN
   = 0
#endif
;

// global RFM100 Transmit Task message pool
PUBEXT _pool_id   RFM100TransmitTaskMessagePoolID
#ifdef MAIN
   = 0
#endif
;

// global update server message pool
PUBEXT _pool_id   updateTaskMessagePoolID
#ifdef MAIN
   = 0
#endif
;

// global Eliot REST task message pool
PUBEXT _pool_id   EliotTaskMessagePoolID
#ifdef MAIN
   = 0
#endif
;

// global Eliot MQTT task message pool
PUBEXT _pool_id   MqttTaskMessagePoolID
#ifdef MAIN
   = 0
#endif
;

// global qlink send message pool
PUBEXT bool_t     manufacturingTestModeFlag
#ifdef MAIN
   = false
#endif
;

// global cloud operations state
PUBEXT bool_t     cloudState
#ifdef MAIN
   = true
#endif
;

PUBEXT MUTEX_STRUCT     JSONTransmitTimerMutex;

PUBEXT MUTEX_STRUCT     ZoneArrayMutex;

PUBEXT MUTEX_STRUCT     ScenePropMutex;

PUBEXT MUTEX_STRUCT     TaskHeartbeatMutex;

PUBEXT MUTEX_STRUCT     JSONWatchMutex;   //Mutex for sharing jsonSockets between Watch and JSON ports

PUBEXT LWSEM_STRUCT     transmitAckSemaphore;

PUBEXT unsigned long    BlinkDisplayCount[3];
PUBEXT unsigned char    BlinkDisplayFlag[3];

#define Event_Timer_Task_HEARTBEAT      0x00000001
#define Socket_Task_HEARTBEAT           0x00000002
PUBEXT unsigned long    task_heartbeat_bitmask;

PUBEXT byte_t           global_houseID;

PUBEXT bool_t           refreshTokenFlag;
PUBEXT bool_t           refreshUserFlag;
PUBEXT bool_t           userTokenChangedFlag;
PUBEXT uint16_t         refreshTokenFailCount;
PUBEXT bool_t           hubNeedsAsyncRegistration; // says it needs to be registered (to keep it online)
PUBEXT byte_t           zoneNeedsAsyncRegistrationBitArray[ZONE_BIT_ARRAY_BYTES]; // one bit per zone, says it needs to be registered (to keep it online)
PUBEXT byte_t           sceneNeedsAsyncRegistrationBitArray[SCENE_BIT_ARRAY_BYTES]; // one bit per zone, says it needs to be registered (to keep it online)

#ifdef SHADE_CONTROL_ADDED
PUBEXT nonvol_shade_properties shadeInfoStruct; // shadeInfo[MAX_NUMBER_OF_ZONES];
PUBEXT MUTEX_STRUCT     qubeSocketMutex;
PUBEXT volatile _ip_address      qubeAddress
#ifdef MAIN
   = 0
#endif
;
#endif


// Number of times the LCM attempted to add a device to Eliot Cloud
PUBEXT unsigned long eliot_add_count
#ifdef MAIN
   = 0
#endif
;

PUBEXT uint8_t currHubRegErrors
#ifdef MAIN
   = 0
#endif
;

PUBEXT uint8_t currDeviceRegErrors
#ifdef MAIN
   = 0
#endif
;

PUBEXT uint8_t currSceneRegErrors
#ifdef MAIN
   = 0
#endif
;

PUBEXT unsigned long qmotion_try_find_count
#ifdef MAIN
   = 0
#endif
;

PUBEXT unsigned long qmotion_found_count
#ifdef MAIN
   = 0
#endif
;

PUBEXT unsigned long qmotion_try_add_count
#ifdef MAIN
   = 0
#endif
;

PUBEXT unsigned long qmotion_add_success_count
#ifdef MAIN
   = 0
#endif
;

PUBEXT unsigned long qmotion_add_shades_count
#ifdef MAIN
   = 0
#endif
;

PUBEXT unsigned long qmotion_send_error_count
#ifdef MAIN
   = 0
#endif
;

PUBEXT unsigned long qmotion_recv_error_count
#ifdef MAIN
   = 0
#endif
;

PUBEXT unsigned long qmotion_missed_packet_count
#ifdef MAIN
   = 0
#endif
;

// The last error code received from socket send
PUBEXT unsigned long last_send_err
#ifdef MAIN
   = 0
#endif
;

// The last error code received from socket recv
PUBEXT unsigned long last_recv_err
#ifdef MAIN
   = 0
#endif
;

// Number of times the LCM had some unexpected error sending to a socket
PUBEXT unsigned long other_send_err_count
#ifdef MAIN
   = 0
#endif
;

// Number of times the LCM had some unexpected error receiving from a socket
PUBEXT unsigned long other_recv_err_count
#ifdef MAIN
   = 0
#endif
;

// Number of times an error was detected while parsing JSON data
PUBEXT unsigned long json_err_count
#ifdef MAIN
   = 0
#endif
;

// Number of active_incoming_json_connections 
PUBEXT unsigned short active_incoming_json_connections
#ifdef MAIN
   = 0
#endif
;

// The following increments when there is a failure to get a msgq object from the pool 
PUBEXT unsigned short no_json_msg_slot_err
#ifdef MAIN
   = 0
#endif
;

// The following increments when there is a failure to allocate a msgq object 
PUBEXT unsigned long rs232ReceiveTaskMsgAllocErr
#ifdef MAIN
   = 0
#endif
;

// The following increments when there is a failure to allocate a msgq object 
PUBEXT unsigned long rfm100TransmitTaskMsgAllocErr
#ifdef MAIN
   = 0
#endif
;
// The following increments when there is a failure to allocate a msgq object 
PUBEXT unsigned long socketMonMsgAllocErr
#ifdef MAIN
   = 0
#endif
;
// The following increments when there is a failure to allocate a msgq object 
PUBEXT unsigned long updateTaskMsgAllocErr
#ifdef MAIN
   = 0
#endif
;

// message sequence number
PUBEXT unsigned short msgSequenceNum
#ifdef MAIN
   = 0
#endif
;

// Connection sequence number
PUBEXT int connectionSequenceNum
#ifdef MAIN
   = 0
#endif
;

PUBEXT bool_t led1Green_default_state;

PUBEXT unsigned int led1Green_function
#ifdef MAIN
   = LED1_GREEN_FUNCTION_DEFAULT
#endif
;

// This stores the first system time the LCM received.  It is used for diagnostic purposes to identify a reboot cycle.
PUBEXT unsigned long firstSystemTime
#ifdef MAIN
   = 0
#endif
;

#if (__cplusplus)
}
#endif
#endif /* GLOBALS_H_ */
