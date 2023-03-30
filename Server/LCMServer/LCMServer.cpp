#include "LCMServer.h"
#include "Debug.h"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sstream>
#include <string.h>
#include "rflc_scenes.h"

// Include the C headers from the embedded code
extern "C" {
#include "includes.h"
}

using namespace legrand::rflc::scenes;

LCMServer::LCMServer(const int &listenPort, std::shared_ptr<std::mutex> mutexPtr) :
   port(listenPort),
   socketDesc(-1),
   priorityMutexPtr(mutexPtr)
{
   logVerbose("Enter");

   client = new AvahiClient();

   // Initialize the zone list properties list
   InitializeZoneArray();

   // Initialize the scene properties list
   InitializeSceneArray();

   // Initialize the scene controller list
   InitializeSceneControllerArray();

   // Initialize the system properties
   InitializeSystemProperties();

   // Load zones from flash
   int numZonesLoaded = LoadZonesFromFlash();

   // Load scenes from flash
   int numScenesLoaded = LoadScenesFromFlash();

   // Load scene controllers from flash
   int numSceneControllersLoaded = LoadSceneControllersFromFlash();

   logInfo("Loaded %d Zones, %d Scenes, and %d Scene Controllers", numZonesLoaded, numScenesLoaded, numSceneControllersLoaded);

   if(numZonesLoaded > 0)
   {
      // Set the system property to configured if any zones exist
      systemProperties.configured = true;
   }

   // Give the thread some time to actually start before returning
   std::this_thread::sleep_for(std::chrono::milliseconds(20));

   logVerbose("Exit");
}

LCMServer::~LCMServer()
{
   logVerbose("Enter");

   if(client)
   {
      delete(client);
      client = NULL;
   }

   stopLCMServer();
   
   logVerbose("Exit");
}

void LCMServer::startLCMServer()
{
   logVerbose("Enter");

   serverThread = std::move(std::thread(&LCMServer::runServerThread, this));

   // Give the thread some time to actually start before returning
   std::this_thread::sleep_for(std::chrono::milliseconds(20));
   
   logVerbose("Exit");
}

void LCMServer::stopLCMServer()
{
   logVerbose("Enter");

   logInfo("Waiting for the LCM Server to stop %d", socketDesc);

   // Close the socket
   if(socketDesc != -1)
   {
      logDebug("Shutdown Socket");
      int rv = shutdown(socketDesc, SHUT_RDWR);

      logDebug("Shutdown returned %d. Closing Socket", rv);
      close(socketDesc);
      logDebug("Close Finished");
   }

   // Wait for all of the clients to return
   logDebug("Waiting for Clients to finish");
   for(size_t i = 0; i < clientSocketHandlers.size(); i++)
   {
      // Stop the receiving thread
      clientSocketHandlers[i]->stopReceiving();
      clientSocketHandlers[i]->stopPinging();
      delete clientSocketHandlers[i];
   }
   clientSocketHandlers.clear();

   // Join the server thread
   if(serverThread.joinable())
   {
      serverThread.join();
   }
   
   logInfo("LCM Server Exiting");
   logVerbose("Exit");
}

void LCMServer::runServerThread()
{
   logVerbose("Enter");

   logDebug("Running Server Thread!");
   int clientSock;
   int c;
   struct sockaddr_in server;
   struct sockaddr_in client;

   // Create the socket
   socketDesc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   if(socketDesc == -1)
   {
      logError("Could not create socket");
   }
   else
   {
      logDebug("Socket created %d", socketDesc);

      // Set the Socket Option to allow reuse of local addresses
      int optionValue = 1;
      if(setsockopt(socketDesc, SOL_SOCKET, SO_REUSEADDR, (char*)&optionValue, sizeof(optionValue)) < 0)
      {
         logError("Error setting SO_REUSEADDR: %s", strerror(errno));
      }

      // Prepare the sockaddr_in structure
      server.sin_family = AF_INET;
      server.sin_addr.s_addr = INADDR_ANY;
      server.sin_port = htons(port);

      // Attempt to bind to this socket
      int bindReturn = -1;
      while(bindReturn < 0)
      {
         bindReturn = bind(socketDesc, (sockaddr *)&server, sizeof(server));

         if(bindReturn < 0)
         {
            logError("Bind Failed: %s", strerror(errno));
            logInfo("Sleep for a second and try again");
            std::this_thread::sleep_for(std::chrono::seconds(1));
         }
      }

      logDebug("Bind Complete");

      // Listen
      listen(socketDesc, 3);

      // Accept any incoming connection
      c = sizeof(struct sockaddr_in);

      logDebug("Waiting for socket accept");
      while( (clientSock = accept(socketDesc, (sockaddr*)&client, 
                  (socklen_t*)&c)) > 0)
      {
         logDebug("Socket connected. Starting Client Handler");
         LCMClientSocketHandler *clientSocketHandler = 
            new LCMClientSocketHandler(priorityMutexPtr, clientSock, this);
         clientSocketHandler->startReceiving();
         clientSocketHandler->startPinging(std::chrono::seconds(5));

         {
            logVerbose("Locking Client Mutex");
            std::lock_guard<std::mutex> lock(clientHandlerMutex);
            clientSocketHandlers.push_back(clientSocketHandler);
            logVerbose("Unlocking Client Mutex");
         }
      }

      logDebug("Accept returned -1");
   }
   
   logVerbose("Exit");
}

