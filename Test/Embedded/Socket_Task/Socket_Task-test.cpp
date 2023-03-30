#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Socket_TaskTest

#define MAIN
#include <boost/test/unit_test.hpp>
#include "includes.h"
#include "Debug.h"
#include "FlashStub.h"
#include <cstdio>


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

      
      // Free the memory that was allocated
      _msg_free(message);

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
struct JsonFixture
{
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   std::string zoneFilename = "Zones.txt";
   std::string sceneFilename = "Scenes.txt";
   std::string sceneControllerFilename = "SceneControllers.txt";
   std::string samsungZonePropertiesFilename = "SamsungZoneProperties.txt";
   std::string samsungScenePropertiesFilename = "SamsungSceneProperties.txt";

   JsonFixture()
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
   
   ~JsonFixture()
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

BOOST_FIXTURE_TEST_CASE(ListSceneCtlrs, JsonFixture)
{
   printTestCase(current_test_case().p_name.value);
   
   json_t * resp = 0;
   json_t * sceneCtlList;
   json_t * sceneListArrayElement;
   json_t * valueObject;
   byte_t   sctlIdx = 0;   
   byte_t   scid = 0;
   byte_t   listSize = 0;
 
   // Check to see how Handle deals with a Null Pointer
   HandleListSceneControllers(resp);
   BOOST_CHECK_EQUAL((long)resp, 0);  
 
   // call Handle to get resp filled in with array of SCIDs
   resp = json_object();
   HandleListSceneControllers(resp);
  
   sceneCtlList = json_object_get(resp, "SceneControllerList"); 

   listSize = json_array_size(sceneCtlList);
   BOOST_REQUIRE(listSize > 0);

   sctlIdx = 0;
   // go thru each array element
   while (sctlIdx < listSize)
   {
      // get the ID out of the list built to be returned
      sceneListArrayElement = json_array_get(sceneCtlList, sctlIdx);
      valueObject = json_object_get(sceneListArrayElement, "SCID");
      scid = json_integer_value(valueObject);

      BOOST_CHECK_EQUAL(scid,  sctlIdx); // Scene Ctl Index should be ctr
    
      sctlIdx++;
   }

   json_decref(resp);

   logInfo("#############################################################");
   logInfo("    Finished with ListSceneCtlrs  ");
   logInfo("#############################################################");
   logInfo(" ");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(HdlReportSceneControllerProps, JsonFixture)
{
   printTestCase(current_test_case().p_name.value);

   json_t * resp = 0;   
   json_t * root = 0;
   json_t * propObj;
   json_t * valueObject;
   json_t * sceneCtlList;
   json_t * sceneListArrayElement;
   byte_t   scid;
   byte_t   sid = 0;
   byte_t   bankIdx=0;
   byte_t   listSize = 0;
   byte_t   noScene = -1;
   char     name[MAX_SIZE_NAME_STRING];
   char     nameCHK[MAX_SIZE_NAME_STRING];

   // Check that function handles a Null Pointer
   HandleReportSceneControllerProperties(root, resp);
   BOOST_CHECK_EQUAL((long)resp, 0);
   
   // check the first 4 scene controllers
   for (int ix=0; ix < 4; ix++)
   {
      // set up root JSON Packet to request property from the SCTL 
      root = json_object();
      resp = json_object();
      json_object_set_new(root, "ID", json_integer(1230));
      json_object_set_new(root, "Service", json_string("ReportSceneControllerProperties"));
      json_object_set_new(root, "SCID", json_integer(ix));
   
      // call Handle to fill in the properties into resp
      HandleReportSceneControllerProperties(root, resp);

      // check the properties
      // SC ID should be equal ix, because that is what I asked for above
      valueObject = json_object_get(resp, "SCID");
      scid = json_integer_value(valueObject);

      // check that we get a ID for the scene controller
      BOOST_CHECK_EQUAL(scid, ix);
  
      // Check the name of the scene controller
      propObj = json_object_get(resp, "PropertyList");   
   
      valueObject = json_object_get(propObj, "Name");
      snprintf(name, sizeof(name), "%s", json_string_value(valueObject));
      logInfo("%d) SCTL Name: %s", scid, name);
      snprintf(nameCHK, sizeof(nameCHK), "SC %d", scid);
      BOOST_CHECK_EQUAL(strcmp(name, nameCHK), 0);
     
      sceneCtlList = json_object_get(propObj, "Banks"); // now have the list of Banks
 
      listSize = json_array_size(sceneCtlList);
 
      bankIdx = 0;
      // go thru each array element
      while (bankIdx < listSize)
      {
         // get the ID out of the list built to be returned
         sceneListArrayElement = json_array_get(sceneCtlList, bankIdx);
         valueObject = json_object_get(sceneListArrayElement, "SID");
         sid = json_integer_value(valueObject);
         if (bankIdx == 0)
              BOOST_CHECK_EQUAL(sid, bankIdx); // Scene Index should be ctr
         else
              BOOST_CHECK_EQUAL(sid, noScene);  // no scene associated with the rest
         bankIdx++;
      }
   

      json_decref(resp);
      json_decref(root);

   }
 
   logInfo("################################################################");
   logInfo("       Finished with HdlReportSceneControllerProps");
   logInfo("################################################################");
   logInfo(" ");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(HdlListZones, JsonFixture)
{
   printTestCase(current_test_case().p_name.value);

   json_t * resp = 0;
   json_t * zoneList;
   json_t * zoneListArrayElement;
   json_t * valueObject;
   byte_t   zoneIdx = 0;   
   byte_t   zid = 0;
   byte_t   listSize = 0;
  
   // Check that function handles a Null Pointer
   HandleListZones(resp);
   BOOST_CHECK_EQUAL((long)resp, 0);

   // call Handle to get resp filled in with array of zones
   resp = json_object();
   HandleListZones(resp);
  
   zoneList = json_object_get(resp, "ZoneList"); // now have the list of zones

   listSize = json_array_size(zoneList);
 
   zoneIdx = 0;
   // go thru each array element
   while (zoneIdx < listSize)
   {
      zoneListArrayElement = json_array_get(zoneList, zoneIdx);
      valueObject = json_object_get(zoneListArrayElement, "ZID");
      zid = json_integer_value(valueObject);
      BOOST_CHECK_EQUAL(zid,  zoneIdx); // ZoneIndex should equal ctr
  
      zoneIdx++;
   }

   json_decref(resp);

   logInfo("#############################################################");
   logInfo("     Finished with HdlListZones");
   logInfo("#############################################################");
   logInfo(" ");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(HdlZoneProps, JsonFixture)
{
   printTestCase(current_test_case().p_name.value);

   json_t * resp = 0;
   json_t * root = 0;
   json_t * propObj;
   json_t * valueObject;
   byte_t   zid;
   byte_t   value;
   char     name[MAX_SIZE_NAME_STRING];
   char     nameCHK[MAX_SIZE_NAME_STRING];

   // Check that the function handles a Null Pointer
   HandleReportZoneProperties(root, resp);
   BOOST_CHECK_EQUAL((long)resp, 0);

   // request the first 10 different zone properties
   for (int ix=0; ix < 10; ix++)
   {
      // set up root JSON Packet to request property 
      root = json_object();
      resp = json_object();
      json_object_set_new(root, "ID", json_integer(1230));
      json_object_set_new(root, "Service", json_string("ReportZoneProperties"));
      json_object_set_new(root, "ZID", json_integer(ix));
  
      // call Handle to fill in the properties into resp
      HandleReportZoneProperties(root, resp);
  
      // check the properties
      // ZID should be equal ix, because that is what was asked for above
      valueObject = json_object_get(resp, "ZID");
      zid = json_integer_value(valueObject);
      BOOST_CHECK_EQUAL(zid, ix);
  
      // Check the name of the scene controller
      propObj = json_object_get(resp, "PropertyList");   
   
      valueObject = json_object_get(propObj, "Name");
      snprintf(name, sizeof(name), "%s", json_string_value(valueObject));
      switch ( ix )
      {
         case 0:
		snprintf(nameCHK, sizeof(nameCHK), "Kitchen");
                break;
         case 1:
		snprintf(nameCHK, sizeof(nameCHK), "Living Room");
                break;
         case 2:
		snprintf(nameCHK, sizeof(nameCHK), "Hall Light");
                break;
         case 3:
		snprintf(nameCHK, sizeof(nameCHK), "Kids Bedroom");
                break;
         case 4:
		snprintf(nameCHK, sizeof(nameCHK), "Bathroom");
                break;
         case 5:
		snprintf(nameCHK, sizeof(nameCHK), "Master Bedroom");
                break;
         case 6:
		snprintf(nameCHK, sizeof(nameCHK), "Front Porch");
                break;
         case 7:
		snprintf(nameCHK, sizeof(nameCHK), "Back Door");
                break;
         case 8:
		snprintf(nameCHK, sizeof(nameCHK), "Stairs");
                break;
         case 9:
		snprintf(nameCHK, sizeof(nameCHK), "Desk Light");
                break;
 
         default:
		snprintf(nameCHK, sizeof(nameCHK), "Zone %d", ix);
                break;
      }
      BOOST_CHECK_EQUAL(strcmp(name, nameCHK), 0);
 
 
      valueObject = json_object_get(propObj, "PowerLevel");
      value = json_integer_value(valueObject);
      BOOST_CHECK_EQUAL(value, 100);
 
      valueObject = json_object_get(propObj, "DeviceType");
      snprintf(name, sizeof(name), "%s", json_string_value(valueObject));
      logInfo("Device Type :%s", name);
      if ( ix < 9 )
         BOOST_CHECK_EQUAL(strcmp(name, "Dimmer"), 0);
      else
         BOOST_CHECK_EQUAL(strcmp(name, "Switch"), 0);


      json_decref(resp);
      json_decref(root);
   }

   logInfo("################################################################");
   logInfo("   Finished with HdlReportZoneProps");
   logInfo("################################################################");
   logInfo(" ");
}



// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(HdlListScenes, JsonFixture)
{
   printTestCase(current_test_case().p_name.value);

   json_t * resp = 0;
   json_t * sceneList;
   json_t * sceneListArrayElement;
   json_t * valueObject;
   byte_t   sceneIdx = 0;   
   byte_t   sid = 0;
   byte_t   listSize = 0;

   // Check that the function handles a Null Pointer
   HandleListScenes(resp);
   BOOST_CHECK_EQUAL((long)resp, 0);

   resp = json_object();

   // call Handle to get resp filled in with array of scenes
   HandleListScenes(resp);

   sceneList = json_object_get(resp, "SceneList"); 

   listSize = json_array_size(sceneList);
 
   sceneIdx = 0;
   // go thru each scene
   while (sceneIdx < listSize)
   {
      sceneListArrayElement = json_array_get(sceneList, sceneIdx);
      valueObject = json_object_get(sceneListArrayElement, "SID");
      sid = json_integer_value(valueObject);

      BOOST_CHECK_EQUAL(sid,  sceneIdx); // sceneIndex should equal ctr
  
      sceneIdx++;
   }

   json_decref(resp);

   logInfo("###############################################################");
   logInfo("      Finished with HdlListScenes");
   logInfo("###############################################################");
   logInfo(" ");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(HdlReportSceneProps, JsonFixture)
{
   printTestCase(current_test_case().p_name.value);

   json_t * resp = 0;
   json_t * root = 0;
   json_t * propObj;
   json_t * valueObject;
   byte_t   sid;
   byte_t   value;
   char     name[MAX_SIZE_NAME_STRING];
   char     nameCHK[MAX_SIZE_NAME_STRING];
   byte_t   deltaValue = 0;
 
   // Check that the function handles a Null Pointer
   HandleReportSceneProperties(root, resp);
   int checkValue = (long)resp;
   BOOST_CHECK_EQUAL(checkValue, 0);
 
   // try 4 different scene properties
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
      BOOST_CHECK_EQUAL(sid, ix);
  
      // Check the name of the scene controller
      propObj = json_object_get(resp, "PropertyList");   
   
      valueObject = json_object_get(propObj, "Name");
      snprintf(name, sizeof(name), "%s", json_string_value(valueObject));
      logInfo("%d) SID Name: %s", sid, name);
      switch ( ix )
      {
         case 0:
            snprintf(nameCHK, sizeof(nameCHK), "Wake Up");
            deltaValue = 30;
            break;
         case 1:
            snprintf(nameCHK, sizeof(nameCHK), "Dinner");
            deltaValue = 0;
            break;
         case 2:
            snprintf(nameCHK, sizeof(nameCHK), "Go to Bed");
            deltaValue = 0;
            break;
         case 3:
            snprintf(nameCHK, sizeof(nameCHK), "Movie");
            deltaValue = 90;
            break;

         default:
            snprintf(nameCHK, sizeof(nameCHK), "Scene %d", ix);
            deltaValue = 90;
            break;
      }
      BOOST_CHECK_EQUAL(strcmp(name, nameCHK), 0);
  
      // check a few properties
      valueObject = json_object_get(propObj, "Frequency");
      value = json_integer_value(valueObject);
      BOOST_CHECK_EQUAL(value, 2);

      valueObject = json_object_get(propObj, "Delta");
      value = json_integer_value(valueObject);
      BOOST_CHECK_EQUAL(value, deltaValue);
 
      json_decref(resp);
      json_decref(root);
   } // end for loop

   logInfo("##############################################################");
   logInfo("   Finished with HdlReportSceneProps");
   logInfo("##############################################################");
   logInfo(" ");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(HdlSystemInfo, JsonFixture)
{
   printTestCase(current_test_case().p_name.value);

   json_t * resp = 0;
   json_t * valueObject;
   char     str1[20];
   char     modelStr[] = "LCM1";
 
   // Check that the function handles a Null Pointer
   HandleSystemInfo(resp);
   BOOST_CHECK_EQUAL((long)resp, 0);
 
   // Now, get a JSON packet with system info
   resp = json_object();
   HandleSystemInfo(resp);
  
   valueObject = json_object_get(resp, "Model");
   snprintf(str1, sizeof(str1), "%s", json_string_value(valueObject));
   BOOST_CHECK_EQUAL( str1, modelStr);

   valueObject = json_object_get(resp, "FirmwareVersion");
   snprintf(str1, sizeof(str1), "%s", json_string_value(valueObject));
   logInfo("FirmwareVersion : %s\n", str1);
 
   valueObject = json_object_get(resp, "MACAddress");
   snprintf(str1, sizeof(str1), "%s", json_string_value(valueObject));
   logInfo("MACAddress : %s\n", str1);
 
   json_decref(resp);
 
   logInfo("##############################################################");
   logInfo("   Finished with HdlSystemInfo");
   logInfo("##############################################################");
   logInfo(" ");
 
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(HdlReportSystemProps, JsonFixture)
{
   printTestCase(current_test_case().p_name.value);

   json_t * resp = 0;
   json_t * propObj;
   json_t * valueObject;
   boolean  chk = TRUE;
   int      value;

   // Check that the function handles a Null Pointer
   HandleReportSystemProperties(resp);
   BOOST_CHECK_EQUAL((long)resp, 0);

   // initialize the global system properties
   InitializeSystemProperties();

   // Now, get a JSON packet with system info
   resp = json_object();
   HandleReportSystemProperties(resp);

   propObj = json_object_get(resp, "PropertyList");

   valueObject = json_object_get(propObj, "AddASceneController");
   chk = json_is_true(valueObject);
   // Add A Scene Controller should be true
   BOOST_CHECK_EQUAL(chk, FALSE);

   valueObject = json_object_get(propObj, "AddALight");
   chk = json_is_true(valueObject);
   // Add A Light should be true
   BOOST_CHECK_EQUAL(chk, FALSE);

   // check the timezone returned
   valueObject = json_object_get(propObj, "TimeZone");
   value = json_integer_value(valueObject);
   BOOST_CHECK_EQUAL(value, 0);

   // check the effective time zone returned
   valueObject = json_object_get(propObj, "EffectiveTimeZone");
   value = json_integer_value(valueObject);
   BOOST_CHECK_EQUAL(value, 0);

   json_decref(resp);

   logInfo("##############################################################");
   logInfo("   Finished with HdlReportSystemProps");
   logInfo("##############################################################");
   logInfo(" ");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(HdlSetZoneProp, JsonFixture)
{
   printTestCase(current_test_case().p_name.value);

   json_t * root = 0;
   json_t * resp = 0;
   json_t * broad = 0;
   json_t * propObj;
   json_t * valueObject;
   byte_t   zid = 0;
   char     name[MAX_SIZE_NAME_STRING];

   // Check that the function handles a Null Pointer
   HandleSetZoneProperties(root, resp, &broad);
   BOOST_CHECK_EQUAL((long)resp, 0);
 
   // Test set by changing Zone 9 name, then reading it back
   logInfo(">>>>>>>>>>>>> Setting Zone 9 << NAME: TEST 9 <<<<<<<<<<<<<<");
   ////////////////////////////////////////////////////////////////////
   // set up root JSON Packet for property from ZID 
   root = json_object();
   resp = json_object();
   propObj = json_object();
   json_object_set_new(root, "ID", json_integer(1230));
   json_object_set_new(root, "Service", json_string("SetZoneProperties"));
   json_object_set_new(root, "ZID", json_integer(9));
   json_object_set_new(propObj, "RampRate", json_integer(42));
   json_object_set_new(root, "PropertyList", propObj);

   snprintf(name, sizeof(name), "%s", "TEST 9");
   json_object_set_new(propObj, "Name", json_string(name));
   HandleSetZoneProperties(root, resp, &broad);

   json_decref(resp);
   json_decref(root);

   if(broad)
   {
      json_decref(broad);
   }

   logInfo(">>>>>>>>>>>>> Reading Zone 9 <<<<<<<<<<<<<<<<");
   ////////////////////////////////////////////////////////////////////////////
   // Now read back Zone
   root = json_object();
   resp = json_object();
   json_object_set_new(root, "ID", json_integer(1230));
   json_object_set_new(root, "Service", json_string("ReportZoneProperties"));
   json_object_set_new(root, "ZID", json_integer(9));

   // call Handle to fill in the properties into resp, NOTE: HANDLE does not report back
   //                                                         RampRate 
   HandleReportZoneProperties(root, resp);
  
   // check the properties
   // ZID should be equal 9, because that is what was asked for above

   valueObject = json_object_get(resp, "ZID");
   zid = json_integer_value(valueObject);
   BOOST_CHECK_EQUAL(zid, 9);
  
   // Check the name of the scene controller
   propObj = json_object_get(resp, "PropertyList");   
   
   valueObject = json_object_get(propObj, "Name");
   snprintf(name, sizeof(name), "%s", json_string_value(valueObject));
   logInfo("%d) ZID Name: %s", zid, name);

   BOOST_CHECK_EQUAL(strcmp(name, "TEST 9"), 0);

   json_decref(resp);
   json_decref(root);

   logInfo("##############################################################");
   logInfo("   Finished with HandleSetZoneProperties");
   logInfo("##############################################################");
   logInfo(" ");

}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(HdlSetSceneProp, JsonFixture)
{
   printTestCase(current_test_case().p_name.value);

   json_t * root = 0;
   json_t * resp = 0;
   json_t * broad = 0;
   json_t * propObj;
   json_t * valueObject;
   byte_t   sid = 0;
   char     name[MAX_SIZE_NAME_STRING];

   // Check that the function handles a Null Pointer
   HandleSetSceneProperties(root, resp, &broad);
   BOOST_CHECK_EQUAL((long)resp, 0);
   
   logInfo(">>>>>>>>>>>>> Setting Scene 3 << NAME: SCENE TEST  <<<<<<<<<<<<<<");
   ////////////////////////////////////////////////////////////////////
   // set up root JSON Packet for property from SID 
   root = json_object();
   resp = json_object();
   propObj = json_object();
   json_object_set_new(root, "ID", json_integer(1230));
   json_object_set_new(root, "Service", json_string("SetSceneProperties"));
   json_object_set_new(root, "SID", json_integer(3));
   json_object_set_new(root, "PropertyList", propObj);

   snprintf(name, sizeof(name), "%s", "SCENE TEST");
   json_object_set_new(propObj, "Name", json_string(name));
   HandleSetSceneProperties(root, resp, &broad);

   json_decref(resp);
   json_decref(root);

   if(broad)
   {
      json_decref(broad);
   }

   logInfo(">>>>>>>>>>>>> Reading Scene 3 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
   root = json_object();
   resp = json_object();
   ////////////////////////////////////////////////////////////////////////////
   // Now read back Scene
   json_object_set_new(root, "ID", json_integer(1230));
   json_object_set_new(root, "Service", json_string("ReportSceneProperties"));
   json_object_set_new(root, "SID", json_integer(3));

   // call Handle to fill in the properties into resp
   HandleReportSceneProperties(root, resp);
  
   // check the properties
   // SID should be equal 3, because that is what was asked for above

   valueObject = json_object_get(resp, "SID");
   sid = json_integer_value(valueObject);
   BOOST_CHECK_EQUAL(sid, 3);
  
   // Check the name of the scene controller
   propObj = json_object_get(resp, "PropertyList");   
   
   valueObject = json_object_get(propObj, "Name");
   snprintf(name, sizeof(name), "%s", json_string_value(valueObject));

   BOOST_CHECK_EQUAL(strcmp(name, "SCENE TEST"), 0);
   
   json_decref(resp);
   json_decref(root);

   logInfo("##############################################################");
   logInfo("   Finished with HandleSetSceneProperties");
   logInfo("##############################################################");
   logInfo(" ");

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(HdlSetSceneCtlrProp, JsonFixture)
{
   printTestCase(current_test_case().p_name.value);

   json_t * root = 0;
   json_t * resp = 0;
   json_t * broad = 0;
   json_t * propObj;
   json_t * valueObject;
   byte_t   scid = 0;
   char     name[MAX_SIZE_NAME_STRING];

   RFM100TransmitTaskMessagePoolID = _msgpool_create(sizeof(RFM100_TRANSMIT_TASK_MESSAGE), NUMBER_OF_RFM100_TRANSMIT_MESSAGES_IN_POOL, 0, 0);

   // Check that the function handles a Null Pointer
   HandleSetSceneControllerProperties(root, resp, &broad);
   BOOST_CHECK_EQUAL((long)resp, 0);

   logInfo(">>>>>>>>>>>>> Setting Scene Ctl 1 << NAME: SCENE CTL TEST  <<<<<<<<<<<<<<");
   ////////////////////////////////////////////////////////////////////
   // set up root JSON Packet for property from SCID 
   root = json_object();
   resp = json_object();
   propObj = json_object();
   json_object_set_new(root, "ID", json_integer(1230));
   json_object_set_new(root, "Service", json_string("SetSceneControllerProperties"));
   json_object_set_new(root, "SCID", json_integer(1));
   json_object_set_new(root, "PropertyList", propObj);

   snprintf(name, sizeof(name), "%s", "SCENE CTL TEST");
   json_object_set_new(propObj, "Name", json_string(name));
 
   HandleSetSceneControllerProperties(root, resp, &broad);

   json_decref(resp);
   json_decref(root);

   if(broad)
   {
      json_decref(broad);
   }

   logInfo(">>>>>>>>>>>>> Reading Scene Ctl 1 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
   root = json_object();
   resp = json_object();
   ////////////////////////////////////////////////////////////////////////////
   // Now read back Scene
   json_object_set_new(root, "ID", json_integer(1230));
   json_object_set_new(root, "Service", json_string("ReportSceneControllerProperties"));
   json_object_set_new(root, "SCID", json_integer(1));

   // call Handle to fill in the properties into resp
   HandleReportSceneControllerProperties(root, resp);
  
   // check the properties
   // SCID should be equal 1, because that is what was asked for above

   valueObject = json_object_get(resp, "SCID");
   scid = json_integer_value(valueObject);
   BOOST_CHECK_EQUAL(scid, 1);
  
   // Check the name of the scene controller
   propObj = json_object_get(resp, "PropertyList");   
   
   valueObject = json_object_get(propObj, "Name");
   snprintf(name, sizeof(name), "%s", json_string_value(valueObject));
   logInfo("%d) SCID Name: %s", scid, name);

   BOOST_CHECK_EQUAL(strcmp(name, "SCENE CTL TEST"), 0);

   json_decref(resp);
   json_decref(root);

   logInfo("##############################################################");
   logInfo("   Finished with HandleSetSceneControllerProperties");
   logInfo("##############################################################");
   logInfo(" ");

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
BOOST_FIXTURE_TEST_CASE(HdlSetSystemProperties, JsonFixture)
{
   printTestCase(current_test_case().p_name.value);

   json_t * root = 0;
   json_t * resp = 0;
   json_t * broad = 0;
   json_t * propObj;
   json_t * valueObject;
   char     location[MAX_SIZE_NAME_STRING];
   int      timeZone;

   // Check that the function handles a Null Pointer
   HandleSetSystemProperties(root, resp, &broad);
   BOOST_CHECK_EQUAL((long)resp, 0);
 
   logInfo(">>>>>>>>>>>>> Setting System Properties <<<<<<<<<<<<<<");
   ////////////////////////////////////////////////////////////////////
   // set up root JSON Packet for property
   root = json_object();
   resp = json_object();
   propObj = json_object();
   json_object_set_new(root, "ID", json_integer(1230));
   json_object_set_new(root, "Service", json_string("SetSystemProperties"));
   json_object_set_new(root, "PropertyList", propObj);

   // set some properties
   json_object_set_new(propObj, "TimeZone", json_integer(5544));
   snprintf(location, sizeof(location), "%s", "Timbuktoo");
   json_object_set_new(propObj, "LocationInfo", json_string(location));
    
   HandleSetSystemProperties(root, resp, &broad);
 
   json_decref(resp);
   json_decref(root);

   if(broad)
   {
      json_decref(broad);
   }

   logInfo(">>>>>>>>>>>>> Reading System Properties <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
   root = json_object();
   resp = json_object();
   ////////////////////////////////////////////////////////////////////////////
   // Now read back Scene
   json_object_set_new(root, "ID", json_integer(1230));
   json_object_set_new(root, "Service", json_string("ReportSystemProperties"));
 
   // call Handle to fill in the properties into resp
   HandleReportSystemProperties(resp);
  
   // check the properties
   propObj = json_object_get(resp, "PropertyList");   
   
   valueObject = json_object_get(propObj, "TimeZone");
   timeZone = json_integer_value(valueObject);
   BOOST_CHECK_EQUAL(timeZone, 5544);

   valueObject = json_object_get(propObj, "LocationInfo");
   snprintf(location, sizeof(location), "%s", json_string_value(valueObject));
   logInfo("TimeZone:%d location: %s", timeZone, location);

   int chk = strcmp(location, "Timbuktoo"); // strcmp rets 0 if strings are equal
   BOOST_CHECK_EQUAL(chk, 0);
 
   json_decref(resp);
   json_decref(root);

   logInfo("##############################################################");
   logInfo("   Finished with HandleSetSystemProperties");
   logInfo("##############################################################");
   logInfo(" ");


}
