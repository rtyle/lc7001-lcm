/*
 * typedefs.h
 *
 *  Created on: Jun 17, 2013
 *      Author: Legrand
 */

#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

#ifdef AMDBUILD
#include "MQXWrapper.h"
#include "onq_standard_types.h"
#include "jansson.h"
#include "rflc_scenes.h"
#else
typedef uint16_t     socklen_t;
#endif

#ifdef K64
typedef bool_t       boolean;

typedef int8_t       int_8;

typedef uint8_t      uint_8;
typedef uint8_t      byte;
typedef uint8_t      uchar;

typedef int16_t      int_16;

typedef uint16_t     uint_16;

typedef int32_t      int_32;

typedef uint32_t     uint_32;
typedef uint32_t     uint32;

typedef void *       pointer;
      
#define _PTR_        *
#endif

enum
{
   RS232_RECEIVE_TASK = 1,
   RFM100_RECEIVE_TASK,
   RFM100_TRANSMIT_TASK,
   EVENT_CONTROLLER_TASK,
   SOCKET_TASK,
   JSON_ACCEPT_TASK,
   WATCH_ACCEPT_TASK,
   FIRMWARE_UPDATE_TASK,
   MDNS_TASK,
   JSON_TRANSMIT_TASK,
   SHELL_TASK,
   LOGGING_TASK,
   EVENT_TIMER_TASK,
   USB_TASK,
   ELIOT_MQTT_TASK,
   ELIOT_REST_TASK,

   NUM_TASKS_PLUS_ONE // This must always be the last entry in this enum
};

//-----------------------------------------------------------------------------

// Values for json_socket_struct.jsa_state:
#define JSA_MASK_JSON_ENABLED            0x80
#define JSA_MASK_JSON_LIMITED            0x40

#define JSA_STATE_IDLE                   0x00
#define JSA_STATE_START                  0x01
#define JSA_STATE_CHALLENGE_SENT         0x02
#define JSA_STATE_AUTH_RECEIVED          0x03
#define JSA_STATE_AUTH_DENIED            0x04
#define JSA_STATE_LOW_MEM                0x05

#define JSA_STATE_AUTH_DEFEATED          (0x06 | JSA_MASK_JSON_ENABLED)
#define JSA_STATE_AUTH_VALID             (0x07 | JSA_MASK_JSON_ENABLED)
#define JSA_STATE_REQUIRE_SET_KEY        (0x08 | JSA_MASK_JSON_ENABLED | JSA_MASK_JSON_LIMITED)

#define JSA_VALID_TIME_MIN               1575158400    // 12/01/2019 @ 12:00am (UTC)

// JSON socket authentication parameters
#define JSA_GREETING                     "Hello V1 "
#define JSA_OUT_OF_MEMORY_MESSAGE        "LowMem"
#define JSA_AUTH_OK_MESSAGE              "[OK]\n\r\n"
#define JSA_AUTH_DENIED_MESSAGE          "[INVALID]"
#define JSA_AUTH_SET_KEY_MESSAGE         "[SETKEY]"
// dummy string to send after SET_KEY and MAC address packets for JSA_STATE_REQUIRE_SET_KEY.
// note that the string length is critical for mitigating an app bug related to receive packet sizing.
#define JSA_AUTH_SET_KEY_PADDING_MESSAGE "123456789012345678901234"
#define JSA_EVAL_WAIT_SEC                2

typedef struct json_socket_auth_struct
{
	byte_t challenge_phrase[16];
	byte_t challenge_response[16];
	byte_t rx_count;
	uint32_t event_timer;
} json_socket_auth;


typedef struct json_socket_struct
{
//   byte_t   jsonReceiveMessage[400];
   byte_t * jsonReceiveMessage;
   word_t   jsonReceiveMessageAllocSize;
   dword_t  JSONSocketConnection;
   word_t   jsonReceiveMessageIndex;
   byte_t   jsonReceiveMessageOverrunFlag;
   byte_t   needsAttachSock;
   boolean  WatchSocket;  //TRUE This is an Apple Watch
   boolean  httpSocket;   // TRUE is this is an HTTP Socket - determined by message
   byte_t   jsa_state;    // JSON socket authentication state
} json_socket;


