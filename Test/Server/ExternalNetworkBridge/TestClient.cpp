#include "TestClient.h"
#include "Debug.h"
#include "SSLCommon.h"
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

TestClient::TestClient(const uint64_t &deviceID) :
   deviceId(deviceID),
   connected(false),
   socketHandle(-1),
   ctx(NULL),
   sslHandle(NULL)
{
   logVerbose("Enter");

   signal(SIGPIPE, SIG_IGN);

   socketHandle = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

TestClient::~TestClient()
{
   logVerbose("Enter");
   
   // Close any open connections
   closeConnection();

   if(receiveThread.joinable())
   {
      receiveThread.join();
   }

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

bool TestClient::openConnection(const std::string &serverHostname, const int &port, const std::string &localCertificateFilename, const std::string &localKeyFilename, const std::string &remoteCertificateFilename)
{
   logVerbose("Enter");

   if(!connected)
   {
      // Create the SSL context if the certificate files were provided
      if((!localCertificateFilename.empty()) &&
         (!localKeyFilename.empty()) &&
         (!remoteCertificateFilename.empty()))
      {
         ctx = SSLCommon::createSSLContext(localCertificateFilename, localKeyFilename, remoteCertificateFilename);
      }

      struct sockaddr_in serverAddr;
      memset(&serverAddr, 0, sizeof(serverAddr));
      serverAddr.sin_family = AF_INET;
      serverAddr.sin_addr.s_addr = inet_addr(serverHostname.c_str());
      serverAddr.sin_port = htons(port);

      if(socketHandle > 0)
      {
         int connectReturn = connect(socketHandle, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

         if(connectReturn < 0)
         {
            logError("Failed to connect to the server: %s", strerror(errno));
            close(socketHandle);
            socketHandle = -1;
         }
         else
         {
            if(ctx)
            {
               logDebug("Creating the SSL Handle");
               sslHandle = SSL_new(ctx);
               SSL_set_fd(sslHandle, socketHandle);

               logDebug("Connecting to the SSL Socket");
               connectReturn = SSL_connect(sslHandle);
               if(connectReturn <= 0)
               {
                  logError("Failed to connect to the SSL Socket");
                  ERR_print_errors_fp(stderr);
                  closeConnection();
               }
               else
               {
                  // Start the receive thread
                  receiveThread = std::move(std::thread(&TestClient::runReceiveThread, this));

                  logInfo("Successfully connected to the SSL Server");
                  connected = true;
                  connectionCV.notify_all();
               }
            }
            else
            {
               // Start the receive thread
               receiveThread = std::move(std::thread(&TestClient::runReceiveThread, this));

               connected = true;
               connectionCV.notify_all();
            }
         }
      }
   }

   logVerbose("Exit %d", connected);
   return connected;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool TestClient::closeConnection()
{
   logVerbose("Enter");
   bool connectionClosed = false;

   if(sslHandle)
   {
      SSL_shutdown(sslHandle);
   }

   if(socketHandle > 0)
   {
      shutdown(socketHandle, SHUT_RDWR);
      close(socketHandle);
      connected = false;
      connectionClosed = true;
      connectionCV.notify_all();
   }
   
   logVerbose("Exit %d", connectionClosed);
   return connectionClosed;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

std::mutex *TestClient::getConnectionMutex()
{
   return &connectionMutex;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

std::condition_variable *TestClient::getConnectionCV()
{
   return &connectionCV;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool TestClient::isConnected()
{
   logInfo("Returning Connected=%d", connected);
   return connected;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void TestClient::runReceiveThread()
{
   logVerbose("Enter");

   if(socketHandle != -1)
   {
      char dataBytes[1];
      std::string jsonPacket;
      int readSize = 0;

      do
      {
         if(sslHandle)
         {
            readSize = SSL_read(sslHandle, dataBytes, sizeof(dataBytes));
         }
         else
         {
            readSize = recv(socketHandle, dataBytes, 1, 0);
         }

         if(readSize > 0)
         {
            if(dataBytes[0] == 0x00)
            {
               logDebug("Handling JSON Packet %s", jsonPacket.c_str());
               handleJsonPacket(jsonPacket);
               jsonPacket.clear();
            }
            else
            {
               jsonPacket.push_back(dataBytes[0]);
            }
         }
         else if(readSize <= 0)
         {
            logDebug("Test Client disconnected");
            connected = false;
            connectionCV.notify_all();
         }
      } while(readSize > 0);
   }

   logVerbose("Exit");

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool TestClient::sendJsonPacket(json_t *jsonObject)
{
   logVerbose("Enter");

   bool jsonSent = false;

   if(socketHandle < 0)
   {
      logError("Socket handle is invalid. Cannot send JSON packet");
   }
   else if(jsonObject == NULL)
   {
      logError("JSON object is NULL");
   }
   else
   {
      char * localString = json_dumps(jsonObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
      int messageLen = strlen((const char *)localString) + 1;

      logDebug("Sending %s", localString);
      int bytesWritten = -1; 
      
      if(sslHandle)
      {
         bytesWritten = SSL_write(sslHandle, localString, messageLen);
      }
      else
      {
         bytesWritten = write(socketHandle, localString, messageLen);
      }

      // Free the string memory after sending
      free(localString);

      if(bytesWritten == -1)
      {
         logError("Write Failed (%s)", strerror(errno));
      }
      else
      {
         jsonSent = true;
      }
   }

   logVerbose("Exit %d", jsonSent);
   return jsonSent;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool TestClient::handleSystemInfo(json_t *systemInfoObject)
{
   // Implemented in child class
   logVerbose("Enter");
   logVerbose("Exit");
   return false;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool TestClient::handleReportAppID(json_t * appIdObject)
{
   // Implemented in child class
   logVerbose("Enter");
   logVerbose("Exit");
   return false;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool TestClient::handleOther(json_t * jsonObject)
{
   logVerbose("Enter");
   
   // Notify the subscribers of the received json message
   logDebug("Notifying subscribers");
   auto notifyJson = std::make_shared<PublishJson>(jsonObject);
   publish(notifyJson);

   logVerbose("Exit");
   return true;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool TestClient::handleJsonPacket(const std::string &jsonString)
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
               handled = handleSystemInfo(root);
            }
            else if(!strcmp("ReportAppID", serviceText))
            {
               handled = handleReportAppID(root);
            }
            else
            {
               handled = handleOther(root);
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

