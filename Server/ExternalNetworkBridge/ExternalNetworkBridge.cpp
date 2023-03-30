#include "ExternalNetworkBridge.h"
#include "LCMSocketHandler.h"
#include "Debug.h"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sstream>
#include <string.h>
#include <algorithm>

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

ExternalNetworkBridge::ExternalNetworkBridge(const int &APPPort, const int &LCMPort, const int &RegistrationPort, const std::string &databaseFilename, const std::string &inLocalCertificateFilename, const std::string &inLocalKeyFilename, const std::string &inRemoteCertificateFilename) :
   appPort(APPPort),
   lcmPort(LCMPort),
   registrationPort(RegistrationPort),
   db(NULL),
   appSocket(-1),
   lcmSocket(-1),
   cleanupThreadRunning(false),
   localCertificateFilename(inLocalCertificateFilename),
   localKeyFilename(inLocalKeyFilename),
   remoteCertificateFilename(inRemoteCertificateFilename)
{
   logVerbose("Enter");

   // Open the database file
   int result = sqlite3_open_v2(databaseFilename.c_str(), &db, SQLITE_OPEN_READWRITE, NULL);
   if(result != SQLITE_OK)
   {
      logError("Error opening the database (%d)", result);
      db = NULL;
   }
   else
   {
      // Update the database to say that all LCM devices are disconnected
      // This will get updated once the devices actually connect
      char *errorString;
      result = sqlite3_exec(db, "update devices set connected=0", NULL, NULL, &errorString);
      if(result != SQLITE_OK)
      {
         logError("Error setting all devices to not connected: (%s)", errorString);
         sqlite3_free(errorString);
      }
   }

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

ExternalNetworkBridge::ExternalNetworkBridge(const int &APPPort, const int &LCMPort, const int &RegistrationPort, const std::string &databaseFilename) :
   ExternalNetworkBridge(APPPort, LCMPort, RegistrationPort, databaseFilename, "", "", "")
{
   logVerbose("Enter");
   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

ExternalNetworkBridge::~ExternalNetworkBridge()
{
   logVerbose("Enter");

   stop();
   
   // Update the database to say that all LCM devices are disconnected
   if(db)
   {
      char *errorString;
      int result = sqlite3_exec(db, "update devices set connected=0", NULL, NULL, &errorString);
      if(result != SQLITE_OK)
      {
         logError("Error setting all devices to not connected: (%s)", errorString);
         sqlite3_free(errorString);
      }

      // Close the database and set the pointer to NULL
      result = sqlite3_close(db);
      if(result != SQLITE_OK)
      {
         logError("Error closing the database");
      }

      db = NULL;
   }

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ExternalNetworkBridge::start()
{
   logVerbose("Enter");

   // Start the cleanup thread
   cleanupThreadRunning = true;
   cleanupThread = std::move(std::thread(&ExternalNetworkBridge::runCleanupThread, this));

   // Start the registration listener thread
   registrationThread = std::move(std::thread(&ExternalNetworkBridge::runRegistrationThread, this));

   // Start the app listener thread
   appThread = std::move(std::thread(&ExternalNetworkBridge::runAppThread, this));

   // Start the LCM listener thread
   lcmThread = std::move(std::thread(&ExternalNetworkBridge::runLCMThread, this));

   // Give the threads some time to actually start before returning
   std::this_thread::sleep_for(std::chrono::milliseconds(200));
   
   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ExternalNetworkBridge::stop()
{
   logVerbose("Enter");

   logDebug("Waiting for the threads to stop %d, %d", appSocket, lcmSocket);

   // Close the app socket
   if(appSocket != -1)
   {
      logDebug("Shutdown App Socket");
      int rv = shutdown(appSocket, SHUT_RDWR);

      logDebug("Shutdown returned %d. Closing Socket", rv);
      close(appSocket);
      logDebug("Close Finished");
      appSocket = -1;
   }

   // Close the LCM socket
   if(lcmSocket != -1)
   {
      logDebug("Shutdown LCM Socket");
      int rv = shutdown(lcmSocket, SHUT_RDWR);

      logDebug("Shutdown return %d. Closing Socket", rv);
      close(lcmSocket);
      logDebug("Close Finished");
      lcmSocket = -1;
   }

   // Close the registration socket
   if(registrationSocket != -1)
   {
      logDebug("Shutdown Registration Socket");
      int rv = shutdown(registrationSocket, SHUT_RDWR);

      logDebug("Shutdown return %d. Closing Socket", rv);
      close(registrationSocket);
      logDebug("Close Finished");
      registrationSocket = -1;
   }

   // Join the cleanup thread
   if(cleanupThread.joinable())
   {
      cleanupThreadRunning = false;
      cleanupThreadCondition.notify_all();
      cleanupThread.join();
   }

   // Clear all of the handler maps
   logInfo("Clear all of the handler maps");
   bridgeMap.clear();
   lcmHandlers.clear();
   appHandlers.clear();
   registrationHandlers.clear();

   // Join the app thread
   if(appThread.joinable())
   {
      appThread.join();
   }
   
   // Join the lcm thread
   if(lcmThread.joinable())
   {
      lcmThread.join();
   }

   // Join the registration thread
   if(registrationThread.joinable())
   {
      registrationThread.join();
   }

   logDebug("External Network Bridge Exiting");
   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ExternalNetworkBridge::printLCMTable()
{
   logVerbose("Enter");

   std::cout << std::endl << "Count\tLCM ID           \tNumber of Apps Connected" << std::endl;
   std::cout << "========================================================" << std::endl;

   // Limit access to the bridge map to one thread at a time
   std::lock_guard<std::mutex> lock(bridgeMapMutex);

   int deviceIndex = 1;
   for(auto it = bridgeMap.begin(); it != bridgeMap.end(); it++, deviceIndex++)
   {
      std::cout << deviceIndex << "\t" 
                << it->second.lcmSocket->getDeviceIdStr() << "\t"
                << it->second.appSockets.size() << std::endl;
   }

   std::cout << "========================================================" << std::endl << std::endl;

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ExternalNetworkBridge::printAppTable()
{
   logVerbose("Enter");

   std::cout << std::endl << "Count\tApp ID\tApp IP         \tLCM ID" << std::endl;
   std::cout << "==================================================" << std::endl;

   // Limit access to the bridge map to one thread at a time
   std::lock_guard<std::mutex> lock(bridgeMapMutex);

   int appIndex = 1;
   for(auto it = bridgeMap.begin(); it != bridgeMap.end(); it++)
   {
      for(auto appIt = it->second.appSockets.begin(); appIt != it->second.appSockets.end(); appIt++, appIndex++)
      {
         std::cout << appIndex << "\t"
                   << (*appIt)->getDeviceId() << "\t"
                   << (*appIt)->getClientIPAddressString() << "\t"
                   << it->second.lcmSocket->getDeviceIdStr() << std::endl;
      }
   }

   std::cout << "==================================================" << std::endl << std::endl;

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ExternalNetworkBridge::update(Publisher *who, std::shared_ptr<PublishObject> obj)
{
   logVerbose("Enter");

   auto registrationData = std::dynamic_pointer_cast<RegistrationData>(obj);
   auto lcmHandlerData = std::dynamic_pointer_cast<LCMHandlerData>(obj);
   auto appHandlerData = std::dynamic_pointer_cast<AppHandlerData>(obj);

   if(registrationData)
   {
      uint64_t appId = registrationData->getAppId();
      uint64_t lcmId = registrationData->getLCMId();
      int handlerId = registrationData->getHandlerId();
      logDebug("Registration data received for handler (%d) App(%lu) -> LCM(%lu)", handlerId, static_cast<long unsigned int>(appId), static_cast<long unsigned int>(lcmId));

      // Store the data in the database
      if(db)
      {
         // Add the LCM to the database with the current settings
         // If the device does not exist in the database it will get added with
         // uid=0, name=LC7001, and a connection status of 0
         std::stringstream query;
         query << "insert or replace into devices(id, uid, name, connected) "
            << "values(" 
            << lcmId << ","
            << "coalesce((select uid from devices where id=" << lcmId << "), 0), "
            << "coalesce((select name from devices where id=" << lcmId << "), \"LC7001\"), "
            << "coalesce((select connected from devices where id=" << lcmId << "), 0))";

         char *errorString;
         int result = sqlite3_exec(db, query.str().c_str(), NULL, NULL, &errorString);
         if(result != SQLITE_OK)
         {
            logError("Error inserting LCM ID to database (%s)", errorString);
         }
         sqlite3_free(errorString);

         // Add the app ID to the database with this mapping
         // If the app does not exist in the database it will get added with uid=0
         query.str("");
         query.clear();

         query << "insert or replace into apps(id, uid, deviceId) "
            << "values("
            << appId << ","
            << "coalesce((select uid from apps where id=" << appId << "), 0), "
            << lcmId << ")";
         result = sqlite3_exec(db, query.str().c_str(), NULL, NULL, &errorString);
         if(result != SQLITE_OK)
         {
            logError("Error inserting app mapping to database (%s)", errorString);
         }
         sqlite3_free(errorString);
      }
      else
      {
         logInfo("Database pointer is null. Not updating the database with connection status");
      }

      // Disconnect the handler socket so it gets cleaned up
      std::lock_guard<std::mutex> lock(registrationHandlerMutex);
      auto iter = registrationHandlers.find(handlerId);
      if(iter != registrationHandlers.end())
      {
         logInfo("Disconnecting registration handler");
         iter->second->stopReceiving();
      }
   }
   else if(lcmHandlerData)
   {
      logDebug("LCM handler data received");

      // Get the socket handler from the handle ID
      std::lock_guard<std::mutex> lcmLock(lcmHandlerMutex);
      auto iter = lcmHandlers.find(lcmHandlerData->getHandlerId());

      // Get the device ID from the data
      uint64_t lcmDeviceId = lcmHandlerData->getDeviceId();

      if(iter == lcmHandlers.end())
      {
         logError("LCM Handler not in map");
      }
      else if(lcmDeviceId == 0)
      {
         logError("LCM Device ID is invalid. Not adding to the database");
         iter->second->stopReceiving();
      }
      else
      {
         {
            // Add the handler to the bridge map indexed by the device ID
            socketHandlerStruct handlerStruct;
            handlerStruct.lcmSocket = iter->second;

            // Remove the handler from the list because it was added to the bridge map
            logInfo("Removing from the handler map before added to bridge map");
            lcmHandlers.erase(iter);

            // Add the handler to the bridge map
            std::lock_guard<std::mutex> lock(bridgeMapMutex);
            bridgeMap[lcmDeviceId] = handlerStruct;
         }

         if(db)
         {
            // Update the database setting the connection status to 1
            // If the device does not exist in the database it will get added with
            // uid=0 and name=LC7001
            std::stringstream query;
            query << "insert or replace into devices(id, uid, name, connected) "
               << "values(" 
               << lcmDeviceId << ","
               << "coalesce((select uid from devices where id=" << lcmDeviceId << "), 0), "
               << "coalesce((select name from devices where id=" << lcmDeviceId<< "), \"LC7001\"), "
               << "1)";

            char *errorString;
            int result = sqlite3_exec(db, query.str().c_str(), NULL, NULL, &errorString);
            if(result != SQLITE_OK)
            {
               logError("Error inserting connection status to database (%s)", errorString);
            }
            sqlite3_free(errorString);
         }
         else
         {
            logInfo("Database pointer is null. Not updating the database with connection status");
         }
      }
   }
   else if(appHandlerData)
   {
      logDebug("App handler data received");

      uint64_t appDeviceId = appHandlerData->getDeviceId();

      // Get the LCM ID mapping from the database for this app
      uint64_t lcmDeviceIdFromDB = 0;

      if(db)
      {
         // Get the device id for the appId from the database
         std::stringstream query;
         query << "select * from apps where id=" << appDeviceId;

         sqlite3_stmt *statement;
         int prepareRv = sqlite3_prepare_v2(db, query.str().c_str(), -1, &statement, 0);

         if(prepareRv == SQLITE_OK)
         {
            int numColumns = sqlite3_column_count(statement);
            int result = 0;

            result = sqlite3_step(statement);

            if(result == SQLITE_ROW)
            {
               for(int columnIndex = 0; columnIndex < numColumns; columnIndex++)
               {
                  if(!strcmp(sqlite3_column_name(statement, columnIndex), "deviceId"))
                  {
                     lcmDeviceIdFromDB = static_cast<uint64_t>(sqlite3_column_int64(statement, columnIndex));
                     logInfo("Device ID received %lu", static_cast<long unsigned int>(lcmDeviceIdFromDB));
                  }
               }
            }
            else
            {
               logError("Could not get the LCM ID from the database: %s", sqlite3_errstr(result));
            }
         }

         sqlite3_finalize(statement);
      }

      // Get the socket handler from the handle ID
      std::lock_guard<std::mutex> lock(appHandlerMutex);
      auto iter = appHandlers.find(appHandlerData->getHandlerId());

      // Limit access to the bridge map to one thread at a time
      std::lock_guard<std::mutex> bridgeLock(bridgeMapMutex);

      if(iter == appHandlers.end())
      {
         logError("App Handler not found");
      }
      else if(lcmDeviceIdFromDB == 0)
      {
         logError("LCM Id not found in the database for appId(%lu)", static_cast<long unsigned int>(appDeviceId));
         iter->second->stopReceiving();
      }
      else if(bridgeMap.size() == 0)
      {
         logError("No LCMs connected");
         iter->second->stopReceiving();
      }
      else
      {
         // Determine if the LCM is connected and in the bridge map
         auto connection = bridgeMap.find(lcmDeviceIdFromDB);

         if(connection != bridgeMap.end())
         {
            logDebug("Connection is valid %lu", static_cast<unsigned long int>(connection->second.lcmSocket->getDeviceId()));

            // Remove the handler from this list because it is about to be
            // added to the bridge map
            logDebug("Removing from the handler map before added to bridge map");
            auto appHandler = iter->second;
            appHandlers.erase(iter);

            SSL *lcmSSLHandle = connection->second.lcmSocket->getClientSSLHandle();
            if(lcmSSLHandle)
            {
               appHandler->addBridgedSocketHandle(lcmSSLHandle);
            }
            else
            {
               appHandler->addBridgedSocketHandle(connection->second.lcmSocket->getClientSocketHandle());
            }

            logInfo("Adding New App %lu to map", static_cast<unsigned long int>(appDeviceId));
            connection->second.appSockets.push_back(appHandler);

            SSL *appSSLHandle = appHandler->getClientSSLHandle();
            if(appSSLHandle)
            {
               connection->second.lcmSocket->addBridgedSocketHandle(appSSLHandle);
            }
            else
            {
               connection->second.lcmSocket->addBridgedSocketHandle(appHandler->getClientSocketHandle());
            }

            logInfo("BridgeMap size = %d, %d", static_cast<int>(bridgeMap.size()), static_cast<int>(connection->second.appSockets.size()));
         }
         else
         {
            logError("LCM (%lu) not connected for app (%lu)", static_cast<long unsigned int>(lcmDeviceIdFromDB), static_cast<long unsigned int>(appDeviceId));
            iter->second->stopReceiving();
         }
      }
   }

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ExternalNetworkBridge::runCleanupThread()
{
   while(cleanupThreadRunning)
   {
      std::unique_lock<std::mutex> lock(cleanupThreadMutex);
      if(cleanupThreadCondition.wait_for(lock, std::chrono::milliseconds(100)) == std::cv_status::timeout)
      {
         // Timer expired 

         {
            // Cleanup any registration handlers that are disconnected
            std::lock_guard<std::mutex> handlerLock(registrationHandlerMutex);

            for(auto iter = registrationHandlers.begin(); iter != registrationHandlers.end();)
            {
               if(!iter->second->isConnected())
               {
                  logInfo("Registration socket disconnected. Cleaning up handler");
                  iter = registrationHandlers.erase(iter);
               }
               else
               {
                  iter++;
               }
            }
         }

         {
            // Cleanup any app sockets that are disconnected but not yet bridged
            std::lock_guard<std::mutex> lock(appHandlerMutex);

            for(auto iter = appHandlers.begin(); iter != appHandlers.end();)
            {
               if(!iter->second->isConnected())
               {
                  logInfo("AppSocket Disconnected. Cleaning up handler");
                  iter = appHandlers.erase(iter);
               }
               else
               {
                  iter++;
               }
            }
         }

         {
            // Cleanup any lcm sockets that are disconnected but not yet bridged
            std::lock_guard<std::mutex> lock(lcmHandlerMutex);

            for(auto iter = lcmHandlers.begin(); iter != lcmHandlers.end();)
            {
               if(!iter->second->isConnected())
               {
                  logInfo("LCM Socket Disconnected. Cleaning up handler");
                  iter = lcmHandlers.erase(iter);
               }
               else
               {
                  iter++;
               }
            }
         }

         // Clean up any bridged sockets
         {
            std::lock_guard<std::mutex> lock(bridgeMapMutex);

            for(auto iter = bridgeMap.begin(); iter != bridgeMap.end();)
            {
               if(!iter->second.lcmSocket->isConnected())
               {
                  // Update the database setting the connection status to for the LCM
                  uint64_t lcmDeviceId = iter->second.lcmSocket->getDeviceId();

                  // The LCM handler is disconnected. Remove it from the map
                  // Doing so will also remove all of the app handlers as well
                  logInfo("LCMSocket Disconnected. Cleaning up Map");
                  iter = bridgeMap.erase(iter);

                  if(db)
                  {
                     std::stringstream query;
                     query << "update devices set connected=0 where id=" << lcmDeviceId;
                     char *errorString;
                     int result = sqlite3_exec(db, query.str().c_str(), NULL, NULL, &errorString);
                     if(result != SQLITE_OK)
                     {
                        logError("Error updating connection status in database (%s)", errorString);
                     }
                     else if(sqlite3_changes(db) == 0)
                     {
                        logError("Did not update connection status because device was not in database");
                     }
                     else
                     {
                        logDebug("Successfully updated connection status to disconnected");
                     }

                     logInfo("Free String");
                     sqlite3_free(errorString);
                  }
               }
               else
               {
                  // The LCM handler is still connected. Iterate through all of
                  // the app handlers and remove any that are disconnected
                  for(auto appIt = iter->second.appSockets.begin(); appIt != iter->second.appSockets.end();)
                  {
                     if(!(*appIt)->isConnected())
                     {
                        // App handler is disconnected. Remove it.
                        logInfo("AppSocket Disconnected. Cleaning up Map");
                        appIt = iter->second.appSockets.erase(appIt);
                     }
                     else
                     {
                        appIt++;
                     }
                  }

                  // Increment the iterator for the next loop
                  iter++;
               }
            }
         }
      }
   }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ExternalNetworkBridge::runRegistrationThread()
{
   logVerbose("Enter");

   // Create the socket
   registrationSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   if(registrationSocket == -1)
   {
      logError("Could not create the registration socket");
   }
   else
   {
      logDebug("Socket Created %d", registrationSocket);

      // Set the socket option to allow reuse of the local address
      int optionValue = 1;
      if(setsockopt(registrationSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optionValue, sizeof(optionValue)) < 0)
      {
         logError("Error setting SO_REUSEADDR: %s", strerror(errno));
      }

      // Prepare the sockaddr_in structure
      struct sockaddr_in registrationAddr;
      registrationAddr.sin_family = AF_INET;
      registrationAddr.sin_addr.s_addr = INADDR_ANY;
      registrationAddr.sin_port = htons(registrationPort);

      // Attempt to bind to the socket
      int bindReturn = -1;
      while(bindReturn < 0)
      {
         bindReturn = bind(registrationSocket, (sockaddr *)&registrationAddr, sizeof(registrationAddr));

         if(bindReturn < 0)
         {
            logError("Bind Failed: %s", strerror(errno));
            logInfo("Sleep for a second and try again");
            std::this_thread::sleep_for(std::chrono::seconds(1));
         }
      }

      logDebug("Bind Complete");

      // Listen for up to 10 pending connections before accept needs to be 
      // called. If a connection request arrives when the queue is full, the 
      // client may receive an error with an indication of ECONNREFUSED
      listen(registrationSocket, 10);

      // Accept any incoming connection
      int connection = sizeof(struct sockaddr_in);

      logDebug("Waiting for socket accept");
      int client = -1;
      struct sockaddr_in clientAddr;
      int handlerId = 0;
      while((client = accept(registrationSocket, (sockaddr*)&clientAddr, (socklen_t*)&connection)) > 0)
      {
         logInfo("Registration Socket connected from %s. Starting Registration Handler", inet_ntoa(clientAddr.sin_addr));

         // Increment the handler ID
         handlerId++;

         auto registrationSocketHandler = std::make_shared<RegistrationSocketHandler>(client, clientAddr, handlerId, localCertificateFilename, localKeyFilename, remoteCertificateFilename);

         // Start Receiving
         registrationSocketHandler->startReceiving();

         // Subscribe to receive the registration data
         registrationSocketHandler->subscribe(this);

         // Add the handler to the map of handlers so it can be cleaned up after
         // the data is received
         // Limit access to the map to one thread at a time
         std::lock_guard<std::mutex> lock(registrationHandlerMutex);
         registrationHandlers[handlerId] = registrationSocketHandler;
      }

      logDebug("Accept returned -1");
   }
   
   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ExternalNetworkBridge::runAppThread()
{
   logVerbose("Enter");

   logDebug("Running App Listener Thread");
   int appClientSock;
   int connection;
   uint64_t handlerId = 0;
   struct sockaddr_in appListenerAddr;
   struct sockaddr_in appClientAddr;

   // Create the socket
   appSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   if(appSocket == -1)
   {
      logError("Could not create app listener socket");
   }
   else
   {
      logDebug("Socket created %d", appSocket);

      // Set the Socket Option to allow reuse of local addresses
      int optionValue = 1;
      if(setsockopt(appSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optionValue, sizeof(optionValue)) < 0)
      {
         logError("Error setting SO_REUSEADDR: %s", strerror(errno));
      }

      // Prepare the sockaddr_in structure
      appListenerAddr.sin_family = AF_INET;
      appListenerAddr.sin_addr.s_addr = INADDR_ANY;
      appListenerAddr.sin_port = htons(appPort);

      // Attempt to bind to this socket
      int bindReturn = -1;
      while(bindReturn < 0)
      {
         bindReturn = bind(appSocket, (sockaddr *)&appListenerAddr, sizeof(appListenerAddr));

         if(bindReturn < 0)
         {
            logError("Bind Failed: %s", strerror(errno));
            logInfo("Sleep for a second and try again");
            std::this_thread::sleep_for(std::chrono::seconds(1));
         }
      }

      logDebug("Bind Complete");

      // Listen for up to 10 pending connections before accept needs to be 
      // called. If a connection request arrives when the queue is full, the 
      // client may receive an error with an indication of ECONNREFUSED
      listen(appSocket, 10);

      // Accept any incoming connection
      connection = sizeof(struct sockaddr_in);

      logDebug("Waiting for socket accept");
      while( (appClientSock = accept(appSocket, (sockaddr*)&appClientAddr, 
                  (socklen_t*)&connection)) > 0)
      {
         logInfo("App Socket connected from %s. Starting App Handler", inet_ntoa(appClientAddr.sin_addr));

         // Increment the handler ID
         handlerId++;

         auto appSocketHandler = std::make_shared<AppSocketHandler>(appClientSock, appClientAddr, handlerId, localCertificateFilename, localKeyFilename, remoteCertificateFilename);

         // Start the receive thread
         appSocketHandler->startReceiving();

         // Subscribe to receive updates from this handler
         appSocketHandler->subscribe(this);

         // Add the app handler to the app handler map
         std::lock_guard<std::mutex> lock(appHandlerMutex);
         appHandlers[handlerId] = appSocketHandler;
      }

      logDebug("Accept returned -1");
   }
   
   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ExternalNetworkBridge::runLCMThread()
{
   logVerbose("Enter");

   logDebug("Running LCM Listener Thread");
   int lcmClientSock;
   int connection;
   struct sockaddr_in lcmListenerAddr;
   struct sockaddr_in lcmClientAddr;
   uint64_t handlerId = 0;

   // Create the socket
   lcmSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   if(lcmSocket == -1)
   {
      logError("Could not create the LCM listener socket");
   }
   else
   {
      logDebug("Socket created %d", lcmSocket);

      // Set the Socket Option to allow reuse of local addresses
      int optionValue = 1;
      if(setsockopt(lcmSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optionValue, sizeof(optionValue)) < 0)
      {
         logError("Error setting SO_REUSEADDR: %s", strerror(errno));
      }

      // Prepare the sockaddr_in structure
      lcmListenerAddr.sin_family = AF_INET;
      lcmListenerAddr.sin_addr.s_addr = INADDR_ANY;
      lcmListenerAddr.sin_port = htons(lcmPort);

      // Attempt to bind to this socket
      int bindReturn = -1;
      while(bindReturn < 0)
      {
         bindReturn = bind(lcmSocket, (sockaddr *)&lcmListenerAddr, sizeof(lcmListenerAddr));

         if(bindReturn < 0)
         {
            logError("Bind Failed: %s", strerror(errno));
            logInfo("Sleep for a second and try again");
            std::this_thread::sleep_for(std::chrono::seconds(1));
         }
      }

      logDebug("Bind Complete");

      // Listen for up to 10 pending connections before accept needs to be 
      // called. If a connection request arrives when the queue is full, the 
      // client may receive an error with an indication of ECONNREFUSED
      listen(lcmSocket, 10);

      // Accept any incoming connection
      connection = sizeof(struct sockaddr_in);

      logDebug("Waiting for socket accept");
      while( (lcmClientSock = accept(lcmSocket, (sockaddr*)&lcmClientAddr, 
                  (socklen_t*)&connection)) > 0)
      {
         logInfo("LCM Socket connected from %s. Starting LCM Handler", inet_ntoa(lcmClientAddr.sin_addr));

         // Set the timeout for the server to disconnect if no messages are received
         struct timeval tv;
         tv.tv_sec = LCM_DISCONNECT_TIMEOUT_SECONDS;
         tv.tv_usec = 0;
         if(setsockopt(lcmClientSock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval)) < 0)
         {
            logError("Error setting SO_RCVTIMEO: %s", strerror(errno));
         }

         // Increment the handler ID
         handlerId++;

         auto lcmSocketHandler = std::make_shared<LCMSocketHandler>(lcmClientSock, lcmClientAddr, handlerId, localCertificateFilename, localKeyFilename, remoteCertificateFilename);

         // Start the receive thread
         lcmSocketHandler->startReceiving();

         // Subscribe to receive the device ID data
         lcmSocketHandler->subscribe(this);

         // Add the handler to the handler map
         std::lock_guard<std::mutex> lock(lcmHandlerMutex);
         lcmHandlers[handlerId] = lcmSocketHandler;
      }

      logDebug("Accept returned -1");
   }
   
   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

