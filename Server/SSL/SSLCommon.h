#ifndef _SSLCOMMON_H_
#define _SSLCOMMON_H_

#include <string>
#include <vector>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "PublishObject.h"

class SSLBytes : public PublishObject
{
   public:
      SSLBytes(const std::vector<uint8_t> inBytes) :
         bytes(inBytes)
      {
      }

      const std::vector<uint8_t> &getBytes() const
      {
         return bytes;
      }

   private:
      std::vector<uint8_t> bytes;
};

namespace SSLCommon 
{
   SSL_CTX* createSSLContext(const std::string &localCertificateFile, const std::string &localKeyFile, const std::string &remoteCertificateFile);
};

#endif
