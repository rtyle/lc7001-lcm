/*
 * defines.h
 *
 *  Created on: Jun 13, 2013
 *      Author: Legrand
 */

#ifndef DEFINES_H_
#define DEFINES_H_

//#define K60DN512
#define K64

#define DIAGNOSTICS_HEARTBEAT
//#define SIMPLE_HEARTBEAT
//#define KNIGHT_RIDER_HEARTBEAT

//#define MEMORY_DIAGNOSTIC_PRINTF

// uncomment for shade control
#define SHADE_CONTROL_ADDED

#define HOUSE_WITH_MULT_SCTL

#define MAX_SIZE_ZID_STRING                           12

#define TIME_BETWEEN_ANNOUNCEMENTS                    100
#define TIME_TO_WAIT_FOR_SOCKET_MONITOR_CLEAR         10000
#define MAX_SOCKET_QUIET_TIME                         5000

#define SOCKET_RECEIVE_BUFFER_ALLOC_CHUNK             0x200
#define SOCKET_RECEIVE_BUFFER_MAX_SIZE                0x1000

//-----------------------------------------------------------------------------

#define NUMBER_OF_JSON_PACKETS_TO_SEND_PER_CYCLE   2

//-----------------------------------------------------------------------------

// total number of messages allowed to be simultaneously queued = (MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS * NUMBER_OF_JSON_TRANSMIT_MESSAGES_IN_QUEUE) 
#define NUMBER_OF_JSON_TRANSMIT_MESSAGES_IN_QUEUE     11 // orig=10

#define ELIOT_DEVICE_ID_SIZE                          37
#define ELIOT_PRIMARY_KEY_SIZE                        48
#define ELIOT_TOKEN_SIZE                              1536
#define ELIOT_LAST_SEEN_SIZE                          20
#define ELIOT_MAX_VERSION_SIZE                        19

#define SERVER_PING_TIMEOUT_SECONDS           301 // bumped from 61 
#define MAX_SIZE_NAME_STRING                          21
#define MAX_SIZE_GUID_STRING                          41
#define MAX_SIZE_IP_ADDRESS_STRING                    16

#define MTM_INPUT_ACTIVE_DEBOUNCE_TIME                25
#define MTM_INPUT_INACTIVE_DEBOUNCE_TIME              25

#define MAX_NUMBER_OF_TIMEOUTS_FOR_UPDATE_RESPONSE    5
#define MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS       7

#define InitializeGPIOPoint(A,B,C,D)               lwgpio_init(A,B,C,D)
#define SetGPIOOutput(outputPoint, state)          (state) ? lwgpio_set_value(&outputPoint, LWGPIO_VALUE_HIGH):lwgpio_set_value(&outputPoint, LWGPIO_VALUE_LOW);             

#define SetBit(variable,bitmask)                   variable |= bitmask
#define CheckForBitSet(variable,bitmask)           (variable & bitmask)

//#define SetZoneBitmask(zoneBitmask,mask)           zoneBitmask |= mask
//#define ResetZoneBitmask(zoneBitmask,mask)         zoneBitmask &= (~((dword_t) mask))
//#define CheckIfInZoneBitmask(zoneBitmask,mask)     (zoneBitmask & ((dword_t) mask))

#define RFM100_MESSAGE_HIGH_PRIORITY 1
#define RFM100_MESSAGE_LOW_PRIORITY 0

#define RFM100_ACK_BYTE       0xC5
#define RFM100_NAK_BYTE       0xC6

#define GREEN_LED1_UPDATE_TIMER       10000
#define IPCFG_TIMER           1000
#define FLASH_SAVE_TIMER      2000
#define FACTORY_RESET_TIMER   5000
#define PASSWORD_RESET_TIMER  15000
#define TOGGLE_CLOUD_TIMER    5000

#define CANARY_BYTE_STRING       "MM7" // return to this when we handle updates cleanly (without forcing a flash erase)
//#define CANARY_BYTE_STRING       "MM8"

#define MILLISECOND_DELAY_FOR_RFM100_RECEIVE_ABORT      2

#define IDLE                     0
#define RECEIVING_RF100_LENGTH   1
#define RECEIVING_RF100_RSSI     2
#define RECEIVING_RF100_PAYLOAD  3

