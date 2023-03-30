/*
 * Event_Task.c
 *
 *  Created on: Mar 31, 2015
 *      Author: Legrand
 */
#define TRY_OTHER_TIME 1
#define USE_NEW_SUNRISE_CALCULATOR  1

#include "includes.h"
#ifndef AMDBUILD
#include <stdlib.h>
#include <ipcfg.h>
#else
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include <math.h>
// #include <ansi_parms.h>
// #include <cmath>

#define MIN_IN_DAY 1440L
#define SUN_PACKET_TX_SIZE 750
#define SUN_PACKET_RX_SIZE 1028
static char sendPkt[SUN_PACKET_TX_SIZE];
static char recvPkt[SUN_PACKET_RX_SIZE*2];

#define TRY_AGAIN_FOR_BUFFER  1000

//=================
// Debug settings
//=================
#define DEBUG_EVENTTASK_ANY                     0 // check this in as 0
#define DEBUG_EVENTTASK                         (1 && DEBUG_EVENTTASK_ANY) 
#define DEBUG_EVENTTASK_SUNRISE                 (1 && DEBUG_EVENTTASK_ANY) 
#define DEBUG_EVENTTASK_SUNRISE_VERBOSE         (0 && DEBUG_EVENTTASK_ANY) 
#define DEBUG_EVENTTASK_VERBOSE                 (0 && DEBUG_EVENTTASK_ANY) 
#define DEBUG_QUBE_MUTEX                        (0 && DEBUG_EVENTTASK_ANY) 
#define DEBUG_ZONEARRAY_MUTEX                   (0 && DEBUG_EVENTTASK_ANY)
#define DEBUG_QUBE_MUTEX_MAX_MS          10
#define DEBUG_ZONEARRAY_MUTEX_MAX_MS     10
#define DEBUG_EVENT_TASK_FORCE_ALT_TIME_CHECK   0 // NTP fails, try NIST
#define DEBUG_DUMP_LONG_DATASTR                 0 // check in as 0!  if you want to use broadcastDebug to output long strings, this may help
#define DEBUG_ENABLE_PERIODIC_QMOTION_DIAGNOSTICS 1
#define DEBUG_ENABLE_PERIODIC_SOCKET_DIAGNOSTICS 1

char nonDebugDummyBuf[2];
#if DEBUG_EVENTTASK_ANY
#define DEBUGBUF                              debugBuf
#define SIZEOF_DEBUGBUF                       sizeof(debugBuf)
#define DEBUG_BEGIN_TICK         debugLocalStartTick
#define DEBUG_END_TICK           debugLocalEndTick
#define DEBUG_TICK_OVERFLOW      debugLocalOverflow
#define DEBUG_TICK_OVERFLOW_MS   debugLocalOverflowMs
// declare these locally: char                       debugBuf[100];
// declare these locally: MQX_TICK_STRUCT            debugLocalStartTick;
// declare these locally: MQX_TICK_STRUCT            debugLocalEndTick;
// declare these locally: bool                       debugLocalOverflow;
// declare these locally: uint32_t                   debugLocalOverflowMs;
#else
#define DEBUGBUF                              nonDebugDummyBuf
#define SIZEOF_DEBUGBUF                       sizeof(nonDebugDummyBuf)
MQX_TICK_STRUCT                  nonDebugStartTick;
MQX_TICK_STRUCT                  nonDebugEndTick;
bool                             nonDebugOverflow;
uint32_t                         nonDebugOverflowMs;
#define DEBUG_BEGIN_TICK         nonDebugStartTick
#define DEBUG_END_TICK           nonDebugEndTick
#define DEBUG_TICK_OVERFLOW      nonDebugOverflow
#define DEBUG_TICK_OVERFLOW_MS   nonDebugOverflowMs
#endif

#define DBGPRINT_INTSTRSTR                      if (DEBUG_EVENTTASK) broadcastDebug
#define DBGPRINT_VERBOSE_INTSTRSTR              if (DEBUG_EVENTTASK_VERBOSE) broadcastDebug
#define DBGSNPRINTF                             if (DEBUG_EVENTTASK) snprintf
#define DBGPRINT_INTSTRSTR_QUBE_MUTEX           if (DEBUG_QUBE_MUTEX) broadcastDebug
#define DBGPRINT_INTSTRSTR_ZONEARRAY_MUTEX      if (DEBUG_ZONEARRAY_MUTEX) broadcastDebug

#define DEBUG_QUBE_MUTEX_START if (DEBUG_QUBE_MUTEX) _time_get_ticks(&DEBUG_BEGIN_TICK) 
#define DEBUG_QUBE_MUTEX_GOT   if (DEBUG_QUBE_MUTEX) _time_get_ticks(&DEBUG_END_TICK) 
#define DEBUG_QUBE_MUTEX_IS_OVERFLOW (DEBUG_QUBE_MUTEX && (DEBUG_QUBE_MUTEX_MAX_MS < (DEBUG_TICK_OVERFLOW_MS = (uint32_t) _time_diff_milliseconds(&DEBUG_END_TICK, &DEBUG_BEGIN_TICK, &DEBUG_TICK_OVERFLOW))))   
#define DBGSNPRINT_MS_STR_IF_QUBE_MUTEX_OVERFLOW if (DEBUG_QUBE_MUTEX && DEBUG_QUBE_MUTEX_IS_OVERFLOW) DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"qubeSocketMutex %dms at %s:%d",DEBUG_TICK_OVERFLOW_MS, __PRETTY_FUNCTION__, __LINE__); else DEBUGBUF[0] = 0; 
#if DEBUG_QUBE_MUTEX
#define DBGPRINT_INTSTRSTR_IF_QUBE_MUTEX_OVERFLOW  DEBUG_QUBE_MUTEX_GOT; DBGSNPRINT_MS_STR_IF_QUBE_MUTEX_OVERFLOW; if (DEBUG_QUBE_MUTEX && DEBUG_TICK_OVERFLOW) broadcastDebug 
#else
#define DBGPRINT_INTSTRSTR_IF_QUBE_MUTEX_OVERFLOW  if (DEBUG_QUBE_MUTEX) broadcastDebug // optimizes away, no warnings 
#endif

#define DEBUG_ZONEARRAY_MUTEX_START if (DEBUG_ZONEARRAY_MUTEX) _time_get_ticks(&DEBUG_BEGIN_TICK) 
#define DEBUG_ZONEARRAY_MUTEX_GOT   if (DEBUG_ZONEARRAY_MUTEX) _time_get_ticks(&DEBUG_END_TICK) 
#define DEBUG_ZONEARRAY_MUTEX_IS_OVERFLOW (DEBUG_ZONEARRAY_MUTEX && (DEBUG_ZONEARRAY_MUTEX_MAX_MS < (DEBUG_TICK_OVERFLOW_MS = (uint32_t) _time_diff_milliseconds(&DEBUG_END_TICK, &DEBUG_BEGIN_TICK, &DEBUG_TICK_OVERFLOW))))   
#define DBGSNPRINT_MS_STR_IF_ZONEARRAY_MUTEX_OVERFLOW if (DEBUG_ZONEARRAY_MUTEX && DEBUG_ZONEARRAY_MUTEX_IS_OVERFLOW) DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"zoneArrayMutex %dms at %s:%d",DEBUG_TICK_OVERFLOW_MS, __PRETTY_FUNCTION__, __LINE__); else DEBUGBUF[0] = 0; 
#if DEBUG_ZONEARRAY_MUTEX
#define DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW  DEBUG_ZONEARRAY_MUTEX_GOT; DBGSNPRINT_MS_STR_IF_ZONEARRAY_MUTEX_OVERFLOW; if (DEBUG_ZONEARRAY_MUTEX_IS_OVERFLOW) broadcastDebug 
#else
#define DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW  if (DEBUG_ZONEARRAY_MUTEX) broadcastDebug 
#endif

#if DEBUG_EVENTTASK_SUNRISE
char debugBuf2[240];
#define DEBUGBUF2              debugBuf2
#define SIZEOF_DEBUGBUF2       sizeof(debugBuf2)
#else
#define DEBUGBUF2              nonDebugDummyBuf
#define SIZEOF_DEBUGBUF2       sizeof(nonDebugDummyBuf)
#endif // DEBUG_EVENTTASK_SUNRISE

int dbgCnt = 0;
int dbgTotalCnt = 0;
#define DBG_SUNRISE_START_DBGCNT    34
#define DBGSNPRINTF_SUNRISE         if (DEBUG_EVENTTASK_SUNRISE) DBGSNPRINTF   
#define DBGPRINT_SUNRISE            if (DEBUG_EVENTTASK_SUNRISE) broadcastDebug
#define DBGSNPRINTF_SUNRISE_VERBOSE if (DEBUG_EVENTTASK_SUNRISE_VERBOSE && (++dbgTotalCnt >= DBG_SUNRISE_START_DBGCNT)) DBGSNPRINTF   
#define DBGPRINT_SUNRISE_VERBOSE    if (DEBUG_EVENTTASK_SUNRISE_VERBOSE && (dbgTotalCnt >= DBG_SUNRISE_START_DBGCNT)) broadcastDebug

//================================================================================================

// Timeout AddALight mode after 5 minutes of inactivity
#define ADD_A_LIGHT_TIMEOUT_SECONDS 300

// Initialize minute counter to minutes in a day so the task attempts 
// to get time from the NTP server at start up
static int minuteCounter = MIN_IN_DAY;
static pointer event_ptr;

uint32_t memFree = 0;
uint32_t memFreeLowWater = 1000000;

extern unsigned long malloc_count;
extern unsigned long free_count;
extern unsigned long malloc_fail_count;

#if USE_NEW_SUNRISE_CALCULATOR
#define M_PI (3.14159265358979323846264338327)
double calcSunEqOfCenter(double t);
#endif // USE_NEW_SUNRISE_CALCULATOR

// ----------------------------------------------------------------------------

void broadcastDebug(int num, const char *serviceStr, const char *dataStr) // SamF debug
{
   json_t * broadcastObject;
   char     buf[200];
   static int debugCount = 0;
   int len = strlen(dataStr);
   uint16_t uSec = _time_get_microseconds();
   TIME_STRUCT mqxTime;
   _time_get(&mqxTime);
   
   if (serviceStr)
   {
      snprintf(buf, sizeof(buf), "%06dDebug %s", debugCount, serviceStr);
   }
   else
   {
      snprintf(buf, sizeof(buf), "%06dDebug", debugCount);      
   }
   buf[sizeof(buf)-1] = 0;
   debugCount++;
   
   // json broadcast SystemPropertiesChanged for addALight property
   broadcastObject = json_object();
   if (broadcastObject)
   {   
      json_object_set_new(broadcastObject, "ID", json_integer(0));
      json_object_set_new(broadcastObject, "Context", json_string(buf));
   
      if (dataStr && dataStr[0])
      {
         strncpy(buf,dataStr,sizeof(buf)-1);
         buf[sizeof(buf)-1] = 0;
         json_object_set_new(broadcastObject, "DebugStr", json_string(buf));
      }
   
      // Need to subtract out the effective GMT offset so the 
      // time display looks correct in the app
      json_object_set_new(broadcastObject, "CurrentTime", json_integer(mqxTime.SECONDS - systemProperties.effectiveGmtOffsetSeconds));
      
      json_object_set_new(broadcastObject, "uSec", json_integer(uSec)); // unit is actually 1/5000th of a sec!
      json_object_set_new(broadcastObject, "Seqno", json_integer((int) ++msgSequenceNum));
      json_object_set_new(broadcastObject, "Number", json_integer(num));
      
      SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
      json_decref(broadcastObject);
#if DEBUG_DUMP_LONG_DATASTR
      if (len > 0)
      {
         len -= (sizeof(buf) - 1);
         if (len > 0)
         {
            // recurse to dump the whole dataStr
            broadcastDebug(num, "debugStr remainder", &dataStr[(sizeof(buf) - 1)]);          
         }
      }
#endif
   }
}

// ----------------------------------------------------------------------------

void broadcastNTP(bool_t success, const char *str) // TODO useful for debugging
{
   json_t * broadcastObject;

   // json broadcast SystemPropertiesChanged for addALight property
   broadcastObject = json_object();
   if (broadcastObject)
   {   
      json_object_set_new(broadcastObject, "ID", json_integer(0));
      json_object_set_new(broadcastObject, "Service", json_string("BroadcastNTP"));
   
      BuildSystemPropertiesObject(broadcastObject, SYSTEM_PROPERTY_NEW_TIME_BITMASK);
   
      if (success)
      {
         json_object_set_new(broadcastObject, "Status", json_true());
      }
      else
      {
         json_object_set_new(broadcastObject, "Status", json_false());    
      }
      json_object_set_new(broadcastObject, "Service", json_string(str));
   
      
      SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
      json_decref(broadcastObject);
   }  
}

// ----------------------------------------------------------------------------

