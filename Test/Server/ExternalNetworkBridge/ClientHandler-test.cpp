#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ClientHandlerTest

#include <boost/test/unit_test.hpp>
#include "Debug.h"
#include "TestCommon.h"
#include "ExternalNetworkBridge.h"
#include "LCMSocketHandler.h"
#include "RegistrationSocketHandler.h"
#include "AppSocketHandler.h"
#include "TestClient.h"
#include "LCMTestClient.h"
#include "AppTestClient.h"
#include "RegistrationTestClient.h"
#include "Subscriber.h"
#include "defines.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>

using namespace boost::unit_test::framework;
static const int SERVER_PORT=1979;
static const std::string SERVER_HOST="127.0.0.1";

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

class ClientHandlerTestFixture : public Subscriber
{
   public:
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      ClientHandlerTestFixture() :
         lastClientSocket(-1),
         serverSocket(-1),
         jsonId(0),
         lastJsonObject(nullptr),
         clientConnected(false),
         lastAppId(0),
         lastLcmId(0),
         lastHandlerId(0)
      {
         TestCommon::printTestCase();

         serverThread = std::thread(&ClientHandlerTestFixture::runServerThread, this);

         // Allow the server thread to start and listen for connections
         std::unique_lock<std::mutex> lock(serverMutex);
         serverCondition.wait_for(lock, std::chrono::milliseconds(500));
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      ~ClientHandlerTestFixture()
      {
         stopServerThread();

         // Free any json objects that were not released
         if(lastJsonObject)
         {
            lastJsonObject = nullptr;
         }
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      void stopServerThread()
      {
         // Shutdown the server socket
         if(serverSocket != -1)
         {
            shutdown(serverSocket, SHUT_RDWR);
            close(serverSocket);
            serverSocket = -1;
         }

         // Join the server thread
         if(serverThread.joinable())
         {
            serverThread.join();
         }
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      bool waitForClientConnection()
      {
         bool clientConnectedInTime = false;

         if(clientConnected)
         {
            clientConnected = false;
            clientConnectedInTime = true;
         }
         else
         {
            std::unique_lock<std::mutex> lock(serverMutex);
            if(serverCondition.wait_for(lock, std::chrono::milliseconds(1000), [this](){ return clientConnected; }))
            {
               clientConnectedInTime = true;
               clientConnected = false;
            }
         }

         return clientConnectedInTime;
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      int getLastClientSocket()
      {
         return lastClientSocket;
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      struct sockaddr_in getLastClientAddress()
      {
         return lastClientAddress;
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      bool waitForRegistrationData(const uint64_t &expectedAppId, const uint64_t &expectedLcmId, const int &expectedHandlerId, std::chrono::milliseconds timeoutVal)
      {
         bool match = false;

         if((lastAppId == 0) && 
            (lastLcmId == 0) && 
            (lastHandlerId == 0))
         {
            std::unique_lock<std::mutex> lock(responseMutex);
            auto result = responseCondition.wait_for(lock, timeoutVal);

            if(result == std::cv_status::no_timeout)
            {
               if((expectedAppId == lastAppId) &&
                  (expectedLcmId == lastLcmId) &&
                  (expectedHandlerId == lastHandlerId))
               {
                  match = true;
               }

               lastAppId = 0;
               lastLcmId = 0;
               lastHandlerId = 0;
            }
         }
         else
         {
            if((expectedAppId == lastAppId) &&
               (expectedLcmId == lastLcmId) &&
               (expectedHandlerId == lastHandlerId))
            {
               match = true;
            }

            lastAppId = 0;
            lastLcmId = 0;
            lastHandlerId = 0;
         }

         return match;
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
                     if(lastJsonObject != nullptr)
                     {
                        if(json_equal(message, lastJsonObject->getJsonObject()))
                        {
                           messageReceived = true;
                        }
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

      virtual bool waitForDeviceId(ClientHandler &handler, const uint64_t &deviceId, const std::chrono::milliseconds &timeoutVal)
      {
         bool deviceIdMatch = false;
         bool timeout = false;

         auto startTime = std::chrono::system_clock::now();

         while((!deviceIdMatch) &&
               (!timeout))
         {
            if(handler.getDeviceId() == deviceId)
            {
               deviceIdMatch = true;
            }
            else
            {
               auto currentTime = std::chrono::system_clock::now();
               std::chrono::duration<double> diffTime = currentTime - startTime;

               if((timeoutVal - diffTime) < std::chrono::milliseconds(0))
               {
                  timeout = true;
               }
               else
               {
                  // Sleep until the next check
                  std::this_thread::sleep_for(std::chrono::milliseconds(10));
               }
            }
         }

         return deviceIdMatch;
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
         else
         {
            auto registrationData = std::dynamic_pointer_cast<RegistrationData>(obj);
            if(registrationData)
            {
               lastAppId = registrationData->getAppId();
               lastLcmId = registrationData->getLCMId();
               lastHandlerId = registrationData->getHandlerId();
               logDebug("Registration data = %lu, %lu, %d", static_cast<long unsigned int>(lastAppId), static_cast<long unsigned int>(lastLcmId), lastHandlerId);
               responseCondition.notify_all();
            }
         }
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

   protected:
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      json_t * createTestObject()
      {
         json_t * jsonObject = json_object();

         if(jsonObject)
         {
            json_object_set_new(jsonObject, "ID", json_integer(jsonId++));
            json_object_set_new(jsonObject, "Service", json_string("TestService"));
         }

         return jsonObject;
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

   private:
      int lastClientSocket;
      struct sockaddr_in lastClientAddress;
      int serverSocket;
      std::thread serverThread;
      std::mutex serverMutex;
      std::condition_variable serverCondition;
      std::mutex callbackMutex;
      std::condition_variable callbackCondition;
      std::mutex responseMutex;
      std::condition_variable responseCondition;
      int jsonId;
      std::shared_ptr<PublishJson> lastJsonObject;
      bool clientConnected;
      uint64_t lastAppId;
      uint64_t lastLcmId;
      int lastHandlerId;

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      void runServerThread()
      {
         serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
         if(serverSocket != -1)
         {
            struct sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_addr.s_addr = INADDR_ANY;
            serverAddr.sin_port = htons(SERVER_PORT);

            // Reuse the local address
            int optionValue = 1;
            setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&optionValue, sizeof(optionValue));

            if(bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
            {
               logError("Failed to bind the server address");
            }
            else
            {
               listen(serverSocket, 10);

               int sizeofAddress = sizeof(lastClientAddress);
               logDebug("Waiting for connection");
               serverCondition.notify_all();
               while((lastClientSocket = accept(serverSocket, (sockaddr*)&lastClientAddress, (socklen_t*)&sizeofAddress)) > 0)
               {
                  logDebug("Connection Made. Notifying condition");
                  clientConnected = true;
                  serverCondition.notify_all();
               }
            }

         }
      }

};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_LCMHandlerInvalidSocket, ClientHandlerTestFixture)
{
   // Create the sockaddr_in to test the IP Address string
   struct sockaddr_in clientAddr;
   int addressSet = inet_aton("192.168.10.10", &clientAddr.sin_addr);
   BOOST_REQUIRE(addressSet != 0);

   // Open an LCM Socket Handler with an invalid socket handle and the address created above
   LCMSocketHandler *handler = new LCMSocketHandler(-1, clientAddr, 1, "", "", "");
   handler->startReceiving();
   std::string expectedIPAddressString("192.168.10.10");
   BOOST_REQUIRE_EQUAL(expectedIPAddressString, handler->getClientIPAddressString());

   // Try to get the device id. Expected 0 because we are not connected to a valid socket
   BOOST_REQUIRE_EQUAL(true, waitForDeviceId((*handler), 0, std::chrono::milliseconds(1000)));

   // Try to get the device id string but it should return "Unknown Device ID"
   std::string expectedDeviceIdString("Unknown Device ID");
   BOOST_REQUIRE_EQUAL(expectedDeviceIdString, handler->getDeviceIdStr());

   // Get the client socket handle. Should return -1 because that is what was given to the constructor
   BOOST_REQUIRE_EQUAL(-1, handler->getClientSocketHandle());

   // Attempt to start pinging. Should fail because the socket handle is invalid
   BOOST_REQUIRE_EQUAL(false, handler->startPinging());

   // Attempt to stop pinging. Should return true because the ping thread is not running
   BOOST_REQUIRE_EQUAL(true, handler->stopPinging());

   // Make sure the socket handler can be destroyed
   delete(handler);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_AppHandlerInvalidSocket, ClientHandlerTestFixture)
{
   // Create the sockaddr_in to test the IP Address string
   struct sockaddr_in clientAddr;
   int addressSet = inet_aton("192.168.10.10", &clientAddr.sin_addr);
   BOOST_REQUIRE(addressSet != 0);

   // Open an App Socket Handler with an invalid socket handle and the address created above
   AppSocketHandler *handler = new AppSocketHandler(-1, clientAddr, 1, "", "", "");
   handler->startReceiving();
   std::string expectedIPAddressString("192.168.10.10");
   BOOST_REQUIRE_EQUAL(expectedIPAddressString, handler->getClientIPAddressString());

   // Try to get the device id. Expected 0 because we are not connected to a valid socket
   BOOST_REQUIRE_EQUAL(true, waitForDeviceId((*handler), 0, std::chrono::milliseconds(1000)));

   // Try to get the device id string but it should return "Unknown Device ID"
   std::string expectedDeviceIdString("Unknown App ID");
   BOOST_REQUIRE_EQUAL(expectedDeviceIdString, handler->getDeviceIdStr());

   // Get the client socket handle. Should return -1 because that is what was given to the constructor
   BOOST_REQUIRE_EQUAL(-1, handler->getClientSocketHandle());

   // Attempt to start pinging. Should fail because the socket handle is invalid
   BOOST_REQUIRE_EQUAL(false, handler->startPinging());

   // Attempt to stop pinging. Should return true because the ping thread is not running
   BOOST_REQUIRE_EQUAL(true, handler->stopPinging());

   // Make sure the socket handler can be destroyed
   delete(handler);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_RegistrationHandlerInvalidSocket, ClientHandlerTestFixture)
{
   // Create the sockaddr_in to test the IP Address string
   struct sockaddr_in clientAddr;
   int addressSet = inet_aton("192.168.10.10", &clientAddr.sin_addr);
   BOOST_REQUIRE(addressSet != 0);

   // Open a Registration Socket Handler with an invalid socket handle and the address created above
   RegistrationSocketHandler *handler = new RegistrationSocketHandler(-1, clientAddr, 1, "", "", "");
   handler->startReceiving();

   // Make sure the socket handler can be destroyed
   delete(handler);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_GetDeviceIDNoResponse, ClientHandlerTestFixture)
{
   // Create a client with no device ID so no response is generated
   TestClient client;

   // Open the connection
   BOOST_REQUIRE_EQUAL(true, client.openConnection(SERVER_HOST, SERVER_PORT));

   // Wait for the connection to be established
   BOOST_REQUIRE_EQUAL(true, waitForClientConnection());

   // Create the socket handler with this connection
   LCMSocketHandler LCMHandler(getLastClientSocket(), getLastClientAddress(), 1, "", "", "");
   LCMHandler.startReceiving();

   // Get the device ID from the handler
   BOOST_CHECK_EQUAL(true, waitForDeviceId(LCMHandler, 0, std::chrono::milliseconds(1000)));
   BOOST_CHECK_EQUAL("Unknown Device ID", LCMHandler.getDeviceIdStr());

   // Create the app handler with this connection
   AppSocketHandler appHandler(getLastClientSocket(), getLastClientAddress(), 1, "", "", "");
   appHandler.startReceiving();
   BOOST_CHECK_EQUAL(true, waitForDeviceId(appHandler, 0, std::chrono::milliseconds(1000)));
   BOOST_CHECK_EQUAL("Unknown App ID", appHandler.getDeviceIdStr());

   // Close the client connection
   std::this_thread::sleep_for(std::chrono::milliseconds(500));
   BOOST_REQUIRE_EQUAL(true, client.closeConnection());
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_LCMGetDeviceID, ClientHandlerTestFixture)
{
   // Create a client with a device id of 1
   uint64_t deviceId = 1;
   LCMTestClient client(deviceId);

   // Open the connection
   BOOST_REQUIRE_EQUAL(true, client.openConnection(SERVER_HOST, SERVER_PORT));

   // Wait for the connection to be established
   BOOST_REQUIRE_EQUAL(true, waitForClientConnection());

   // Create the socket handler with this connection
   LCMSocketHandler handler(getLastClientSocket(), getLastClientAddress(), 1, "", "", "");
   handler.startReceiving();

   // Get the device ID from the handler and make sure it matches the expected ID
   BOOST_REQUIRE_EQUAL(true, waitForDeviceId(handler, deviceId, std::chrono::milliseconds(1000)));
   BOOST_REQUIRE_EQUAL("00:00:00:00:00:01", handler.getDeviceIdStr());

   // Close the client connection
   std::this_thread::sleep_for(std::chrono::milliseconds(500));
   BOOST_REQUIRE_EQUAL(true, client.closeConnection());
   std::this_thread::sleep_for(std::chrono::milliseconds(1000));

   // Repeat the test for a client device id filling in all bytes of the MAC Address
   uint64_t deviceId2 = 0x0123456789AB;
   LCMTestClient client2(deviceId2);
   BOOST_REQUIRE_EQUAL(true, client2.openConnection(SERVER_HOST, SERVER_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForClientConnection());
   LCMSocketHandler handler2(getLastClientSocket(), getLastClientAddress(), 2, "", "", "");
   handler2.startReceiving();
   BOOST_REQUIRE_EQUAL(true, waitForDeviceId(handler2, deviceId2, std::chrono::milliseconds(1000)));
   BOOST_REQUIRE_EQUAL("01:23:45:67:89:AB", handler2.getDeviceIdStr());
   std::this_thread::sleep_for(std::chrono::milliseconds(500));
   BOOST_REQUIRE_EQUAL(true, client2.closeConnection());
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_AppGetDeviceID, ClientHandlerTestFixture)
{
   // Create a client with a device id of 1
   uint64_t deviceId = 1;
   std::stringstream deviceIdSS;
   deviceIdSS << deviceId;
   AppTestClient client(deviceId);

   // Open the connection
   BOOST_REQUIRE_EQUAL(true, client.openConnection(SERVER_HOST, SERVER_PORT));

   // Wait for the connection to be established
   BOOST_REQUIRE_EQUAL(true, waitForClientConnection());

   // Create the socket handler with this connection
   AppSocketHandler handler(getLastClientSocket(), getLastClientAddress(), 1, "", "", "");
   handler.startReceiving();

   // Get the device ID from the handler and make sure it matches the expected ID
   BOOST_REQUIRE_EQUAL(true, waitForDeviceId(handler, deviceId, std::chrono::milliseconds(1000)));
   BOOST_REQUIRE_EQUAL(deviceIdSS.str(), handler.getDeviceIdStr());

   // Close the client connection
   std::this_thread::sleep_for(std::chrono::milliseconds(500));
   BOOST_REQUIRE_EQUAL(true, client.closeConnection());
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_LCMReceivePing, ClientHandlerTestFixture)
{
   // Create a client
   TestClient client;

   // Open the connection
   BOOST_REQUIRE_EQUAL(true, client.openConnection(SERVER_HOST, SERVER_PORT));

   // Wait for the connection to be established
   BOOST_REQUIRE_EQUAL(true, waitForClientConnection());

   // Create the socket handler with this connection
   LCMSocketHandler handler(getLastClientSocket(), getLastClientAddress(), 1, "", "", "");
   handler.startReceiving();

   // Attempt to start pinging. Should fail because the socket handle is invalid
   BOOST_REQUIRE_EQUAL(true, handler.startPinging());

   // Create the expected message
   json_t * pingObject = json_object();
   json_object_set_new(pingObject, "ID", json_integer(0));
   json_object_set_new(pingObject, "Service", json_string("ping"));
   json_object_set_new(pingObject, "Status", json_string("Success"));

   // Wait to receive a ping
   client.subscribe(this);
   BOOST_REQUIRE_EQUAL(true, waitForJsonMessage(pingObject, std::chrono::seconds(10)));

   // Free memory
   json_decref(pingObject);

   // Stop Pinging
   BOOST_REQUIRE_EQUAL(true, handler.stopPinging());

   // Close the client connection
   std::this_thread::sleep_for(std::chrono::milliseconds(500));
   BOOST_REQUIRE_EQUAL(true, client.closeConnection());
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_AddBridgedSocket, ClientHandlerTestFixture)
{
   // Open the receiving socket connection
   uint64_t receiverId = 1;
   LCMTestClient receiver(receiverId);
   BOOST_REQUIRE_EQUAL(true, receiver.openConnection(SERVER_HOST, SERVER_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForClientConnection());

   // Subscribe for any messages that the receiver gets
   receiver.subscribe(this);

   // Store the receiver socket handle
   int receiverSocketHandle = getLastClientSocket();

   // Open a sending socket connection
   uint64_t senderId = 2;
   LCMTestClient sender(senderId);
   BOOST_REQUIRE_EQUAL(true, sender.openConnection(SERVER_HOST, SERVER_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForClientConnection());

   // Create the socket handler with the sender connection
   int senderSocketHandle = getLastClientSocket();
   LCMSocketHandler handler(senderSocketHandle, getLastClientAddress(), 1, "", "", "");
   handler.startReceiving();

   // Need to get the device ID from the handler so data will be forwarded 
   // to any bridged socket
   BOOST_REQUIRE_EQUAL(true, waitForDeviceId(handler, senderId, std::chrono::milliseconds(1000)));

   // Add the bridged socket for the receiver
   handler.addBridgedSocketHandle(receiverSocketHandle);

   // Send the json data to the socket
   json_t * sendObject = createTestObject();
   BOOST_REQUIRE_EQUAL(true, sender.sendJsonPacket(sendObject));

   // Verify that data is received at the receiver
   BOOST_REQUIRE_EQUAL(true, waitForJsonMessage(sendObject, std::chrono::milliseconds(500)));

   // Clean up the memory
   json_decref(sendObject);

   // Close the connections
   std::this_thread::sleep_for(std::chrono::milliseconds(500));
   sender.closeConnection();
   receiver.closeConnection();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_RegistrationSocketHandlerNoAppIDObject, ClientHandlerTestFixture)
{
   // Open the receiving socket connection
   RegistrationTestClient registrationClient;
   BOOST_REQUIRE_EQUAL(true, registrationClient.openConnection(SERVER_HOST, SERVER_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForClientConnection());

   // Create the registration socket handler with the connection
   int handlerId = 1;
   RegistrationSocketHandler handler(getLastClientSocket(), getLastClientAddress(), handlerId, "", "", "");
   handler.startReceiving();

   // Subscribe to receive updates
   registrationClient.subscribe(this);

   // Send the registration data to the socket with no App ID Object
   json_t * jsonObject = json_object();
   json_object_set_new(jsonObject, "ID", json_integer(1));
   json_object_set_new(jsonObject, "Service", json_string("AppRegistration"));
   json_object_set_new(jsonObject, "lcmID", json_string("01:23:45:67:89:AB"));
   BOOST_REQUIRE_EQUAL(true, registrationClient.sendJsonPacket(jsonObject));
   json_decref(jsonObject);

   // Wait for the error response
   json_t * responseObject = json_object();
   json_object_set_new(responseObject, "ID", json_integer(1));
   json_object_set_new(responseObject, "Service", json_string("AppRegistration"));
   json_object_set_new(responseObject, "Status", json_string("Error"));
   json_object_set_new(responseObject, "ErrorText", json_string("appID object is NULL"));
   json_object_set_new(responseObject, "ErrorCode", json_integer(SW_APP_ID_NULL));
   BOOST_REQUIRE_EQUAL(true, waitForJsonMessage(responseObject, std::chrono::milliseconds(500)));
   json_decref(responseObject);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_RegistrationSocketHandlerNoLCMIDObject, ClientHandlerTestFixture)
{
   // Open the receiving socket connection
   RegistrationTestClient registrationClient;
   BOOST_REQUIRE_EQUAL(true, registrationClient.openConnection(SERVER_HOST, SERVER_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForClientConnection());

   // Create the registration socket handler with the connection
   int handlerId = 1;
   RegistrationSocketHandler handler(getLastClientSocket(), getLastClientAddress(), handlerId, "", "", "");
   handler.startReceiving();

   // Subscribe to receive updates
   registrationClient.subscribe(this);

   // Send the registration data to the socket with no App ID Object
   json_t * jsonObject = json_object();
   json_object_set_new(jsonObject, "ID", json_integer(1));
   json_object_set_new(jsonObject, "Service", json_string("AppRegistration"));
   json_object_set_new(jsonObject, "appID", json_integer(1234));
   BOOST_REQUIRE_EQUAL(true, registrationClient.sendJsonPacket(jsonObject));
   json_decref(jsonObject);

   // Wait for the error response
   json_t * responseObject = json_object();
   json_object_set_new(responseObject, "ID", json_integer(1));
   json_object_set_new(responseObject, "Service", json_string("AppRegistration"));
   json_object_set_new(responseObject, "Status", json_string("Error"));
   json_object_set_new(responseObject, "ErrorText", json_string("lcmID object is NULL"));
   json_object_set_new(responseObject, "ErrorCode", json_integer(SW_LCM_ID_NULL));
   BOOST_REQUIRE_EQUAL(true, waitForJsonMessage(responseObject, std::chrono::milliseconds(500)));
   json_decref(responseObject);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_RegistrationSocketHandlerAppIDNotInt, ClientHandlerTestFixture)
{
   // Open the receiving socket connection
   RegistrationTestClient registrationClient;
   BOOST_REQUIRE_EQUAL(true, registrationClient.openConnection(SERVER_HOST, SERVER_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForClientConnection());

   // Create the registration socket handler with the connection
   int handlerId = 1;
   RegistrationSocketHandler handler(getLastClientSocket(), getLastClientAddress(), handlerId, "", "", "");
   handler.startReceiving();

   // Subscribe to receive updates
   registrationClient.subscribe(this);

   // Send the registration data to the socket with no App ID Object
   json_t * jsonObject = json_object();
   json_object_set_new(jsonObject, "ID", json_integer(1));
   json_object_set_new(jsonObject, "Service", json_string("AppRegistration"));
   json_object_set_new(jsonObject, "appID", json_string("1234"));
   json_object_set_new(jsonObject, "lcmID", json_string("01:23:45:67:89:AB"));
   BOOST_REQUIRE_EQUAL(true, registrationClient.sendJsonPacket(jsonObject));
   json_decref(jsonObject);

   // Wait for the error response
   json_t * responseObject = json_object();
   json_object_set_new(responseObject, "ID", json_integer(1));
   json_object_set_new(responseObject, "Service", json_string("AppRegistration"));
   json_object_set_new(responseObject, "Status", json_string("Error"));
   json_object_set_new(responseObject, "ErrorText", json_string("appID object must be an integer"));
   json_object_set_new(responseObject, "ErrorCode", json_integer(SW_APP_ID_MUST_BE_INTEGER));
   BOOST_REQUIRE_EQUAL(true, waitForJsonMessage(responseObject, std::chrono::milliseconds(500)));
   json_decref(responseObject);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_RegistrationSocketHandlerLCMIDNotString, ClientHandlerTestFixture)
{
   // Open the receiving socket connection
   RegistrationTestClient registrationClient;
   BOOST_REQUIRE_EQUAL(true, registrationClient.openConnection(SERVER_HOST, SERVER_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForClientConnection());

   // Create the registration socket handler with the connection
   int handlerId = 1;
   RegistrationSocketHandler handler(getLastClientSocket(), getLastClientAddress(), handlerId, "", "", "");
   handler.startReceiving();

   // Subscribe to receive updates
   registrationClient.subscribe(this);

   // Send the registration data to the socket with no App ID Object
   json_t * jsonObject = json_object();
   json_object_set_new(jsonObject, "ID", json_integer(1));
   json_object_set_new(jsonObject, "Service", json_string("AppRegistration"));
   json_object_set_new(jsonObject, "appID", json_integer(1234));
   json_object_set_new(jsonObject, "lcmID", json_integer(0x1234567));
   BOOST_REQUIRE_EQUAL(true, registrationClient.sendJsonPacket(jsonObject));
   json_decref(jsonObject);

   // Wait for the error response
   json_t * responseObject = json_object();
   json_object_set_new(responseObject, "ID", json_integer(1));
   json_object_set_new(responseObject, "Service", json_string("AppRegistration"));
   json_object_set_new(responseObject, "Status", json_string("Error"));
   json_object_set_new(responseObject, "ErrorText", json_string("lcmID object must be a string"));
   json_object_set_new(responseObject, "ErrorCode", json_integer(SW_LCM_ID_MUST_BE_STRING));
   BOOST_REQUIRE_EQUAL(true, waitForJsonMessage(responseObject, std::chrono::milliseconds(500)));
   json_decref(responseObject);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_RegistrationSocketHandler, ClientHandlerTestFixture)
{
   // Open the receiving socket connection
   RegistrationTestClient registrationClient;
   BOOST_REQUIRE_EQUAL(true, registrationClient.openConnection(SERVER_HOST, SERVER_PORT));
   BOOST_REQUIRE_EQUAL(true, waitForClientConnection());

   // Create the registration socket handler with the connection
   int handlerId = 1;
   RegistrationSocketHandler handler(getLastClientSocket(), getLastClientAddress(), handlerId, "", "", "");
   handler.startReceiving();

   // Subscribe to receive updates when the registration is received
   handler.subscribe(this);
   registrationClient.subscribe(this);

   // Send the registration data to the socket
   BOOST_REQUIRE_EQUAL(true, registrationClient.sendRegistrationMessage(1234, "01:23:45:67:89:AB"));

   // Make sure the status success was returned
   json_t * responseObject = json_object();
   json_object_set_new(responseObject, "ID", json_integer(1));
   json_object_set_new(responseObject, "Service", json_string("AppRegistration"));
   json_object_set_new(responseObject, "Status", json_string("Success"));
   BOOST_REQUIRE_EQUAL(true, waitForJsonMessage(responseObject, std::chrono::milliseconds(500)));

   // Wait for registration data to be received by the subscriber
   BOOST_REQUIRE_EQUAL(true, waitForRegistrationData(1234, 0x0123456789AB, handlerId, std::chrono::milliseconds(500)));
   json_decref(responseObject);

   // Close the connections
   std::this_thread::sleep_for(std::chrono::milliseconds(500));
   registrationClient.closeConnection();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