#define MAX_NUMBER_OF_DEVICES    20

#define MAX_NUMBER_OF_ZONES      100  
#define MAX_ZONES_IN_SCENE       100
#define MAX_NUMBER_OF_SCENES	   100

#define ZONE_BIT_ARRAY_BYTES     ((MAX_NUMBER_OF_ZONES+7)/8)
#define SCENE_BIT_ARRAY_BYTES     ((MAX_NUMBER_OF_SCENES+7)/8)
#define GetZoneBit(p,n)    ((p[n/8]) & ((1) << (n%8)))
#define ClearZoneBit(p,n)  ((p[n/8]) &= ~((1) << (n%8)))  
#define SetZoneBit(p,n)    ((p[n/8]) |= ((1) << (n%8))) 

#ifdef HOUSE_WITH_MULT_SCTL
#define MAX_NUMBER_OF_SCENE_CONTROLLERS  40

#else
#define MAX_NUMBER_OF_SCENE_CONTROLLERS  20
#endif

#define MAX_SCENES_IN_SCENE_CONTROLLER   4

#define MASTER_OFF_FOR_SCENE_CONTROLLER 100

#define SLOW_DEACTIVATION_RAMP_VALUE    3

#define PERCENT_CHANGE_FOR_ADJUST       5     

//------ TOP DOG CONSTANTS -----------------
#define   TOP_DOG_BAUD_RATE 38400
#define   TOPDOG_F1         0x01 //0x20
#define   TOPDOG_F2         0x06 //0xc0
#define   TOPDOG_CADIR      0x04 //0x80

#define   START_BYTE 0xb2


#define   TDAM_BROADCAST      0
#define   TDAM_ANON_MULTICAST 0x01 
#define   TDAM_UNICAST        0x02 
#define   TDAM_MULTICAST      0x03

#define   SHORT_RANDOM_TIME    0
#define   LONG_RANDOM_TIME     4

#define   TTL_ZERO    0
#define   TTL_ONE     1
#define   TTL_TWO     2
#define   TTL_THREE   3

#define   MAX_PWR_LEVEL 0xff
#define   RAMP_FUNCTION_BYTE  0x85
#define   ASSIGN_SCENE_FUNCTION_BYTE     0xAA
#define   SCENE_EXECUTED_FUNCTION_BYTE   0xAC
#define   SCENE_CTL_SET_FUNCTION_BYTE    0xAE

#define   SECONDS_PER_DAY 86400

#define   MIN_SYSTEM_TIME       1429810320 // UTC: Thursday 23rd April 2015 05:32:00 PM
#define   MAX_SYSTEM_TIME       2147483647 // UTC: Tuesday 19th January 2038 03:14:07 AM
#define   DEFAULT_SYSTEM_TIME   MIN_SYSTEM_TIME

//---------------------------------------------------------------------------


#define OPERATING_FLAGS_POWER_PROPERTY_BITMASK                 0x00000001
#define SOURCE_PROPERTY_BITMASK                                0x00000002
#define VOLUME_PROPERTY_BITMASK                                0x00000004
#define MAXIMUM_VOLUME_PROPERTY_BITMASK                        0x00000008
#define TURN_ON_VOLUME_PROPERTY_BITMASK                        0x00000010
#define INDIVIDUAL_SETTINGS_FLAGS_PROPERTY_BITMASK             0x00000020
#define COMMON_SETTINGS_FLAGS_PROPERTY_BITMASK                 0x00000040
#define BRIGHTNESS_PROPERTY_BITMASK                            0x00000080
#define TIMEOUT_PROPERTY_BITMASK                               0x00000100
#define OPERATING_FLAGS_MUTE_PROPERTY_BITMASK                  0x00000200
#define OPERATING_FLAGS_DND_PROPERTY_BITMASK                   0x00000400

#define OPERATING_FLAGS_PROPERTY_BITMASK                       0x00000601
	
#define COMMON_ANALOG_AUDIO_BOOST_SETTINGS_PROPERTY_BITMASK    0x00000800

