#include "LCMSocketHandler.h"
#include "Debug.h"
#include "ServerUtil.h"
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

LCMSocketHandler::LCMSocketHandler(const int &socketHandle, const struct sockaddr_in &clientAddress, const uint64_t &handlerId, const std::string &localCertificateFilename, const std::string &localKeyFilename, const std::string &remoteCertificateFilename) :
   ClientHandler(socketHandle, clientAddress, handlerId, localCertificateFilename, localKeyFilename, remoteCertificateFilename)
{
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

LCMSocketHandler::~LCMSocketHandler()
{
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool LCMSocketHandler::sendDeviceIdRequest()
{
   logVerbose("Enter");
   bool sent = false;

   if(clientSocketHandle < 0)
   {
      logError("Socket Handle is not valid. Cannot send SystemInfo request");
   }
   else
   {
      // Send the System Info command to the LCM
      json_t *systemInfoRequest = json_object();
      json_object_set_new(systemInfoRequest, "ID", json_integer(0));
      json_object_set_new(systemInfoRequest, "Service", json_string("SystemInfo"));

      char *localString = json_dumps(systemInfoRequest, (JSON_COMPACT | JSON_PRESERVE_ORDER));
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
         logError("Write Failed (%s)", strerror(errno));
      }
      else
      {
         // Set the return value
         sent = true;
      }

      // Free the string memory after sending
      free(localString);

      // Free the json memory
      json_decref(systemInfoRequest);
   }

   logVerbose("Exit");
   return sent;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

std::string LCMSocketHandler::getDeviceIdStr()
{
   logVerbose("Enter");

   std::string deviceIdStr;

   if(deviceId == 0)
   {
      deviceIdStr = "Unknown Device ID";
   }
   else
   {
      deviceIdStr = ServerUtil::MACAddressToString(deviceId);
   }

   logVerbose("Exit %s", deviceIdStr.c_str());
   return deviceIdStr;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void LCMSocketHandler::publishDeviceId()
{
   auto handlerData = std::make_shared<LCMHandlerData>(deviceId, handlerId);
   publish(handlerData);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