void broadcastTimeChange(void)
{
   json_t * broadcastObject;

   // json broadcast SystemPropertiesChanged for addALight property
   broadcastObject = json_object();
   if (broadcastObject)
   {   
      json_object_set_new(broadcastObject, "ID", json_integer(0));
      json_object_set_new(broadcastObject, "Service", json_string("SystemPropertiesChanged"));
   
      BuildSystemPropertiesObject(broadcastObject, SYSTEM_PROPERTY_NEW_TIME_BITMASK);
   
      json_object_set_new(broadcastObject, "Status", json_string("Success"));
      SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
      json_decref(broadcastObject);
   }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void broadcastMemoryUse(void)
{
   json_t * broadcastObject;
   uint16_t uSec = _time_get_microseconds();
   TIME_STRUCT mqxTime;
   _time_get(&mqxTime);

   memFree = _mem_get_free();
   if (memFree < memFreeLowWater)
      memFreeLowWater = memFree;

   broadcastObject = json_object();
   if (broadcastObject)
   {   
      json_object_set_new(broadcastObject, "ID", json_integer(0));
      json_object_set_new(broadcastObject, "Service", json_string("BroadcastMemory"));
   
       // build PropertyList 
      json_object_set_new(broadcastObject, "FreeMemory:", json_integer(memFree));      
      json_object_set_new(broadcastObject, "FreeMemLowWater:", json_integer(memFreeLowWater));
      json_object_set_new(broadcastObject, "Malloc_Count:", json_integer(malloc_count));
      json_object_set_new(broadcastObject, "Free_Count:", json_integer(free_count));
      
      json_object_set_new(broadcastObject, "JsonConnections:", json_integer(active_incoming_json_connections));
      if (malloc_fail_count)
      {
         json_object_set_new(broadcastObject, "Malloc_Fail_Count:", json_integer(malloc_fail_count));
      }
      
      json_object_set_new(broadcastObject, "StaticRamUsage:", json_integer((uint32_t)_mqx_get_initialization()->START_OF_KERNEL_MEMORY - 0x1FFF0000));
      json_object_set_new(broadcastObject, "PeakRamUsage:", json_integer((uint32_t)_mem_get_highwater() - 0x1FFF0000));
      // json_object_set_new(broadcastObject, "PeakRamPct:", json_integer(((uint32_t)_mem_get_highwater() - 0x1FFF0000)/0x51E));
      // `json_object_set_new(broadcastObject, "CurrRamPct:", json_integer(memFree / 0x51E));

      // Need to subtract out the effective GMT offset so the 
      // time display looks correct in the app
      json_object_set_new(broadcastObject, "CurrentTime", json_integer(mqxTime.SECONDS - systemProperties.effectiveGmtOffsetSeconds));
      json_object_set_new(broadcastObject, "uSec", json_integer(uSec)); // unit is actually 1/5000th of a sec!
      json_object_set_new(broadcastObject, "Seqno", json_integer((int) ++msgSequenceNum));

      json_object_set_new(broadcastObject, "Status", json_string("Success"));
      SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
      json_decref(broadcastObject);
   }
   
}

// ----------------------------------------------------------------------------

void broadcastDiagnostics(void)
{
   char MACAddressString[18];
   char MCUIDString[50];
   json_t * broadcastObject = json_object();
   uint16_t uSec = _time_get_microseconds();
   TIME_STRUCT mqxTime;
   _time_get(&mqxTime);
   
   if (!broadcastObject)
   {
       return;
   }

   json_object_set_new(broadcastObject, "ID", json_integer(0));
   json_object_set_new(broadcastObject, "Service", json_string("BroadcastDiagnostics"));
   json_object_set_new(broadcastObject, "FirmwareVersion", json_string(SYSTEM_INFO_FIRMWARE_VERSION));
   json_object_set_new(broadcastObject, "firstSystemTime", json_integer(firstSystemTime));
   
   snprintf(MACAddressString, sizeof(MACAddressString), "%02X:%02X:%02X", myMACAddress[3], myMACAddress[4], myMACAddress[5]);
   json_object_set_new(broadcastObject, "MACAddress", json_string(MACAddressString));

   snprintf(MCUIDString, sizeof(MCUIDString), "%08X %08X %08X %08X", SIM_UIDH, SIM_UIDMH, SIM_UIDML, SIM_UIDL);
   json_object_set_new(broadcastObject, "MCUID", json_string(MCUIDString));

   json_object_set_new(broadcastObject, "connectionSequenceNum", json_integer(connectionSequenceNum));


   if(extendedSystemProperties.factory_auth_init == FACTORY_AUTH_INIT_ENABLED)
   {
      json_object_set_new(broadcastObject, "FactoryAuthInit", json_integer(1));
   }

   if(JSA_FirstTimeRecordedIsValid())
   {
      json_object_set_new(broadcastObject, "FirstTimeRecorded", json_integer(extendedSystemProperties.first_time_recorded));
   }
   else
   {
      json_object_set_new(broadcastObject, "FirstTimeRecorded", json_string("Undefined"));
   }

#if extended_system_properties_size_check
   if(sizeof(extended_system_properties)==120)
   {
      json_object_set_new(broadcastObject, "EspSizeOK", json_integer(sizeof(extended_system_properties)));
   }
   else
   {
      json_object_set_new(broadcastObject, "EspSizeError", json_integer(sizeof(extended_system_properties)));
   }
#endif

   if(DEBUG_JUMPER)
   {
      json_object_set_new(broadcastObject, "DebugJumper", json_integer(1));
   }

   if(extendedSystemProperties.auth_exempt == AUTH_EXEMPT)
   {
      json_object_set_new(broadcastObject, "AuthExempt", json_string("Yes"));
   }
   else if(extendedSystemProperties.auth_exempt == AUTH_NOT_EXEMPT)
   {
      json_object_set_new(broadcastObject, "AuthExempt", json_string("No"));
   }
   else
   {
      json_object_set_new(broadcastObject, "AuthExempt", json_string("Undefined"));
   }


#if DEBUG_ENABLE_PERIODIC_SOCKET_DIAGNOSTICS
   if (other_send_err_count)
   {
      json_object_set_new(broadcastObject, "other_send_err:", json_integer(other_send_err_count));
   }
   
   if (other_recv_err_count)
   {
      json_object_set_new(broadcastObject, "other_recv_err:", json_integer(other_recv_err_count));
   }

   if (last_send_err)
   {
      json_object_set_new(broadcastObject, "last_send_err:", json_integer(last_send_err));
   }
   
   if (last_recv_err)
   {
      json_object_set_new(broadcastObject, "last_recv_err:", json_integer(last_recv_err));
   }

   if (rs232ReceiveTaskMsgAllocErr)
   {
      json_object_set_new(broadcastObject, "rs232MsgAllocErr:", json_integer(rs232ReceiveTaskMsgAllocErr));
   }

   if (rfm100TransmitTaskMsgAllocErr)
   {
      json_object_set_new(broadcastObject, "rfm100MsgAllocErr:", json_integer(rfm100TransmitTaskMsgAllocErr));
   }

   if (socketMonMsgAllocErr)
   {
      json_object_set_new(broadcastObject, "socketMonMsgAllocErr:", json_integer(socketMonMsgAllocErr));
   }

   if (updateTaskMsgAllocErr)
   {
      json_object_set_new(broadcastObject, "updateTaskMsgAllocErr:", json_integer(updateTaskMsgAllocErr));
   }

   if (json_err_count)
   {
      json_object_set_new(broadcastObject, "json_err:", json_integer(json_err_count));
   }   
#endif
   
#if DEBUG_ENABLE_PERIODIC_QMOTION_DIAGNOSTICS
   // Add Qmotion diagnostics to PropertyList
   if (qmotion_try_find_count)
   {
      json_object_set_new(broadcastObject, "qmotion_try_find:", json_integer(qmotion_try_find_count));
   }
   
   if (qmotion_found_count)
   {
      json_object_set_new(broadcastObject, "qmotion_found:", json_integer(qmotion_found_count));
   }
   
   if (qmotion_try_add_count)
   {
      json_object_set_new(broadcastObject, "qmotion_try_add:", json_integer(qmotion_try_add_count));
   }
   
   if (qmotion_add_success_count)
   {
      json_object_set_new(broadcastObject, "qmotion_add_success:", json_integer(qmotion_add_success_count));
   }
   
   if (qmotion_add_shades_count)
   {
      json_object_set_new(broadcastObject, "qmotion_add_shades:", json_integer(qmotion_add_shades_count));
   }

   if (qmotion_send_error_count)
   {
      json_object_set_new(broadcastObject, "qmotion_send_error:", json_integer(qmotion_send_error_count));
   }
   
   if (qmotion_recv_error_count)
   {
      json_object_set_new(broadcastObject, "qmotion_recv_error:", json_integer(qmotion_recv_error_count));
   }
   
   if (qmotion_missed_packet_count)
   {
      json_object_set_new(broadcastObject, "qmotion_missed_packet:", json_integer(qmotion_missed_packet_count));
   }
   
#endif

   if (no_json_msg_slot_err)
   {
      json_object_set_new(broadcastObject, "no_json_msg_slot:", json_integer(no_json_msg_slot_err));
   }
   json_object_set_new(broadcastObject, "task_heartbeat_bitmask:", json_integer(task_heartbeat_bitmask));
   // Need to subtract out the effective GMT offset so the 
   // time display looks correct in the app
   json_object_set_new(broadcastObject, "CurrentTime", json_integer(mqxTime.SECONDS - systemProperties.effectiveGmtOffsetSeconds));
   json_object_set_new(broadcastObject, "uSec", json_integer(uSec)); // unit is actually 1/5000th of a sec!
   json_object_set_new(broadcastObject, "Seqno", json_integer((int) ++msgSequenceNum));
   json_object_set_new(broadcastObject, "Status", json_string("Success"));
   SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
   json_decref(broadcastObject);
}

// ----------------------------------------------------------------------------

void broadcastEffGmtOffsetChange(void)
{
   json_t * broadcastObject;

   // json broadcast SystemPropertiesChanged for addALight property
   broadcastObject = json_object();

   json_object_set_new(broadcastObject, "ID", json_integer(0));
   json_object_set_new(broadcastObject, "Service", json_string("SystemPropertiesChanged"));

   BuildSystemPropertiesObject(broadcastObject, SYSTEM_PROPERTY_EFFECTIVE_TIME_ZONE_BITMASK);

   json_object_set_new(broadcastObject, "Status", json_string("Success"));
   SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
   json_decref(broadcastObject);
  
}

// ----------------------------------------------------------------------------

void broadcastNewSceneTriggerTime(int sceneIndex, scene_properties * scenePropertiesPtr)
{
   json_t * broadcastObject;
   json_t * propObject;

   // json broadcast SystemPropertiesChanged for addALight property
   broadcastObject = json_object();
   if (broadcastObject)
   {
      // create an object to store the property list    
      propObject = json_object();
      if (propObject)
      {
         json_object_set_new(broadcastObject, "ID", json_integer(0));
         json_object_set_new(broadcastObject, "Service", json_string("ScenePropertiesChanged"));
               
         // build PropertyList 
         json_object_set_new(broadcastObject, "SID", json_integer(sceneIndex));
         
         json_object_set_new(propObject, "TriggerTime", json_integer(scenePropertiesPtr->nextTriggerTime));
      
         // store the property list into the broadcast object
         json_object_set_new(broadcastObject, "PropertyList", propObject);
            
         json_object_set_new(broadcastObject, "Status", json_string("Success"));
         SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
      }
      json_decref(broadcastObject);
   }
  
}

// ----------------------------------------------------------------------------

void IncrementMinuteCounter()
{
   minuteCounter++;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void HandleMinuteChange()
{
   TIME_STRUCT mqxTime;
   TIME_STRUCT myTime;
   DATE_STRUCT myDate;
#if DEBUG_EVENTTASK_ANY
   char                       debugBuf[100];
   MQX_TICK_STRUCT            debugLocalStartTick;
   MQX_TICK_STRUCT            debugLocalEndTick;
   bool                       debugLocalOverflow;
   uint32_t                   debugLocalOverflowMs;
#endif
   
   // Attempt to get time from the NTP server until the sytem time is valid
   // or at least once a day to keep the system up to date
   if ((!systemTimeValid) ||
       (minuteCounter >= MIN_IN_DAY))
   {
      // Restart the minute counter
      minuteCounter = 0;

      for (int retryCount = 0; retryCount < 4; retryCount++)
      {
         // Returns true if time was not received
         // Real Time Clock is set in this function
         if(!getLocalTime(SNTP_REQUEST_TIMEOUT))
         {
            systemTimeValid = true;
            AlignToMinuteBoundary();
            break;
         }
      }
   }

   // Get the current system time
   _time_get(&mqxTime);
   myTime.SECONDS = mqxTime.SECONDS;
   myTime.MILLISECONDS = 0;

   // Send the time change message to any connected sockets
   broadcastTimeChange();

   // convert seconds to date/time
   _time_to_date(&myTime, &myDate);
   
   // Check if we should timeout AddALight.  If so, reset the addALight system property
   // and notify any JSON clients.
   if (systemProperties.addALight && myTime.SECONDS > lastAddALightTime.SECONDS + ADD_A_LIGHT_TIMEOUT_SECONDS)
   {
      systemProperties.addALight = false;

      json_t* broadcastObject = json_object();
      if (broadcastObject)
      {
         json_object_set_new(broadcastObject, "ID", json_integer(0));
         json_object_set_new(broadcastObject, "Service", json_string("SystemPropertiesChanged"));
         BuildSystemPropertiesObject(broadcastObject, SYSTEM_PROPERTY_ADD_A_LIGHT_BITMASK);
         json_object_set_new(broadcastObject, "Status", json_string("Success"));
         SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
         json_decref(broadcastObject);
      }
   }
   
   // Check to see if a scene needs to be activated this minute
   checkScenes(mqxTime);

   // Check to see if we changed into or out of DST
   checkForDST(myDate);
#ifdef SHADE_CONTROL_ADDED
   DEBUG_QUBE_MUTEX_START;
   _mutex_lock(&qubeSocketMutex);
   DBGPRINT_INTSTRSTR_IF_QUBE_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW HandleMinuteChange before qubeSocketMutex",DEBUGBUF);
   DEBUG_QUBE_MUTEX_START;
   if (!qubeAddress)
   {
      TryToFindQmotion();
   }
   if (qubeAddress)
   {
      QueryQmotionStatus();
   }
   DBGPRINT_INTSTRSTR_IF_QUBE_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW HandleMinuteChange after QueryQmotionStatus",DEBUGBUF);
   _mutex_unlock(&qubeSocketMutex);
#endif
   
   broadcastMemoryUse();
   broadcastDiagnostics();

}
#ifdef TRY_OTHER_TIME
// ----------------------------------------------------------------------------
// goToLower()
//  given a pointer to a character, returns the lower case of it
// if needed
//
//-------------------------------------------------------------

char goToLower(char* ch)
{
   if ((*ch >= 'A') && (*ch <= 'Z'))
         return *ch + 32;
   else 
      return *ch;
}

//-------------------------------------------------------------
// getMonthFromString()
//  given a string, returns the month integer associated with it
//
//  Jan:1 ... Dec:12
//
//  returns 0 if unable to get month
//-------------------------------------------------------------

int16_t getMonthFromString(char *substr)
{
   const char* monthNames[] = 
   {
         "jan", "feb", "mar", "apr",
         "may", "jun", "jul", "aug",
         "sep", "oct", "nov", "dec",NULL
   };
   
   
   int index = 0;
   
   // first convert the 3 chars to lower case, in case they are not
   substr[0] = goToLower(&substr[0]);
   substr[1] = goToLower(&substr[1]);
   substr[2] = goToLower(&substr[2]);
    
   while (index < 12)
   {
      if (strncmp(substr, monthNames[index], 3) == 0)
         break;
      index++;
   }
   
   if (index == 12)
   {
      return 0;
   }
   else
   {
      return index + 1;   // need month in the range of 1 - 12
   }
}

//-------------------------------------------------------------
// parseNistTimes()
//
//  This will parse out of the buffer sent in the latest time
//  this is the secondary way to get time, instead of thru the
//  NTP servers
//
//  NOTE: This time will return time in GMT time.
//   So at return, need to adjust with effective offset
//
//  Buffer should have the following format for time:
//
//  .... Date:Fri, 12 Aug 2016 16:12:37 GMT .......
//
//  returns TRUE if unable to get time from the string
//-------------------------------------------------------------
bool parseNistTimes(char *recvBuff, DATE_STRUCT *newTime)
{
   char *substrings;

   // Initialize time to some valid date
   newTime->HOUR = 0;
   newTime->MINUTE = 0;   
   newTime->SECOND = 0;
   newTime->MILLISEC = 0;
   newTime->YEAR = 2017;
   newTime->MONTH = 2;     // note: month is 1-12
   newTime->DAY = 13;
   
    
   // strstr will return pointer to first occurrence of Date: in string
   substrings = strstr(recvBuff, "Date:");
   if (substrings != NULL)  // if we found it
   {
      substrings += 11;  // skip past "Date:Mon, " 
      
      // now pointing at the blank char before the Day of the month in the substrings of the recv buff
      //  note: atoi will skip whitespace before the first 'number' in the string
      newTime->DAY = atoi(&substrings[0]);
      
      substrings++;
      // skip to the name of the Month
      while ((*substrings != ' ') && (*substrings != 0))
      {
         substrings++;
      }      
      substrings++;  // point to the actual month chars 
      // should be pointing to 3 char Month -- convert month string to 0-11
      newTime->MONTH = getMonthFromString(substrings);
      
      substrings++;
      // skip to the year
      while ((*substrings != ' ') && (*substrings != 0))
      {
         substrings++;
      }  
      newTime->YEAR = atoi(&substrings[0]);
      
      substrings++;
      // skip to the hour
      while ((*substrings != ' ') && (*substrings != 0))
      {
         substrings++;
      }  
      newTime->HOUR = atoi(&substrings[0]);
      
      substrings++;
      // skip to the minute
      while ((*substrings != ':') && (*substrings != 0))
      {
         substrings++;
      }  
      substrings++;
      
      newTime->MINUTE = atoi(&substrings[0]);    
      
      // got all the information from the string 
      // clear the seconds and milliseconds to send back to 0
      
      return FALSE;  // got it
      
   } // else, we got a bad buffer, it has no Date:
   
   return TRUE;  // didn't get any data with time in it
 
}

// ----------------------------------------------------------------------------
// TryToGetTimeFromNistDirect()
//
// This will try to get time from the NIST server using GET, as opposed to normal
//  way to get time via NTP servers.  It seems that some customers routers or ISP's
//  are not allowing NTP packets to go thru.
//  
//  This will use www.nist.gov website.
//
// ----------------------------------------------------------------------------

boolean TryToGetTimeFromNistDirect()
{
   char hostname[MAX_HOSTNAMESIZE];
   struct sockaddr_in remote_sin;
   uint_32 nistSock = RTCS_SOCKET_ERROR;
   int_32 result;
   _ip_address hostaddr = 0;
   boolean returnErrorFlag = TRUE;
   byte_t retryCount;
   uint_16 rcvBufCt;
   boolean parseErrorFlag;
   DATE_STRUCT newTime;
   TIME_STRUCT mqxTime;
   
   // Get the IP address of the time server
   // By giving the host name, this returns the IP Address
   for(retryCount = 0; (retryCount < MAX_NUMBER_OF_IP_ADDRESS_RESOLUTION_ATTEMPTS) && (hostaddr == 0); retryCount++)
   {
      RTCS_resolve_ip_address("www.nist.gov", &hostaddr, hostname, MAX_HOSTNAMESIZE);
   }


   if (hostaddr != 0)
   {
      // Open a TCP socket to talk to the navy server
      nistSock = socket(PF_INET, SOCK_STREAM, 0);

      if (nistSock != RTCS_SOCKET_ERROR)
      {
         // Set the socket options to timeout after 0.5 seconds
#ifndef AMDBUILD
         uint_32 opt_value = 500;
         result = setsockopt(nistSock, SOL_TCP, OPT_CONNECT_TIMEOUT, &opt_value, sizeof(opt_value));
#else
         // For the server build, the timeout value is a timeval and is set in
         // seconds and microseconds.
         struct timeval opt_value;
         opt_value.tv_sec = 1;
         opt_value.tv_usec = 0;
         result = setsockopt(sunSock, SOL_SOCKET, SO_RCVTIMEO, &opt_value, sizeof(opt_value));
#endif

         if (result != RTCS_ERROR)
         {
            // Set up the remote time server address
            memset(&remote_sin, 0, sizeof(remote_sin));
            remote_sin.sin_family = AF_INET;
#ifndef AMDBUILD
            remote_sin.sin_port = 80;
#else
            // For server build, the port needs to be set to network byte order
            remote_sin.sin_port = htons(80);
#endif
            remote_sin.sin_addr.s_addr = hostaddr;

            result = connect(nistSock, (struct sockaddr *) &remote_sin, sizeof(remote_sin));

            if (result == RTCS_OK)
            {
               memset(sendPkt, 0, SUN_PACKET_TX_SIZE);

               int zoneOffset = systemProperties.effectiveGmtOffsetSeconds / 3600;

               snprintf(sendPkt, sizeof(sendPkt),
                     "GET /commonspot/commonspot.css HTTP/1.1\r\n"
                     "HOST: www.nist.gov\r\n"
                     "Accept: text/css\r\n"
                     "Connection: keep-alive\r\n\r\n"
                     );

               result = send(nistSock, &sendPkt, sizeof(sendPkt), 0);
         
               if(result != RTCS_ERROR)
               {
                  rcvBufCt = 0;  // start buffer ct at 0
                  parseErrorFlag = TRUE;
                  retryCount = 0;

                  // clear out any junk in memory
                  memset(recvPkt, 0, sizeof(recvPkt)); 

                  returnErrorFlag = FALSE;   // assume we're going to find our data
                  while ((parseErrorFlag == TRUE) && 
                         (retryCount < TRY_AGAIN_FOR_BUFFER) && 
                         (returnErrorFlag == FALSE))
                  {
                     // wait for response from server
                     // NOTE: we leave one byte 
                     result = recv(nistSock, &(recvPkt[rcvBufCt]), ((sizeof(recvPkt) - 1) - rcvBufCt), 0);
                     
                     if ((result == RTCS_ERROR) || (result == RTCS_SOCKET_ERROR))
                     {
                        // Failed to receive data. Break the loop
                        returnErrorFlag = TRUE;
                     }
                     else if (result > 0)  // if we have some data in our buffer, parse it
                     {
  
                        // null terminate buffer, for strstr use in parser
                        recvPkt[rcvBufCt + result] = 0;

                        // get the time from the packet, ignore the rest of the packets
                        //  Time will be in the following packet format:
                        // HTTP/1.1 400 Bad Request Date:Fri, 12 Aug 2016 16:12:37 GMT ........
                        parseErrorFlag = parseNistTimes(recvPkt, &newTime);
                        
                        if (parseErrorFlag == FALSE) 
                        {
                           // convert DATE STRUCT we filled in to a time value
                           _time_from_date(&newTime, &mqxTime);
                           // we got the time, we need to adjust
                           mqxTime.SECONDS += systemProperties.effectiveGmtOffsetSeconds;
                           // set the real time clock with the received time
                           _time_set(&mqxTime);
                        }
                        
                        rcvBufCt += result;
                        if (rcvBufCt >= (sizeof(recvPkt) - 1))
                        {
                           memcpy(&recvPkt[0], &recvPkt[sizeof(recvPkt) / 2], sizeof(recvPkt) / 2);
                           rcvBufCt = (sizeof(recvPkt) / 2) - 1;  // subtract one to ensure we overwrite the previous null terminator
                        }

                     }

                     // Increment the loop counter
                     retryCount++;

                  }  // end of while (flag == TRUE) && (retryCount < TRY_AGAIN_FOR_BUFFER ) && (returnErrorFlag == FALSE)
                  
                  // if parseErrorFlag is TRUE; we didn't get a time from the packets
                  if ((retryCount == TRY_AGAIN_FOR_BUFFER) && (parseErrorFlag == TRUE))
                  {
                      returnErrorFlag = TRUE;   // indicate an error, if we ran out of retries and still didn't get a suntime
                  }

               }  // if(result != RTCS_ERROR)
            
            }  // if (result == RTCS_OK)
         
         }  // if (result != RTCS_ERROR)
      
      }  // if (sunSock != RTCS_SOCKET_ERROR)
   
   }  // if (hostaddr != 0)

   if (nistSock != RTCS_SOCKET_ERROR)
   {
      // Shutdown and close the socket
       
      shutdown(nistSock, FLAG_CLOSE_TX);
#ifdef AMDBUILD
      close(nistSock);
#else
      // TODO: No way to close socket in our version of MQX. Assuming shutdown takes care of it
      // closesocket(sunSock); is available in the most current version
#endif
   }

   return returnErrorFlag;
}

#endif

//-------------------------------------------------------------
//  getLocalTime()
//  This will transmit a request to the time server on the internet
//  and sets the real time clock of the board.
//
//   returns: TRUE if error occurs (clock not changed)
//-------------------------------------------------------------

boolean getLocalTime(uint32_t timeout)
{
   char           hostname[MAX_HOSTNAMESIZE];
   _ip_address    hostaddr = 0;
   TIME_STRUCT    systemTime;
   uint32_t       SNTPQueryResult = 1;
   boolean        error = TRUE;
   char           buff[100];
   
   // Get the IP address of the time server
   // By giving the host name, this returns the IP Address
   for(byte_t retryCount = 0; (retryCount < MAX_NUMBER_OF_IP_ADDRESS_RESOLUTION_ATTEMPTS) && (hostaddr == 0); retryCount++)
   {
      RTCS_resolve_ip_address(NTP_SERVER, &hostaddr, hostname, MAX_HOSTNAMESIZE);
   }

   if (hostaddr && (DEBUG_EVENT_TASK_FORCE_ALT_TIME_CHECK == 0))
   {
      
      SNTPQueryResult = SNTP_oneshot(hostaddr, timeout);
      
      if (SNTPQueryResult == RTCS_OK)
      {
         _time_get(&systemTime);
         systemTime.SECONDS += systemProperties.effectiveGmtOffsetSeconds;
         // set the real time clock with the received time
         _time_set(&systemTime);
         error = FALSE;
         snprintf(buff, sizeof(buff), "****** Got NTP -- IP: %d.%d.%d.%d, SNTP_Result:0x%x *******", 
               HighByteOfDword(hostaddr),
               UpperMiddleByteOfDword(hostaddr),
               LowerMiddleByteOfDword(hostaddr),
               LowByteOfDword(hostaddr),
               SNTPQueryResult);
         broadcastNTP(TRUE, buff);  // broadcast that we got time
         flashSaveTimer = FLASH_SAVE_TIMER;
      }
      else
      {
          snprintf(buff, sizeof(buff), "***** NTP one shot did not work -- IP: %d.%d.%d.%d,  SNTP_Result:0x%x TRYING ALTERNATE ******", 
                HighByteOfDword(hostaddr),
                UpperMiddleByteOfDword(hostaddr),
                LowerMiddleByteOfDword(hostaddr),
                LowByteOfDword(hostaddr),
                SNTPQueryResult);
          broadcastNTP(FALSE, buff);  // broadcast that we did not get time
      }
   } // if (hostaddr)
   
   if (error)  // if error is still true, then NTP did not work, try alternate
   {
 
#ifdef TRY_OTHER_TIME
          error = TryToGetTimeFromNistDirect();
          if (!error) // if we got time from NIST, return will be FALSE
          {
            snprintf(buff, sizeof(buff), "***** NIST ALT TIME WORKED ******");
            broadcastNTP(TRUE, buff);  // broadcast that we did get time
            flashSaveTimer = FLASH_SAVE_TIMER;
          }
          else
          {
              snprintf(buff, sizeof(buff), "***** NIST ALT TIME DID NOT WORK ******");
              broadcastNTP(FALSE, buff);  // broadcast that we did not get time        	  
          }
#endif
   }
 
   if (!error && firstSystemTime == 0)
   {
       // Store the time for diagnostic purposes
       _time_get(&systemTime);
       firstSystemTime = systemTime.SECONDS - systemProperties.effectiveGmtOffsetSeconds;
   }

   return error;
}

//-------------------------------------------------------------
//  checkScenes (current time in UTC)
//
//      * Iterate through all scenes:
//          if Frequency = NEVER: done
//          if Frequency = ONCE
//             if TriggerTime satisfied:
//                Activate scene
//                Set frequency to NEVER
//             else TriggerTime not satisfied done
//          if Frequency = WEEKLY
//             if TriggerTime satisfied:
//                Activate scene
//                Calculate new TriggerTime
//             else if TriggerTime not satisfied done
//  
//-------------------------------------------------------------

void checkScenes(TIME_STRUCT rtcTime)
{
   json_t * broadcastObject;
   json_t * propObject;
   int sceneIdx;
   scene_properties sceneProperties;
#if DEBUG_EVENTTASK_ANY
   char debugBuf[200];
#endif
   
   DBGPRINT_VERBOSE_INTSTRSTR(__LINE__, "checkScenes", "BEGIN");

   if (!systemProperties.addALight)
   {  
      for (sceneIdx = 0; sceneIdx < MAX_NUMBER_OF_SCENES; sceneIdx++)
      {
         // too much: DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"sceneIdx=%d", sceneIdx);
         // too much:DBGPRINT_VERBOSE_INTSTRSTR(__LINE__,"checkScenes",DEBUGBUF);

         _mutex_lock(&ScenePropMutex);

         if (GetSceneProperties(sceneIdx, &sceneProperties))
         {
            // Clean up any invalid data (new solution for case 58611)
            if (sceneProperties.trigger != REGULAR_TIME && sceneProperties.trigger != SUNRISE && sceneProperties.trigger != SUNSET)
            {
               sceneProperties.trigger = REGULAR_TIME;
            }
            
            if (sceneProperties.nextTriggerTime != 0 && !(sceneProperties.nextTriggerTime >= MIN_SYSTEM_TIME && sceneProperties.nextTriggerTime <= MAX_SYSTEM_TIME))
            {
               sceneProperties.nextTriggerTime = 0;
            }
            
            if (sceneProperties.delta > MAX_DELTA_VALUE || sceneProperties.delta < MIN_DELTA_VALUE)
            {
               sceneProperties.delta = 0;
            }
            
            if (sceneProperties.freq != NONE && sceneProperties.freq != ONCE && sceneProperties.freq != EVERY_WEEK)
            {
               sceneProperties.freq = NONE;
            }

            DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"sceneIdx=%d trigger=%s time=%ld delta=%d repeat=%s", sceneIdx, 
                  (sceneProperties.trigger == REGULAR_TIME) ? "TIME" : (sceneProperties.trigger == SUNRISE) ? "SUNRISE" : (sceneProperties.trigger == SUNSET) ? "SUNRISE" : "?",
                  (long int) sceneProperties.nextTriggerTime, (int) sceneProperties.delta,
                  (sceneProperties.freq == NONE) ? "NONE" : (sceneProperties.freq == ONCE) ? "ONCE" : (sceneProperties.freq == EVERY_WEEK) ? "WEEKLY" : "?" );
            DBGPRINT_VERBOSE_INTSTRSTR(__LINE__,"checkScenes",DEBUGBUF);

            if (sceneProperties.SceneNameString[0] != 0)
            {
               DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"SceneNameString=%s freq=%d nextTriggerTime=%d rtcTime.SECONDS=%d", sceneProperties.SceneNameString, sceneProperties.freq, sceneProperties.nextTriggerTime, rtcTime.SECONDS);
               DBGPRINT_INTSTRSTR(__LINE__,"checkScenes",DEBUGBUF);

               switch (sceneProperties.freq)
               {
                  case NONE:
                     {
                        break;
                     }

                  case ONCE:
                     {
                        if (sceneProperties.nextTriggerTime <= rtcTime.SECONDS)  // time is now or in the past
                        {
                           broadcastObject = json_object();
                           if (broadcastObject)
                           {
                              propObject = json_object();
                              if (propObject)
                              {
                                 json_object_set_new(broadcastObject, "ID", json_integer(0));
                                 json_object_set_new(broadcastObject, "Service", json_string("ScenePropertiesChanged"));
      
                                 if (!sceneProperties.skipNext)
                                 {
                                     DBGPRINT_INTSTRSTR(__LINE__,"checkScenes ExecuteScene ONCE",sceneProperties.SceneNameString);
                                     ExecuteScene(sceneIdx, &sceneProperties, 0, TRUE, 0, 1, 0);
                                 }
                                 else
                                 {
                                    DBGPRINT_INTSTRSTR(__LINE__,"checkScenes skipNext ExecuteScene ONCE",sceneProperties.SceneNameString);
                                    sceneProperties.skipNext = FALSE;
                                    json_object_set_new(propObject, "Skip", json_false());
                                 }
      
                                 sceneProperties.freq = NONE;
      
                                 // build PropertyList 
                                 json_object_set_new(broadcastObject, "SID", json_integer(sceneIdx));
                                 json_object_set_new(propObject, "Frequency", json_integer(sceneProperties.freq));
      
                                 // store the property list into the broadcast object
                                 json_object_set_new(broadcastObject, "PropertyList", propObject);
      
                                 //broadcast zonePropertiesChanged with created PropertyList
                                 json_object_set_new(broadcastObject, "Status", json_string("Success"));
                                 SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
                              }
                              json_decref(broadcastObject);
                           }
                        }
                        break;
                     }

                  case EVERY_WEEK:
                     {
                        
                        if (sceneProperties.nextTriggerTime <= rtcTime.SECONDS)  // time is now or in the past
                        {
                           broadcastObject = json_object();
                           if (broadcastObject)
                           {
                              propObject = json_object();
                              if (propObject)
                              {
                                 json_object_set_new(broadcastObject, "ID", json_integer(0));
                                 json_object_set_new(broadcastObject, "Service", json_string("ScenePropertiesChanged"));
      
                                 if (!sceneProperties.skipNext)
                                 {
                                    DBGPRINT_INTSTRSTR(__LINE__,"checkScenes ExecuteScene EVERY_WEEK",sceneProperties.SceneNameString);
                                    ExecuteScene(sceneIdx, &sceneProperties, 0, TRUE, 0, 1, 0);
                                 }
                                 else
                                 {
                                    DBGPRINT_INTSTRSTR(__LINE__,"checkScenes skipNext ExecuteScene EVERY_WEEK",sceneProperties.SceneNameString);
                                    sceneProperties.skipNext = FALSE;
                                    json_object_set_new(propObject, "Skip", json_false());
                                 }
      
                                 calculateNewTrigger(&sceneProperties, rtcTime);
      
                                 // build PropertyList 
                                 json_object_set_new(broadcastObject, "SID", json_integer(sceneIdx));
                                 json_object_set_new(propObject, "TriggerTime", json_integer(sceneProperties.nextTriggerTime));
      
                                 // store the property list into the broadcast object
                                 json_object_set_new(broadcastObject, "PropertyList", propObject);
      
                                 //broadcast zonePropertiesChanged with created PropertyList
                                 json_object_set_new(broadcastObject, "Status", json_string("Success"));
                                 SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
                              }
                              json_decref(broadcastObject);
                           }
                        }
                        
                        break;
                     }
               }  //  switch (sceneProperties.freq)

               // Scene was valid, need to save anything that changed
               SetSceneProperties(sceneIdx, &sceneProperties);
            }  // if (sceneProperties.SceneNameString[0] != 0)
         } // if (GetSceneProperties(sceneIdx, &sceneProperties)

         _mutex_unlock(&ScenePropMutex);
      }  // for all scenes
   
   } // if (!systemProperties.addALight)
  
}