#define ALL_PROPERTIES_BITMASK                                 0x00000037  // don't include ramp rate
#define NAME_PROPERTIES_BITMASK                                0x00000001
#define DEV_PROPERTIES_BITMASK                                 0x00000002
#define PWR_LVL_PROPERTIES_BITMASK                             0x00000004
#define RAMP_RATE_PROPERTIES_BITMASK                           0x00000008
#define PWR_BOOL_PROPERTIES_BITMASK                            0x00000010
#define CLOUD_ID_PROPERTIES_BITMASK                            0x00000020

#define ALL_SYSTEM_PROPERTIES_BITMASK                          0x00003FFF
#define SYSTEM_PROPERTY_ADD_A_LIGHT_BITMASK                    0x00000001
#define SYSTEM_PROPERTY_TIME_ZONE_BITMASK                      0x00000002
#define SYSTEM_PROPERTY_DAYLIGHT_SAVING_TIME_BITMASK           0x00000004
#define SYSTEM_PROPERTY_LATITUDE_BITMASK                       0x00000008
#define SYSTEM_PROPERTY_LONGITUDE_BITMASK                      0x00000010
#define SYSTEM_PROPERTY_LOCATION_INFO_BITMASK                  0x00000020
#define SYSTEM_PROPERTY_EFFECTIVE_TIME_ZONE_BITMASK            0x00000040
#define SYSTEM_PROPERTY_CONFIGURED_BITMASK                     0x00000080
#define SYSTEM_PROPERTY_NEW_TIME_BITMASK                       0x00000100
#define SYSTEM_PROPERTY_ADD_A_SCENE_CONTROLLER_BITMASK         0x00000200
#define SYSTEM_PROPERTY_SUPPORT_FADE_RATE_BITMASK              0x00000400
#define SYSTEM_PROPERTY_BEARER_ID_BITMASK                      0x00000800
#define SYSTEM_PROPERTY_REFRESH_ID_BITMASK                     0x00001000
#define SYSTEM_PROPERTY_MOBILE_APP_DATA_BITMASK                0x00002000
#define SYSTEM_PROPERTY_ADD_A_SHADE_BITMASK                    0x00008000



#define MAX_TD_POWER 0xff
#define MIN_TD_POWER 0

#define MAX_FREQS       8
#define MAX_TRIGS       3
#define MAX_DELTA_VALUE 120
#define MIN_DELTA_VALUE -120
#define MAX_POWER_LEVEL 100
#define MAX_RAMP_RATE   255
#define DEFAULT_RAMP_RATE 50
//-----------------------------------------------------------------------------

#define TIMEOUT_NEVER                        0
#define TIMEOUT_10_MINUTES                   1
#define TIMEOUT_30_MINUTES                   2
#define TIMEOUT_1_HOUR                       3
#define TIMEOUT_2_HOURS                      4
#define TIMEOUT_4_HOURS                      5
#define TIMEOUT_MAX_INDEX                    5

//-----------------------------------------------------------------------------

// SetProperty instance
#define ZONE_INFO_ATTRIBUTES_BITMASK      0xB0
#define RELATIVE_VOLUME                   0xB1
#define QLINK_MSG_TYPE_SET_VOLUME_MULT    0xB2

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define UPDATE_TASK_MESSAGE_QUEUE                        8
#define NUMBER_OF_UPDATE_MESSAGES_IN_POOL                8

#define RS232_RECEIVE_TASK_MESSAGE_QUEUE                 9
#define NUMBER_OF_RS232_RECEIVE_MESSAGES_IN_POOL         64

#define RFM100_TRANSMIT_ACK_MESSAGE_QUEUE                10
#define NUMBER_OF_RFM100_TRANSMIT_ACK_MESSAGES_IN_POOL   2

#define RFM100_TRANSMIT_TASK_MESSAGE_QUEUE               11
#define NUMBER_OF_RFM100_TRANSMIT_MESSAGES_IN_POOL       200

