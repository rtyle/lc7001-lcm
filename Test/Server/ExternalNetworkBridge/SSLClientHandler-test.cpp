#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SSLClientHandlerTest

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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <getopt.h>

using namespace boost::unit_test::framework;
static const int SERVER_PORT=1979;
static const std::string SERVER_HOST="127.0.0.1";

typedef enum
{
   LCM_HANDLER,
   APP_HANDLER,
   REGISTRATION_HANDLER
} TEST_TYPE;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

class SSLClientHandlerTestFixture : public Subscriber
{
   public:
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      SSLClientHandlerTestFixture() :
         serverCertFilename(""),
         serverKeyFilename(""),
         clientCertFilename(""),
         clientKeyFilename(""),
         serverSocket(-1),
         lastAppId(0),
         lastLcmId(0),
         lastJsonObject(nullptr)
      {
         TestCommon::printTestCase();

         // There should be 9 arguments for this test
         // First argument is the executable name
         // The next 8 arguments are for the cert/key filenames
         //    4 arguments to get which option is being set
         //    4 arguments to get the argument for the option
         BOOST_REQUIRE(master_test_suite().argc == 9);

         static struct option longOptions[] =
         {
            {"serverCert", required_argument, 0, 0},
            {"serverKey", required_argument, 0, 0},
            {"clientCert", required_argument, 0, 0},
            {"clientKey", required_argument, 0, 0},
            {0, 0, 0, 0}
         };

         optind = 0;
         int optionIndex = 0;
         int option = 0;
         while ((option = getopt_long(master_test_suite().argc, master_test_suite().argv, "", longOptions, &optionIndex)) != -1)
         {
            switch(option)
            {
               case 0:
                  if(longOptions[optionIndex].flag == 0)
                  {
                     if(optarg)
                     {
                        switch(optionIndex)
                        {
                           case 0:
                              serverCertFilename = optarg;
                              break;
                           case 1:
                              serverKeyFilename = optarg;
                              break;
                           case 2:
                              clientCertFilename = optarg;
                              break;
                           case 3:
                              clientKeyFilename = optarg;
                              break;
                           default:
                              BOOST_FAIL("Invalid option received");
                        }
                     }
                     else
                     {
                        BOOST_FAIL("Optarg is invalid for index" << optionIndex);
                     }
                  }
                  break;
               default:
                  BOOST_FAIL("Invalid option received");
            }
         }


         // Ensure that all of the filenames were set properly
         if(serverCertFilename.empty())
         {
            BOOST_FAIL("Server Certificate file name is empty");
         }
         else
         {
            logDebug("Server Certificate filename is %s", serverCertFilename.c_str());
         }

         if(serverKeyFilename.empty())
         {
            BOOST_FAIL("Server Key file name is empty");
         }
         else
         {
            logDebug("Server Key filename is %s", serverKeyFilename.c_str());
         }

         if(clientCertFilename.empty())
         {
            BOOST_FAIL("Client Certificate file name is empty");
         }
         else
         {
            logDebug("Client Certificate filename is %s", clientCertFilename.c_str());
         }

         if(clientKeyFilename.empty())
         {
            BOOST_FAIL("Client Key file name is empty");
         }
         else
         {
            logDebug("Client Key filename is %s", clientKeyFilename.c_str());
         }
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      ~SSLClientHandlerTestFixture()
      {
         stopServerThread();

         handlers.clear();
         registrationHandlers.clear();

         if(lastJsonObject)
         {
            lastJsonObject = nullptr;
         }
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      void startServerThread(const bool &useSSL, const TEST_TYPE &testType)
      {
         serverThread = std::thread(&SSLClientHandlerTestFixture::runServerThread, this, useSSL, testType);

         // Allow the server thread to start and listen for connections
         std::unique_lock<std::mutex> lock(serverMutex);
         serverCondition.wait_for(lock, std::chrono::milliseconds(500));
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

      bool waitForRegistrationData(const uint64_t &expectedAppId, const uint64_t &expectedLcmId, std::chrono::milliseconds timeoutVal)
      {
         bool match = false;

         if((lastAppId == 0) && 
            (lastLcmId == 0))
         {
            std::unique_lock<std::mutex> lock(responseMutex);
            auto result = responseCondition.wait_for(lock, timeoutVal);

            if(result == std::cv_status::no_timeout)
            {
               if((expectedAppId == lastAppId) &&
                  (expectedLcmId == lastLcmId))
               {
                  match = true;
               }

               lastAppId = 0;
               lastLcmId = 0;
            }
         }
         else
         {
            if((expectedAppId == lastAppId) &&
               (expectedLcmId == lastLcmId))
            {
               match = true;
            }

            lastAppId = 0;
            lastLcmId = 0;
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

      virtual bool waitForDeviceId(std::shared_ptr<ClientHandler> handler, const uint64_t &deviceId, const std::chrono::milliseconds &timeoutVal)
      {
         bool deviceIdMatch = false;
         bool timeout = false;

         auto startTime = std::chrono::system_clock::now();

         while((!deviceIdMatch) &&
               (!timeout))
         {
            if(handler->getDeviceId() == deviceId)
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
            // Convert the object to registration data
            auto registrationData = std::dynamic_pointer_cast<RegistrationData>(obj);
            if(registrationData)
            {
               lastAppId = registrationData->getAppId();
               lastLcmId = registrationData->getLCMId();

               logDebug("Registration data = %lu, %lu", static_cast<long unsigned int>(lastAppId), static_cast<long unsigned int>(lastLcmId));
               responseCondition.notify_all();
            }
         }
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

   protected:
      std::vector<std::shared_ptr<ClientHandler>> handlers;
      std::vector<std::shared_ptr<RegistrationSocketHandler>> registrationHandlers;
      std::string serverCertFilename;
      std::string serverKeyFilename;
      std::string clientCertFilename;
      std::string clientKeyFilename;

   private:
      int serverSocket;
      std::thread serverThread;
      std::mutex serverMutex;
      std::condition_variable serverCondition;
      std::mutex responseMutex;
      std::condition_variable responseCondition;
      uint64_t lastAppId;
      uint64_t lastLcmId;
      std::shared_ptr<PublishJson> lastJsonObject;

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      void runServerThread(const bool &useSSL, const TEST_TYPE &testType)
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

               int clientSocket;
               struct sockaddr_in clientAddress;
               int sizeofAddress = sizeof(clientAddress);

               std::string testClientCert;
               std::string testServerCert;
               std::string testServerKey;

               int registrationHandlerId = 0;
               uint64_t lcmHandlerId = 0;
               uint64_t appHandlerId = 0;

               if(useSSL)
               {
                  testClientCert = clientCertFilename;
                  testServerCert = serverCertFilename;
                  testServerKey = serverKeyFilename;
               }

               logDebug("Waiting for connection");
               serverCondition.notify_all();
               while((clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, (socklen_t*)&sizeofAddress)) > 0)
               {
                  logInfo("Creating the handler");
                  if(testType == LCM_HANDLER)
                  {
                     auto handler = std::make_shared<LCMSocketHandler>(clientSocket, clientAddress, lcmHandlerId++, serverCertFilename, serverKeyFilename, clientCertFilename);
                     handler->startReceiving();
                     handlers.push_back(handler);
                  }
                  else if(testType == APP_HANDLER)
                  {
                     auto handler = std::make_shared<AppSocketHandler>(clientSocket, clientAddress, appHandlerId++, serverCertFilename, serverKeyFilename, clientCertFilename);
                     handler->startReceiving();
                     handlers.push_back(handler);
                  }
                  else if(testType == REGISTRATION_HANDLER)
                  {
                     auto handler = std::make_shared<RegistrationSocketHandler>(clientSocket, clientAddress, registrationHandlerId++, serverCertFilename, serverKeyFilename, clientCertFilename);
                     handler->startReceiving();
                     registrationHandlers.push_back(handler);
                  }
                  else
                  {
                     BOOST_FAIL("Unknown Test Type");
                  }
               }
            }

         }
      }

};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_SSLLCMHandler, SSLClientHandlerTestFixture)
{
   // Start the server thread using SSL
   startServerThread(true, LCM_HANDLER);

   // Create a client with a device id of 1
   uint64_t deviceId = 1;
   LCMTestClient client(deviceId);
   BOOST_REQUIRE_EQUAL(true, client.openConnection(SERVER_HOST, SERVER_PORT, clientCertFilename, clientKeyFilename, serverCertFilename));
   BOOST_REQUIRE_EQUAL(1, handlers.size());
   BOOST_REQUIRE_EQUAL(true, waitForDeviceId(handlers[0], 1, std::chrono::milliseconds(100)));
   BOOST_REQUIRE_EQUAL("00:00:00:00:00:01", handlers[0]->getDeviceIdStr());
   BOOST_REQUIRE_EQUAL(true, client.closeConnection());
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_SSLAppHandler, SSLClientHandlerTestFixture)
{
   // Start the server thread using SSL
   startServerThread(true, APP_HANDLER);

   // Create a client with a device id of 1
   uint64_t deviceId = 1;
   std::stringstream deviceIdSS;
   deviceIdSS << deviceId;
   AppTestClient client(deviceId);
   BOOST_REQUIRE_EQUAL(true, client.openConnection(SERVER_HOST, SERVER_PORT, clientCertFilename, clientKeyFilename, serverCertFilename));
   BOOST_REQUIRE_EQUAL(1, handlers.size());
   BOOST_REQUIRE_EQUAL(true, waitForDeviceId(handlers[0], 1, std::chrono::milliseconds(100)));
   BOOST_REQUIRE_EQUAL(deviceIdSS.str(), handlers[0]->getDeviceIdStr());
   BOOST_REQUIRE_EQUAL(true, client.closeConnection());

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_SSLRegistrationHandler, SSLClientHandlerTestFixture)
{
   // Start the server thread using SSL
   startServerThread(true, REGISTRATION_HANDLER);

   // Create a registration test client
   RegistrationTestClient registrationClient;
   BOOST_REQUIRE_EQUAL(true, registrationClient.openConnection(SERVER_HOST, SERVER_PORT, clientCertFilename, clientKeyFilename, serverCertFilename));
   BOOST_REQUIRE_EQUAL(1, registrationHandlers.size());

   // Subscribe to receive updates when the registration is received
   registrationHandlers[0]->subscribe(this);
   registrationClient.subscribe(this);

   // Send the registration data to the socket
   BOOST_REQUIRE_EQUAL(true, registrationClient.sendRegistrationMessage(1234, "01:23:45:67:89:AB"));

   // Make sure the status success was returned
   json_t * responseObject = json_object();
   json_object_set_new(responseObject, "ID", json_integer(1));
   json_object_set_new(responseObject, "Service", json_string("AppRegistration"));
   json_object_set_new(responseObject, "Status", json_string("Success"));
   BOOST_REQUIRE_EQUAL(true, waitForJsonMessage(responseObject, std::chrono::milliseconds(500)));

   // Wait for the registration data to be received by the subscriber
   BOOST_REQUIRE_EQUAL(true, waitForRegistrationData(1234, 0x0123456789AB, std::chrono::milliseconds(500)));
   json_decref(responseObject);

   // Close the connection
   registrationClient.closeConnection();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