//---------------------------------------------------------------------
// isInDaylist()
//   if the day of the week passed in is in the day list of the scene,
//  this will return TRUE.
//---------------------------------------------------------------------

bool isInDayList(scene_properties * scenePropertiesPtr,  int16_t day)
{
   bool flag = FALSE;

   if (scenePropertiesPtr->dayBitMask.DayBits.sunday && (day == 0))
   {
      flag = TRUE;
   }
   else if (scenePropertiesPtr->dayBitMask.DayBits.monday && (day == 1))
   {
      flag = TRUE;
   }
   else if (scenePropertiesPtr->dayBitMask.DayBits.tuesday && (day == 2))
   {
      flag = TRUE;
   }
   else if (scenePropertiesPtr->dayBitMask.DayBits.wednesday && (day == 3))
   {
      flag = TRUE;
   }
   else if (scenePropertiesPtr->dayBitMask.DayBits.thursday && (day == 4))
   {
      flag = TRUE;
   }
   else if (scenePropertiesPtr->dayBitMask.DayBits.friday && (day == 5))
   {
      flag = TRUE;
   }
   else if (scenePropertiesPtr->dayBitMask.DayBits.saturday && (day == 6))
   {
      flag = TRUE;
   }

   return flag;

}


//---------------------------------------------------------------------
// anyDaysInList()
//   if the day of the week passed in is in the day list of the scene,
//  this will return TRUE.
//---------------------------------------------------------------------

