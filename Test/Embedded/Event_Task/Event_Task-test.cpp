#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Event_TaskTest

#define MAIN
#include <boost/test/unit_test.hpp>
#include "includes.h"
#include "Debug.h"
#include "FlashStub.h"

using namespace legrand::rflc::scenes;
using namespace boost::unit_test::framework;

extern "C"
{
   extern pthread_mutex_t ZoneArrayMutex;
   extern pthread_mutex_t ScenePropMutex;

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------

   void SendJSONPacket(byte_t socketIndex, json_t *packetToSend)
   {
      logVerbose("Send Json Packet");

      if(socketIndex == MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS)
      {
           logInfo("Sending a broadcast packet");
      }
      else
      {
         logError("Only handles broadcast calls at this time");
      }
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------

   int _msgq_send_priority(RFM100_TRANSMIT_TASK_MESSAGE_PTR message, uint8_t priority)
   {
      logDebug("Adding message to the transmit queue");

      
      return 1;
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------

   int RFM100_Send(byte_t *buffer, int transmitLength)
   {
      // Simulate time it takes to send a message and receive the
      // acknowledgement over the RF
      _time_delay(500);
      return transmitLength;
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------

   void AlignToMinuteBoundary()
   {
      
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
}


void printTestCase(std::string testCase)
{
   logInfo(" ");
   logInfo("************************************************************");
   logInfo("Running Test Case %s", testCase.c_str());
   logInfo("************************************************************");
   logInfo(" ");
}

// Fixture to use for the test cases that are FIXTURE_TEST_CASES
struct flashFixture
{
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   std::string zoneFilename = "Zones.txt";
   std::string sceneFilename = "Scenes.txt";
   std::string sceneControllerFilename = "SceneControllers.txt";
   std::string samsungZonePropertiesFilename = "SamsungZoneProperties.txt";
   std::string samsungScenePropertiesFilename = "SamsungSceneProperties.txt";

   
   flashFixture()
   {
      // Remove the files if they already exist. So each test uses the defaults
      std::remove(zoneFilename.c_str());
      std::remove(sceneFilename.c_str());
      std::remove(sceneControllerFilename.c_str());
      std::remove(samsungZonePropertiesFilename.c_str());
      std::remove(samsungScenePropertiesFilename.c_str());

      // start flash stub code - read/write properties to a file, instead of NVram
      fl = new FlashStub(zoneFilename, sceneFilename, sceneControllerFilename, samsungZonePropertiesFilename, samsungScenePropertiesFilename);
        
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   
   ~flashFixture()
   {
      logDebug("Closing the JsonFixture");
      delete fl;
      fl = NULL;

      // Remove the files
      std::remove(zoneFilename.c_str());
      std::remove(sceneFilename.c_str());
      std::remove(sceneControllerFilename.c_str());
      std::remove(samsungZonePropertiesFilename.c_str());
      std::remove(samsungScenePropertiesFilename.c_str());
   }

   FlashStub *fl;
   
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Event_getLocalTime)
{
   printTestCase(current_test_case().p_name.value);
   
   TIME_STRUCT systemTime;
   systemTime.SECONDS = 0;
   boolean chkTime = false;
   int ct=0;
   int ctGood=0;
   int ctBad=0;

   // initialize the global system properties
   InitializeSystemProperties();

   // run the getLocalTime() 180 times and check for # of failures
   while ( ct < 180 )
   {
      chkTime = getLocalTime(5000);   
      _time_get(&systemTime);
      if (!chkTime)
      {
          ctGood++;  // no error
      }
      else // we got an error
      {
          logInfo("%d) chkTime:%d,  Time:%d", ct, chkTime, systemTime.SECONDS);
          ctBad++;
      }

      ct++;
   }
  
   logInfo("Ct 0:%d", ctGood);
   logInfo("Ct 1:%d", ctBad);

   logInfo("chkTime:%d,  Time:%d", chkTime, systemTime.SECONDS);

   BOOST_CHECK(ctGood > 120);  // ctZero cts the good returns,ie:we got time
   BOOST_CHECK(ctBad  < 30);   // ctOne - # of errors - should be less than 30


   logInfo("#############################################################");
   logInfo("        Finished with getLocalTime   ");
   logInfo("#############################################################");
   logInfo(" ");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Event_getNextSunTimes)
{
   printTestCase(current_test_case().p_name.value);
   
   boolean     chkDay;
   DATE_STRUCT nextDate;
   boolean     sunriseFlag = TRUE;
   DATE_STRUCT newDate;

   // initialize the global system properties
   InitializeSystemProperties();
   systemProperties.latitude.degrees = 40;
   systemProperties.longitude.degrees = -77;
   systemProperties.gmtOffsetSeconds = -18000;
   systemProperties.effectiveGmtOffsetSeconds = -14400;
   inDST = TRUE;

   // set up date to be 10/2/2015, sunrise should be on the 6th hour
   nextDate.YEAR   = 2015;
   nextDate.MONTH  = 10;
   nextDate.DAY    = 2;
   nextDate.HOUR   = 8;
   nextDate.MINUTE = 35;
   nextDate.SECOND = 0;
   nextDate.WDAY   = 3;
   nextDate.YDAY   = 244;
   
   chkDay = getNextSunTimes(nextDate, sunriseFlag, &newDate);

   logInfo("chkDay:%d,  SUNRISE:%02d/%02d/%02d  %02d:%02d", chkDay, 
                                      newDate.MONTH, newDate.DAY, newDate.YEAR, newDate.HOUR, newDate.MINUTE);

   BOOST_CHECK_EQUAL(chkDay, 0);  // getNextSunTimes returns 0 if OK

   BOOST_CHECK_EQUAL( newDate.HOUR, 7 );    // should be 7 am in Oct

   BOOST_CHECK_EQUAL( newDate.DAY, 2 );     // should be 2, like I asked for
  
   BOOST_CHECK_EQUAL( newDate.MONTH, 10 );  // should be 10, like I asked for
 
   BOOST_CHECK_EQUAL( newDate.YEAR, 2015 ); // should be 2015, like I asked for

   sunriseFlag = FALSE;  // now ask for sunset

   chkDay = getNextSunTimes(nextDate, sunriseFlag, &newDate);
  
   logInfo("chkDay:%d,  SUNSET: %02d/%02d/%02d  %02d:%02d", chkDay, 
                                      newDate.MONTH, newDate.DAY, newDate.YEAR, newDate.HOUR, newDate.MINUTE);


   BOOST_CHECK_EQUAL(chkDay, 0);  // getNextSunTimes returns 0 if OK

   BOOST_CHECK_EQUAL( newDate.HOUR, 18 ); // should be 6pm in Oct

   BOOST_CHECK_EQUAL( newDate.DAY, 2 ); // should be 2, like I asked for
 
   BOOST_CHECK_EQUAL( newDate.MONTH, 10 ); // should be 10, like I asked for

   BOOST_CHECK_EQUAL( newDate.YEAR, 2015 ); // should be 2015, like I asked for

   ////// Check function for handling of past times
   // set up date to be 10/2/2010, sunrise should be on the 7th hour
   nextDate.YEAR   = 2010;
   nextDate.MONTH  = 10;
   nextDate.DAY    = 2;
   nextDate.HOUR   = 8;
   nextDate.MINUTE = 35;
   nextDate.SECOND = 0;
   nextDate.WDAY   = 3;
   nextDate.YDAY   = 244;
   
   sunriseFlag = TRUE; // get sunrise

   chkDay = getNextSunTimes(nextDate, sunriseFlag, &newDate);

   logInfo("chkDay:%d,  SUNRISE:%02d/%02d/%02d  %02d:%02d", chkDay, 
                                      newDate.MONTH, newDate.DAY, newDate.YEAR, newDate.HOUR, newDate.MINUTE);

   BOOST_CHECK_EQUAL(chkDay, 0);  // getNextSunTimes returns 0 if OK

   BOOST_CHECK_EQUAL( newDate.HOUR, 7 );   // should be 7 am in Oct

   BOOST_CHECK_EQUAL( newDate.DAY, 2 );    // should be 2, like I asked for
   
   BOOST_CHECK_EQUAL( newDate.MONTH, 10 ); // should be 10, like I asked for
 
   BOOST_CHECK_EQUAL( newDate.YEAR, 2010 ); // should be 2010, like I asked for
  

   logInfo("#############################################################");
   logInfo("        Finished with getNextSunTimes   ");
   logInfo("#############################################################");
   logInfo(" ");
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Event_calculateNewTrigger)
{
   printTestCase(current_test_case().p_name.value);

   scene_properties sceneProperties;
   TIME_STRUCT rtcTime;

   // initialize the global system properties
   InitializeSystemProperties();
   systemProperties.latitude.degrees = 40;
   systemProperties.longitude.degrees = -77;
   systemProperties.gmtOffsetSeconds = -18000;
   systemProperties.effectiveGmtOffsetSeconds = -14400;
   inDST = TRUE;

   rtcTime.SECONDS = 1441099765; // 9/1/2015 5:29 am EST, tuesday
   
   /////////////////////////////////////////////////////////////////////////////////////////
   // setup scene to be regular time, going every day, so that trigger time should be 
   //    rtcTime.SECONDS + 24 hours
   sceneProperties.numZonesInScene = 1;
   sceneProperties.zoneData[0].zoneId = 0;
   sceneProperties.zoneData[0].powerLevel = 100;
   sceneProperties.zoneData[0].state = TRUE;
   sceneProperties.nextTriggerTime = 1441018800;   // go at 11
   sceneProperties.freq = EVERY_WEEK;              // every week
   sceneProperties.trigger = REGULAR_TIME;
   sceneProperties.dayBitMask.days = 0xff;  // all days
   sceneProperties.delta = 0;

   /////////////////////////////////////////////////////////////////////////////////////////////
   // now, calculate a new trigger
   calculateNewTrigger(&sceneProperties, rtcTime);
  
   logInfo("Next Trigger:%d", sceneProperties.nextTriggerTime);
   logInfo("#############################################################");
 
   BOOST_CHECK_EQUAL(sceneProperties.nextTriggerTime, 1441105200);  // next trigger should be 9/1/15 at 11:00

   // New check: Reg Time, every day, at 18:00
   sceneProperties.nextTriggerTime = 1441044000;   // go at 18:00

   // now, calculate a new trigger
   calculateNewTrigger(&sceneProperties, rtcTime);
  
   // logInfo("indicator:%d Time:%d", indicator, rtcTime.SECONDS);
   logInfo("Next Trigger:%d", sceneProperties.nextTriggerTime);
   logInfo("#############################################################");
 
   BOOST_CHECK_EQUAL(sceneProperties.nextTriggerTime, 1441130400);  // next trigger should be 9/1/15 at 18:00
  
   /////////////////////////////////////////////////////////////////////////////////////////////
   // New Check, Regular time, at 18:00 but only one day-->Sun
   sceneProperties.nextTriggerTime = 1441044000;             // go at 18:00
   sceneProperties.dayBitMask.days = 0;  
   sceneProperties.dayBitMask.DayBits.sunday = 0x01;         // just sunday
 
   // now, calculate a new trigger
   calculateNewTrigger(&sceneProperties, rtcTime);
  
   //logInfo("indicator:%d Time:%d", indicator, rtcTime.SECONDS);
   logInfo("Next Trigger:%d", sceneProperties.nextTriggerTime);
   logInfo("#############################################################");
 
   BOOST_CHECK_EQUAL(sceneProperties.nextTriggerTime, 1441562400);  // next trigger should be Sun, 9/6/15 at 18:00
 

   /////////////////////////////////////////////////////////////////////////////////////////////
   // New Check, Regular time, at 18:00 but only one day-->Fri
   sceneProperties.nextTriggerTime = 1441044000;             // go at 18:00
   sceneProperties.dayBitMask.days = 0;  
   sceneProperties.dayBitMask.DayBits.friday = 0x01;         // just friday
 
   // now, calculate a new trigger
   calculateNewTrigger(&sceneProperties, rtcTime);
  
   // logInfo("indicator:%d Time:%d", indicator, rtcTime.SECONDS);
   logInfo("Next Trigger:%d", sceneProperties.nextTriggerTime);
   logInfo("#############################################################");
 
   BOOST_CHECK_EQUAL(sceneProperties.nextTriggerTime, 1441389600);  // next trigger should be Fri, 9/4/15 at 18:00
 
   /////////////////////////////////////////////////////////////////////////////////////////////
   // New Check, SUNRISE time, only one day-->Fri
   rtcTime.SECONDS = 1441099765; // CURRENT TIME: 9/1/2015 5:29 am EST, tuesday

   sceneProperties.nextTriggerTime = 1441044000;             // go at 18:00
   sceneProperties.dayBitMask.days = 0;  
   sceneProperties.dayBitMask.DayBits.friday = 0x01;         // just friday
   sceneProperties.trigger = SUNRISE;

   // now, calculate a new trigger
   calculateNewTrigger(&sceneProperties, rtcTime);
 
   logInfo("Next Trigger:%d", sceneProperties.nextTriggerTime);

   BOOST_CHECK_EQUAL(sceneProperties.nextTriggerTime, 1441348560);  // next trigger should be Fri, 9/4/15 
   logInfo("#############################################################");
 
   /////////////////////////////////////////////////////////////////////////////////////////////
   // test delta from sunrise of 10 minutes
   sceneProperties.delta = 10;
   calculateNewTrigger(&sceneProperties, rtcTime);
 
   logInfo("Next Trigger:%d", sceneProperties.nextTriggerTime);
   BOOST_CHECK_EQUAL(sceneProperties.nextTriggerTime, (1441348560+(10*60)));  // next trigger should be Fri, 9/4/15 
   logInfo("#############################################################");

   /////////////////////////////////////////////////////////////////////////////////////////////
   // test delta from sunrise of -10 minutes
   sceneProperties.delta = -10;
   calculateNewTrigger(&sceneProperties, rtcTime);
 
   logInfo("Next Trigger:%d", sceneProperties.nextTriggerTime);

   BOOST_CHECK_EQUAL(sceneProperties.nextTriggerTime, (1441348560-(10*60)));  // next trigger should be Fri, 9/4/15 
   logInfo("#############################################################");

   ///////////////////////////////////////////////////////////////////
   // New Check, SUNSET time, only one day-->Fri
   rtcTime.SECONDS = 1441099765; // CURRENT TIME: 9/1/2015 5:29 am EST, tuesday

   sceneProperties.nextTriggerTime = 1441044000;             // go at 18:00
   sceneProperties.dayBitMask.days = 0;  
   sceneProperties.dayBitMask.DayBits.friday = 0x01;         // just friday
   sceneProperties.trigger = SUNSET;
   sceneProperties.delta = 0;
 
   // now, calculate a new trigger
   calculateNewTrigger(&sceneProperties, rtcTime);
 
   logInfo("Next Trigger:%d", sceneProperties.nextTriggerTime);
 
   BOOST_CHECK_EQUAL(sceneProperties.nextTriggerTime, 1441395600);  // next trigger should be 9/4/15 
   logInfo("#############################################################");
 
   /////////////////////////////////////////////////////////////////////////////////////////////
   // test delta from sunset of 10 minutes
   sceneProperties.delta = 10;
   calculateNewTrigger(&sceneProperties, rtcTime);
 
   logInfo("Next Trigger:%d", sceneProperties.nextTriggerTime);
 
   BOOST_CHECK_EQUAL(sceneProperties.nextTriggerTime, (1441395600+(10*60)));  // next trigger should be 9/4/15 
   logInfo("#############################################################");

   /////////////////////////////////////////////////////////////////////////////////////////////
   // test delta from sunset of -10 minutes
   sceneProperties.delta = -10;
   calculateNewTrigger(&sceneProperties, rtcTime);
 
   logInfo("Next Trigger:%d", sceneProperties.nextTriggerTime);
 
   BOOST_CHECK_EQUAL(sceneProperties.nextTriggerTime, (1441395600-(10*60)));  // next trigger should be 9/4/15 


   logInfo("#############################################################");
   logInfo("        Finished with calculateNewTrigger   ");
   logInfo("#############################################################");
   logInfo(" ");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Event_isItDST)
{
   printTestCase(current_test_case().p_name.value);

   DATE_STRUCT now;
   DATE_STRUCT readDate;
   TIME_STRUCT systemTime;
   boolean check;

   // set up time now to be 9/3/2015 at 11:00
   now.MONTH  = 9;
   now.YEAR   = 2015;
   now.DAY    = 3;
   now.HOUR   = 11;
   now.MINUTE = 0;
   now.WDAY   = 5;
   
   InitializeSystemProperties();
   systemProperties.latitude.degrees = 40;
   systemProperties.longitude.degrees = -77;
   systemProperties.gmtOffsetSeconds = -18000;
   systemProperties.effectiveGmtOffsetSeconds = -14400;
   inDST = TRUE;

   getLocalTime(5000);
   _time_get(&systemTime);
   _time_to_date(&systemTime, &readDate);
   logInfo("Time now:%02d/%02d/%02d  %02d:%02d", readDate.MONTH, readDate.DAY, readDate.YEAR, readDate.HOUR, readDate.MINUTE);
   check = isItDST( now );

   logInfo(" check:%d, it is DST, Sept 2015, gmtOffset:%d", (int)check, systemProperties.effectiveGmtOffsetSeconds );
   
   BOOST_CHECK_EQUAL(check, true );

   /////////////////////////////////////////////////////////////
   now.MONTH  = 12;  // go to december

   check = isItDST( now );

   logInfo(" check:%d, it is not DST, DEC, gmtOffset:%d", (int)check, systemProperties.effectiveGmtOffsetSeconds);
   
   BOOST_CHECK_EQUAL(check, false );

   // adjust the offset and check time, should be back one hour
   systemProperties.effectiveGmtOffsetSeconds -= 3600;
   getLocalTime(5000);
   _time_get(&systemTime);
   _time_to_date(&systemTime, &readDate);
   logInfo("Time now:%02d/%02d/%02d  %02d:%02d", readDate.MONTH, readDate.DAY, readDate.YEAR, readDate.HOUR, readDate.MINUTE);
 
   /////////////////////////////////////////////////////////////
   now.MONTH  = 3; // go to march
   now.DAY    = 3;
   now.HOUR   = 11;
   now.MINUTE = 0;
   now.WDAY   = 2;
 
   check = isItDST( now );

   logInfo(" check:%d, it is not DST, Mar 3, gmtOffset:%d", (int)check, systemProperties.effectiveGmtOffsetSeconds);
   
   BOOST_CHECK_EQUAL(check, false );

   /////////////////////////////////////////////////////////////
   now.MONTH  = 3;
   now.DAY    = 8;
   now.HOUR   = 11;
   now.MINUTE = 0;
   now.WDAY   = 0;
 
   check = isItDST( now );

   logInfo(" check:%d, it is DST, Mar 8, gmtOffset:%d", (int)check, systemProperties.effectiveGmtOffsetSeconds);
   
   BOOST_CHECK_EQUAL(check, true );

  /////////////////////////////////////////////////////////////
   now.MONTH  = 3;
   now.DAY    = 7;
   now.HOUR   = 11;
   now.MINUTE = 0;
   now.WDAY   = 6;
 
   check = isItDST( now );

   logInfo(" check:%d, it is not DST, Mar 7, gmtOffset:%d", (int)check, systemProperties.effectiveGmtOffsetSeconds);
   
   BOOST_CHECK_EQUAL(check, false );


   /////////////////////////////////////////////////////////////
   now.MONTH  = 3;
   now.DAY    = 7;
   now.HOUR   = 23;
   now.MINUTE = 59;
   now.WDAY   = 6;
 
   check = isItDST( now );

   logInfo(" check:%d, it is not DST, Mar 7, gmtOffset:%d", (int)check, systemProperties.effectiveGmtOffsetSeconds);
   
   BOOST_CHECK_EQUAL(check, false );

   /////////////////////////////////////////////////////////////
   now.MONTH  = 3;
   now.DAY    = 8;
   now.HOUR   = 0;
   now.MINUTE = 1;
   now.WDAY   = 0;
 
   check = isItDST( now );

   logInfo(" check:%d, it is not DST, Mar 8, 00:01, gmtOffset:%d", (int)check, systemProperties.effectiveGmtOffsetSeconds);
   
   BOOST_CHECK_EQUAL(check, false );

   /////////////////////////////////////////////////////////////
   now.MONTH  = 3;
   now.DAY    = 8;
   now.HOUR   = 2;
   now.MINUTE = 1;
   now.WDAY   = 0;
 
   check = isItDST( now );

   logInfo(" check:%d, it is DST, Mar 8, 02:01, gmtOffset:%d", (int)check, systemProperties.effectiveGmtOffsetSeconds);
   
   BOOST_CHECK_EQUAL(check, true );

   /////////////////////////////////////////////////////////////
   now.MONTH  = 11;
   now.DAY    = 1;
   now.HOUR   = 1;
   now.MINUTE = 1;
   now.WDAY   = 0;
 
   check = isItDST( now );

   logInfo(" check:%d, it is DST, Nov 1, 01:01, gmtOffset:%d", (int)check, systemProperties.effectiveGmtOffsetSeconds);
   
   BOOST_CHECK_EQUAL(check, true );

   /////////////////////////////////////////////////////////////
   now.MONTH  = 11;
   now.DAY    = 1;
   now.HOUR   = 2;
   now.MINUTE = 1;
   now.WDAY   = 0;
 
   check = isItDST( now );

   logInfo(" check:%d, it is NOT DST, Nov 1, 02:01, gmtOffset:%d", (int)check, systemProperties.effectiveGmtOffsetSeconds);
   
   BOOST_CHECK_EQUAL(check, false );

   /////////////////////////////////////////////////////////////
   now.MONTH  = 11;
   now.YEAR   = 2021;
   now.DAY    = 6;
   now.HOUR   = 2;
   now.MINUTE = 1;
   now.WDAY   = 6;
 
   check = isItDST( now );

   logInfo(" check:%d, it is  DST, Nov 6, 2021, 02:01, gmtOffset:%d", (int)check, systemProperties.effectiveGmtOffsetSeconds);
   
   BOOST_CHECK_EQUAL(check, true );

   /////////////////////////////////////////////////////////////
   now.MONTH  = 11;
   now.YEAR   = 2021;
   now.DAY    = 7;
   now.HOUR   = 2;
   now.MINUTE = 1;
   now.WDAY   = 0;
 
   check = isItDST( now );

   logInfo(" check:%d, it is not DST, Nov 7, 2021, 02:01, gmtOffset:%d", (int)check, systemProperties.effectiveGmtOffsetSeconds);
   
   BOOST_CHECK_EQUAL(check, false );

   // set gmt offset back to what it should be
   systemProperties.gmtOffsetSeconds = -18000;
   systemProperties.effectiveGmtOffsetSeconds = -14400;

   logInfo("#############################################################");
   logInfo("        Finished with isItDST   ");
   logInfo("#############################################################");
   logInfo(" ");

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE( Event_checkForDST, flashFixture)
{
   printTestCase(current_test_case().p_name.value);
  
   InitializeSystemProperties();
   systemProperties.gmtOffsetSeconds = -18000;
   systemProperties.effectiveGmtOffsetSeconds = -14400;
   inDST = TRUE;

   DATE_STRUCT now;
   TIME_STRUCT systemTime;
   uint32_t trigTimes[4];
   json_t * resp = 0;
   json_t * root = 0;
   json_t * propObj;
   json_t * valueObject;
   byte_t   sid;
   uint32_t value;
   uint32_t diff;
   int      trigtype;
   char     name[MAX_SIZE_NAME_STRING];

   // want 'now'  to be DST is off
   now.MONTH  = 11;
   now.YEAR   = 2015;
   now.DAY    = 15;
   now.HOUR   = 11;
   now.MINUTE = 0;   
   now.SECOND = 0;
   now.MILLISEC  = 0;
   now.WDAY   = 1;
   _time_from_date(&now, &systemTime);
 
   ///////////////////////////////////////////////////////////////////////////////
   // LCM starts with inDST = TRUE
   // read all of the scene triggers and set inDST false and 
   //   call checkForDST, which should adjust all scene triggers that are 
   //   Sunset or Sunrise

   // read the scene triggers into an array
   for (int ix=0; ix < 4; ix++)
   {
      // set up root JSON Packet to request property from SID 
      root = json_object();
      resp = json_object();
      json_object_set_new(root, "ID", json_integer(1230));
      json_object_set_new(root, "Service", json_string("ReportSceneProperties"));
      json_object_set_new(root, "SID", json_integer(ix));
  
      // call Handle to fill in the properties into resp
      HandleReportSceneProperties(root, resp);
  
      // check the properties
      // SID should be equal ix, because that is what I asked for above
      valueObject = json_object_get(resp, "SID");
      sid = json_integer_value(valueObject);
   
      // Check the name of the scene controller
      propObj = json_object_get(resp, "PropertyList");   
   
      valueObject = json_object_get(propObj, "Name");
      snprintf(name, sizeof(name), "%s", json_string_value(valueObject));
      logInfo("%d) SID Name: %s", sid, name);
 
      // check a few properties
      valueObject = json_object_get(propObj, "TriggerTime");
      trigTimes[ix] = json_integer_value(valueObject);
 
      valueObject = json_object_get(propObj, "TriggerType");
      trigtype = json_integer_value(valueObject);
      logInfo("%d) OLD triggers are :%d, type:%d", ix, trigTimes[ix], trigtype);

      json_decref(resp);
      json_decref(root);
   } // end for loop
 
   logInfo("#############################################################");
   logInfo("BEFORE CHECK--> gmtOffset:%d", systemProperties.effectiveGmtOffsetSeconds);
 
   // call checkForDST( Nov  2015 ) , which is the current time
   checkForDST(now);

   logInfo("AFTER CHECK--> gmtOffset:%d", systemProperties.effectiveGmtOffsetSeconds);
 
   // compare old triggers with new, they should all be adjusted back by an hour in checkForDST()
   for (int ix=0; ix < 4; ix++)
   {
      // set up root JSON Packet to request property from SID 
      root = json_object();
      resp = json_object();
      json_object_set_new(root, "ID", json_integer(1230));
      json_object_set_new(root, "Service", json_string("ReportSceneProperties"));
      json_object_set_new(root, "SID", json_integer(ix));
  
      // call Handle to fill in the properties into resp
      HandleReportSceneProperties(root, resp);
  
      // check the properties
      // SID should be equal ix, because that is what I asked for above
      valueObject = json_object_get(resp, "SID");
      sid = json_integer_value(valueObject);
   
      // Check the name of the scene controller
      propObj = json_object_get(resp, "PropertyList");   
   
      valueObject = json_object_get(propObj, "Name");
      snprintf(name, sizeof(name), "%s", json_string_value(valueObject));
      logInfo("%d) SID Name: %s", sid, name);
 
      // check a few properties
      valueObject = json_object_get(propObj, "TriggerTime");
      value  = json_integer_value(valueObject);

      valueObject = json_object_get(propObj, "TriggerType");
      trigtype = json_integer_value(valueObject);

      logInfo("%d) NEW triggers are :%d", ix, value);
      diff = trigTimes[ix] - value;
      logInfo("Difference :%d", diff);
      logInfo("#############################################################");
       
      if(trigtype == legrand::rflc::scenes::REGULAR_TIME)
      {
         // Only Sunrise / Sunset times are adjusted based on DST. The other values are not affected
         BOOST_CHECK_EQUAL(diff, 0);
      }
      else
      {
         BOOST_CHECK_EQUAL(diff, 3600);
      }
      trigTimes[ix] = value;  // store new value into the array for compare below
      json_decref(resp);
      json_decref(root);
   } // end for loop

   logInfo("##### NOW, BACK INTO DST #########################################");
   logInfo("BEFORE CHECK--> gmtOffset:%d", systemProperties.effectiveGmtOffsetSeconds);
   now.MONTH  = 4;
   now.WDAY   = 4;
 
   // call checkForDST( April  2015 ) , which is the current time
   checkForDST(now);

   logInfo("AFTER CHECK--> gmtOffset:%d", systemProperties.effectiveGmtOffsetSeconds);
 
   // compare old triggers with new, they should all be adjusted back by an hour in checkForDST()
   for (int ix=0; ix < 4; ix++)
   {
      // set up root JSON Packet to request property from SID 
      root = json_object();
      resp = json_object();
      json_object_set_new(root, "ID", json_integer(1230));
      json_object_set_new(root, "Service", json_string("ReportSceneProperties"));
      json_object_set_new(root, "SID", json_integer(ix));
  
      // call Handle to fill in the properties into resp
      HandleReportSceneProperties(root, resp);
  
      // check the properties
      // SID should be equal ix, because that is what I asked for above
      valueObject = json_object_get(resp, "SID");
      sid = json_integer_value(valueObject);
   
      // Check the name of the scene controller
      propObj = json_object_get(resp, "PropertyList");   
   
      valueObject = json_object_get(propObj, "Name");
      snprintf(name, sizeof(name), "%s", json_string_value(valueObject));
      logInfo("%d) SID Name: %s", sid, name);
 
      // check a few properties
      valueObject = json_object_get(propObj, "TriggerTime");
      value  = json_integer_value(valueObject);

      valueObject = json_object_get(propObj, "TriggerType");
      trigtype = json_integer_value(valueObject);

      logInfo("%d) NEW triggers are :%d", ix, value);
      diff = trigTimes[ix] - value;
      logInfo("Difference :%d", diff);
      logInfo("#############################################################");
       
      if(trigtype == legrand::rflc::scenes::REGULAR_TIME)
      {
         // Only Sunrise / Sunset times are adjusted based on DST. The other values are not affected
         BOOST_CHECK_EQUAL(diff, 0);
      }
      else
      {
         BOOST_CHECK_EQUAL(diff, -3600);
      }

      json_decref(resp);
      json_decref(root);
   } // end for loop



   logInfo("#############################################################");
   logInfo("        Finished with checkForDST   ");
   logInfo("#############################################################");
   logInfo(" ");

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(TEST_DST_November)
{
   printTestCase(current_test_case().p_name.value);

   DATE_STRUCT currentDate;
   currentDate.MONTH = 11;
   currentDate.HOUR = 0;
   currentDate.MINUTE = 0;
   currentDate.SECOND = 0;
   currentDate.MILLISEC = 0;

   // Check every possible first sunday in november for DST changes
   for(uint8_t sunday = 1; sunday < 8; sunday++)
   {
      for(uint8_t day = 1; day < 30; day++)
      {
         currentDate.DAY = day;

         if(day < sunday)
         {
            currentDate.WDAY = (7-sunday) + day;
         }
         else
         {
            currentDate.WDAY = ((day-sunday) % 7);
         }

         if(day == sunday)
         {
            // Anytime before 1 should return true regardless of the inDST flag
            currentDate.HOUR = 0;
            currentDate.MINUTE=59;
            inDST = TRUE;
            BOOST_REQUIRE_EQUAL(true, isItDST(currentDate));
            inDST = FALSE;
            BOOST_REQUIRE_EQUAL(true, isItDST(currentDate));

            // Between 1 and 2 on the day where the time changes,
            // we are in a gray period where we could be in either
            // DST or regular time. At those times, the function should
            // return the state of the DST flag
            currentDate.HOUR = 1;
            currentDate.MINUTE = 59;
            inDST = TRUE;
            BOOST_REQUIRE_EQUAL(true, isItDST(currentDate));
            inDST = FALSE;
            BOOST_REQUIRE_EQUAL(false, isItDST(currentDate));

            // Anytime after 2 should return false regardless of the inDST flag
            currentDate.HOUR = 2;
            currentDate.MINUTE=0;
            inDST = TRUE;
            BOOST_REQUIRE_EQUAL(false, isItDST(currentDate));
            inDST = FALSE;
            BOOST_REQUIRE_EQUAL(false, isItDST(currentDate));
         }
         else if(day < sunday)
         {
            // Current day is before the first sunday. Always in DST
            BOOST_REQUIRE_EQUAL(true, isItDST(currentDate));
         }
         else
         {
            // Current day is after the first sunday. Always not in DST
            BOOST_REQUIRE_EQUAL(false, isItDST(currentDate));
         }
      }
   }
}
   
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(TEST_DST_March)
{
   printTestCase(current_test_case().p_name.value);

   DATE_STRUCT currentDate;
   currentDate.MONTH = 3;
   currentDate.HOUR = 0;
   currentDate.MINUTE = 0;
   currentDate.SECOND = 0;
   currentDate.MILLISEC = 0;

   // Check every possible second sunday in march for DST changes
   for(uint8_t sunday = 8; sunday < 14; sunday++)
   {
      for(uint8_t day = 1; day < 31; day++)
      {
         currentDate.DAY = day;
         currentDate.HOUR = 0;
         currentDate.MINUTE = 0;

         if(day < sunday)
         {
            currentDate.WDAY = (7-sunday) + day;
         }
         else
         {
            currentDate.WDAY = ((day-sunday) % 7);
         }

         if(day == sunday)
         {
            // Anytime before 2 should return false regardless of the inDST flag
            currentDate.HOUR = 1;
            currentDate.MINUTE=59;
            inDST = TRUE;
            BOOST_REQUIRE_EQUAL(false, isItDST(currentDate));
            inDST = FALSE;
            BOOST_REQUIRE_EQUAL(false, isItDST(currentDate));

            // Anytime after 2 should return true regardless of the inDST flag
            currentDate.HOUR = 2;
            currentDate.MINUTE=0;
            inDST = TRUE;
            BOOST_REQUIRE_EQUAL(true, isItDST(currentDate));
            inDST = FALSE;
            BOOST_REQUIRE_EQUAL(true, isItDST(currentDate));
         }
         else if(day < sunday)
         {
            // Current day is before the second sunday. Always not in DST
            BOOST_REQUIRE_EQUAL(false, isItDST(currentDate));
         }
         else
         {
            // Current day is after the second sunday. Always in DST
            BOOST_REQUIRE_EQUAL(true, isItDST(currentDate));
         }
      }
   }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(TEST_DST_OtherMonths)
{
   printTestCase(current_test_case().p_name.value);

   DATE_STRUCT currentDate;

   // Check every month excluding November and March because they are
   // handled in separate test cases
   for(uint8_t month = 1; month < 13; month++)
   {
      currentDate.MONTH = month;

      if(month < 3)
      {
         BOOST_REQUIRE_EQUAL(false, isItDST(currentDate));
      }
      else if((month > 3) &&
              (month < 11))
      {
         BOOST_REQUIRE_EQUAL(true, isItDST(currentDate));
      }
      else if(month > 11)
      {
         BOOST_REQUIRE_EQUAL(false, isItDST(currentDate));
      }
   }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(TEST_DST_checkForDST_Exit)
{
   printTestCase(current_test_case().p_name.value);

   // Initialize the system properties
   InitializeSceneArray();
   InitializeSystemProperties();

   // Set the system properties like it is in DST
   int32_t initialEffectiveOffset = -14400;
   systemProperties.effectiveGmtOffsetSeconds = initialEffectiveOffset;
   systemProperties.gmtOffsetSeconds = -18000;
   inDST = true;

   // Set the current time to Sunday November 1, 2015 at 1:59 AM
   // This time is 1 minute before DST ends
   DATE_STRUCT initialDate;
   initialDate.YEAR = 2015;
   initialDate.MONTH = 11;
   initialDate.DAY = 1;
   initialDate.HOUR = 1;
   initialDate.MINUTE = 59;
   initialDate.SECOND = 0;
   initialDate.MILLISEC = 0;
   initialDate.WDAY = 0;

   // Set the system time to this value
   TIME_STRUCT initialTime;
   _time_from_date(&initialDate, &initialTime);
   _time_set(&initialTime);

   // Make sure the isDST function returns true
   BOOST_REQUIRE_EQUAL(true, isItDST(initialDate));

   // Call checkForDST 
   checkForDST(initialDate);

   // Make sure the flag did not change
   BOOST_REQUIRE_EQUAL(true, inDST);

   // Make sure the effective GMT offset did not change
   BOOST_REQUIRE_EQUAL(initialEffectiveOffset, systemProperties.effectiveGmtOffsetSeconds);

   // Make sure the system time did not change
   TIME_STRUCT currentTime;
   _time_get(&currentTime);
   BOOST_REQUIRE_EQUAL(initialTime.SECONDS, currentTime.SECONDS);

   // Add one minute so DST ends 
   initialDate.HOUR = 2;
   initialDate.MINUTE = 0;
   _time_from_date(&initialDate, &initialTime);
   _time_set(&initialTime);

   // Make sure the isDST function returns false for this date
   BOOST_REQUIRE_EQUAL(false, isItDST(initialDate));

   // call checkForDST again 
   checkForDST(initialDate);

   // Make sure the flag is updated correctly
   BOOST_REQUIRE_EQUAL(false, inDST);

   // Make sure the effective GMT offset changed by subtracting one hour
   BOOST_REQUIRE_EQUAL((initialEffectiveOffset-3600), systemProperties.effectiveGmtOffsetSeconds);

   // Make sure the system time was modified by an hour
   _time_get(&currentTime);
   BOOST_REQUIRE_EQUAL((initialTime.SECONDS-3600), currentTime.SECONDS);

   // Since the current time is now an hour back, make sure subsequent calls
   // do not reset it back to DST time
   DATE_STRUCT currentDate;
   _time_to_date(&currentTime, &currentDate);
   BOOST_REQUIRE_EQUAL(false, isItDST(currentDate));

   checkForDST(currentDate);

   BOOST_REQUIRE_EQUAL(false, inDST);
   BOOST_REQUIRE_EQUAL((initialEffectiveOffset-3600), systemProperties.effectiveGmtOffsetSeconds);
   _time_get(&currentTime);
   BOOST_REQUIRE_EQUAL((initialTime.SECONDS-3600), currentTime.SECONDS);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(TEST_EffectiveOffsetInvalid_OutOfDST)
{
   printTestCase(current_test_case().p_name.value);

   // Initialize the system properties
   InitializeSystemProperties();

   // Set the system properties to an invalid value
   systemProperties.effectiveGmtOffsetSeconds = -21600;
   systemProperties.gmtOffsetSeconds = -18000;
   inDST = true;

   // Set the current date so that it is not in DST
   DATE_STRUCT currentDate;
   currentDate.MONTH = 1;

   // Make sure the isDST function returns false
   BOOST_REQUIRE_EQUAL(false, isItDST(currentDate));

   // Call checkForDST 
   checkForDST(currentDate);

   // Verify that the effective GMT offset equals the GMT offset
   BOOST_REQUIRE_EQUAL(systemProperties.effectiveGmtOffsetSeconds, systemProperties.gmtOffsetSeconds);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(TEST_EffectiveOffsetInvalid_IntoDST)
{
   printTestCase(current_test_case().p_name.value);

   // Initialize the system properties
   InitializeSystemProperties();

   // Set the system properties to an invalid value
   systemProperties.effectiveGmtOffsetSeconds = -21600;
   systemProperties.gmtOffsetSeconds = -18000;
   inDST = false;

   // Set the current date so that it is in DST
   DATE_STRUCT currentDate;
   currentDate.MONTH = 4;

   // Make sure the isDST function returns true
   BOOST_REQUIRE_EQUAL(true, isItDST(currentDate));

   // Call checkForDST 
   checkForDST(currentDate);

   // Verify that the effective GMT offset equals the GMT offset plus an hour
   BOOST_REQUIRE_EQUAL(systemProperties.effectiveGmtOffsetSeconds, systemProperties.gmtOffsetSeconds + 3600);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

