#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ExternalNetworkBridgeTest

#include "Debug.h"
#include "TestCommon.h"
#include "ExternalNetworkBridge.h"
#include "LCMTestClient.h"
#include "RegistrationTestClient.h"
#include "AppTestClient.h"
#include "ServerUtil.h"
#include "Subscriber.h"
#include "Publisher.h"
#include "PublishObject.h"
#include <boost/test/unit_test.hpp>
#include <sqlite3.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <memory>

using namespace boost::unit_test::framework;

static const int LCM_DB=0;
static const int APP_DB=1;
static const int LCM_PORT=2525;
static const int APP_PORT=2526;
static const int REGISTRATION_PORT=2527;
static const std::string SERVER_HOST="127.0.0.1";
static const std::string DB_NAME="test.db";

class BridgeFixture : public Subscriber
{
   public:
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      BridgeFixture()
      {
         TestCommon::printTestCase();

         // Open the database
         logDebug("Opening the database %s", DB_NAME.c_str());
         int result = sqlite3_open_v2(DB_NAME.c_str(), &db, SQLITE_OPEN_READWRITE, NULL);
         BOOST_REQUIRE_EQUAL(result, SQLITE_OK);

         BOOST_REQUIRE_EQUAL(SQLITE_OK, sqlite3_busy_timeout(db, 1000));

         if(db)
         {
            // Turn on foreign key support
            logDebug("Turning on foreign key support");
            result = sqlite3_exec(db, "PRAGMA foreign_keys = ON", NULL, NULL, NULL);
            BOOST_REQUIRE_EQUAL(result, SQLITE_OK);

            // Create the bridge
            bridge = std::make_shared<ExternalNetworkBridge>(APP_PORT, LCM_PORT, REGISTRATION_PORT, DB_NAME);
            bridge->start();
         }
         else
         {
            BOOST_FAIL("Could not open DB");
         }
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      ~BridgeFixture()
      {
         logVerbose("Enter");

         // Clear the database
         if(db)
         {
            // clear the apps table
            logDebug("Clearing the apps table");
            int result = sqlite3_exec(db, "delete from apps", NULL, NULL, NULL);
            BOOST_REQUIRE_EQUAL(result, SQLITE_OK);

            // Clear the devices table
            logDebug("Clearing the devices table");
            result = sqlite3_exec(db, "delete from devices", NULL, NULL, NULL);
            BOOST_REQUIRE_EQUAL(result, SQLITE_OK);

            logInfo("Closing the database");
            BOOST_REQUIRE_EQUAL(SQLITE_OK, sqlite3_close_v2(db));
            db = NULL;
         }

         logVerbose("Exit");
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      
      bool waitForConnectionStatus(TestClient &client, const bool &connectionStatus, const std::chrono::milliseconds &timeout)
      {
         bool statusMatches = false;

         if(client.isConnected() != connectionStatus)
         {
            std::unique_lock<std::mutex> lock(*client.getConnectionMutex());
            statusMatches = client.getConnectionCV()->wait_for(lock, timeout, [&client, &connectionStatus](){return (client.isConnected() == connectionStatus); });
         }
         else
         {
            statusMatches = true;
         }

         return statusMatches;
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      
      void verifyIdInDB(const int &DB, const uint64_t &deviceId)
      {
         if(db)
         {
            logInfo("Verifying device %lu is in the database", static_cast<long unsigned int>(deviceId));

            sqlite3_stmt *statement;
            std::stringstream querySS;
            if(DB == LCM_DB)
            {
               querySS << "select * from devices where id=" << deviceId;
            }
            else if(DB == APP_DB)
            {
               querySS << "select * from apps where id=" << deviceId;
            }
            else
            {
               BOOST_FAIL("Invalid Database provided");
            }

            int prepareRv = sqlite3_prepare_v2(db, querySS.str().c_str(), -1, &statement, 0);
            BOOST_REQUIRE_EQUAL(prepareRv, SQLITE_OK);

            int numColumns = sqlite3_column_count(statement);

            if(DB == LCM_DB)
            {
               BOOST_REQUIRE_EQUAL(4, numColumns);
            }
            else if(DB == APP_DB)
            {
               BOOST_REQUIRE_EQUAL(3, numColumns);
            }

            // First step should return a row
            int result = sqlite3_step(statement);
            BOOST_REQUIRE_EQUAL(SQLITE_ROW, result);

            for(int columnIndex = 0; columnIndex < numColumns; columnIndex++)
            {
               std::string columnName(sqlite3_column_name(statement, columnIndex));
               if(!columnName.compare("id"))
               {
                  BOOST_REQUIRE_EQUAL(deviceId, static_cast<uint64_t>(sqlite3_column_int64(statement, columnIndex)));
               }
               else if(!columnName.compare("uid"))
               {
                  BOOST_REQUIRE_EQUAL(0, sqlite3_column_int(statement, columnIndex));
               }
               else if((DB == LCM_DB) &&
                       (!columnName.compare("name")))
               {
                  std::string nameString(reinterpret_cast<const char*>(sqlite3_column_text(statement, columnIndex)));
                  BOOST_REQUIRE_EQUAL(0, nameString.compare("LC7001"));
               }
               else if((DB == APP_DB) &&
                       (!columnName.compare("deviceId")))
               {
               }
               else if(!columnName.compare("connected"))
               {
               }
               else
               {
                  std::stringstream errorSS;
                  errorSS << "Unknown column name received: " << columnName;
                  BOOST_FAIL(errorSS.str());
               }
            }

            // Next step should return an end
            result = sqlite3_step(statement);
            BOOST_REQUIRE_EQUAL(SQLITE_DONE, result);

            // Finalize the statement
            sqlite3_finalize(statement);
         }
         else
         {
            BOOST_FAIL("DB pointer is NULL");
         }
      }
      
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      bool waitForJsonMessage(json_t * message, std::chrono::milliseconds timeoutVal)
      {
         bool messageReceived = false;
         bool timeout = false;

         auto startTime = std::chrono::system_clock::now();

         while((!messageReceived) &&
               (!timeout))
         {
            if(lastJsonObject != nullptr)
            {
               if(json_equal(message, lastJsonObject->getJsonObject()))
               {
                  messageReceived = true;
               }

               lastJsonObject = nullptr;
            }
            else
            {
               auto currentTime = std::chrono::system_clock::now();
               std::chrono::duration<double> diffTime = currentTime - startTime;

               std::chrono::duration<double> timeToWait = timeoutVal - diffTime;

               if(timeToWait < std::chrono::seconds(0))
               {
                  timeout = true;
               }
               else
               {
                  std::unique_lock<std::mutex> lock(responseMutex);
                  auto result = responseCondition.wait_for(lock, timeToWait);

                  if(result == std::cv_status::no_timeout)
                  {
                     // Compare the message
                     if(json_equal(message, lastJsonObject->getJsonObject()))
                     {
                        messageReceived = true;
                     }

                     lastJsonObject = nullptr;
                  }
                  else
                  {
                     timeout = true;
                  }
               }
            }
         }

         return messageReceived;
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      virtual void update(Publisher *who, std::shared_ptr<PublishObject> obj)
      {
         // Convert the object into a JSON message
         auto jsonPublished = std::dynamic_pointer_cast<PublishJson>(obj);

         if(jsonPublished)
         {
            lastJsonObject = jsonPublished;
            responseCondition.notify_all();
         }
      }

   protected:
      sqlite3 *db;
      std::shared_ptr<ExternalNetworkBridge> bridge;

   private:
      std::mutex responseMutex;
      std::condition_variable responseCondition;
      std::shared_ptr<PublishJson> lastJsonObject;
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_ExternalNetworkBridge)
{
   TestCommon::printTestCase();
   ExternalNetworkBridge bridge(APP_PORT, LCM_PORT, REGISTRATION_PORT, DB_NAME);
   bridge.start();
   bridge.stop();

   logInfo("Passed");
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_LCMClientConnected, BridgeFixture)
{
   // Connect an LCM Client with the smallest device ID possible
   uint64_t smallId = 1;
   LCMTestClient small(smallId);

   // Connect an LCM Client with a random device ID in the range of possible addresses
   uint64_t randomId = rand() % 0xFFFFFFFFFFFF + 1;
   LCMTestClient random(randomId);

   // Connect an LCM Client with the largest device IDs possible
   uint64_t largeId = 0xFFFFFFFFFFFF;
   LCMTestClient large(largeId);

   // Open the connection to the bridge for each client
   BOOST_REQUIRE_EQUAL(true, small.openConnection(SERVER_HOST, LCM_PORT));
   BOOST_REQUIRE_EQUAL(true, random.openConnection(SERVER_HOST, LCM_PORT));
   BOOST_REQUIRE_EQUAL(true, large.openConnection(SERVER_HOST, LCM_PORT));

   // Wait for the connections to be made
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(small, true, std::chrono::milliseconds(1000)));
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(random, true, std::chrono::milliseconds(1000)));
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(large, true, std::chrono::milliseconds(1000)));
   
   // Sleep until the device IDs are exchanged
   std::this_thread::sleep_for(std::chrono::milliseconds(1000));

   // Make sure that the devices were added to the database
   verifyIdInDB(LCM_DB, smallId);
   verifyIdInDB(LCM_DB, randomId);
   verifyIdInDB(LCM_DB, largeId);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_RegistrationClientConnected, BridgeFixture)
{
   uint64_t lcmId = 0x123456789AB;
   uint64_t appId = 1234;

   // Connect a Registration client and register the app and lcm IDs
   RegistrationTestClient registrationClient;

   BOOST_REQUIRE_EQUAL(true, registrationClient.openConnection(SERVER_HOST, REGISTRATION_PORT)); 

   BOOST_REQUIRE_EQUAL(true, registrationClient.sendRegistrationMessage(appId, ServerUtil::MACAddressToString(lcmId)));

   // Wait for the message to be received
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   // Make sure that the registration was added to the database
   verifyIdInDB(LCM_DB, lcmId);
   verifyIdInDB(APP_DB, appId);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_AppClientConnected_DBEmpty, BridgeFixture)
{
   // Connection will get closed because no LCM is added to the DB
   AppTestClient appClient(1);

   BOOST_REQUIRE_EQUAL(true, appClient.openConnection(SERVER_HOST, APP_PORT));

   // Wait for the client to be connected
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(appClient, true, std::chrono::milliseconds(1000)));