bool anyDaysInList(scene_properties * scenePropertiesPtr)
{
   bool flag = FALSE;

   if (scenePropertiesPtr->dayBitMask.DayBits.sunday || scenePropertiesPtr->dayBitMask.DayBits.monday ||
       scenePropertiesPtr->dayBitMask.DayBits.tuesday || scenePropertiesPtr->dayBitMask.DayBits.wednesday || 
       scenePropertiesPtr->dayBitMask.DayBits.thursday || scenePropertiesPtr->dayBitMask.DayBits.friday || 
       scenePropertiesPtr->dayBitMask.DayBits.saturday)
   {
      flag = TRUE;
   }

   return flag;

}

//-------------------------------------------------------------
// calculateNewTrigger()
//   this is called after a scene has been activated and 
// the next trigger time needs to be calculated for the scene.
// 
// 6/10 - added delta calculation to trigger for sun times
//-------------------------------------------------------------

void calculateNewTrigger(scene_properties * scenePropertiesPtr, TIME_STRUCT rtcTime)
{
   DATE_STRUCT currentDate;
   TIME_STRUCT currentTime;
   DATE_STRUCT sceneDate;
   TIME_STRUCT sceneTime;
   DATE_STRUCT newTriggerDate;
   TIME_STRUCT newTriggerTime;
   DATE_STRUCT sunTime;
   int sceneMinuteOfDay;
   int currentMinuteOfDay;
   boolean nextIsToday = FALSE;
   
   // Set the current time and date structures
   currentTime = rtcTime;
   _time_to_date(&currentTime, &currentDate);

   // Set the scene time and date structures
   sceneTime.SECONDS = scenePropertiesPtr->nextTriggerTime;
   sceneTime.MILLISECONDS = 0;
   _time_to_date(&sceneTime, &sceneDate);

   DBGSNPRINTF_SUNRISE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%s:%d start freq=%s trig=%s scene %d/%d/%d %d:%02d:%02d\n", __PRETTY_FUNCTION__, __LINE__, 
         (scenePropertiesPtr->trigger == SUNRISE) ? "sunrise" : (scenePropertiesPtr->trigger == SUNSET) ? "sunset" : "regular", 
         (scenePropertiesPtr->freq == NONE) ? "NONE" : (scenePropertiesPtr->freq == ONCE) ? "ONCE" : (scenePropertiesPtr->freq == EVERY_WEEK) ? "EVERY_WEEK" : "?",
          sceneDate.YEAR, sceneDate.MONTH, sceneDate.DAY, sceneDate.HOUR, sceneDate.MINUTE, sceneDate.SECOND);
   DBGPRINT_SUNRISE(__LINE__, DEBUGBUF2, scenePropertiesPtr->SceneNameString); 
   
   // Calculate the current minute of the day
   currentMinuteOfDay = (currentDate.HOUR * MINUTES_IN_HOUR) + currentDate.MINUTE;

   if (scenePropertiesPtr->freq == ONCE)
   {
      newTriggerDate = sceneDate;
      if ((newTriggerDate.YEAR == currentDate.YEAR) && (newTriggerDate.MONTH == currentDate.MONTH) && (newTriggerDate.DAY == currentDate.DAY))
      {
         nextIsToday = TRUE;
      }
   }
   else
   {
      // Trigger date initialized as today
      newTriggerDate = currentDate;
      nextIsToday = TRUE;
   }
   
   newTriggerDate.SECOND = 0;
   newTriggerDate.MILLISEC = 0;

   DBGSNPRINTF_SUNRISE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%s:%d starting newTriggerDate freq=%s trig=%s %d/%d/%d %d:%02d:%02d\n", __PRETTY_FUNCTION__, __LINE__, 
         (scenePropertiesPtr->trigger == SUNRISE) ? "sunrise" : (scenePropertiesPtr->trigger == SUNSET) ? "sunset" : "regular", 
         (scenePropertiesPtr->freq == NONE) ? "NONE" : (scenePropertiesPtr->freq == ONCE) ? "ONCE" : (scenePropertiesPtr->freq == EVERY_WEEK) ? "EVERY_WEEK" : "?",
         newTriggerDate.YEAR, newTriggerDate.MONTH, newTriggerDate.DAY, newTriggerDate.HOUR, newTriggerDate.MINUTE, newTriggerDate.SECOND);
   DBGPRINT_SUNRISE(__LINE__, DEBUGBUF2, (char *) NULL);                 

   if (scenePropertiesPtr->trigger == REGULAR_TIME)
   {
      // Trigger Time is the scene time
      newTriggerDate.HOUR = sceneDate.HOUR;
      newTriggerDate.MINUTE = sceneDate.MINUTE;

      // Calculate the minute of the day that the scene is executed
      sceneMinuteOfDay = (newTriggerDate.HOUR * MINUTES_IN_HOUR) + newTriggerDate.MINUTE;
   }
   else
   {
      // Sunrise or Sunset Time
      (void) getNextSunTimes(newTriggerDate, (scenePropertiesPtr->trigger == SUNRISE), &sunTime);

      DBGSNPRINTF_SUNRISE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%s:%d %s = %d/%d/%d %d:%02d:%02d\n", __PRETTY_FUNCTION__, __LINE__, 
            (scenePropertiesPtr->trigger == SUNRISE) ? "sunrise" : "sunset", sunTime.YEAR, sunTime.MONTH, sunTime.DAY, sunTime.HOUR, sunTime.MINUTE, sunTime.SECOND);
      DBGPRINT_SUNRISE(__LINE__, DEBUGBUF2, (char *) NULL);                 

      newTriggerDate.HOUR = sunTime.HOUR;
      newTriggerDate.MINUTE = sunTime.MINUTE;
      
      // Calculate the minute of the day that the scene is executed. if we are setting Sunrise or Sunset, adjust by delta, if exists.
      sceneMinuteOfDay = (newTriggerDate.HOUR * MINUTES_IN_HOUR) + newTriggerDate.MINUTE + (int32_t)scenePropertiesPtr->delta;
   }

   DBGSNPRINTF_SUNRISE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%s:%d scene.trig=%s scene.freq=%s sceneMin=%d currMin=%d\n", __PRETTY_FUNCTION__, __LINE__,
         (scenePropertiesPtr->trigger == SUNRISE) ? "sunrise" : (scenePropertiesPtr->trigger == SUNSET) ? "sunset" : "regular",
         (scenePropertiesPtr->freq == NONE) ? "NONE" : (scenePropertiesPtr->freq == ONCE) ? "ONCE" : (scenePropertiesPtr->freq == EVERY_WEEK) ? "EVERY_WEEK" : "?",  
               sceneMinuteOfDay, currentMinuteOfDay);
   DBGPRINT_SUNRISE(__LINE__, DEBUGBUF2, (char *) NULL);                 

   // If the scene will not be be executed later in the same day as today, advance by a day until valid
   if ((((sceneMinuteOfDay <= currentMinuteOfDay) && nextIsToday) || 
       ((!isInDayList(scenePropertiesPtr, currentDate.WDAY)) && anyDaysInList(scenePropertiesPtr)))
        && (scenePropertiesPtr->freq != NONE))
   {
      TIME_STRUCT tempTime;
      nextIsToday = FALSE;
      do
      {
         // Increment the day
         _time_from_date(&newTriggerDate, &tempTime);
         tempTime.SECONDS += SECONDS_PER_DAY;
         _time_to_date(&tempTime, &newTriggerDate);
      } while ((isInDayList(scenePropertiesPtr, newTriggerDate.WDAY) == FALSE) && (scenePropertiesPtr->freq == EVERY_WEEK) && anyDaysInList(scenePropertiesPtr));
   }
   
   DBGSNPRINTF_SUNRISE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%s:%d newTrig %04d-%02d-%02d isInDayList(%d)=%d freq=%s anyDaysInList=%d\n", __PRETTY_FUNCTION__, __LINE__, 
            newTriggerDate.YEAR, newTriggerDate.MONTH, newTriggerDate.DAY, 
            newTriggerDate.WDAY, isInDayList(scenePropertiesPtr,newTriggerDate.WDAY), 
            (scenePropertiesPtr->freq == NONE) ? "NONE" : (scenePropertiesPtr->freq == ONCE) ? "ONCE" : (scenePropertiesPtr->freq == EVERY_WEEK) ? "EVERY_WEEK" : "?",
            anyDaysInList(scenePropertiesPtr) );
   DBGPRINT_SUNRISE(__LINE__, DEBUGBUF2, (char *) NULL);
   
   // Found the correct year, month, and date for the trigger time
   // Need to calculate the correct hour and minute. Seconds and milliseconds
   // are set to 0 because scenes are executed on minute boundaries
   newTriggerDate.SECOND = 0;
   newTriggerDate.MILLISEC = 0;

   // Convert the new trigger DATE_STRUCT to a TIME_STRUCT
   _time_from_date(&newTriggerDate, &newTriggerTime);

   // if we are setting Sunrise or Sunset, adjust by delta, if exists
   if (scenePropertiesPtr->trigger != REGULAR_TIME)
   {
      // adjust by delta, convert delta to seconds
      scenePropertiesPtr->nextTriggerTime = newTriggerTime.SECONDS + ((int32_t)scenePropertiesPtr->delta * SECS_IN_MINUTE);
   }
   else
   {
      scenePropertiesPtr->nextTriggerTime = newTriggerTime.SECONDS;
   }

   DBGSNPRINTF_SUNRISE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%s:%d end Time=%d = %d/%d/%d %d:%02d:%02d d=%d\n", __PRETTY_FUNCTION__, __LINE__, scenePropertiesPtr->nextTriggerTime, 
         newTriggerDate.YEAR, newTriggerDate.MONTH, newTriggerDate.DAY, newTriggerDate.HOUR, newTriggerDate.MINUTE, newTriggerDate.SECOND,
         (scenePropertiesPtr->trigger != REGULAR_TIME) ? scenePropertiesPtr->delta : 0);
   DBGPRINT_SUNRISE(__LINE__, DEBUGBUF2, scenePropertiesPtr->SceneNameString);                 
}