#define SOCKET_MONITOR_MESSAGE_SERVER_QUEUE_BASE         14


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define ELIOT_TASK_MESSAGE_QUEUE                         12
#define MQTT_TASK_MESSAGE_QUEUE                          13
#define NUMBER_OF_ELIOT_MESSAGES_IN_POOL                 36 // orig=32

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define ELIOT_COMMAND_ADD_ZONE                           0
#define ELIOT_COMMAND_ZONE_CHANGED                       1
#define ELIOT_COMMAND_DELETE_ZONE                        2
#define ELIOT_COMMAND_ADD_SCENE                          3
#define ELIOT_COMMAND_SCENE_STARTED                      4
#define ELIOT_COMMAND_RENAME_ZONE                        5
#define ELIOT_COMMAND_RENAME_SCENE                       6
#define ELIOT_COMMAND_SCENE_FINISHED                     7


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define NUM_SCENES_IN_FLASH_SECTOR   9

#define NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR  50
#define NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR  50

#define FLASH_UPPER_BLOCK_START_ADDRESS                  0x80000
#define LCM_DATA_STARTING_BLOCK                          0xdb000

#define LCM_DATA_REFRESH_TOKEN_OFFSET                    0
#define LCM_DATA_PRIMARY_KEY_OFFSET                      LCM_DATA_REFRESH_TOKEN_OFFSET+ELIOT_TOKEN_SIZE
#define LCM_DATA_PLANT_ID_OFFSET                         LCM_DATA_PRIMARY_KEY_OFFSET+ELIOT_PRIMARY_KEY_SIZE
#define LCM_DATA_JSON_SOCKET_USER_KEY_OFFSET             LCM_DATA_PLANT_ID_OFFSET+ELIOT_DEVICE_ID_SIZE // 18 bytes

#define LCM_DATA_ELIOT_BANK_0_START_ADDRESS              0xdb000
#define LCM_DATA_ELIOT_BANK_1_START_ADDRESS              0xdc000

#define LCM_DATA_ZONES_FLASH_START_ADDRESS               0xdd000
#define LCM_DATA_ZONES_FLASH_END_ADDRESS                 0xddfff

#define LCM_DATA_SCENES_FLASH_START_ADDRESS              0xde000
#define LCM_DATA_SCENES_FLASH_END_ADDRESS                0xe9fff

#define LCM_DATA_SCENE_CONTROLLERS_FLASH_START_ADDRESS   0xea000
#define LCM_DATA_SCENE_CONTROLLERS_FLASH_END_ADDRESS     0xeafff

#define LCM_DATA_ELIOT_ZONE_PROPERTIES_START_ADDRESS   0xeb000
#define LCM_DATA_ELIOT_ZONE_PROPERTIES_END_ADDRESS     0xecfff

#define LCM_DATA_ELIOT_SCENE_PROPERTIES_START_ADDRESS  0xed000
#define LCM_DATA_ELIOT_SCENE_PROPERTIES_END_ADDRESS    0xedfff

#ifdef SHADE_CONTROL_ADDED
#define LCM_DATA_SHADE_ID_START_ADDRESS                  0xef000
#define LCM_DATA_SHADE_ID_END_ADDRESS                    0xf0fff
#endif

#define SYSTEM_PROPERTIES_FLASH_ADDRESS                  0xfe000
#define SYSTEM_PROPERTIES_FLASH_CRC_ADDRESS              0xfeff0
#define FLASH_UPPER_BLOCK_END_ADDRESS                    0xff000

#define FLASH_LOWER_BLOCK(x)                             (x & (~0x80000))

//-----------------------------------------------------------------------------

#define MAX_NUMBER_OF_IP_ADDRESS_RESOLUTION_ATTEMPTS    3

//-----------------------------------------------------------------------------

#define UPDATE_STATE_NONE           0
#define UPDATE_STATE_DOWNLOADING    1
#define UPDATE_STATE_READY          2
#define UPDATE_STATE_INSTALLING     3

#define NUMBER_OF_PINGS_BEFORE_UPDATE_READY_ALERT     6

//-----------------------------------------------------------------------------

#define UPDATE_COMMAND_CHECK_FOR_UPDATE      0     // instructs the update task to look for a later version.  This command will use
                                                   // the existing firmware's branch info to query latest-branch.  If the response
                                                   // indicates a newer version available than the existing firmware, we will
                                                   // change our state to UPDATE_STATE_DOWNLOADING and begin downloading the
                                                   // latest firmware in our branch.
