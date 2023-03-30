#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SSLTest

#include <boost/test/unit_test.hpp>
#include <getopt.h>
#include "Debug.h"
#include "SSLServer.h"
#include "SSLClient.h"
#include "Publisher.h"
#include "Subscriber.h"

using namespace boost::unit_test::framework;

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 1979

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void printTestCase(std::string testCase)
{
   logInfo("************************************************************");
   logInfo("Running Test Case %s", testCase.c_str());
   logInfo("************************************************************");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

class SSLFixture : public Subscriber
{
   public:
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      
      SSLFixture()
      {
         // There should be 13 arguments for this test
         // First argument is the executable name
         // The next 12 arguments are for the cert/key filenames
         //    6 arguments to get which option is being set
         //    6 arguments to get the argument for the option
         BOOST_REQUIRE(master_test_suite().argc == 13);

         static struct option longOptions[] =
         {
            {"serverCert", required_argument, 0, 0},
            {"serverKey", required_argument, 0, 0},
            {"expiredCert", required_argument, 0, 0},
            {"expiredKey", required_argument, 0, 0},
            {"clientCert", required_argument, 0, 0},
            {"clientKey", required_argument, 0, 0},
            {0, 0, 0, 0}
         };

         // getopt_long stores the option index here
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
                              expiredCertFilename = optarg;
                              break;
                           case 3:
                              expiredKeyFilename = optarg;
                              break;
                           case 4:
                              clientCertFilename = optarg;
                              break;
                           case 5:
                              clientKeyFilename = optarg;
                              break;
                           default:
                              BOOST_FAIL("Invalid option received");
                        }
                     }
                     else
                     {
                        BOOST_FAIL("Optarg is invalid for index " << optionIndex);
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

         if(expiredCertFilename.empty())
         {
            BOOST_FAIL("Expired Certificate file name is empty");
         }
         else
         {
            logDebug("Expired Certificate filename is %s", expiredCertFilename.c_str());
         }

         if(expiredKeyFilename.empty())
         {
            BOOST_FAIL("Expired Key file name is empty");
         }
         else
         {
            logDebug("Expired Key filename is %s", expiredKeyFilename.c_str());
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
            logDebug("Client Key filename is %s", clientCertFilename.c_str());
         }
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      virtual ~SSLFixture()
      {
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      int waitForBytesFromServer(size_t expectedCount, std::chrono::milliseconds timeout)
      {
         std::unique_lock<std::mutex> lock(serverMutex);
         size_t byteCount = 0;
         serverCondition.wait_for(lock, timeout, [this, &byteCount, expectedCount]()
            {
               byteCount += serverBytes.size();

               return (byteCount == expectedCount);
            });

         return byteCount;
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      int waitForBytesFromClient(size_t expectedCount, std::chrono::milliseconds timeout)
      {
         std::unique_lock<std::mutex> lock(clientMutex);
         size_t byteCount = 0;
         clientCondition.wait_for(lock, timeout, [this, &byteCount, expectedCount]()
            {
               byteCount += clientBytes.size();
               return (byteCount == expectedCount);
            });

         return byteCount;
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      virtual void update(Publisher *who, std::shared_ptr<PublishObject> obj)
      {
         // Convert the object into the SSL Bytes
         auto sslObject = std::dynamic_pointer_cast<SSLBytes>(obj);

         if(who == server)
         {
            logDebug("Received %d bytes from server", static_cast<int>(sslObject->getBytes().size()));

            std::lock_guard<std::mutex> lock(serverMutex);
            serverBytes = sslObject->getBytes();
            serverCondition.notify_one();
         }
         else if(who == client)
         {
            logDebug("Received %d bytes from client", static_cast<int>(sslObject->getBytes().size()));
            std::lock_guard<std::mutex> lock(clientMutex);
            clientBytes = sslObject->getBytes();
            clientCondition.notify_one();
         }
         else
         {
            logDebug("Received %d bytes from unknown %p", static_cast<int>(sslObject->getBytes().size()), who);
         }
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      void setServerPointer(Publisher *publisher)
      {
         server = publisher;
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

      void setClientPointer(Publisher *publisher)
      {
         client = publisher;
      }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

   protected:
      std::string serverCertFilename;
      std::string serverKeyFilename;
      std::string expiredCertFilename;
      std::string expiredKeyFilename;
      std::string clientCertFilename;
      std::string clientKeyFilename;

      std::vector<uint8_t> serverBytes;
      std::vector<uint8_t> clientBytes;

   private:
      Publisher *server;
      Publisher *client;

      std::mutex serverMutex;
      std::mutex clientMutex;
      std::condition_variable serverCondition;
      std::condition_variable clientCondition;
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Unit Tests:
// 1) Start server and shutdown cleanly
// 2) Start client and shutdown cleanly
// 3) Connect client to server using expired certificate on server
// 4) Connect client to server with unexpected certificate on server
// 5) Connect client to server using expired certificate from client
// 6) Connect client to server using unexpected certificate from client
// 7) Connect client to server with valid certificate
// 8) Connect client to server.
//     Send data from client to server. Ensure that data is received by server
//     Send data from server to client. Ensure that data is received by client
// 9) Connect client to server. 
//    Shutdown client and attempt to write to socket.
//    Ensure server detects disconnected client and shuts down connection
// 10)Connect client to server. 
//    Shutdown server then attempt to write to socket.
//    Ensure client detects disconnected server and shuts down connection
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test1_ServerShutdown, SSLFixture)
{
   printTestCase(current_test_case().p_name.value);

   SSLServer server(SERVER_PORT, serverCertFilename, serverKeyFilename, clientCertFilename);
   server.start();
   server.stop();

   // If it makes it to here the test passed because there were no problems
   logDebug("Server created and shut down successfully");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test2_ClientShutdown, SSLFixture)
{
   printTestCase(current_test_case().p_name.value);

   SSLClient client(clientCertFilename, clientKeyFilename, serverCertFilename);

   // Client will not connect because the server is not running
   logDebug("Expect failure to connect to server");
   BOOST_REQUIRE_EQUAL(client.connectToHost(SERVER_ADDRESS, SERVER_PORT), false);

   // Send will fail because client is not connected
   logDebug("Expect failure to send data because of invalid handle");
   std::vector<uint8_t> testBytes = {0x01, 0x02};
   BOOST_REQUIRE_EQUAL(client.send(testBytes), false);

   // Disconnect will pass because client is disconnected
   BOOST_REQUIRE_EQUAL(client.disconnect(), true);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test3_ServerExpiredCertificate, SSLFixture)
{
   printTestCase(current_test_case().p_name.value);

   // Create a server using the expired certificate and key
   SSLServer server(SERVER_PORT, expiredCertFilename, expiredKeyFilename, clientCertFilename);
   server.start();
   
   // Create a client
   SSLClient client(clientCertFilename, clientKeyFilename, expiredCertFilename);

   // Client will not connect because the server has expired certificates
   logDebug("Expect failure because of expired certificate");
   BOOST_REQUIRE_EQUAL(client.connectToHost(SERVER_ADDRESS, SERVER_PORT), false);

   // Disconnect the client
   BOOST_REQUIRE_EQUAL(client.disconnect(), true);

   // Shutdown the server
   server.stop();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test4_ServerUnexpectedCertificate, SSLFixture)
{
   printTestCase(current_test_case().p_name.value);

   // Create a server using a valid certificate
   SSLServer server(SERVER_PORT, serverCertFilename, serverKeyFilename, clientCertFilename);
   server.start();

   // Create a client using the wrong certificate
   SSLClient client(clientCertFilename, clientKeyFilename, clientCertFilename);

   // Client will not connect because the certificates are different
   logDebug("Expect failure because of different certificates");
   BOOST_REQUIRE_EQUAL(client.connectToHost(SERVER_ADDRESS, SERVER_PORT), false);

   // Disconnect the client
   BOOST_REQUIRE_EQUAL(client.disconnect(), true);

   // Shutdown the server
   server.stop();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test5_ClientExpiredCertificate, SSLFixture)
{
   printTestCase(current_test_case().p_name.value);

   // Create a server using the server certificate and key
   SSLServer server(SERVER_PORT, serverCertFilename, serverKeyFilename, expiredCertFilename);
   server.start();
   
   // Create a client with the expired certificate and key
   SSLClient client(expiredCertFilename, expiredKeyFilename, serverCertFilename);

   // Client will successfully connect because the server has valid certificates
   // However, the server will immediately shut down the socket because the 
   // client's certificate is expired
   BOOST_REQUIRE_EQUAL(client.connectToHost(SERVER_ADDRESS, SERVER_PORT), true);

   // Client will not be able to send data to the server because the connection
   // was not established
   std::vector<uint8_t> dataBytes = {0x00, 0x01, 0x02, 0x03, 0x04};
   BOOST_REQUIRE_EQUAL(client.send(dataBytes), false);

   // Wait for the server to receive the correct data
   BOOST_REQUIRE_EQUAL(waitForBytesFromServer(5, std::chrono::milliseconds(500)), 0);

   // Disconnect the client
   BOOST_REQUIRE_EQUAL(client.disconnect(), true);

   // Shutdown the server
   server.stop();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test6_ClientUnexpectedCertificate, SSLFixture)
{
   printTestCase(current_test_case().p_name.value);

   // Create a server using the server certificate and key
   SSLServer server(SERVER_PORT, serverCertFilename, serverKeyFilename, expiredCertFilename);
   server.start();
   
   // Create a client with the server certificate and key
   SSLClient client(serverCertFilename, serverKeyFilename, serverCertFilename);

   // Client will successfully connect because the server has valid certificates
   // However, the server will immediately shut down the socket because the 
   // client's certificate is not the one it is expectign
   BOOST_REQUIRE_EQUAL(client.connectToHost(SERVER_ADDRESS, SERVER_PORT), true);

   // Client will not be able to send data to the server because the connection
   // was not established
   std::vector<uint8_t> dataBytes = {0x00, 0x01, 0x02, 0x03, 0x04};
   BOOST_REQUIRE_EQUAL(client.send(dataBytes), false);

   // Wait for the server to receive the correct data
   BOOST_REQUIRE_EQUAL(waitForBytesFromServer(5, std::chrono::milliseconds(500)), 0);

   // Disconnect the client
   BOOST_REQUIRE_EQUAL(client.disconnect(), true);

   // Shutdown the server
   server.stop();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test7_ValidConnection, SSLFixture)
{
   printTestCase(current_test_case().p_name.value);

   // Create a server using a valid certificate
   SSLServer server(SERVER_PORT, serverCertFilename, serverKeyFilename, clientCertFilename);
   server.start();

   // Create a client using a valid certificate
   SSLClient client(clientCertFilename, clientKeyFilename, serverCertFilename);

   // Client will connect because the certificates are the same and valid
   BOOST_REQUIRE_EQUAL(client.connectToHost(SERVER_ADDRESS, SERVER_PORT), true);

   // Disconnect the client
   BOOST_REQUIRE_EQUAL(client.disconnect(), true);

   // Shutdown the server
   server.stop();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test8_BidirectionalCommunication, SSLFixture)
{
   printTestCase(current_test_case().p_name.value);

   // Create a server using a valid certificate
   SSLServer server(SERVER_PORT, serverCertFilename, serverKeyFilename, clientCertFilename);
   setServerPointer(&server);
   server.subscribe(this);
   server.start();

   // Create a client using a valid certificate
   SSLClient client(clientCertFilename, clientKeyFilename, serverCertFilename);
   setClientPointer(&client);
   client.subscribe(this);

   // Client will connect because the certificate is valid
   BOOST_REQUIRE_EQUAL(client.connectToHost(SERVER_ADDRESS, SERVER_PORT), true);

   // Client will be able to send data to the server once connected
   std::vector<uint8_t> dataBytes = {0x00, 0x01, 0x02, 0x03, 0x04};
   BOOST_REQUIRE_EQUAL(client.send(dataBytes), true);

   // Wait for the server to receive the correct data
   BOOST_REQUIRE_EQUAL(waitForBytesFromServer(5, std::chrono::milliseconds(500)), 5);

   // Make sure the data received is correct
   BOOST_REQUIRE(dataBytes == serverBytes);

   // Send data from the server to the client
   std::vector<uint8_t> data2Bytes = {0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
   server.sendBroadcast(data2Bytes);

   // Wait for the client to receive the correct data
   BOOST_REQUIRE_EQUAL(waitForBytesFromClient(6, std::chrono::milliseconds(500)), 6);

   // Make sure the data received is correct
   BOOST_REQUIRE(data2Bytes == clientBytes);

   // Disconnect the client
   BOOST_REQUIRE_EQUAL(client.disconnect(), true);

   // Shutdown the server
   server.stop();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test9_ServerDetectsClientShutdown, SSLFixture)
{
   printTestCase(current_test_case().p_name.value);

   // Create a server using a valid certificate
   SSLServer server(SERVER_PORT, serverCertFilename, serverKeyFilename, clientCertFilename);
   setServerPointer(&server);
   server.subscribe(this);
   server.start();

   // Create a client using a valid certificate
   SSLClient client(clientCertFilename, clientKeyFilename, serverCertFilename);

   // Client will connect because the certificate is valid
   BOOST_REQUIRE_EQUAL(client.connectToHost(SERVER_ADDRESS, SERVER_PORT), true);

   // Client will be able to send data to the server once connected
   std::vector<uint8_t> dataBytes = {0x00, 0x01, 0x02, 0x03, 0x04};
   BOOST_REQUIRE_EQUAL(client.send(dataBytes), true);

   // Wait for the server to receive the correct data
   BOOST_REQUIRE_EQUAL(waitForBytesFromServer(5, std::chrono::milliseconds(500)), 5);

   // Make sure the data received is correct
   BOOST_REQUIRE(dataBytes == serverBytes);

   // Disconnect the client
   BOOST_REQUIRE_EQUAL(client.disconnect(), true);

   // Client will not be able to send the data to the server anymore
   BOOST_REQUIRE_EQUAL(client.send(dataBytes), false);

   // Create another client and attempt to connect
   SSLClient client2(clientCertFilename, clientKeyFilename, serverCertFilename);
   BOOST_REQUIRE_EQUAL(client2.connectToHost(SERVER_ADDRESS, SERVER_PORT), true);

   // Client will be able to send data to the server once connected
   std::vector<uint8_t> data2Bytes = {0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
   serverBytes.clear();
   BOOST_REQUIRE_EQUAL(client2.send(data2Bytes), true);

   // Wait for the server to receive the correct data
   BOOST_REQUIRE_EQUAL(waitForBytesFromServer(6, std::chrono::milliseconds(500)), 6);

   // Make sure the data received is correct
   BOOST_REQUIRE(data2Bytes == serverBytes);

   // Disconnect the client
   BOOST_REQUIRE_EQUAL(client2.disconnect(), true);

   // Client will not be able to send the data to the server anymore
   BOOST_REQUIRE_EQUAL(client2.send(data2Bytes), false);

   // Shutdown the server
   server.stop();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test10_ClientDetectsServerShutdown, SSLFixture)
{
   printTestCase(current_test_case().p_name.value);

   // Create a server using a valid certificate
   SSLServer server(SERVER_PORT, serverCertFilename, serverKeyFilename, clientCertFilename);
   server.subscribe(this);
   setServerPointer(&server);
   server.start();

   // Create a client using a valid certificate
   SSLClient client(clientCertFilename, clientKeyFilename, serverCertFilename);

   // Client will connect because the certificate is valid
   BOOST_REQUIRE_EQUAL(client.connectToHost(SERVER_ADDRESS, SERVER_PORT), true);

   // Client will be able to send data to the server once connected
   std::vector<uint8_t> dataBytes = {0x00, 0x01, 0x02, 0x03, 0x04};
   BOOST_REQUIRE_EQUAL(client.send(dataBytes), true);

   // Wait for the server to receive the correct data
   BOOST_REQUIRE_EQUAL(waitForBytesFromServer(5, std::chrono::milliseconds(500)), 5);

   // Make sure the data received is correct
   BOOST_REQUIRE(dataBytes == serverBytes);

   // Stop the Server
   server.stop();

   // Give the server some time to shutdown
   std::this_thread::sleep_for(std::chrono::milliseconds(300));

   // Attempt to write with the client
   // The first attempt will succeed because ...
   serverBytes.clear();
   BOOST_REQUIRE_EQUAL(client.send(dataBytes), false);

   // The server should never publish that it received those bytes
   BOOST_REQUIRE_EQUAL(waitForBytesFromServer(5, std::chrono::milliseconds(500)), 0);

   // Disconnect the client
   BOOST_REQUIRE_EQUAL(client.disconnect(), true);

   // Attempt to shutdown the server again to make sure this call 
   // does not cause any problems
   server.stop();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