#if USE_NEW_SUNRISE_CALCULATOR

//---------------------------------------------------------------------------------------------------------------------------
// helper functions for the new getNextSunTimes
// These were converted to C from the NOAA site https://www.esrl.noaa.gov/gmd/grad/solcalc/sunrise.html
// See section 105 of https://en.wikisource.org/wiki/Copyright_Law_Revision_(House_Report_No._94-1476) for copyright info
//---------------------------------------------------------------------------------------------------------------------------

// Convert radian angle to degrees
double radToDeg(double angleRad)
{
   return (180.0 * angleRad / M_PI);
}

// Convert degree angle to radians
double  degToRad(double angleDeg)
{
   double rad;
   rad = (M_PI * angleDeg / 180.0);

   return rad;
}

//***********************************************************************/
//* Name:    calcJD                          */
//* Type:    Function                           */
//* Purpose: Julian day from calendar day                */
//* Arguments:                            */
//*   year : 4 digit year                       */
//*   month: January = 1                        */
//*   day  : 1 - 31                          */
//* Return value:                            */
//*   The Julian day corresponding to the date              */
//* Note:                                 */
//*   Number is returned for start of day.  Fractional days should be   */
//*   added later.                           */
//***********************************************************************/
double calcJD(int year,int month,int day)
{
   if (month <= 2) 
   {
      year -= 1;
      month += 12;
   }
   int A = floor(year/100);
   int B = 2 - A + floor(A/4);
   
   double JD = floor(365.25*(year + 4716)) + floor(30.6001*(month+1)) + day + B - 1524.5;
   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%d,%d,%d) = %5.3lf\n", ++dbgCnt, __PRETTY_FUNCTION__, year, month, day, JD);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
   return JD;
}

//***********************************************************************/
//* Name:    calcTimeJulianCent                    */
//* Type:    Function                           */
//* Purpose: convert Julian Day to centuries since J2000.0.       */
//* Arguments:                            */
//*   jd : the Julian Day to convert                  */
//* Return value:                            */
//*   the T value corresponding to the Julian Day           */
//***********************************************************************/
double calcTimeJulianCent(double jd)
{
   double T = ( jd - 2451545.0)/36525.0;
   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%5.3lf) = %5.3lf\n", ++dbgCnt, __PRETTY_FUNCTION__, jd, T);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
   return T;
}

//***********************************************************************/
//* Name:    calcJDFromJulianCent                     */
//* Type:    Function                           */
//* Purpose: convert centuries since J2000.0 to Julian Day.       */
//* Arguments:                            */
//*   t : number of Julian centuries since J2000.0          */
//* Return value:                            */
//*   the Julian Day corresponding to the t value           */
//***********************************************************************/
double calcJDFromJulianCent(double t)
{
   double JD = t * 36525.0 + 2451545.0;

   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%5.3lf) = %5.3lf\n", ++dbgCnt, __PRETTY_FUNCTION__, t, JD);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
   return JD;
}

//***********************************************************************/
//* Name:    calGeomMeanLongSun                    */
//* Type:    Function                           */
//* Purpose: calculate the Geometric Mean Longitude of the Sun    */
//* Arguments:                            */
//*   t : number of Julian centuries since J2000.0          */
//* Return value:                            */
//*   the Geometric Mean Longitude of the Sun in degrees       */
//***********************************************************************/
double calcGeomMeanLongSun(double t)
{
   double L = 280.46646 + t * (36000.76983 + 0.0003032 * t);
   while( (int) L >  360 )
   {
      L -= 360.0;
   }
   
   while(  L <  0)
   {
      L += 360.0;   
   }

   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%5.3lf) = %5.3lf\n", ++dbgCnt, __PRETTY_FUNCTION__, t, L);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
   return L;              // in degrees
}

//***********************************************************************/
//* Name:    calGeomAnomalySun                     */
//* Type:    Function                           */
//* Purpose: calculate the Geometric Mean Anomaly of the Sun      */
//* Arguments:                            */
//*   t : number of Julian centuries since J2000.0          */
//* Return value:                            */
//*   the Geometric Mean Anomaly of the Sun in degrees         */
//***********************************************************************/
double calcGeomMeanAnomalySun(double t)
{
   double M = 357.52911 + t * (35999.05029 - 0.0001537 * t);
   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%5.3lf) = %5.3lf\n", ++dbgCnt, __PRETTY_FUNCTION__, t, M);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
   return M;               // in degrees
}

//***********************************************************************/
//* Name:    calcEccentricityEarthOrbit                  */
//* Type:    Function                           */
//* Purpose: calculate the eccentricity of earth's orbit       */
//* Arguments:                            */
//*   t : number of Julian centuries since J2000.0          */
//* Return value:                            */
//*   the unitless eccentricity                    */
//***********************************************************************/
double calcEccentricityEarthOrbit(double t)
{
   double e = 0.016708634 - t * (0.000042037 + 0.0000001267 * t);
   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%5.3lf) = %5.3lf\n", ++dbgCnt, __PRETTY_FUNCTION__, t, e);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
   return e;               // unitless
}

//***********************************************************************/
//* Name:    calcSunEqOfCenter                     */
//* Type:    Function                           */
//* Purpose: calculate the equation of center for the sun         */
//* Arguments:                            */
//*   t : number of Julian centuries since J2000.0          */
//* Return value:                            */
//*   in degrees                             */
//***********************************************************************/
double calcSunEqOfCenter(double t)
{
   double m = calcGeomMeanAnomalySun(t);
   
   double mrad; // = degToRad(m);
   double sinm; // = sin(mrad);
   double sin2m; // = sin(mrad+mrad);
   double sin3m; // = sin(mrad+mrad+mrad);   
   double C; // = sinm * (1.914602 - t * (0.004817 + 0.000014 * t)) + sin2m * (0.019993 - 0.000101 * t) + sin3m * 0.000289;

   mrad = degToRad(m);
   sinm = sin(mrad);
   sin2m = sin(mrad+mrad);
   sin3m = sin(mrad+mrad+mrad);   
   C = sinm * (1.914602 - t * (0.004817 + 0.000014 * t)) + sin2m * (0.019993 - 0.000101 * t) + sin3m * 0.000289;

   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%5.3lf) = %5.3lf [mrad=%5.3lf,sinm=%5.3lf,sin2m=%5.3lf,sin3m=%5.3lf]\n", ++dbgCnt, __PRETTY_FUNCTION__, t, C, mrad, sinm, sin2m,sin3m);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
   return C;               // in degrees
}

