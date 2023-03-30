#ifndef _APP_SOCKET_HANDLER_H
#define _APP_SOCKET_HANDLER_H

#include "ClientHandler.h"
#include "Publisher.h"
#include "PublishObject.h"

class AppHandlerData : public PublishObject
{
   public:
      AppHandlerData(const uint64_t &inDeviceId, const uint64_t &inHandlerId) :
         deviceId(inDeviceId),
         handlerId(inHandlerId)
      {
      }

      uint64_t getDeviceId()
      {
         return deviceId;
      }

      uint64_t getHandlerId()
      {
         return handlerId;
      }

   private:
      uint64_t deviceId;
      uint64_t handlerId;
};

class AppSocketHandler : public ClientHandler, public Publisher
{
   public:
      AppSocketHandler(const int &clientSocketHandle, const struct sockaddr_in &clientAddress, const uint64_t &handlerId, const std::string &localCertificateFilename, const std::string &localKeyFilename, const std::string &remoteCertificateFilename);
      virtual ~AppSocketHandler();
      virtual std::string getDeviceIdStr();

   protected:
      virtual bool sendDeviceIdRequest();
      virtual void publishDeviceId();
};

#endif