#define UPDATE_COMMAND_DOWNLOAD_FIRMWARE     1
#define UPDATE_COMMAND_INSTALL_FIRMWARE      2
#define UPDATE_COMMAND_DISCARD_UPDATE        3     // this command works similarly to UPDATE_COMMAND_CHECK_FOR_UPDATE, except that
                                                   // it will NOT clear any directed update data.  So if the previous update had
                                                   // been to a directed branch, we will re-download that same branch.

//-----------------------------------------------------------------------------

#define MAX_SIZE_OF_UPDATE_SERVER_STRING           64
#define MAX_SIZE_OF_UPDATE_VERSION_BRANCH_STRING   64
#define MAX_SIZE_OF_UPDATE_RELEASE_NOTES_STRING    200
#define MAX_SIZE_OF_UPDATE_SHA256_HASH_STRING      65  // There are 256 bits (32 bytes) in a SHA256 hash
                                                       // with each byte requring two ascii hex digits, plus a null terminator.
#define MAX_SIZE_OF_UPDATE_PATH_STRING             64
#define MAX_SIZE_OF_UPDATE_SSCANF_FORMAT_STRING    64

//-----------------------------------------------------------------------------

#define NUMBER_OF_RFM100_TRANSMIT_ATTEMPTS         3
#define RFM100_TRANSMIT_ACK_TIMEOUT                1000

//-----------------------------------------------------------------------------

// DEBUG_JUMPER is true when either a programming interface or a jumper
// between pins 3 and 4 is present on the programming header.
#define DEBUG_JUMPER                               (!lwgpio_get_value(&debugJumperInput))

#define LED_NUMBER_1                               0
#define LED_NUMBER_2                               1
#define LED_NUMBER_3                               2

// Amazon Cloud Server for remote connections 54.152.106.172
// TODO: Change the cloud server socket index to a value between 0 and 7 to use the cloud server implementation
#define CLOUD_SERVER_IP_ADDRESS              0x36986AAC
#define CLOUD_SERVER_PORT                    2525
#define CLOUD_SERVER_SOCKET_INDEX            8
#define CLOUD_SERVER_PING_TIMEOUT_SECONDS    12

#define NTP_SERVER   "pool.ntp.org"
//#define NTP_SERVER "europe.pool.ntp.org"
//#define NTP_SERVER "asia.pool.ntp.org"
//#define NTP_SERVER "oceania.pool.ntp.org"
//#define NTP_SERVER "north-america.pool.ntp.org"
//#define NTP_SERVER "south-america.pool.ntp.org"
//#define NTP_SERVER "africa.pool.ntp.org"
#define SNTP_REQUEST_TIMEOUT                 20000 // 20 seconds for reply

//-----------------------------------------------------------------------------

#define UTF8_2_BYTE_MASK   0xE0
#define UTF8_2_BYTE_TAG    0xC0

#define UTF8_3_BYTE_MASK   0xF0
#define UTF8_3_BYTE_TAG    0xE0

#define UTF8_4_BYTE_MASK   0xF8
#define UTF8_4_BYTE_TAG    0xF0

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define LED_ON                   false
#define LED_OFF                  true
#define CLOUD_SOCKET_RED_LED     led1Red
#define HOUSE_ID_NOT_SET         led2Red
#define POWER_LED                led2Green

#define NUMBER_OF_MTM_INPUTS  5
#define NUMBER_OF_MTM_OUTPUTS 6

//-----------------------------------------------------------------------------
// Long term timers
//-----------------------------------------------------------------------------

#define REFRESH_TOKEN_TIME              (24*60*60*1000)  // 24 hours
#define REFRESH_QUBE_ADDRESS_TIME       (2*60*1000)      // 2 minutes
#define INITIAL_UPDATE_CHECK_TIME       (90*1000)        // 90 seconds, one minute is a bad choice collides with Artik sync
#define SUBSEQUENT_UPDATE_CHECK_TIME    (24*60*60*1000)  // firmware update
#define WATCHDOG_TIME                   (5*60*1000)      // check watchdog counters every 5 minutes