//***********************************************************************/
//* Name:    calcSunTrueLong                       */
//* Type:    Function                           */
//* Purpose: calculate the true longitude of the sun           */
//* Arguments:                            */
//*   t : number of Julian centuries since J2000.0          */
//* Return value:                            */
//*   sun's true longitude in degrees                 */
//***********************************************************************/
double calcSunTrueLong(double t)
{
   double l0 = calcGeomMeanLongSun(t);
   double c = calcSunEqOfCenter(t);
   
   double O = l0 + c;
   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%5.3lf) = %5.3lf\n", ++dbgCnt, __PRETTY_FUNCTION__, t, O);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
   return O;               // in degrees
}

//***********************************************************************/
//* Name:    calcSunApparentLong                   */
//* Type:    Function                           */
//* Purpose: calculate the apparent longitude of the sun       */
//* Arguments:                            */
//*   t : number of Julian centuries since J2000.0          */
//* Return value:                            */
//*   sun's apparent longitude in degrees                */
//***********************************************************************/
double calcSunApparentLong(double t)
{
   double o = calcSunTrueLong(t);
   double omega = 125.04 - 1934.136 * t;
   double lambda = o - 0.00569 - 0.00478 * sin(degToRad(omega));
   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%5.3lf) = %5.3lf\n", ++dbgCnt, __PRETTY_FUNCTION__, t, lambda);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
   return lambda;          // in degrees
}

//***********************************************************************/
//* Name:    calcMeanObliquityOfEcliptic                 */
//* Type:    Function                           */
//* Purpose: calculate the mean obliquity of the ecliptic         */
//* Arguments:                            */
//*   t : number of Julian centuries since J2000.0          */
//* Return value:                            */
//*   mean obliquity in degrees                    */
//***********************************************************************/
double calcMeanObliquityOfEcliptic(double t)
{
   double seconds = 21.448 - t*(46.8150 + t*(0.00059 - t*(0.001813)));
   double e0 = 23.0 + (26.0 + (seconds/60.0))/60.0;

   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%5.3lf) = %5.3lf seconds=%5.3lf\n", ++dbgCnt, __PRETTY_FUNCTION__, t, e0, seconds);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
  return e0;              // in degrees
}

//***********************************************************************/
//* Name:    calcObliquityCorrection                  */
//* Type:    Function                           */
//* Purpose: calculate the corrected obliquity of the ecliptic    */
//* Arguments:                            */
//*   t : number of Julian centuries since J2000.0          */
//* Return value:                            */
//*   corrected obliquity in degrees                  */
//***********************************************************************/
double calcObliquityCorrection(double t)
{
   double e0;
   double omega; 
   double omega_rad; 
   double e; 

   e0 = calcMeanObliquityOfEcliptic(t);
   omega = 125.04 - 1934.136 * t;
   omega_rad = degToRad(omega);
   
   e = e0 + 0.00256 * cos(omega_rad);

   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%5.3lf) = %5.3lf\n", ++dbgCnt, __PRETTY_FUNCTION__, t, e);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
   return e;               // in degrees
}

//***********************************************************************/
//* Name:    calcSunDeclination                    */
//* Type:    Function                           */
//* Purpose: calculate the declination of the sun           */
//* Arguments:                            */
//*   t : number of Julian centuries since J2000.0          */
//* Return value:                            */
//*   sun's declination in degrees                    */
//***********************************************************************/
double calcSunDeclination(double t)
{
   double e = calcObliquityCorrection(t);
   double lambda = calcSunApparentLong(t);   
   double sint = sin(degToRad(e)) * sin(degToRad(lambda));
   double theta = radToDeg( (double) asin(sint));
   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%5.3lf) = %5.3lf\n", ++dbgCnt, __PRETTY_FUNCTION__, t, theta);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
   return theta;           // in degrees
}

//***********************************************************************/
//* Name:    calcEquationOfTime                    */
//* Type:    Function                           */
//* Purpose: calculate the difference between true solar time and mean  */
//*      solar time                          */
//* Arguments:                            */
//*   t : number of Julian centuries since J2000.0          */
//* Return value:                            */
//*   equation of time in minutes of time                */
//***********************************************************************/
double calcEquationOfTime(double t)
{
   double ret;
   double epsilon = calcObliquityCorrection(t);
   double  l0 = calcGeomMeanLongSun(t);
   double e = calcEccentricityEarthOrbit(t);
   double m = calcGeomMeanAnomalySun(t);
   double y = tan(degToRad(epsilon)/2.0);
   y *= y;
   double sin2l0 = sin(2.0 * degToRad(l0));
   double sinm   = sin(degToRad(m));
   double cos2l0 = cos(2.0 * degToRad(l0));
   double sin4l0 = sin(4.0 * degToRad(l0));
   double sin2m  = sin(2.0 * degToRad(m));
   double Etime = y * sin2l0 - 2.0 * e * sinm + 4.0 * e * y * sinm * cos2l0
                                - 0.5 * y * y * sin4l0 - 1.25 * e * e * sin2m;
   
   ret = radToDeg(Etime)*4.0;   // in minutes of time
   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%5.3lf) = %5.3lf\n", ++dbgCnt, __PRETTY_FUNCTION__, t, ret);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
   return ret;   // in minutes of time
}

//***********************************************************************/
//* Name:    calcHourAngleSunrise                     */
//* Type:    Function                           */
//* Purpose: calculate the hour angle of the sun at sunrise for the  */
//*         latitude                      */
//* Arguments:                            */
//*   lat : latitude of observer in degrees              */
//*   solarDec : declination angle of sun in degrees           */
//* Return value:                            */
//*   hour angle of sunrise in radians                */
//***********************************************************************/
double calcHourAngleSunrise(double lat, double solarDec)
{
   double latRad = degToRad(lat);
   double sdRad  = degToRad(solarDec);
   double HA = (acos( (double)( cos(degToRad(90.833))/(cos(latRad)*cos(sdRad))-tan(latRad) * tan(sdRad))));
   
   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%5.3lf,%5.3lf) = %5.3lf\n", ++dbgCnt, __PRETTY_FUNCTION__, lat, solarDec, HA);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
   return HA;              // in radians
}

//***********************************************************************/
//* Name:    calcHourAngleSunset                   */
//* Type:    Function                           */
//* Purpose: calculate the hour angle of the sun at sunset for the   */
//*         latitude                      */
//* Arguments:                            */
//*   lat : latitude of observer in degrees              */
//*   solarDec : declination angle of sun in degrees           */
//* Return value:                            */
//*   hour angle of sunset in radians                 */
//***********************************************************************/
double calcHourAngleSunset(double lat, double solarDec)
{
   double latRad = degToRad(lat);
   double sdRad  = degToRad(solarDec);   
   double HA = (acos(cos(degToRad(90.833))/(cos(latRad)*cos(sdRad))-tan(latRad) * tan(sdRad)));
   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%5.3lf,%5.3lf) = %5.3lf\n", ++dbgCnt, __PRETTY_FUNCTION__, lat, solarDec, -HA);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
   
   return -HA;              // in radians
}

//***********************************************************************/
//* Name:    calcSunriseUTC                        */
//* Type:    Function                           */
//* Purpose: calculate the Universal Coordinated Time (UTC) of sunrise  */
//*         for the given day at the given location on earth   */
//* Arguments:                            */
//*   JD  : julian day                          */
//*   latitude : latitude of observer in degrees            */
//*   longitude : longitude of observer in degrees          */
//* Return value:                            */
//*   time in minutes from zero Z                     */
//***********************************************************************/
double calcSunriseUTC(double JD, double latitude, double longitude)
{
   double t = calcTimeJulianCent(JD); // *** First pass to approximate sunrise
   double  eqTime = calcEquationOfTime(t);
   double  solarDec = calcSunDeclination(t);
   double  hourAngle = calcHourAngleSunrise(latitude, solarDec);
   double  delta = longitude - radToDeg(hourAngle);
   double  timeDiff = 4 * delta;   // in minutes of time
   double  timeUTC = 720 + timeDiff - eqTime;      // in minutes
   double  newt = calcTimeJulianCent(calcJDFromJulianCent(t) + timeUTC/1440.0);

   eqTime = calcEquationOfTime(newt);
   solarDec = calcSunDeclination(newt);

   hourAngle = calcHourAngleSunrise(latitude, solarDec);
   delta = longitude - radToDeg(hourAngle);
   timeDiff = 4 * delta;
   timeUTC = 720 + timeDiff - eqTime; // in minutes
   
   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%5.3lf,%5.3lf,%5.3lf) = %5.3lf\n", ++dbgCnt, __PRETTY_FUNCTION__, JD, latitude, longitude, timeUTC);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
   return timeUTC;
}

//***********************************************************************/
//* Name:    calcSunsetUTC                      */
//* Type:    Function                           */
//* Purpose: calculate the Universal Coordinated Time (UTC) of sunset   */
//*         for the given day at the given location on earth   */
//* Arguments:                            */
//*   JD  : julian day                          */
//*   latitude : latitude of observer in degrees            */
//*   longitude : longitude of observer in degrees          */
//* Return value:                            */
//*   time in minutes from zero Z                     */
//***********************************************************************/
double calcSunsetUTC(double JD, double latitude, double longitude)
{
   double t = calcTimeJulianCent(JD);
   // *** First pass to approximate sunset
   double  eqTime = calcEquationOfTime(t);
   double  solarDec = calcSunDeclination(t);
   double  hourAngle = calcHourAngleSunset(latitude, solarDec);
   double  delta = longitude - radToDeg(hourAngle);
   double  timeDiff = 4 * delta;   // in minutes of time
   double  timeUTC = 720 + timeDiff - eqTime;      // in minutes
   double  newt = calcTimeJulianCent(calcJDFromJulianCent(t) + timeUTC/1440.0);
    
   eqTime = calcEquationOfTime(newt);
   solarDec = calcSunDeclination(newt);   
   
   hourAngle = calcHourAngleSunset(latitude, solarDec);
   delta = longitude - radToDeg(hourAngle);
   timeDiff = 4 * delta;
   timeUTC = 720 + timeDiff - eqTime; // in minutes
   
   // printf("************ eqTime = %f  \nsolarDec = %f \ntimeUTC = %f\n\n",eqTime,solarDec,timeUTC);   
   DBGSNPRINTF_SUNRISE_VERBOSE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%03d: %s(%5.3lf,%5.3lf,%5.3lf) = %5.3lf\n", ++dbgCnt, __PRETTY_FUNCTION__, JD, latitude, longitude, timeUTC);
   DBGPRINT_SUNRISE_VERBOSE(__LINE__, DEBUGBUF2, (char *) NULL);
   return timeUTC;
}

//-------------------------------------------------------------
//  getNextSunTimes()
//   This will transmit a request to the Naval Observatory to
//  get the sunrise and sunset for the long/lat that we 
//  have been given by the App.  
//
//   nextDate = date wanted for sunrise/sunset
//
//   sunriseFlag = True if requesting sun rise
//                 False if requesting sunset
//
//   newDate = requested sunrise or sunset time/date
//
//   returns TRUE if error occurs
//  
//-------------------------------------------------------------