typedef enum 
{
   NO_TYPE                          = 0x00,
   DIMMER_TYPE                      = 0x41,
   SWITCH_TYPE                      = 0x42,
   FAN_CONTROLLER_TYPE              = 0x43,
   NETWORK_REMOTE_TYPE              = 0x21,
   SCENE_CONTROLLER_4_BUTTON_TYPE   = 0x2B,
#ifdef SHADE_CONTROL_ADDED
   SCENE_TYPE                       = 0x7F,   // Used for Samsung Cloud Devices
   SHADE_TYPE                       = 0xf0,
   SHADE_GROUP_TYPE                 = 0xf1
#else
   SCENE_TYPE                       = 0x7F    // Used for Samsung Cloud Devices
#endif
} Dev_Type;

//-----------------------------------------------------------------------------
// this type will contain the actual values associated with the device

typedef struct zone_properties_struct
{
   uint32_t AppContextId;
   word_t   groupId;
   boolean  state;  // whether either Dimmer or Switch are On or Off (use of bitmask could save 87 bytes)
   boolean  targetState; // (use of bitmask could save 87 bytes)
   byte_t   powerLevel;  // currently set level
   byte_t   targetPowerLevel;
   byte_t   rampRate;  // currently set ramp rate
   Dev_Type deviceType;  // Dimmer or Switch
   byte_t   buildingId;
   byte_t   houseId;
   byte_t   roomId;
   char ZoneNameString[MAX_SIZE_NAME_STRING];
   char EliotDeviceId[ELIOT_DEVICE_ID_SIZE];
} zone_properties;

typedef struct nonvolatile_zone_properties_struct
{
   char ZoneNameString[MAX_SIZE_NAME_STRING];
   word_t groupId;
   Dev_Type deviceType;
   byte_t rampRate;
   byte_t buildingId;
   byte_t houseId;
   byte_t roomId;
} nonvolatile_zone_properties;

typedef struct nonvolatile_eliot_zone_properties_struct
{
   char EliotDeviceId[ELIOT_DEVICE_ID_SIZE];
} nonvolatile_eliot_zone_properties;

typedef struct volatile_zone_properties_struct
{
   uint32_t AppContextId;
   bool_t state; // (use of bitmask could save 87 bytes)
   bool_t targetState; // (use of bitmask could save 87 bytes)
   byte_t powerLevel; 
   byte_t targetPowerLevel;
   nonvolatile_zone_properties * nvZoneProperties;
   nonvolatile_eliot_zone_properties * nvEliotZoneProperties;
} volatile_zone_properties;

typedef struct sent_message_struct
{
   TIME_STRUCT timeSent;
   word_t groupId;
   byte_t buildingId;
   byte_t houseId;
   byte_t powerLevel;
} sent_message;

typedef struct location_dms_struct
{
   int16_t degrees;
   uint8_t minutes;
   uint8_t seconds;
} locationDMS_t;


typedef struct json_socket_key_struct
{
	union
	{
		uint8_t b[16];
		uint16_t w[8];
	} content;
	uint16_t checksum; 	// Used only when storing to flash
} json_socket_key;


typedef struct system_properties_struct
{
   char locationInfoString[128];
   locationDMS_t latitude;
   locationDMS_t longitude;
   int32_t gmtOffsetSeconds;

   // This field is the effective GMT offset based on daylight saving time
   int32_t effectiveGmtOffsetSeconds;
   bool_t addALight;
   bool_t useDaylightSaving;
   bool_t configured;
   bool_t addASceneController;
} system_properties;

// These arbitrary values are written to some newly allocated members of the
// extended_system_properties_struct to define a state.  Corresponding locations
// in flash are very unlikely to already be set to one of these:
#define FACTORY_AUTH_INIT_ENABLED               0xA574  // Written to factory_auth_init
#define AUTH_EXEMPT                             0x4DF1  // Written to auth_exempt
#define AUTH_NOT_EXEMPT                         0xE72C  // Written to auth_exempt
// We need a method to determine if the signed firmware update metrics have been used
// to initiialize our LCM to the eliot cloud object.  For an LCM that had instatiated
// its eliot object previously, this new firmware should detect that this non-volatile
// flag is not initialzed - and delete its previous cloud instance and re-initialize.
#define SIGNED_FW_METRICS_INSTANTIATED_TO_ELIOT 0x2112  // Written to signedFWMetricsInstantiatedToEliot