/////////////////////////////////////////////////////////////////////
// These errors will be displayed to user, need translation in App
#define USR_NO_SPACE_LEFT_FOR_SCENE_ADDITION                 1000
#define USR_AddASceneController_NO_SLOTS_LEFT_IN_ARRAY       1001
#define USR_AddALight_NO_SLOTS_LEFT_IN_ARRAY                 1002
#define USR_FIRMWARE_IMAGE_IS_NOT_READY_TO_BE_INSTALLED      1003
#define USR_JSON_PACKET_EXCEEDS_MAX_BUFFER_SIZE              1004
#define USR_NO_ELEMENTS_IN_ZoneList_ARRAY                    1005
#define USR_TOO_MANY_ZONES_IN_THE_SCENE                      1006
#define USR_DUPLICATE_ZONE_NAME                              1007
#define USR_DUPLICATE_SCENE_NAME                             1008
#define USR_DUPLICATE_SCENE_CONTROLLER_NAME                  1009
#define USR_AddAShade_NO_SLOTS_LEFT_IN_ARRAY                 1010

////////////////////////////////////////////////////////////////////////
// These errors should not be seen by User, software to software only
#define SW_APP_CONTEXT_ID_ERROR                             2000
#define SW_BANK_COULD_NOT_GET_PROPERTIES_FOR_SCENE          2001
#define SW_BANK_SID_DOES_NOT_EXIST                          2002
#define SW_BANK_SID_MUST_BE_INTEGER                         2003
#define SW_BANK_TOGGLEABLE_MUST_BE_BOOLEAN                  2004
#define SW_BANK_TOGGLED_MUST_BE_BOOLEAN                     2005
#define SW_BANKINDEX_IS_NOT_INTEGER                         2006
#define SW_BANKINDEX_MUST_BE_BETWEEN                        2007
#define SW_BANK_ARRAY_HAS_AN_INVALID_SIZE                   2008
#define SW_BANK_HAS_TO_BE_AN_ARRAY                          2009
#define SW_BUILDING_ID_IS_NOT_AN_INTEGER                    2010
#define SW_COULD_NOT_GET_PROPERTIES_FOR_SCENE               2011
#define SW_COULD_NOT_GET_PROPERTIES_FOR_ZONE                2012
#define SW_COULD_NOT_GET_SCENE_PROPERTIES_FOR_SCENE_INDEX   2013
#define SW_COULD_NOT_GET_SCENE_PROPERTIES_FOR_ZONE          2014
#define SW_COULD_NOT_GET_ZONE_PROPERTIES_FOR_ZONE           2015
#define SW_COULD_NOT_TRANSMIT_ON_QUEUE                      2016
#define SW_DAYBITS_INVALID                                  2017
#define SW_DAYBITS_MUST_BE_AN_INTEGER                       2018
#define SW_DELTA_MUST_BE_BETWEEN                            2019
#define SW_DELTA_MUST_BE_AN_INTEGER                         2020
#define SW_DEVICE_TYPE_INVALID                              2021
#define SW_DEVICE_TYPE_IS_NOT_AN_INTEGER                    2022
#define SW_FREQUENCY_INDEX_TOO_GREAT                        2023
#define SW_FREQUENCY_MUST_BE_AN_INTEGER                     2024
#define SW_GROUPID_IS_NOT_AN_INTEGER                        2025
#define SW_HOUSEID_IS_NOT_AN_INTEGER                        2026
#define SW_ID_FIELD_IS_NOT_AN_INTEGER                       2027
#define SW_INVALID_PROPERTY_IN_LIST                         2028
#define SW_INVALID_PROPERTY_LIST                            2029
#define SW_LIGHT_REPORT_NO_ZONE_FOUND                       2030
#define SW_NEED_DAYBITS_WHEN_FREQ_IS_EVERY_WEEK             2031
#define SW_NEED_FREQ_EVERY_WEEK_WITH_DAYBITS                2032
#define SW_NO_BANK_INDEX_FIELD                              2033
#define SW_NO_DEVICE_TYPE_FIELD                             2034
#define SW_NO_GROUP_ID_FIELD                                2035
#define SW_NO_HOUSE_ID_FIELD                                2036
#define SW_NO_ID_FIELD                                      2037
#define SW_NO_POWER_LEVEL_FIELD                             2038
#define SW_NO_SCENE_ID_FIELD                                2039
#define SW_NO_SCID_FIELD                                    2040
#define SW_NO_SERVICE_FIELD                                 2041
#define SW_NO_STATE_FIELD                                   2042
#define SW_NO_ZONE_ID_FIELD                                 2043
#define SW_POWER_LEVEL_MUST_BE_BETWEEN                      2044
#define SW_POWER_LEVEL_IS_NOT_AN_INTEGER                    2045
#define SW_PROPERTY_LIST_ADDALIGHT_MUST_BE_BOOLEAN          2046
#define SW_PROPERTY_LIST_ADDASCENECTL_MUST_BE_BOOLEAN       2047
#define SW_PROPERTY_LIST_CAN_ONLY_CHANGE_DEVICE_TYPE_IF_NOT 2048
#define SW_PROPERTY_LIST_CONFIGURED_MUST_BE_BOOLEAN         2049
#define SW_PROPERTY_LIST_DAYLIGHTSAVINGS_MUST_BE_BOOLEAN    2050
#define SW_PROPERTY_LIST_DEG_IS_OUT_RANGE                   2051
#define SW_PROPERTY_LIST_DEG_MUST_BE_AN_INTEGER             2052
#define SW_PROPERTY_LIST_DEVICE_TYPE_MUST_BE_DIM_OR_SW      2053
#define SW_PROPERTY_LIST_DEVICE_TYPE_MUST_BE_A_STRING       2054
#define SW_PROPERTY_LIST_LOCATION_MUST_BE_AN_OBJECT         2055
#define SW_PROPERTY_LIST_LOCATION_PROPERTY_NOT_HANDLED      2056
#define SW_PROPERTY_LIST_LOCATION_INFO_MUST_BE_A_STRING     2057
#define SW_PROPERTY_LIST_MIN_IS_OUT_OF_RANGE                2058
#define SW_PROPERTY_LIST_MIN_MUST_BE_AN_INTEGER             2059
#define SW_PROPERTY_LIST_NAME_MUST_BE_A_STRING              2060
#define SW_PROPERTY_LIST_NIGHTMODE_MUST_BE_BOOLEAN          2061
#define SW_PROPERTY_LIST_NOT_ALL_LOCATION_OBJECTS_WERE_SET  2062
#define SW_PROPERTY_LIST_POWER_LEVEL_MUST_BE_INTEGER        2063
#define SW_PROPERTY_LIST_POWER_LEVEL_MUST_BE_BETWEEN        2064
#define SW_PROPERTY_LIST_POWER_MUST_BE_BOOLEAN              2065
#define SW_PROPERTY_LIST_PROPERTY_NOT_HANDLED               2066
#define SW_PROPERTY_LIST_RAMP_RATE_MUST_BE_BETWEEN          2067
#define SW_PROPERTY_LIST_RAMP_RATE_MUST_BE_AN_INTEGER       2068
#define SW_PROPERTY_LIST_SEC_IS_OUT_OF_RANGE                2069
#define SW_PROPERTY_LIST_SEC_MUST_BE_AN_INTEGER             2070
#define SW_PROPERTY_LIST_UNKNOWN_PROPERTY_NOT_HANDLED       2071
#define SW_S19_LINE                                         2072
#define SW_SCENE_DOES_NOT_EXIST                             2073
#define SW_SCENE_ID_IS_INVALID                              2074
#define SW_SCENE_ID_MUST_BE_AN_INTEGER                      2075
#define SW_SCENE_ID_DOES_NOT_EXIST                          2076
#define SW_SCENE_ID_MUST_BE_LESS_THEN                       2077
#define SW_SCID_IS_INVALID                                  2078
#define SW_SCID_MUST_BE_AN_INTEGER                          2089
#define SW_SERVICE_FIELD_IS_NOT_A_STRING                    2080
#define SW_SKIP_MUST_BE_BOOLEAN                             2081
#define SW_STATE_IS_NOT_BOOLEAN                             2082
#define SW_SWAP_FAILED                                      2083
#define SW_TIMEZONE_MUST_BE_INTEGER                         2084
#define SW_TRIGGER_INDEX_TOO_GREAT                          2085
#define SW_TRIGGER_TIME_MUST_BE_INTEGER                     2086
#define SW_TRIGGER_TYPE_MUST_BE_INTEGER                     2087
#define SW_UNABLE_TO_GET_PROPERTIES_FOR_SCENE               2088
#define SW_UNKNOWN_SERVICE                                  2089
#define SW_ZONE_ID_DOES_NOT_EXIST                           2090
#define SW_ZONE_ID_MUST_BE_LESS_THEN                        2091
#define SW_ZONE_ID_NOT_FOUND                                2092
#define SW_ZONE_ID_MUST_BE_AN_INTEGER                       2093
#define SW_ZONE_LIST_HAS_TO_BE_AN_ARRAY                     2094
#define SW_ZONE_LIST_COULD_NOT_GET_PROPERTIES_FOR_ZONE_ID   2095
#define SW_ZONE_LIST_POWER_LEVEL_MUST_BE_BETWEEN            2096
#define SW_ZONE_LIST_POWER_LEVEL_MUST_BE_AN_INTEGER         2097
#define SW_ZONE_LIST_RAMP_RATE_MUST_BE_BETWEEN              2098
#define SW_ZONE_LIST_RAMP_RATE_MUST_BE_AN_INTEGER           2099
#define SW_ZONE_LIST_STATE_MUST_BE_BOOLEAN                  2100
#define SW_ZONE_LIST_ZONE_ID_DOES_NOT_EXIST                 2101
#define SW_ZONE_LIST_ZONE_ID_DUPLICATE                      2102
#define SW_ZONE_LIST_ZONE_ID_MUST_BE_INTEGER                2103
#define SW_ZONE_LIST_ZONE_ID_MUST_BE_LESS_THEN              2104
#define SW_HOST_NOT_A_STRING                                2105
#define SW_BRANCH_NOT_STRING                                2106
#define SW_SERVICE_NOT_HANDLED_FOR_THIS_INTERFACE           2107
#define SW_NO_BUILDING_ID_FIELD                             2108
#define SW_BANK_TOGGLEABLE_MUST_BE_FALSE_FOR_MASTER_OFF     2109
#define SW_APP_ID_NULL                                      2110
#define SW_LCM_ID_NULL                                      2111
#define SW_APP_ID_MUST_BE_INTEGER                           2112
#define SW_LCM_ID_MUST_BE_STRING                            2113
#define SW_PROPERTY_LIST_ADDASHADE_MUST_BE_BOOLEAN          2114