boolean getNextSunTimes(DATE_STRUCT nextDate, boolean sunriseFlag, DATE_STRUCT *newDate)
{
   int_32 result;
   boolean returnErrorFlag = FALSE; // cannot fail!

   time_t seconds;
   time_t tseconds;
   struct tm  *ptm=NULL;
   struct tm  tm;
   
   int year = nextDate.YEAR;
   int month = nextDate.MONTH;
   int day = nextDate.DAY;
   // int dst = 0;
   // zoneOffset
   double latitude = systemProperties.latitude.degrees + ((double) systemProperties.latitude.minutes / 60.0);
   double longitude = systemProperties.longitude.degrees + ((double) systemProperties.longitude.minutes / 60.0);
   int zoneOffsetMin = systemProperties.effectiveGmtOffsetSeconds / 60;

   longitude = -longitude;
   
#if DEBUG_EVENTTASK_SUNRISE
   dbgCnt = 0;
   dbgTotalCnt = 0;
   DBGSNPRINTF_SUNRISE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%s:%d year=%d month=%d day=%d latitude=%5.3lf longitude=%5.3lf zoneOffsetMin=%d sunriseFlag=%d\n", __PRETTY_FUNCTION__, __LINE__, year, month, day, latitude, longitude, zoneOffsetMin, sunriseFlag);
   DBGPRINT_SUNRISE(__LINE__, DEBUGBUF2, (char *) NULL);
#endif
   
   volatile float utcMinutes = calcJD(year,month,day);
   DBGSNPRINTF_SUNRISE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%s:%d utcMinutes = %5.3lf = calcJD(year,month,day)\n", __PRETTY_FUNCTION__, __LINE__, utcMinutes);
   DBGPRINT_SUNRISE(__LINE__, DEBUGBUF2, (char *) NULL);

   if (sunriseFlag)
   {
      utcMinutes = calcSunriseUTC( utcMinutes,  latitude,  longitude);
      DBGSNPRINTF_SUNRISE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%s:%d utcMinutes = %5.3lf = calcSunriseUTC(utcMinutes,latitude,longitude);\n", __PRETTY_FUNCTION__, __LINE__, utcMinutes);
      DBGPRINT_SUNRISE(__LINE__, DEBUGBUF2, (char *) NULL);
   }
   else
   {
      utcMinutes = calcSunsetUTC( utcMinutes,  latitude,  longitude);
      DBGSNPRINTF_SUNRISE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%s:%d utcMinutes = %5.3lf = calcSunsetUTC(utcMinutes,latitude,longitude);\n", __PRETTY_FUNCTION__, __LINE__, utcMinutes);
      DBGPRINT_SUNRISE(__LINE__, DEBUGBUF2, (char *) NULL);
   }

   utcMinutes += (double) zoneOffsetMin; // negative offset TODO SAMF this number screws us
   if (utcMinutes < 0)
   {
      utcMinutes += (double) (24 * 60);
   }
      
   memcpy(newDate,&nextDate,sizeof(nextDate));
   newDate->HOUR = (int) (utcMinutes / 60.0);
   newDate->MINUTE = (int) (utcMinutes - (double) (newDate->HOUR * 60));
   newDate->SECOND = (int) (60.0 * (utcMinutes - ((double) newDate->MINUTE) - ((double) (newDate->HOUR * 60))));
   newDate->YEAR = year; 
   newDate->MONTH = month;
   newDate->DAY = day;

   DBGSNPRINTF_SUNRISE(DEBUGBUF2,SIZEOF_DEBUGBUF2,"%s:%d %s = %d/%d/%d %d:%02d:%02d utcMinutes=%5.3lf\n", __PRETTY_FUNCTION__, __LINE__, 
         (sunriseFlag) ? "sunrise" : "sunset", newDate->YEAR, newDate->MONTH, newDate->DAY, newDate->HOUR, newDate->MINUTE, newDate->SECOND, utcMinutes);
   DBGPRINT_SUNRISE(__LINE__, DEBUGBUF2, (char *) NULL);
      
   return returnErrorFlag;
}

#else // not USE_NEW_SUNRISE_CALCULATOR

//-------------------------------------------------------------
//  parseSunTimes()
//   given a buffer from the  weather server, this will
// get the sunrise and sunset from the buffer.
// 
//  String will contain:
//      Sunrise="7:13 " Sunset="5:28 "
//
//   sunriseFlag = True if requesting sun rise
//                 False if requesting sunset
//
// returns true if no sunrise or sunset found, else 
//         false when we have found the strings
//  
//-------------------------------------------------------------

bool parseSunTimes(char *recvBuff, DATE_STRUCT myDate, boolean sunriseFlag, DATE_STRUCT *newDate)
{
   char sunSet[12];
   char sunRise[10];
   char *substrings;
   int ix = 0;

   memset(sunRise, 0, sizeof(sunRise));  // clear out any junk in memory
   memset(sunSet, 0, sizeof(sunSet));  // clear out any junk in memory

   DBGPRINT_SUNRISE(__LINE__,__PRETTY_FUNCTION__,recvBuff);

   if (sunriseFlag)  // asking for sun rise
   {
      // find the Sunrise and store in global variable
      substrings = strstr(recvBuff, "Sunrise");
      if (substrings != NULL)  // if we found it
      {
         ix = 0;
         substrings += 16;  // skip past "Sunrise</td><td>" 
         while ((*substrings != ' ') && (*substrings != 0) && (ix < 5))
         {
            sunRise[ix] = *substrings;
            ix++;
            substrings++;
         }
         if (*substrings == 0)
         {
            DBGPRINT_SUNRISE(__LINE__,__PRETTY_FUNCTION__,"sunrise hit a null before we got our complete time");
            return TRUE;   // hit a null before we got our complete time "xx:xx"
         }

         // Hour and Minute from buffer, date is passed in
         newDate->HOUR = atoi(sunRise);
         substrings = strstr(sunRise, ":");
         if (substrings != NULL)
         {
            newDate->MINUTE = atoi(&substrings[1]);  // go past ":" and get the minutes
            newDate->YEAR = myDate.YEAR;
            newDate->MONTH = myDate.MONTH;
            newDate->DAY = myDate.DAY;
            DBGPRINT_SUNRISE(__LINE__,__PRETTY_FUNCTION__,"sunrise looks good");
            return FALSE;
         }
         else  // we didn't find ':', so reset hour to 0 and quit
         {
            newDate->HOUR = 0;
            DBGPRINT_SUNRISE(__LINE__,__PRETTY_FUNCTION__,"sunrise hour zero");
            return TRUE;
         }
      }

   }  // end of if sunriseFlag is true
   else  // else, want the sunset
   {
      // find the Sunset and store in global variable
      substrings = strstr(recvBuff, "Sunset");
      if (substrings != NULL)  // if we found it
      {
         ix = 0;
         substrings += 15;  // skip past "Sunset</td><td>"
         while ((*substrings != ' ') && (*substrings != 0) && (ix < 5))
         {
            sunSet[ix] = *substrings;
            ix++;
            substrings++;
         }
         if (*substrings == 0)
         {
            DBGPRINT_SUNRISE(__LINE__,__PRETTY_FUNCTION__,"sunset hit a null before we got our complete time");
            return TRUE;   // hit a null before we got our complete time "xx:xx"
         }

         // Hour and Minute from buffer, date is passed in
         newDate->HOUR = atoi(sunSet);
         substrings = strstr(sunSet, ":");
         if (substrings != NULL)
         {
            newDate->MINUTE = atoi(&substrings[1]);  // go past ":" and get the minutes
            newDate->YEAR = myDate.YEAR;
            newDate->MONTH = myDate.MONTH;
            newDate->DAY = myDate.DAY;
            DBGPRINT_SUNRISE(__LINE__,__PRETTY_FUNCTION__,"sunset looks ok");
            return FALSE;
         }
         else
         {
            newDate->HOUR = 0;
            DBGPRINT_SUNRISE(__LINE__,__PRETTY_FUNCTION__,"sunset hour zero");
            return TRUE;
         }
      }

   }

   // get here because we didn't find Sunset or SunRise substring

   substrings = strstr(recvBuff, "Missing Sun phenomena indicate");
   if (substrings != NULL)  // if string was found, 
   {
      if (sunriseFlag)
      {
         newDate->HOUR = 0;
         newDate->MINUTE = 1;
         newDate->YEAR = myDate.YEAR;
         newDate->MONTH = myDate.MONTH;
         newDate->DAY = myDate.DAY;
      }
      else
      {
         newDate->HOUR = 23;
         newDate->MINUTE = 59;
         newDate->YEAR = myDate.YEAR;
         newDate->MONTH = myDate.MONTH;
         newDate->DAY = myDate.DAY;
      }
      return FALSE;
   }
   else
   {
      return TRUE;
   }
}

//-------------------------------------------------------------
//  getNextSunTimes()
//   This will transmit a request to the Naval Observatory to
//  get the sunrise and sunset for the long/lat that we 
//  have been given by the App.  
//
//   nextDate = date wanted for sunrise/sunset
//
//   sunriseFlag = True if requesting sun rise
//                 False if requesting sunset
//
//   newDate = requested sunrise or sunset time/date
//
//   returns TRUE if error occurs
//  
//-------------------------------------------------------------

boolean getNextSunTimes(DATE_STRUCT nextDate, boolean sunriseFlag, DATE_STRUCT *newDate)
{
   char hostname[MAX_HOSTNAMESIZE];
   struct sockaddr_in remote_sin;
   uint_32 sunSock = RTCS_SOCKET_ERROR;
   int_32 result;
   _ip_address hostaddr = 0;
   boolean returnErrorFlag = TRUE;
   byte_t retryCount;
   uint_16 rcvBufCt;
   boolean parseErrorFlag;

   // Get the IP address of the time server
   // By giving the host name, this returns the IP Address
   for(retryCount = 0; (retryCount < MAX_NUMBER_OF_IP_ADDRESS_RESOLUTION_ATTEMPTS) && (hostaddr == 0); retryCount++)
   {
      RTCS_resolve_ip_address("aa.usno.navy.mil", &hostaddr, hostname, MAX_HOSTNAMESIZE);
   }

   if (hostaddr != 0)
   {
      DBGPRINT_SUNRISE(__LINE__,__PRETTY_FUNCTION__,"resolved aa.usno.navy.mil for sun times");
      // Open a TCP socket to talk to the navy server
      sunSock = socket(PF_INET, SOCK_STREAM, 0);

      if (sunSock != RTCS_SOCKET_ERROR)
      {
         // Set the socket options to timeout after 0.5 seconds
#ifndef AMDBUILD
         uint_32 opt_value = 500;
         result = setsockopt(sunSock, SOL_TCP, OPT_CONNECT_TIMEOUT, &opt_value, sizeof(opt_value));
#else
         // For the server build, the timeout value is a timeval and is set in
         // seconds and microseconds.
         struct timeval opt_value;
         opt_value.tv_sec = 1;
         opt_value.tv_usec = 0;
         result = setsockopt(sunSock, SOL_SOCKET, SO_RCVTIMEO, &opt_value, sizeof(opt_value));
#endif

         if (result != RTCS_ERROR)
         {
            // Set up the remote time server address
            memset(&remote_sin, 0, sizeof(remote_sin));
            remote_sin.sin_family = AF_INET;
#ifndef AMDBUILD
            remote_sin.sin_port = 80;
#else
            // For server build, the port needs to be set to network byte order
            remote_sin.sin_port = htons(80);
#endif
            remote_sin.sin_addr.s_addr = hostaddr;

            result = connect(sunSock, (struct sockaddr *) &remote_sin, sizeof(remote_sin));

            if (result == RTCS_OK)
            {
               memset(sendPkt, 0, SUN_PACKET_TX_SIZE);

               // get the next days sunrise and sunset using the following http format:
               // http://aa.usno.navy.mil/rstt/onedaytable?form=2&ID=AA&year=2015&month=6&day=20&lat_sign=1&lat_deg=65&lat_min=0&lon_sign=-1&lon_deg=156&lon_min=0&tz_sign=+1&tz=0
               int zoneOffset = systemProperties.effectiveGmtOffsetSeconds / 3600;

               snprintf(sendPkt, sizeof(sendPkt),
                     "GET /rstt/onedaytable?form=2&ID=AA&year=%d&month=%d&day=%d&lat_sign=1&lat_deg=%d&lat_min=%d&lon_sign=-1&lon_deg=%d&lon_min=%d&tz_sign=1&tz=%d HTTP/1.1\r\n"
                     "HOST: aa.usno.navy.mil\r\n"
                     "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
                     "Connection: keep-alive\r\n"
                     "Cache-Control: max-age=0\r\n"
                     "\r\n", nextDate.YEAR, nextDate.MONTH, nextDate.DAY, systemProperties.latitude.degrees, systemProperties.latitude.minutes,
                     systemProperties.longitude.degrees * -1,  // make positive because we indicate long sign as -1
                     systemProperties.longitude.minutes, zoneOffset);

               DBGPRINT_SUNRISE(__LINE__,__PRETTY_FUNCTION__,sendPkt);

               result = send(sunSock, &sendPkt, strlen(sendPkt), 0);

               if(result != RTCS_ERROR)
               {
                  rcvBufCt = 0;  // start buffer ct at 0
                  parseErrorFlag = TRUE;
                  retryCount = 0;

                  // clear out any junk in memory
                  memset(recvPkt, 0, sizeof(recvPkt)); 

                  returnErrorFlag = FALSE;   // assume we're going to find our data
                  while ((parseErrorFlag == TRUE) && 
                         (retryCount < TRY_AGAIN_FOR_BUFFER) && 
                         (returnErrorFlag == FALSE))
                  {
                     // wait for response from server
                     // NOTE: we leave one byte 
                     result = recv(sunSock, &(recvPkt[rcvBufCt]), ((sizeof(recvPkt) - 1) - rcvBufCt), 0);
                     
                     if ((result == RTCS_ERROR) || (result == RTCS_SOCKET_ERROR))
                     {
                        // Failed to receive data. Break the loop
                        returnErrorFlag = TRUE;
                        DBGPRINT_SUNRISE(__LINE__,__PRETTY_FUNCTION__,"failed to recv sun times");
                     }
                     else if (result > 0)  // if we have some data in our buffer, parse it
                     {
  
                        // null terminate buffer, for strstr use in parser
                        recvPkt[rcvBufCt + result] = 0;

                        // get the sunrise and sunset from the buffer that we got from the server
                        parseErrorFlag = parseSunTimes(recvPkt, nextDate, sunriseFlag, newDate);
                        DBGPRINT_SUNRISE(parseErrorFlag,__PRETTY_FUNCTION__,"parseErrorFlag result from parseSunTimes");
                        rcvBufCt += result;
                        if (rcvBufCt >= (sizeof(recvPkt) - 1))
                        {
                           memcpy(&recvPkt[0], &recvPkt[sizeof(recvPkt) / 2], sizeof(recvPkt) / 2);
                           rcvBufCt = (sizeof(recvPkt) / 2) - 1;  // subtract one to ensure we overwrite the previous null terminator
                        }

                     }

                     // Increment the loop counter
                     retryCount++;

                  }  // end of while (flag == TRUE) && (retryCount < TRY_AGAIN_FOR_BUFFER ) && (returnErrorFlag == FALSE)
                  
                  // if parseErrorFlag is TRUE; we didn't get a suntime from the packets
                  if ((retryCount == TRY_AGAIN_FOR_BUFFER) && (parseErrorFlag == TRUE))
                  {
                      returnErrorFlag = TRUE;   // indicate an error, if we ran out of retries and still didn't get a suntime
                      DBGPRINT_SUNRISE(__LINE__,__PRETTY_FUNCTION__,"failed to recv sun times after retry");
                  }

               }  // if(result != RTCS_ERROR)
            
            }  // if (result == RTCS_OK)
         
         }  // if (result != RTCS_ERROR)
      
      }  // if (sunSock != RTCS_SOCKET_ERROR)
   
   }  // if (hostaddr != 0)

   if (sunSock != RTCS_SOCKET_ERROR)
   {
      // Shutdown and close the socket
       
      shutdown(sunSock, FLAG_ABORT_CONNECTION);
#ifdef AMDBUILD
      close(sunSock);
#else
      // TODO: No way to close socket in our version of MQX. Assuming shutdown takes care of it
      // closesocket(sunSock); is available in the most current version
#endif
   }

   DBGPRINT_SUNRISE(returnErrorFlag,__PRETTY_FUNCTION__,"returnErrorFlag after sun times after retry return");
   return returnErrorFlag;
}

