#include "ClientHandler.h"
#include "Debug.h"
#include "ServerUtil.h"
#include "SSLCommon.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <algorithm>
#include <fcntl.h>
#include <signal.h>

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

ClientHandler::ClientHandler(const int &socketHandle, const struct sockaddr_in &clientAddress, const uint64_t &inHandlerId, const std::string &localCertificateFilename, const std::string &localKeyFilename, const std::string &remoteCertificateFilename) :
   deviceId(0),
   handlerId(inHandlerId),
   clientSocketHandle(socketHandle),
   sslHandle(NULL),
   connected(true),
   ipAddressString(""),
   pinging(false),
   ctx(NULL)
{
   logVerbose("Enter");

   signal(SIGPIPE, SIG_IGN);

   // Set the IP address of the client
   ipAddressString = inet_ntoa(clientAddress.sin_addr);

   // Create the SSL Context if the certificate files were provided
   if((!localCertificateFilename.empty()) &&
      (!localKeyFilename.empty()) &&
      (!remoteCertificateFilename.empty()))
   {
      ctx = SSLCommon::createSSLContext(localCertificateFilename, localKeyFilename, remoteCertificateFilename);

      if(ctx == NULL)
      {
         ERR_print_errors_fp(stderr);
      }
   }

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

ClientHandler::~ClientHandler()
{
   logVerbose("Enter");

   stopReceiving();
   stopPinging();

   // Join the receiving thread
   if(receiveThread.joinable())
   {
      receiveThread.join();
   }

   // Clear the bridged sockets vector
   bridgedSocketHandles.clear();
   bridgedSSLHandles.clear();

   if(sslHandle)
   {
      SSL_free(sslHandle);
      sslHandle = NULL;
   }

   if(ctx)
   {
      SSL_CTX_free(ctx);
      ctx = NULL;
   }

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool ClientHandler::isConnected()
{
   return connected;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

uint64_t ClientHandler::getHandlerId()
{
   logVerbose("Enter");
   logVerbose("Exit %lu", static_cast<unsigned long>(handlerId));
   return handlerId;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

uint64_t ClientHandler::getDeviceId()
{
   logVerbose("Enter");
   logVerbose("Exit %lu", static_cast<long unsigned int>(deviceId));
   return deviceId;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int ClientHandler::getClientSocketHandle()
{
   return clientSocketHandle;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

SSL * ClientHandler::getClientSSLHandle()
{
   return sslHandle;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

std::string ClientHandler::getClientIPAddressString()
{
   logVerbose("Enter");
   logVerbose("Exit %s", ipAddressString.c_str());
   return ipAddressString;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool ClientHandler::addBridgedSocketHandle(const int &socketHandle)
{
   logVerbose("Enter %d", socketHandle);
   bool added = false;

   // Only add the socket if it doesn't already exist in the vector
   if(std::find(bridgedSocketHandles.begin(), bridgedSocketHandles.end(), socketHandle) == bridgedSocketHandles.end())
   {
      logDebug("Adding bridged socket handle %d to the client handler", socketHandle);
      bridgedSocketHandles.push_back(socketHandle);
      added = true;
   }

   logVerbose("Exit %d", added);
   return added;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool ClientHandler::addBridgedSocketHandle(SSL *handle)
{
   logVerbose("Enter");
   bool added = false;

   // Only add the handle if it doesn't already exist in the vector
   if(std::find(bridgedSSLHandles.begin(), bridgedSSLHandles.end(), handle) == bridgedSSLHandles.end())
   {
      logDebug("Adding bridged SSL handle %p to the client handler", handle);
      bridgedSSLHandles.push_back(handle);
      added = true;
   }

   logVerbose("Exit %d", added);
   return added;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ClientHandler::removeBridgedSocketHandle(const int &socketHandle)
{
   logVerbose("Enter %d", socketHandle);
   
   // Remove the element from the vector of socket handles
   bridgedSocketHandles.erase(std::remove(bridgedSocketHandles.begin(), bridgedSocketHandles.end(), socketHandle), bridgedSocketHandles.end());

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ClientHandler::removeBridgedSocketHandle(SSL *handle)
{
   logVerbose("Enter");

   // Remove the element from the vector of SSL Handles
   bridgedSSLHandles.erase(std::remove(bridgedSSLHandles.begin(), bridgedSSLHandles.end(), handle), bridgedSSLHandles.end());

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool ClientHandler::startPinging()
{
   logVerbose("Enter");
   bool started = false;

   if(clientSocketHandle > 0)
   {
      if(!pinging)
      {
         pinging = true;
         pingThread = std::move(std::thread(&ClientHandler::runPingThread, this));
         started = true;
      }
      else
      {
         // Pinging was already started
         started = true;
      }
   }

   logVerbose("Exit %d", started);
   return started;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool ClientHandler::stopPinging()
{
   logVerbose("Enter");
   bool stopped = false;

   if(pinging && pingThread.joinable())
   {
      // Set the ping flag to false
      pinging = false;

      // Notify the condition variable
      pingCondition.notify_all();

      // Join the thread
      pingThread.join();

      stopped = true;
   }
   else if(!pinging)
   {
      stopped = true;
   }

   logVerbose("Exit");
   return stopped;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ClientHandler::runReceiveThread()
{
   logVerbose("Enter");

   char dataBytes[MAX_MESSAGE_SIZE];
   std::string jsonPacket;
   int readSize = 0;

   logDebug("Waiting for Bytes");
   if(clientSocketHandle < 0)
   {
      logError("Socket Handle is invalid. Cannot receive");
   }
   else if(acceptSocket())
   {
      if(sslHandle)
      {
         // Give the SSL client a little extra time to finish the connection
         // before attempting to send data. When data was sent too fast, 
         // it was sent as part of the negotation which was causing the
         // connection to fail
         std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }

      // Send the device ID request
      sendDeviceIdRequest();
 
      // Wait for the device ID response
      do
      {
         if(sslHandle)
         {
            readSize = SSL_read(sslHandle, dataBytes, 1);
         }
         else
         {
            readSize = recv(clientSocketHandle, dataBytes, 1, 0);
         }

         if(readSize > 0)
         {
            // The device was not initialized yet so we need to wait for the 
            // device ID to be set. Do not forward any data to the bridged
            // sockets until the device ID is set
            if(dataBytes[0] == 0x00)
            {
               logInfo("Handling JSON Packet %s", jsonPacket.c_str());
               handleJsonPacket(jsonPacket);
               jsonPacket.clear();
            }
            else
            {
               jsonPacket.push_back(dataBytes[0]);
            }
         }
      } while((deviceId == 0) && (readSize > 0));

      if(deviceId != 0)
      {
         // The device ID was set so we can now forward any data to bridged sockets
         do
         {
            if(sslHandle)
            {
               readSize = SSL_read(sslHandle, dataBytes, MAX_MESSAGE_SIZE);
            }
            else
            {
               readSize = recv(clientSocketHandle, dataBytes, MAX_MESSAGE_SIZE, 0);
            }

            if(readSize > 0)
            {
               logVerbose("Received %d bytes from socket", readSize);

               // Handle the bytes
               for(auto it = bridgedSocketHandles.begin(); it != bridgedSocketHandles.end();)
               {
                  logDebug("Forwarding data to bridged sockets");

                  int bytesWritten = send(*it, dataBytes, readSize, MSG_NOSIGNAL);

                  if(bytesWritten == -1)
                  {
                     logError("Write failed (%s)", strerror(errno));

                     // Remove socket so we don't try to write to it again. Erase
                     // will return the iterator to the next socket handle or
                     // end() if it was the last item
                     it = bridgedSocketHandles.erase(it);
                  }
                  else
                  {
                     // Bytes were written successfully
                     // Increment the iterator to get to the next socket
                     it++;
                  }
               }

               for(auto it = bridgedSSLHandles.begin(); it != bridgedSSLHandles.end();)
               {
                  logDebug("Forwarding data to bridged SSL sockets");

                  int bytesWritten = SSL_write(*it, dataBytes, readSize);

                  if(bytesWritten == -1)
                  {
                     logError("Write failed (%s)", strerror(errno));

                     // Remove socket so we don't try to write to it again. Erase
                     // will return the iterator to the next socket handle or
                     // end() if it was the last item
                     it = bridgedSSLHandles.erase(it);
                  }
                  else
                  {
                     // Bytes were written successfully
                     // Increment the iterator to get to the next socket
                     it++;
                  }
               }
            }

            logVerbose("Waiting for next bytes");
         } while(readSize > 0);
      }

      if(readSize == 0)
      {
         logInfo("Disconnected");
      }
      else if(readSize < 0)
      {
         logError("Receive Failed");
      }
   }

   // Set the connection status to false
   connected = false;

   logDebug("Receive Thread Exiting");
   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ClientHandler::startReceiving()
{
   logVerbose("Enter");

   // Start receiving data
   receiveThread = std::move(std::thread(&ClientHandler::runReceiveThread, this));

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ClientHandler::stopReceiving()
{
   logVerbose("Enter");

   if(sslHandle)
   {
      SSL_shutdown(sslHandle);
   }

   if(clientSocketHandle > 0)
   {
      shutdown(clientSocketHandle, SHUT_RDWR);
      close(clientSocketHandle);
      clientSocketHandle = -1;
   }

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool ClientHandler::handleJsonPacket(const std::string &jsonString)
{
   logVerbose("Enter %s", jsonString.c_str());
   bool handled = false;

   json_t *root = NULL;
   json_error_t jsonError;

   root = json_loads((const char *)jsonString.c_str(), 0, &jsonError);
   if(root)
   {
      json_t *idObject = json_object_get(root, "ID");

      if(idObject == NULL)
      {
         logError("ID object is NULL");
      }
      else if(!json_is_integer(idObject))
      {
         logError("ID object is not an integer");
      }
      else
      {
         json_t *serviceObject = json_object_get(root, "Service");

         if(serviceObject == NULL)
         {
            logError("Service object is NULL");
         }
         else if(!json_is_string(serviceObject))
         {
            logError("Service object is not a string");
         }
         else
         {
            const char *serviceText = json_string_value(serviceObject);

            if(!strcmp("SystemInfo", serviceText))
            {
               // Handle SystemInfo
               handled = handleSystemInfo(root);
            }
            else if(!strcmp("ReportAppID", serviceText))
            {
               handled = handleReportAppID(root);
            }
            else
            {
               logWarning("Unhandle Service %s", serviceText);
            }
         }
      }

      json_decref(root);
   }
   else
   {
      logError("JSON Error %s", jsonError.text);
   }

   logVerbose("Exit %d", handled);
   return handled;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool ClientHandler::handleSystemInfo(json_t *systemInfoObject)
{
   logVerbose("Enter");
   bool handled = false;

   // Get the MAC address from the JSON object
   json_t *macAddressObject = json_object_get(systemInfoObject, "MACAddress");

   if(macAddressObject == NULL)
   {
      logError("MACAddress object is null");
   }
   else if(!json_is_string(macAddressObject))
   {
      logError("MACAddress object is not a string");
   }
   else
   {
      // Reset the device ID to 0 because we received a new device ID message
      deviceId = 0;

      // Save the device ID from the string
      const char *macAddressString = json_string_value(macAddressObject);

      logDebug("MAC Address string %s", macAddressString);
      deviceId = ServerUtil::MACAddressFromString(macAddressString);
      logInfo("Device ID Set to %lu", static_cast<long unsigned int>(deviceId));

      // Publish the device ID to subscribers
      publishDeviceId();

      // Set the return value
      handled = true;
   }

   logVerbose("Exit %d", handled);
   return handled;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool ClientHandler::handleReportAppID(json_t *reportAppIDObject)
{
   logVerbose("Enter");
   bool handled = false;

   // Get the app ID from the JSON object
   json_t *appIDObject = json_object_get(reportAppIDObject, "AppID");

   if(appIDObject == NULL)
   {
      logError("appID object is null");
   }
   else if(!json_is_integer(appIDObject))
   {
      logError("appID object is not an integer");
   }
   else
   {
      // Reset the device ID to the received app ID
      deviceId = json_integer_value(appIDObject);

      logInfo("Device ID Set to %lu", static_cast<long unsigned int>(deviceId));

      // Publish the device ID to subscribers
      publishDeviceId();

      // Set the return value
      handled = true;
   }

   logVerbose("Exit %d", handled);
   return handled;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ClientHandler::runPingThread()
{
   logVerbose("Enter");
   logDebug("Running the ping thread");

   // Create the ping string
   json_t *pingObject = json_object();
   char *localString = NULL;
   int messageLen = 0;

   if(pingObject)
   {
      json_object_set_new(pingObject, "ID", json_integer(0));
      json_object_set_new(pingObject, "Service", json_string("ping"));
      json_object_set_new(pingObject, "Status", json_string("Success"));

      localString = json_dumps(pingObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
      messageLen = strlen((const char *)localString) + 1;
   }

   while(pinging)
   {
      // Wait for the ping condition to timeout
      {
         std::unique_lock<std::mutex> lock(pingMutex);
         pingCondition.wait_for(lock, std::chrono::seconds(5));
      }

      if((localString) &&
         (pinging) &&
         (clientSocketHandle > 0))
      {
         // Send the ping
         if(write(clientSocketHandle, localString, messageLen) < 0)
         {
            logError("Write Failed (%s)", strerror(errno));
         }
      }
   }

   // Free the string memory
   free(localString);

   // Free the JSON memory
   json_decref(pingObject);

   logDebug("Ping thread exiting");
   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool ClientHandler::acceptSocket()
{
   logVerbose("Enter");

   bool accepted = false;

   if(ctx)
   {
      sslHandle = SSL_new(ctx);
      SSL_set_fd(sslHandle, clientSocketHandle);

      // Convert the sockets to non-blocking for the accept call
      int flags = fcntl(clientSocketHandle, F_GETFL, 0);
      fcntl(clientSocketHandle, F_SETFL, flags | O_NONBLOCK);

      int acceptReturn = SSL_accept(sslHandle);

      if(acceptReturn == -1)
      {
         int error = SSL_get_error(sslHandle, acceptReturn);

         if((error == SSL_ERROR_WANT_READ) ||
               (error == SSL_ERROR_WANT_WRITE))
         {
            fd_set fd;
            FD_ZERO(&fd);
            FD_SET(clientSocketHandle, &fd);

            struct timeval tv;
            tv.tv_sec = 5;
            tv.tv_usec = 0;

            int result = select(clientSocketHandle+1, &fd, NULL, NULL, &tv);

            if(result <=0)
            {
               logError("Failed to accept SSL Socket");
            }
            else
            {
               logInfo("Socket accepted");

               // Set the socket back to the mode it was originally in
               fcntl(clientSocketHandle, F_SETFL, flags);
               accepted = true;
            }
         }
      }
      else
      {
         logInfo("Socket originally accepted");
         accepted = true;
      }
   }
   else
   {
      // Using normal sockets. Always return true
      accepted = true;
   }

   logVerbose("Exit %d", accepted);
   return accepted;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