void LCMServer::sendJSONBroadcast(json_t *jsonBroadcastObject)
{
   logVerbose("Enter");

   if(jsonBroadcastObject)
   {
      // Iterate through the clients and send the json packet
      logVerbose("Locking Client Mutex");
      std::lock_guard<std::mutex> lock(clientHandlerMutex);
      for(auto iter = clientSocketHandlers.begin(); iter != clientSocketHandlers.end();)
      {
         if(!(*iter)->SendJSONPacket(jsonBroadcastObject))
         {
            logError("Failed to send the broadcast packet to the client");

            // Failed to send. Remove the socket from the list
            (*iter)->stopReceiving();
            (*iter)->stopPinging();
            delete *iter;

            // Remove the pointer from the list of client handlers so we don't
            // try to send any packets to them again. This will return the 
            // iterator of the next object
            iter = clientSocketHandlers.erase(iter);
         }
         else
         {
            // Send succeeded. Increment the iterator
            ++iter;
         }
      }
      logVerbose("Unlocking Client Mutex");
   }
   
   logVerbose("Exit");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
LCMServer::LCMClientSocketHandler::LCMClientSocketHandler(std::shared_ptr<std::mutex> mutexPtr, const int &handle, LCMServer *serverPtr)
   : socketHandle(handle), server(serverPtr), pinging(false), priorityMutexPtr(mutexPtr)
{
   logVerbose("Enter");

   logVerbose("Exit");
}

LCMServer::LCMClientSocketHandler::~LCMClientSocketHandler()
{
   logVerbose("Enter");

   stopReceiving();

   // Join the receiving thread
   if(receiveThread.joinable())
   {
      receiveThread.join();
   }
   
   logVerbose("Exit");
}

void LCMServer::LCMClientSocketHandler::setSocketHandle(const int &handle)
{
   logVerbose("Enter");

   socketHandle = handle;
   logVerbose("Exit");
}

void LCMServer::LCMClientSocketHandler::startReceiving()
{
   logVerbose("Enter");

   // Start thread to receive data
   logDebug("Start Receiving");
   receiveThread = 
      std::move(std::thread(&LCMServer::LCMClientSocketHandler::runReceiveThread, this));
   
   logVerbose("Exit");
}

void LCMServer::LCMClientSocketHandler::stopReceiving()
{
   logVerbose("Enter");

   // Shutdown and close socket
   if(socketHandle != -1)
   {
      shutdown(socketHandle, SHUT_RDWR);
      close(socketHandle);
      socketHandle = -1;
   }
   
   logVerbose("Exit");
}

void LCMServer::LCMClientSocketHandler::startPinging(const std::chrono::seconds &dwellTimeSeconds)
{
   logVerbose("Enter");

   // Start thread to send Pins
   logDebug("Start Pinging");
   pinging = true;
   pingThread = 
      std::move(std::thread(&LCMServer::LCMClientSocketHandler::runPingThread, this,
               dwellTimeSeconds));
   
   logVerbose("Exit");
}

void LCMServer::LCMClientSocketHandler::stopPinging()
{
   logVerbose("Enter");

   if(pinging && pingThread.joinable())
   {
      // Set the ping flag to false
      pinging = false;

      // Notify the condition variable
      pingCondition.notify_all();

      // Join the thread
      pingThread.join();
   }
   
   logVerbose("Exit");
}

void LCMServer::LCMClientSocketHandler::runReceiveThread()
{
   logVerbose("Enter");

   char dataByte[1];
   int readSize = 0;

   logDebug("Waiting for Bytes");
   if(socketHandle < 0)
   {
      logError("Socket Handle is invalid");
   }
   else
   {
      std::string jsonPacket;
      while((readSize = recv(socketHandle, dataByte, 1, 0)) > 0)
      {
         logVerbose("Received byte from socket %c", dataByte[0]);

         // Handle byte
         if(dataByte[0] == 0x00)
         {
            std::lock_guard<std::mutex> globalLock(*priorityMutexPtr);

            // Null received. Signals the end of a json message string
            // Handle the json string 
            handleJsonString(jsonPacket);

            // Reset the packet buffer
            jsonPacket.clear();
         }
         else
         {
            // Add the byte to the end of the Json packet
            jsonPacket.push_back(dataByte[0]);
         }

         logVerbose("Waiting for next byte");
      }

      if(readSize == 0)
      {
         logDebug("Client Disconnected");
      }
      else if(readSize < 0)
      {
         logError("Receive Failed");
      }
   }

   // Stop the ping thread
   stopPinging();

   // Stop receiving and shut down the socket
   stopReceiving();

   logDebug("Receive Thread Exiting");
   logVerbose("Exit");
}


void LCMServer::LCMClientSocketHandler::runPingThread(const std::chrono::seconds &dwellTimeSeconds)
{
   logVerbose("Enter");

   logInfo("Running the ping thread");

   while(pinging)
   {
      bool conditionMet = false;
      {
         logDebug("Waiting for condition. Locking Ping Mutex");
         std::unique_lock<std::mutex> lock(pingMutex);
         conditionMet = pingCondition.wait_for(lock, dwellTimeSeconds, [this](){return (pinging == false || messageSent == true);});
         logDebug("Condition exited %d. Unlocking ping mutex", conditionMet);
      }

      if(conditionMet)
      {
         if(pinging == false)
         {
            logInfo("Stop the ping thread");
         }
         else if(messageSent)
         {
            messageSent = false;
            logDebug("Reset ping timer");
         }
      }
      else
      {
         logDebug("Timed out. Send a ping");
         sendPing();
      }
   }

   logInfo("Ping Thread Exiting");
   logVerbose("Exit");
}


bool LCMServer::LCMClientSocketHandler::sendPing()
{
   logVerbose("Enter");

   bool pingSent = false;

   json_t *pingObject = json_object();

   if(pingObject)
   {
      json_object_set_new(pingObject, "ID", json_integer(0));
      json_object_set_new(pingObject, "Service", json_string("ping"));
      json_object_set_new(pingObject, "Status", json_string("Success"));
      logDebug("Sending Ping");
      pingSent = SendJSONPacket(pingObject, false);

      // Free the Json memory after sending
      json_decref(pingObject);
   }
   else
   {
      logError("Ping object is invalid");
   }

   logVerbose("Exit");
   return pingSent;
}


bool LCMServer::LCMClientSocketHandler::SendJSONPacket(json_t *jsonObject,
      bool notifyPing)
{
   logVerbose("Enter");

   bool jsonSent = false;

   if(socketHandle < 0)
   {
      logError("Socket handle is invalid");
   }
   else if(jsonObject)
   {
      char *localString = json_dumps(jsonObject, 
            (JSON_COMPACT | JSON_PRESERVE_ORDER));
      int messageLen = strlen((const char *)localString) + 1;

      logInfo("Sending: %s", localString);
      int bytesWritten = write(socketHandle, localString, messageLen);

      // Free the string memory after sending
      free(localString);

      if(bytesWritten == -1)
      {
         logError("Write Failed (%s)", strerror(errno));
      }
      else
      {
         logDebug("%d bytes out of %d written successfully", bytesWritten,
               messageLen);
         jsonSent = true;

         if(notifyPing)
         {
            // Notify the ping thread that a message was sent
            messageSent = true;
            logDebug("Notifying ping thread");
            pingCondition.notify_one();
         }
      }
   }

   logVerbose("Exit");
   return jsonSent;
}


void LCMServer::LCMClientSocketHandler::handleJsonString(const std::string &jsonString)
{
   logVerbose("Enter");

   logInfo("Handling Json string %s", jsonString.c_str());

   // Convert the string to a Json object
   json_error_t jsonError;
   json_t *jsonResponseObject = NULL;
   json_t *root = NULL;
   json_t *jsonBroadcastObject = NULL;
  
   if(jsonString.length() > SOCKET_RECEIVE_BUFFER_MAX_SIZE)
   {
      jsonResponseObject = json_object();
      std::stringstream jsonErrorString;
      jsonErrorString << "JSON packet exceeds max buffer size of %d."
         << SOCKET_RECEIVE_BUFFER_MAX_SIZE;
      BuildErrorResponseNoID(jsonResponseObject, jsonErrorString.str().c_str(), USR_JSON_PACKET_EXCEEDS_MAX_BUFFER_SIZE);
   }
   else
   {
      root = json_loads((const char *)jsonString.c_str(), 0, &jsonError);
      if(!root)
      {
         // Build an error response with the json error string to send back
         jsonResponseObject = BuildErrorResponseBadJsonFormat(&jsonError);
      }
      else
      {
         logInfo("Processing JSON Packet");
         jsonResponseObject = ProcessJsonPacket(root, &jsonBroadcastObject);
      }
   }

   // Send the responses
   if(jsonResponseObject)
   {
      logInfo("Sending Response Object");
      json_t * jsonResponseStatusObject = json_object_get(jsonResponseObject,
            "Status");
      if(!jsonResponseStatusObject)
      {
         // Add status success to the object
         json_object_set_new(jsonResponseObject, "Status",
               json_string("Success"));
      }

      // Send the response
      SendJSONPacket(jsonResponseObject);

      // Send the broadcast response if it exists
      if(jsonBroadcastObject)
      {
         // Send the broadcast packet to the server to forward to each client
         if(server)
         {
            logInfo("Sending Broadcast Object");
            server->sendJSONBroadcast(jsonBroadcastObject);
         }
      }
   }

   // Clean up memory
   if(root)
   {
      json_decref(root);
      root = NULL;
   }

   if(jsonResponseObject)
   {
      json_decref(jsonResponseObject);
      jsonResponseObject = NULL;
   }

   if(jsonBroadcastObject)
   {
      json_decref(jsonBroadcastObject);
      jsonBroadcastObject = NULL;
   }

   logVerbose("Exit");
}
