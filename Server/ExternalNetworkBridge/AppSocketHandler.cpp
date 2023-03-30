#include "AppSocketHandler.h"
#include "Debug.h"
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

AppSocketHandler::AppSocketHandler(const int &socketHandle, const struct sockaddr_in &clientAddress, const uint64_t &handlerId, const std::string &localCertificateFilename, const std::string &localKeyFilename, const std::string &remoteCertificateFilename) :
   ClientHandler(socketHandle, clientAddress, handlerId, localCertificateFilename, localKeyFilename, remoteCertificateFilename)
{
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

AppSocketHandler::~AppSocketHandler()
{
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool AppSocketHandler::sendDeviceIdRequest()
{
   logVerbose("Enter");
   bool sent = false;

   if(clientSocketHandle < 0)
   {
      logError("Socket Handle is not valid. Cannot send ReportAppID request");
   }
   else
   {
      // Create the command to request the device ID from the App
      json_t *reportAppID = json_object();
      json_object_set_new(reportAppID, "ID", json_integer(0));
      json_object_set_new(reportAppID, "Service", json_string("ReportAppID"));

      // Send the message to the client socket
      char *localString = json_dumps(reportAppID, (JSON_COMPACT | JSON_PRESERVE_ORDER));
      int messageLen = strlen((const char*)localString) + 1;

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
         logError("Write Failed (%s)", strerror(errno));
      }
      else
      {
         sent = true;
      }

      // Free the string memory after sending
      free(localString);

      // Free the json memory
      json_decref(reportAppID);
   }

   logVerbose("Exit");
   return sent;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

std::string AppSocketHandler::getDeviceIdStr()
{
   logVerbose("Enter");

   std::string deviceIdStr;

   if(deviceId == 0)
   {
      deviceIdStr = "Unknown App ID";
   }
   else
   {
      // Convert the app ID to a human readable string
      std::stringstream ss;
      ss << deviceId;
      deviceIdStr = ss.str();
   }

   logVerbose("Exit %s", deviceIdStr.c_str());
   return deviceIdStr;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void AppSocketHandler::publishDeviceId()
{
   auto handlerData = std::make_shared<AppHandlerData>(deviceId, handlerId);
   publish(handlerData);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
