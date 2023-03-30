#include "RegistrationSocketHandler.h"
#include "Debug.h"
#include "ServerUtil.h"
#include "SSLCommon.h"
#include "defines.h"
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <signal.h>
#include <fcntl.h>

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

RegistrationSocketHandler::RegistrationSocketHandler(const int &socketHandle, const struct sockaddr_in &clientAddress, const int &inHandlerId, const std::string &inLocalCertificateFilename, const std::string &inLocalKeyFilename, const std::string &inRemoteCertificateFilename) :
   connected(true),
   clientSocketHandle(socketHandle),
   handlerId(inHandlerId),
   localCertificateFilename(inLocalCertificateFilename),
   localKeyFilename(inLocalKeyFilename),
   remoteCertificateFilename(inRemoteCertificateFilename), 
   ctx(NULL),
   sslHandle(NULL)
{
   logVerbose("Enter");

   signal(SIGPIPE, SIG_IGN);

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

RegistrationSocketHandler::~RegistrationSocketHandler()
{
   logVerbose("Enter");

   stopReceiving();

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

void RegistrationSocketHandler::runReceiveThread()
{
   logVerbose("Enter");

   char dataBytes[1];
   std::string jsonPacket;
   int readSize = 0;

   logDebug("Waiting for bytes %d", clientSocketHandle);
   if(clientSocketHandle < 0)
   {
      logError("Socket Handle is invalid. Cannot receive");
   }
   else if(acceptSocket())
   {
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
         else if(readSize == 0)
         {
            logInfo("Disconnected");
         }
         else
         {
            logInfo("Receive Failed");
         }
      } while(readSize > 0);
   }

   // Set the connection status to false
   connected = false;

   logDebug("Receive Thread Exiting");
   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool RegistrationSocketHandler::isConnected()
{
   return connected;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void RegistrationSocketHandler::startReceiving()
{
   logVerbose("Enter");

   // Start receiving data
   receiveThread = std::move(std::thread(&RegistrationSocketHandler::runReceiveThread, this));

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void RegistrationSocketHandler::stopReceiving()
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

bool RegistrationSocketHandler::handleJsonPacket(const std::string &jsonString)
{
   logVerbose("Enter %s", jsonString.c_str());
   bool handled = false;

   json_t *root = NULL;
   json_error_t jsonError;
   json_t * responseObject = json_object();

   root = json_loads((const char *)jsonString.c_str(), 0, &jsonError);
   if(root)
   {
      json_t *idObject = json_object_get(root, "ID");

      if(idObject == NULL)
      {
         logError("ID object is NULL");
         json_object_set_new(responseObject, "ID", json_integer(0));
         buildErrorResponse(responseObject, "No 'ID' field.", SW_NO_ID_FIELD);
      }
      else if(!json_is_integer(idObject))
      {
         logError("ID object is not an integer");
         json_object_set_new(responseObject, "ID", json_integer(0));
         buildErrorResponse(responseObject, "'ID' field not an integer.", SW_ID_FIELD_IS_NOT_AN_INTEGER);
      }
      else
      {
         json_object_set(responseObject, "ID", idObject);
         json_t *serviceObject = json_object_get(root, "Service");

         if(serviceObject == NULL)
         {
            logError("Service object is NULL");
            buildErrorResponse(responseObject, "No 'Service' field.", SW_NO_SERVICE_FIELD);
         }
         else if(!json_is_string(serviceObject))
         {
            logError("Service object is not a string");
            buildErrorResponse(responseObject, "'Service' field not a string.", SW_SERVICE_FIELD_IS_NOT_A_STRING);
         }
         else
         {
            json_object_set(responseObject, "Service", serviceObject);
            const char *serviceText = json_string_value(serviceObject);

            if(!strcmp("AppRegistration", serviceText))
            {
               handled = handleAppRegistration(root, responseObject);
            }
            else
            {
               logWarning("Unhandle Service %s", serviceText);
               buildErrorResponse(responseObject, "Unknown 'Service'.", SW_UNKNOWN_SERVICE);
            }
         }
      }

      // Free the root memory
      json_decref(root);

      if(responseObject)
      {
         json_t * responseStatusObject = json_object_get(responseObject, "Status");
         if(!responseStatusObject)
         {
            json_object_set_new(responseObject, "Status", json_string("Success"));
         }

         // Send the response object
         char * localString = json_dumps(responseObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
         int messageLen = strlen((const char *)localString) + 1;
         int bytesWritten = -1;

         if(sslHandle)
         {
            bytesWritten = SSL_write(sslHandle, localString, messageLen);
         }
         else
         {
            bytesWritten = write(clientSocketHandle, localString, messageLen);
         }

         if(bytesWritten < 0)
         {
            if(clientSocketHandle < 0)
            {
               logError("Invalid Socket Handle");
            }
            else
            {
               logError("Write Failed (%s)", strerror(errno));
            }
         }

         // Free the memory
         free(localString);
         json_decref(responseObject);
      }
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

bool RegistrationSocketHandler::handleAppRegistration(json_t *appRegistrationObject, json_t * responseObject)
{
   logVerbose("Enter");
   bool handled = false;

   // Get the App and LCM IDs from the JSON object
   if(appRegistrationObject)
   {
      json_t * appObject = json_object_get(appRegistrationObject, "appID");
      json_t * lcmObject = json_object_get(appRegistrationObject, "lcmID");

      if(appObject == NULL)
      {
         logError("appID object is NULL");
         buildErrorResponse(responseObject, "appID object is NULL", SW_APP_ID_NULL);
      }
      else if(lcmObject == NULL)
      {
         logError("lcmID object is NULL");
         buildErrorResponse(responseObject, "lcmID object is NULL", SW_LCM_ID_NULL);
      }
      else if(!json_is_integer(appObject))
      {
         logError("appID object is not an integer");
         buildErrorResponse(responseObject, "appID object must be an integer", SW_APP_ID_MUST_BE_INTEGER);
      }
      else if(!json_is_string(lcmObject))
      {
         logError("lcmID object is not a string");
         buildErrorResponse(responseObject, "lcmID object must be a string", SW_LCM_ID_MUST_BE_STRING);
      }
      else
      {
         // Publish the data
         std::string lcmIdStr = json_string_value(lcmObject);
         uint64_t lcmId = ServerUtil::MACAddressFromString(lcmIdStr);
         auto registrationData = std::make_shared<RegistrationData>(json_integer_value(appObject), lcmId, handlerId);
         publish(registrationData);
         handled = true;
      }
   }
   else
   {
      logError("JSON Object is NULL");
   }
   logVerbose("Exit %d", handled);
   return handled;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void RegistrationSocketHandler::buildErrorResponse(json_t *responseObject, const char *errorString, uint32_t errorCode)
{
   json_object_set_new(responseObject, "Status", json_string("Error"));
   json_object_set_new(responseObject, "ErrorText", json_string(errorString));
   json_object_set_new(responseObject, "ErrorCode", json_integer(errorCode));
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool RegistrationSocketHandler::acceptSocket()
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

   return accepted;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