#endif // 0

//-------------------------------------------------------------
// isItDaylightSavingTime()
//
//   this will take the date and determine if it is  
//  daylight savings time.
//
//  DST Starts in the second Sunday in March at 2:00 am
//    the 2nd Sun will always be between 8th and 14th inclusive 
//
//  DST end on the first Sunday in Nov at 2:00 am
//    the 1st Sun will always be between 1st and 7th inclusive 
//    
//   
//-------------------------------------------------------------

boolean isItDST(DATE_STRUCT myTime)
{
   bool dstFlag = TRUE;

   if ((myTime.MONTH < 3) || (myTime.MONTH > 11))
   {  // if Jan, Feb, or Dec, not DST
      dstFlag = FALSE;
   }   
   else if ((myTime.MONTH > 3) && (myTime.MONTH < 11))
   {  // if between Mar and Nov, it is DST
      dstFlag = TRUE;
   }
   else
   {
      if (myTime.MONTH == 3)
      {  // in March
         if (myTime.DAY < 8)
         {  // Before the second sunday. Always not in DST
            dstFlag = FALSE;
         }
         else if ((myTime.DAY >= 8) && 
                  (myTime.DAY <= 14))
         {
            if (myTime.WDAY == 0)
            {
               if (myTime.HOUR < 2)
               {  // On the sunday of the time change. Before 2 DST = FALSE
                  dstFlag = FALSE;
               }
               else
               {  // On the sunday of the time change. After 2 DST = TRUE
                  dstFlag = TRUE;
               }
            }
            else
            {  // Need to determine if it is before the second sunday or after
               if ((myTime.DAY-7) > myTime.WDAY)
               {  // After the second Sunday
                  dstFlag = TRUE;
               }
               else
               {  // Before the second Sunday
                  dstFlag = FALSE;
               }
            }
         }
         else
         {  // After the second sunday. Always in DST
            dstFlag = TRUE;
         }
      }  // if (myTime.MONTH == 3)
      else
      {  // In November
         // It it is on or after the first Sunday at 2:00 AM, DST = FALSE
         // If it is on or before the first Sunday at 1:00 AM, DST = TRUE
         // If it is on the first Sunday between 1:00 and 2:00 AM, DST = flag
         if (myTime.DAY <= 7)
         {
            if (myTime.WDAY == 0)
            {  // we're on the day when dst changes
               if (myTime.HOUR >= 2)
               {
                  dstFlag = FALSE;  // always in standard time if we're at or past 2:00
               }
               else if (myTime.HOUR < 1)
               {
                  dstFlag = TRUE;  // always in DST if we're before 1:00
               }
               else
               {
                  dstFlag = inDST;  // during this gray time, we remain as we're set
               }
            }
            else if (myTime.DAY > myTime.WDAY)
            {  // After the first Sunday
               dstFlag = FALSE;
            }
            else
            {  // Before the first Sunday
               dstFlag = TRUE;
            }
         }
         else
         {  // After the first Sunday
            dstFlag = FALSE;
         }
      }
   }

   return dstFlag;

}

// ----------------------------------------------------------------------------
// adjustTriggers()
//    given an adjustment value, this function will adjust all of the scene
//  triggers.  The adjustment value can be positive or negative.
//
// ----------------------------------------------------------------------------

void adjustTriggers(int32_t adjustment)
{
   int sceneIdx;
   scene_properties sceneProperties;
   
   _mutex_lock(&ScenePropMutex);  // take mutex because we are changing scene properties
   
   for (sceneIdx = 0; sceneIdx < MAX_NUMBER_OF_SCENES; sceneIdx++)
   {
      if (GetSceneProperties(sceneIdx, &sceneProperties))
      {
         if ((sceneProperties.SceneNameString[0] != 0) &&
             (sceneProperties.trigger != REGULAR_TIME))
         {
            sceneProperties.nextTriggerTime += adjustment;
            broadcastNewSceneTriggerTime(sceneIdx, &sceneProperties);

            SetSceneProperties(sceneIdx, &sceneProperties);
         }
      }
   }
   
   _mutex_unlock(&ScenePropMutex);
}

//-------------------------------------------------------------
// checkForDST()
//   this will check to see if DST has rolled over and adjust 
//  time if has, and it will also broadcast that it has.
//-------------------------------------------------------------

void checkForDST(DATE_STRUCT curTime)
{

   TIME_STRUCT systemTime;

   if(systemProperties.useDaylightSaving)
   {
      if ((!inDST) && (isItDST(curTime) == TRUE))
      {
         inDST = TRUE;
         // spring forward
         systemProperties.effectiveGmtOffsetSeconds = systemProperties.gmtOffsetSeconds + 3600;

         _time_get(&systemTime);
         // update our real time clock to adjust for DST
         systemTime.SECONDS += 3600;
         // set the real time clock to spring forward
         _time_set(&systemTime);

         // broadcast new effectiveGMToffsetSeconds
         broadcastEffGmtOffsetChange();

         // ADJUST ALL OF THE SCENE TIMES THAT ARE Sunrise/Sunset TIME
         adjustTriggers(3600);

         // Save the effective GMT Offset to flash
         flashSaveTimer = FLASH_SAVE_TIMER;
      }
      else if ((inDST) && (isItDST(curTime) == FALSE))
      {
         inDST = FALSE;
         // Fall back
         systemProperties.effectiveGmtOffsetSeconds = systemProperties.gmtOffsetSeconds;

         _time_get(&systemTime);
         // update our real time clock to adjust for DST
         systemTime.SECONDS -= 3600;
         // set the real time clock to fall back
         _time_set(&systemTime);

         // broadcast new effectiveGMToffsetSeconds
         broadcastEffGmtOffsetChange();

         // ADJUST ALL OF THE SCENE TIMES THAT ARE REG TIME
         adjustTriggers(-3600);

         // Save the effective GMT Offset to flash
         flashSaveTimer = FLASH_SAVE_TIMER;
      }
   }
   else if(inDST)
   {
      // Should not use DST but it currently using it
      // Set the DST flag to false
      inDST = FALSE;

      // Update the effective GMT Offset
      systemProperties.effectiveGmtOffsetSeconds = systemProperties.gmtOffsetSeconds;

      // Update the RTC to adjust for DST
      _time_get(&systemTime);
      systemTime.SECONDS -= 3600;
      _time_set(&systemTime);

      // Broadcast the change
      broadcastEffGmtOffsetChange();

      // Adjust all of the scene trigger times
      adjustTriggers(-3600);

      // This function is only called when the useDaylightSavings flag is
      // set in Socket Task. That function sets the flashSaveTimer so it is
      // not necessary to do that from here.
   }
}

// ----------------------------------------------------------------------------

#ifndef AMDBUILD

#define WAIT_ONE_MINUTE         60000L    // milliseconds

static _timer_id oneMinuteTimer;

//-------------------------------------------------------------
// ctTimerFunction() --
//   routine that is called every time the timer fires in the
//  event task.  Set up to go off every minute.  This will post
//  a message to the event task, indicating that it should
//  check for scenes and other work that it required to do
//
//-------------------------------------------------------------

void ctTimerFunction(_timer_id id, pointer event_ptr, uint_32 seconds, uint_32 milliseconds)
{
   // Inc a 24 hour counter, if we have gone one day, go get time again
   //   to re-sync the time in MQX
   IncrementMinuteCounter();

   _event_set(event_ptr, 0x01);

}

// ----------------------------------------------------------------------------

void AlignToMinuteBoundary()
{
   TIME_STRUCT mqxTime;
   TIME_STRUCT myTime;
   DATE_STRUCT myDate;

   // Get the current time
   _time_get(&mqxTime);

   // Cancel the minute timer
   if(oneMinuteTimer != TIMER_NULL_ID)
   {
      _timer_cancel(oneMinuteTimer);

   }

   // Calculate the time needed to delay until the next minute boundary
   myTime.SECONDS = mqxTime.SECONDS;
   myTime.MILLISECONDS = 0;
   _time_to_date(&myTime, &myDate);
   int delayTime = (60 - myDate.SECOND) * 1000; 

   // Delay until the next minute boundary
   _time_delay(delayTime);

   // Restart the periodic timer on the minute boundary
   _time_get(&mqxTime);
   myTime.SECONDS = mqxTime.SECONDS;
   myTime.MILLISECONDS = 0;
   _time_to_date(&myTime, &myDate);  // convert seconds to date/time
   oneMinuteTimer = _timer_start_periodic_every(ctTimerFunction, event_ptr, (_mqx_uint) TIMER_ELAPSED_TIME_MODE, (uint32_t) WAIT_ONE_MINUTE);
}

//-------------------------------------------------------------
// Event_Timer_Task()
//
//    this task will initialize the real time clock, wake up every 
//  minute and determine if any scene should be activated, and
//  call the appropriate function.  It will also update the
//  real-time clock every day.
// 
//
//-------------------------------------------------------------

void Event_Timer_Task()
{
   TIME_STRUCT mqxTime;

   _rtc_init(NULL);  // initialize the real time clock
   mqxTime.SECONDS = DEFAULT_SYSTEM_TIME;

   _time_set(&mqxTime);  // seed  MQX keeps time as unix time

   if (_timer_create_component(TIMER_DEFAULT_TASK_PRIORITY, (TIMER_DEFAULT_STACK_SIZE * 2)) != MQX_OK)
   {
      return;  // can't start if we can't create the event timer
   }

   // set up our event, which indicates that our 1 minute time went off
   _event_create("timer");
   _event_open("timer", &event_ptr);

   // call ctTimerFunction every minute forever
   // wake up every 60,000 milliseconds or one minute
   oneMinuteTimer = _timer_start_periodic_every(ctTimerFunction, event_ptr, (_mqx_uint) TIMER_ELAPSED_TIME_MODE, (uint32_t) WAIT_ONE_MINUTE);

   for (int retryCount = 0; retryCount < 4; retryCount++)
   {
      // Returns true if time was not received
      // Real Time Clock is set in this function
      if(!getLocalTime(SNTP_REQUEST_TIMEOUT))
      {
         systemTimeValid = true;
         broadcastTimeChange();
         break;
      }
   }

   while (1)  // task forever loop
   {
       _mutex_lock(&TaskHeartbeatMutex);
       task_heartbeat_bitmask |= Event_Timer_Task_HEARTBEAT;
       _mutex_unlock(&TaskHeartbeatMutex);
       
      // wait for ever for the event to be set, by timer call back routine
      _event_wait_all(event_ptr, 0x01, 0);  // event_ptr is set in the timer routine, every minute

      _event_clear(event_ptr, 0x01);  // clear the bit so it can be set again by the timer

      // Handle the minute change
      HandleMinuteChange();
   }  // while (1)

}

//-----------------------------------------------------------------------------

void broadcastJsonBufferPacket(char * tagPtr, byte_t * bufferPtr, word_t bufferSize)
{
   json_t * broadcastObject;
   json_t * packetArrayObject;
   char hexString[3];

   broadcastObject = json_object();
   if (broadcastObject)
   {
      packetArrayObject = json_array();
      if (packetArrayObject)
      {
         json_object_set_new(broadcastObject, "ID", json_integer(0));
         json_object_set_new(broadcastObject, "Service", json_string("BufferPacket"));
         json_object_set_new(broadcastObject, "Tag", json_string(tagPtr));
      
         while (bufferSize)
         {
            snprintf(hexString, sizeof(hexString), "%02X", *bufferPtr);
            json_array_append_new(packetArrayObject, json_string(hexString));
            bufferPtr++;
            bufferSize--;
         }
         json_object_set_new(broadcastObject, "Buffer", packetArrayObject);
      
         json_object_set_new(broadcastObject, "Status", json_string("Success"));
         SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
      }
      json_decref(broadcastObject);
   }
}

//-----------------------------------------------------------------------------

#endif
