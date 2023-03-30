#ifndef _TEST_CLIENT_H_
#define _TEST_CLIENT_H_

#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <jansson.h>
#include "Publisher.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

class PublishJson : public PublishObject
{
   public:
      PublishJson(json_t * inObject)
      {
         // Increment the reference count so
         // the object can get destroyed by the
         // original creator and then get destroyed after
         // all of the subscribers use the message
         jsonObject = json_incref(inObject);
      }

      ~PublishJson()
      {
         json_decref(jsonObject);
      }

      json_t * getJsonObject()
      {
         return jsonObject;
      }

   private:
      json_t *jsonObject;
};

class TestClient : public Publisher
{
   public:
      TestClient(const uint64_t &deviceId = 0);
      virtual ~TestClient();

      bool openConnection(const std::string &serverHostname, const int &port, const std::string &localCertificateFilename = "", const std::string &localKeyFilename = "", const std::string &remoteCertificateFilename = "");
      bool closeConnection();
      std::mutex *getConnectionMutex();
      std::condition_variable *getConnectionCV();
      bool isConnected();
      bool sendJsonPacket(json_t * jsonObject);

   protected:
      uint64_t deviceId;
      virtual bool handleSystemInfo(json_t * systemInfoObject);
      virtual bool handleReportAppID(json_t * appIdObject);
      virtual bool handleOther(json_t * pingObject);

   private:
      std::mutex connectionMutex;
      std::condition_variable connectionCV;

      bool connected;
      int socketHandle;
      std::thread receiveThread;
      void runReceiveThread();

      bool handleJsonPacket(const std::string &jsonString);

      // SSL Variables
      SSL_CTX *ctx;
      SSL *sslHandle;
};

#endif
