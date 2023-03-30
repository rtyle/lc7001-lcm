#ifndef _SSLSERVER_H_
#define _SSLSERVER_H_

#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "Publisher.h"
#include "SSLCommon.h"

class SSLServer : public Publisher
{
   private:
      class SSLClientSocketHandler
      {
         public:
            SSLClientSocketHandler(const int &handle, SSL_CTX *ctx, Publisher *pub);
            virtual ~SSLClientSocketHandler();

            void startReceiving();
            void stopReceiving();
            bool send(const std::vector<uint8_t> &data);

         private:
            SSL *sslHandle;
            Publisher *publisher;

            // Thread that receives JSON packets from the client
            std::thread receiveThread;
            virtual void runReceiveThread();
      };

   public:
      SSLServer(const int &port, const std::string &localCertFile, const std::string &localKeyFile, const std::string &remoteCertFile);
      virtual ~SSLServer();
      void start();
      void stop();
      void sendBroadcast(const std::vector<uint8_t> &data);

   private:
      // Port number to listen
      int port = -1;

      // SSL Server Thread
      std::mutex serverThreadMutex;
      std::condition_variable serverThreadCondition;
      std::thread serverThread;
      virtual void runServerThread();
      int socketDesc;

      // SSL Client Socket Handlers
      std::mutex clientHandlerMutex;
      std::vector<SSLClientSocketHandler *> clientSocketHandlers;

      // SSL Functions
      std::string localCertificateFilename;
      std::string localKeyFilename;
      std::string remoteCertificateFilename;
      SSL_CTX *ctx;
};

#endif