   // Wait for the client to get disconnected
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(appClient, false, std::chrono::milliseconds(5000)));

   logInfo("Done");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_AppClientConnected_LCMDisconnected, BridgeFixture)
{
   uint64_t appId = 5678;
   std::string lcmIdStr = "00:12:34:56:78:9A";

   // Register the app and LCM IDs
   RegistrationTestClient registrationClient;
   BOOST_REQUIRE_EQUAL(true, registrationClient.openConnection(SERVER_HOST, REGISTRATION_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(registrationClient, true, std::chrono::milliseconds(1000)));
   BOOST_REQUIRE_EQUAL(true, registrationClient.sendRegistrationMessage(appId, lcmIdStr));

   // Wait for the message to be received
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   // Make sure that the registration was added to the database
   verifyIdInDB(LCM_DB, ServerUtil::MACAddressFromString(lcmIdStr));
   verifyIdInDB(APP_DB, appId);

   // Create the app client
   AppTestClient appClient(appId);
   BOOST_REQUIRE_EQUAL(true, appClient.openConnection(SERVER_HOST, APP_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(appClient, true, std::chrono::milliseconds(1000)));

   // Wait for the client to get disconnected because the LCM is offline
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(appClient, false, std::chrono::milliseconds(5000)));
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_WrongLCMConnected, BridgeFixture)
{
   uint64_t appId = 5678;
   uint64_t lcmId = 0x123456789A;
   uint64_t invalidLCMId = 1;
   std::string lcmIdStr = ServerUtil::MACAddressToString(lcmId);
   std::string invalidLCMIdStr = ServerUtil::MACAddressToString(invalidLCMId);

   // Register the app and LCM IDs
   RegistrationTestClient registrationClient;
   BOOST_REQUIRE_EQUAL(true, registrationClient.openConnection(SERVER_HOST, REGISTRATION_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(registrationClient, true, std::chrono::milliseconds(1000)));
   BOOST_REQUIRE_EQUAL(true, registrationClient.sendRegistrationMessage(appId, invalidLCMIdStr));

   // Wait for the message to be received
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   // Make sure that the registration was added to the database
   verifyIdInDB(LCM_DB, invalidLCMId);
   verifyIdInDB(APP_DB, appId);

   // Create the LCM client with the other LCM Id
   LCMTestClient lcmClient(lcmId);
   BOOST_REQUIRE_EQUAL(true, lcmClient.openConnection(SERVER_HOST, LCM_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(lcmClient, true, std::chrono::milliseconds(1000)));
   
   // Give the LCM client some time to send the device ID data
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   // Create the app client
   AppTestClient appClient(appId);
   BOOST_REQUIRE_EQUAL(true, appClient.openConnection(SERVER_HOST, APP_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(appClient, true, std::chrono::milliseconds(1000)));

   // Give the App client some time to send the device ID data and have the bridge make the connection
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   // Wait until the app connection is closed
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(appClient, false, std::chrono::milliseconds(5000)));
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_LCMClosedFirst, BridgeFixture)
{
   uint64_t appId = 5678;
   uint64_t lcmId = 0x123456789A;
   std::string lcmIdStr = ServerUtil::MACAddressToString(lcmId);

   // Register the app and LCM IDs
   RegistrationTestClient registrationClient;
   BOOST_REQUIRE_EQUAL(true, registrationClient.openConnection(SERVER_HOST, REGISTRATION_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(registrationClient, true, std::chrono::milliseconds(1000)));
   BOOST_REQUIRE_EQUAL(true, registrationClient.sendRegistrationMessage(appId, lcmIdStr));

   // Wait for the message to be received
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   // Make sure that the registration was added to the database
   verifyIdInDB(LCM_DB, lcmId);
   verifyIdInDB(APP_DB, appId);

   // Create the LCM client
   LCMTestClient lcmClient(lcmId);
   BOOST_REQUIRE_EQUAL(true, lcmClient.openConnection(SERVER_HOST, LCM_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(lcmClient, true, std::chrono::milliseconds(1000)));
   
   // Give the LCM client some time to send the device ID data
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   // Create the app client
   AppTestClient appClient(appId);
   BOOST_REQUIRE_EQUAL(true, appClient.openConnection(SERVER_HOST, APP_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(appClient, true, std::chrono::milliseconds(1000)));

   // Give the App client some time to send the device ID data and have the bridge make the connection
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   // Close the LCM connection
   lcmClient.closeConnection();
   
   // Wait until the app connection is closed
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(appClient, false, std::chrono::milliseconds(5000)));
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_AppClosedFirst, BridgeFixture)
{
   uint64_t appId = 5678;
   uint64_t lcmId = 0x123456789A;
   std::string lcmIdStr = ServerUtil::MACAddressToString(lcmId);

   // Register the app and LCM IDs
   RegistrationTestClient registrationClient;
   BOOST_REQUIRE_EQUAL(true, registrationClient.openConnection(SERVER_HOST, REGISTRATION_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(registrationClient, true, std::chrono::milliseconds(1000)));
   BOOST_REQUIRE_EQUAL(true, registrationClient.sendRegistrationMessage(appId, lcmIdStr));

   // Wait for the message to be received
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   // Make sure that the registration was added to the database
   verifyIdInDB(LCM_DB, lcmId);
   verifyIdInDB(APP_DB, appId);

   // Create the LCM client
   LCMTestClient lcmClient(lcmId);
   BOOST_REQUIRE_EQUAL(true, lcmClient.openConnection(SERVER_HOST, LCM_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(lcmClient, true, std::chrono::milliseconds(1000)));
   
   // Give the LCM client some time to send the device ID data
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   // Create the app client
   AppTestClient appClient(appId);
   BOOST_REQUIRE_EQUAL(true, appClient.openConnection(SERVER_HOST, APP_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(appClient, true, std::chrono::milliseconds(1000)));

   // Give the App client some time to send the device ID data and have the bridge make the connection
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   // Close the LCM connection
   appClient.closeConnection();
   
   // LCM Connection will remain open so this call will timeout and return false
   BOOST_REQUIRE_EQUAL(false, waitForConnectionStatus(lcmClient, false, std::chrono::milliseconds(5000)));
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_EndToEnd, BridgeFixture)
{
   uint64_t appId = 5678;
   uint64_t lcmId = 0x123456789A;
   std::string lcmIdStr = ServerUtil::MACAddressToString(lcmId);

   // Register the app and LCM IDs
   RegistrationTestClient registrationClient;
   BOOST_REQUIRE_EQUAL(true, registrationClient.openConnection(SERVER_HOST, REGISTRATION_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(registrationClient, true, std::chrono::milliseconds(1000)));
   BOOST_REQUIRE_EQUAL(true, registrationClient.sendRegistrationMessage(appId, lcmIdStr));

   // Wait for the message to be received
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   // Make sure that the registration was added to the database
   verifyIdInDB(LCM_DB, lcmId);
   verifyIdInDB(APP_DB, appId);

   // Create the LCM client
   LCMTestClient lcmClient(lcmId);
   BOOST_REQUIRE_EQUAL(true, lcmClient.openConnection(SERVER_HOST, LCM_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(lcmClient, true, std::chrono::milliseconds(1000)));
   
   // Give the LCM client some time to send the device ID data
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   // Create the app client
   AppTestClient appClient(appId);
   BOOST_REQUIRE_EQUAL(true, appClient.openConnection(SERVER_HOST, APP_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForConnectionStatus(appClient, true, std::chrono::milliseconds(1000)));

   // Give the App client some time to send the device ID data and have the bridge make the connection
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   // Subscribe for any received data from the app client
   appClient.subscribe(this);

   // Create data and send it from the LCM through the bridge and have the app receive it
   json_t *sendObject = json_object();
   json_object_set_new(sendObject, "ID", json_integer(1));
   json_object_set_new(sendObject, "Service", json_string("TestBridgeService"));

   // Send the object to the bridge
   BOOST_REQUIRE_EQUAL(true, lcmClient.sendJsonPacket(sendObject));

   // Verify that the data is received by the app and eventually published to the Fixture
   BOOST_REQUIRE_EQUAL(true, waitForJsonMessage(sendObject, std::chrono::milliseconds(500)));

   // Clean up the memory
   json_decref(sendObject);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