///////////////////////////////////////////////////////////////////////////
// These errors are returned from manufacturing test functions to software
//    should never be seen by user
#define MTM_STATE_FIELD_NOT_BOOLEAN                      3000
#define MTM_HEADER_FIELD_NOT_AN_INTEGER                  3001
#define MTM_HEADER_FIELD_MUST_BE_BETWEEN                 3002
#define MTM_DESTINATION_NOT_AN_ARRAY                     3003
#define MTM_DESTINATION_ARRAY_INCORRECT_BYTE_COUNT       3004
#define MTM_DESTINATION_BYTE_NOT_AN_INTEGER              3005
#define MTM_DESTINATION_BYTE_IS_INVALID                  3006
#define MTM_PAYLOAD_NOT_AN_ARRAY                         3007
#define MTM_PAYLOAD_ARRAY_INVALID                        3008
#define MTM_PAYLOAD_BYTE_NOT_AN_INTEGER                  3009
#define MTM_PAYLOAD_BYTE_1_INVALID_SHOULD_BE_0_63        3010
#define MTM_PAYLOAD_BYTE_X_INVALID_SHOULD_BE_0_255       3011
#define MTM_OUTPUT_FIELD_NOT_A_STRING                    3012
#define MTM_UNKNOWN_OUTPUT_STRING                        3013
#define MTM_MAC_ADDRESS_NOT_AN_ARRAY                     3014
#define MTM_MAC_ADDRESS_ARRAY_DOES_NOT_CONTAIN_3_BYTES   3015
#define MTM_MAC_ADDRESS_BYTE_NOT_AN_INTEGER              3016
#define MTM_MAC_ADDRESS_BYTE_INVALID                     3017
#define MTM_FAILURE_ERASING_SAVING_FLASH                 3018
#define MTM_UNKNOWN_SERVICE                              3019


#ifdef SHADE_CONTROL_ADDED
#define MAX_SIZE_QMOTION_DEVICE_ID_STRING 40
#endif


#endif /* DEFINES_H_ */