// Struct size must equal 120 bytes - Enable this to confirm struct size in
// BroadcastDiagnostics/EspSize when making changes to this struct.  It
// will expose unwanted padding.
#define extended_system_properties_size_check 0
typedef struct extended_system_properties_struct
{
   // Formerly SAMI data, placeholder to maintain flash structure.
   // This data is not cleared by the reset button.
   char available_memory[66];	// Add single byte members above this and multibyte members below
   uint16_t signedFWMetricsInstantiatedToEliot;   // [ELIOT_SIGNED_FIRMMWARE_UPDATE_METRICS|undefined]
   uint16_t factory_auth_init; // [FACTORY_AUTH_INIT_ENABLED|disabled]
   uint16_t auth_exempt; // [AUTH_EXEMPT|AUTH_NOT_EXEMPT|undefined]
   uint32_t first_time_recorded; // First time when the LCM has at least one zone, written only once.
   uint32_t first_time_recorded_complement; // == ~first_time_recorded when valid
   char mobileAppData[40];
} extended_system_properties;

typedef struct system_shade_support_properties_struct
{
   bool_t addAShade;
} system_shade_support_properties;

typedef struct zone_in_scene_struct
{
   byte_t       zoneId;
   byte_t		 powerLevel;
   byte_t		 rampRate;
   boolean      state;
} zone_in_scene;

typedef struct scene_properties_struct
{
   byte_t         numZonesInScene;
   zone_in_scene  zoneData[MAX_ZONES_IN_SCENE];
   uint32_t       nextTriggerTime;
#if (!defined(AMDBUILD) || !__cplusplus)
   FrequencyType  freq;
   TriggerType    trigger;
   DailySchedule  dayBitMask;
#else
   legrand::rflc::scenes::FrequencyType freq;
   legrand::rflc::scenes::TriggerType trigger;
   legrand::rflc::scenes::DailySchedule dayBitMask;
#endif
   signed char    delta;
   boolean        running;
   boolean        skipNext;
   char           SceneNameString[MAX_SIZE_NAME_STRING];   
   char           EliotDeviceId[ELIOT_DEVICE_ID_SIZE];
} scene_properties;

typedef struct nonvolatile_scene_properties_struct
{
   byte_t         numZonesInScene;
   zone_in_scene  zoneData[MAX_ZONES_IN_SCENE];
   uint32_t       nextTriggerTime;
#if (!defined(AMDBUILD) || !__cplusplus)
   FrequencyType  freq;
   TriggerType    trigger;
   DailySchedule  dayBitMask;
#else
   legrand::rflc::scenes::FrequencyType freq;
   legrand::rflc::scenes::TriggerType trigger;
   legrand::rflc::scenes::DailySchedule dayBitMask;
#endif
   signed char    delta;
   boolean        skipNext;
   char           SceneNameString[MAX_SIZE_NAME_STRING];   
} nonvolatile_scene_properties;

typedef struct nonvolatile_eliot_scene_properties_struct
{
   char EliotDeviceId[ELIOT_DEVICE_ID_SIZE];
} nonvolatile_eliot_scene_properties;

typedef struct volatile_scene_properties_struct
{
   boolean running;
   nonvolatile_scene_properties * nvSceneProperties;
   nonvolatile_eliot_scene_properties * nvEliotSceneProperties;
} volatile_scene_properties;

typedef struct scene_controller_bank_struct
{
   signed char       sceneId; //signed_byte_t     sceneId;
   bool_t            toggleable; 
   bool_t            toggled;
   
} scene_controller_bank;

//-----------------------------------------------------------------------------

