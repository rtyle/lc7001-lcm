#ifndef _REGISTRATION_SOCKET_HANDLER_H
#define _REGISTRATION_SOCKET_HANDLER_H

#include <cstdint>
#include <thread>
#include <jansson.h>
#include <openssl/ssl.h>
#include "PublishObject.h"
#include "Publisher.h"

class RegistrationData : public PublishObject
{
   public:
      RegistrationData(const uint64_t &inAppId, const uint64_t &inLCMId, const int &inHandlerId) :
         appId(inAppId),
         lcmId(inLCMId),
         handlerId(inHandlerId)
      {
      }

      uint64_t getAppId()
      {
         return appId;
      }

      uint64_t getLCMId()
      {
         return lcmId;
      }

      int getHandlerId()
      {
         return handlerId;
      }

   private:
      uint64_t appId;
      uint64_t lcmId;
      int handlerId;
};

class RegistrationSocketHandler : public Publisher
{
   public:
      RegistrationSocketHandler(const int &clientSocketHandle, const struct sockaddr_in &clientAddress, const int &handlerId, const std::string &localCertificateFilename, const std::string &localKeyFilename, const std::string &remoteCertificateFilename);
      virtual ~RegistrationSocketHandler();

      bool isConnected();
      void startReceiving();
      void stopReceiving();

   private:
      bool connected;
      int clientSocketHandle;
      int handlerId;
      std::string localCertificateFilename;
      std::string localKeyFilename;
      std::string remoteCertificateFilename;

      std::thread receiveThread;
      void runReceiveThread();
      bool handleJsonPacket(const std::string &jsonString);
      bool handleAppRegistration(json_t *appRegistrationObject, json_t * responseObject);
      void buildErrorResponse(json_t *responseObject, const char *errorString, uint32_t errorCode);

      // SSL Variables
      SSL_CTX *ctx;
      SSL *sslHandle;
      bool acceptSocket();
};

#endif
