#ifndef _SSLCLIENT_H_
#define _SSLCLIENT_H_

#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "Publisher.h"

class SSLClient : public Publisher
{
   public:
      SSLClient(const std::string &localCertificateFilename, const std::string &localKeyFile, const std::string &remoteCertFile);
      virtual ~SSLClient();
      bool connectToHost(const std::string &hostname, const int &port);
      bool disconnect(const bool &joinReceiveThread = true);
      bool send(const std::vector<uint8_t> &data);

   private:
      std::string localCertificateFilename;
      std::string localKeyFilename;
      std::string remoteCertificateFilename;
      SSL_CTX *ctx;
      SSL *sslHandle;

      std::thread receiveThread;
      virtual void runReceiveThread();
};

#endif