typedef struct scene_controller_properties_struct
{
   char                    SceneControllerNameString[MAX_SIZE_NAME_STRING];
   scene_controller_bank   bank[MAX_SCENES_IN_SCENE_CONTROLLER];
   word_t                  groupId;
   byte_t                  buildingId;
   byte_t                  houseId;
   byte_t                  roomId;
   bool_t                  nightMode;
   Dev_Type                deviceType;
} scene_controller_properties;

typedef struct nonvolatile_scene_controller_properties_struct
{
   char                    SceneControllerNameString[MAX_SIZE_NAME_STRING];
   scene_controller_bank   bank[MAX_SCENES_IN_SCENE_CONTROLLER];
   word_t                  groupId;
   byte_t                  buildingId;
   byte_t                  houseId;
   byte_t                  roomId;
   bool_t                  nightMode;   // True: if scene controller can be set to Night Mode
   Dev_Type                deviceType;
   
} nonvolatile_scene_controller_properties;

typedef struct volatile_scene_controller_properties_struct
{
   nonvolatile_scene_controller_properties * nvSceneControllerProperties;
} volatile_scene_controller_properties;

//-----------------------------------------------------------------------------

typedef enum 
{
   RAMP                     = 0x0005,    // set to ramp the power to a certain level
   CANCEL_RAMP              = 0x0006,
   
   ///// New Scene Controller Commands - 6-29-15 - below
   ASSIGN_SCENE                  = 0x002A,
   SCENE_REQUESTED               = 0x002B,
   SCENE_EXECUTED                = 0x002C,
   SCENE_ADJUST                  = 0x002D,
   SCENE_CTL_SETTING             = 0x002E,
   SCENE_CONTROLLER_MASTER_OFF   = 0x002F,
   ///// New Scene Controller Commands - above

   PROPORTIONATE_RAMP_UP    = 0x3100,
   PROPORTIONATE_RAMP_DOWN  = 0x3101,
   CANCEL_PROPORTIONATE_RAMP = 0x3102,
   IDENTIFY              = 0x3003,
   INQUIRE               = 0x3019,
   SCAN_INQUIRE          = 0x301A,
   IN_USE                = 0x000D,
   OPEN_BINDING          = 0x3013,
   CLOSE_BINDING         = 0x3014,
   BUTTON_PRESS          = 0x3017,
   RECORD_PRESET         = 0x0031,
   GO_TO_PRESET          = 0x0000,
   OVERRIDE_TO_PRESET    = 0x3104,
   REVERT_OVERRIDE       = 0x3104,
   OPEN_CONFIG           = 0x3015,
   CLOSE_CONFIG          = 0x3016,
   STATUS                = 0x0013,
   STATUS_QUERY          = 0x3004,
   BINDING_QUERY         = 0x301D,   // used to retrieve addresses of devices
   BINDING_REPLY         = 0x000F,   // get this from devices when queried
   READ_EEPROM           = 0x301D,
   READ_SRAM             = 0x301C,
   READ_PM               = 0x301B,
   MEMORY_DATA           = 0x000E,
   WRITE_COMM_DATA_STRUCTURE = 0x0010,
   SET_DSM_LEVEL         = 0x3106,
   SNAP_SHOT             = 0x3018,
   EXCEPTION             = 0x0015,
   INVALID               = 0xFFFF
} TopDogCommand;

//-----------------------------------------------------------------------------

typedef struct mtm_input_sense_properties_struct
{
   onqtimer_t     inputSenseTimerTicks;  
   large_timer_t  inputSenseDebounceCount;
   byte_t         inputSenseFlag;
} mtm_input_sense_properties;

//-----------------------------------------------------------------------------

typedef struct JSON_transmit_message_struct
{
   char *   messageStringPtr;
   dword_t  socketBitmask;
   word_t   messageLength;
} JSON_transmit_message;

//-----------------------------------------------------------------------------

typedef struct {
   MESSAGE_HEADER_STRUCT   HEADER;
   word_t                  messageIndex;
} SOCKET_MONITOR_MESSAGE, _PTR_ SOCKET_MONITOR_MESSAGE_PTR;

//-----------------------------------------------------------------------------

typedef struct {
   MESSAGE_HEADER_STRUCT   HEADER;

   json_t * serverStringObj;
   json_t * versionBranchStringObj;

   byte_t updateCommand;

} UPDATE_TASK_MESSAGE, _PTR_ UPDATE_TASK_MESSAGE_PTR;

//-----------------------------------------------------------------------------

typedef struct {
   MESSAGE_HEADER_STRUCT   HEADER;

   json_t * jsonObj;
   byte_t   command;
   byte_t   index;

} ELIOT_TASK_MESSAGE, _PTR_ ELIOT_TASK_MESSAGE_PTR;

//-----------------------------------------------------------------------------

typedef struct {
   MESSAGE_HEADER_STRUCT   HEADER;
   byte_t                  receivedByte;
} RS232_RECEIVE_TASK_MESSAGE, _PTR_ RS232_RECEIVE_TASK_MESSAGE_PTR;

//-----------------------------------------------------------------------------

typedef struct {
   MESSAGE_HEADER_STRUCT   HEADER;
   byte_t                  ackByte;
} RFM100_TRANSMIT_ACK_MESSAGE, _PTR_ RFM100_TRANSMIT_ACK_MESSAGE_PTR;

//-----------------------------------------------------------------------------

typedef struct {
   MESSAGE_HEADER_STRUCT   HEADER;
   word_t                  command;
   word_t                  groupId;
   byte_t                  buildingId;
   byte_t                  houseId;
   byte_t                  sceneId;
   byte_t                  scenePercent;
   byte_t                  buttonIndex;    
   byte_t                  avgLevel;
   byte_t                  value;
   bool_t                  toggled;
   bool_t                  toggleable;
   bool_t                  state;
} RFM100_TRANSMIT_TASK_MESSAGE, _PTR_ RFM100_TRANSMIT_TASK_MESSAGE_PTR;

//-----------------------------------------------------------------------------

typedef struct firmware_info_struct
{
   word_t   S19LineCount;
   word_t   firmwareVersion[4];
//   char     versionRevString[10];
   char     versionHash[20];
   char     dateString[20];
   char     branchString[20];
   char     releaseNotes[MAX_SIZE_OF_UPDATE_RELEASE_NOTES_STRING];
   char     sha256HashString[MAX_SIZE_OF_UPDATE_SHA256_HASH_STRING];
   bool     reportSha256Flag;    // will be set to true when the hash string is to be reported
   char     updateHost[MAX_SIZE_OF_UPDATE_SERVER_STRING];
   char     updatePath[MAX_SIZE_OF_UPDATE_PATH_STRING];
} firmware_info;

//-----------------------------------------------------------------------------

typedef enum 
{
   REST_API_NO_ERROR = 0,
   REST_API_AUTHORIZATION_ERROR = 401,
   WEBSOCKET_USER_AUTHORIZATION_ERROR = 403,
   WEBSOCKET_AUTHORIZATION_ERROR = 404,
   REST_API_QUOTA_EXCEEDED_ERROR = 429,
   REST_API_OTHER_ERROR = 1,
   REST_API_UNEXPECTED_ERROR = 2
} Rest_API_Error_Type;

//-----------------------------------------------------------------------------

#ifdef SHADE_CONTROL_ADDED
typedef struct shade_properties_struct
{
   char idString[MAX_SIZE_QMOTION_DEVICE_ID_STRING];
} shade_properties;

typedef struct nonvol_shade_properties_struct
{
   shade_properties  shadeInfo[MAX_NUMBER_OF_ZONES];
   byte_t            zoneIsShadeGroupBitArray[ZONE_BIT_ARRAY_BYTES]; // This indicates if the zone is a shade group that was first discovered under 3.0.x firmware (13 bytes instead of 100)
} nonvol_shade_properties;
#endif

//-----------------------------------------------------------------------------

typedef enum 
{
    LED1_GREEN_FUNCTION_DEFAULT = 0,
    LED1_GREEN_FUNCTION_FREE_MEMORY,
    LED1_GREEN_FUNCTION_TASK_HEARTBEAT_BITMASK,
    LED1_GREEN_FUNCTION_TOTAL
} LED1_Function_Enum;

#endif /* TYPEDEFS_H_ */
