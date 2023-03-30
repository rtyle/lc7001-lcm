#include "SSLCommon.h"
#include "Debug.h"

namespace SSLCommon
{
   static unsigned char dh2048_p[] = {
      0x4A,0x42,0x57,0xB7,0x08,0x7F,0x08,0x17,0x72,0xA2,0xBA,0xD6,0xA9,0x42,0xF3,0x05,
      0x45,0xF9,0x53,0x11,0x39,0x4F,0xB6,0xF1,0x6E,0xB9,0x4B,0x38,0x20,0xDA,0x01,0xA7,
      0x4E,0xA3,0x14,0xE9,0x8F,0x40,0x55,0xF3,0xD0,0x07,0xC6,0xCB,0x43,0xA9,0x94,0xAD,
      0x4E,0x4C,0x64,0x86,0x49,0xF8,0x0C,0x83,0xBD,0x65,0xE9,0x17,0xD4,0xA1,0xD3,0x50,
      0x59,0xF5,0x59,0x5F,0xDC,0x76,0x52,0x4F,0x3D,0x3D,0x8D,0xDB,0xCE,0x99,0xE1,0x57,
      0x20,0x59,0xCD,0xFD,0xB8,0xAE,0x74,0x4F,0xC5,0xFC,0x76,0xBC,0x83,0xC5,0x47,0x30,
      0x38,0xCE,0x7C,0xC9,0x66,0xFF,0x15,0xF9,0xBB,0xFD,0x91,0x5E,0xC7,0x01,0xAA,0xD3,
      0x36,0x9E,0x8D,0xA0,0xA5,0x72,0x3A,0xD4,0x1A,0xF0,0xBF,0x46,0x00,0x58,0x2B,0xE5,
      0x37,0x88,0xFD,0x58,0x4E,0x49,0xDB,0xCD,0x20,0xB4,0x9D,0xE4,0x91,0x07,0x36,0x6B,
      0x2D,0x6C,0x38,0x0D,0x45,0x1D,0x0F,0x7C,0x88,0xB3,0x1C,0x7C,0x5B,0x2D,0x8E,0xF6,
      0x35,0xC9,0x23,0xC0,0x43,0xF0,0xA5,0x5B,0x18,0x8D,0x8E,0xBB,0x55,0x8C,0xB8,0x5D,
      0x33,0xD3,0x34,0xFD,0x7C,0x17,0x57,0x43,0xA3,0x1D,0x18,0x6C,0xDE,0x33,0x21,0x2C,
      0x30,0x2A,0xFF,0x3C,0xE1,0xB1,0x29,0x40,0x18,0x11,0x8D,0x7C,0x84,0xA7,0x0A,0x72,
      0x39,0x86,0xC4,0x03,0x19,0xC8,0x07,0x29,0x7A,0xCA,0x95,0x0C,0xD9,0x96,0x9F,0xAB,
      0x20,0x0A,0x50,0x9B,0x02,0x46,0xD3,0x08,0x3D,0x66,0xA4,0x5D,0x41,0x9F,0x9C,0x7C,
      0xBD,0x89,0x4B,0x22,0x19,0x26,0xBA,0xAB,0xA2,0x5E,0xC3,0x55,0xE9,0x32,0x0B,0x3B,
   };

   static unsigned char dh2048_g[] = {
      0x03,
   };

   static DH *get_dh2048(void)
   {
      DH *dh=NULL;

      if((dh=DH_new()) == NULL)
      {
         logError("Could not create a new DH");
         return(NULL);
      }

      dh->p=BN_bin2bn(dh2048_p,sizeof(dh2048_p),NULL);
      dh->g=BN_bin2bn(dh2048_g,sizeof(dh2048_g),NULL);
      if((dh->p == NULL) || 
            (dh->g == NULL))
      {
         logError("Could not set p or g");
         return(NULL);
      }

      return(dh);
   }

   SSL_CTX *createSSLContext(const std::string &localCertificateFile, const std::string &localKeyFile, const std::string &remoteCertificateFile)
   {
      logVerbose("Enter");

      // Initialize the SSL Library
      SSL_library_init();
      const SSL_METHOD *method = TLSv1_2_method();
      OpenSSL_add_all_algorithms();
      SSL_load_error_strings();

      SSL_CTX *ctx = SSL_CTX_new(method);

      DH *dh = get_dh2048();
      SSL_CTX_set_tmp_dh(ctx, dh);
      DH_free(dh);

      if(!localCertificateFile.empty())
      {
         // Set the certificate from the certificate file
         if(SSL_CTX_use_certificate_file(ctx, localCertificateFile.c_str(), SSL_FILETYPE_PEM) <= 0)
         {
            ERR_print_errors_fp(stderr);
         }
      }

      if(!localKeyFile.empty())
      {
         // Set the private key from the keyfile
         if(SSL_CTX_use_PrivateKey_file(ctx, localKeyFile.c_str(), SSL_FILETYPE_PEM) <= 0)
         {
            ERR_print_errors_fp(stderr);
         }

         // Verify the private key
         if(!SSL_CTX_check_private_key(ctx))
         {
            logError("Private key does not match the public certificate");
         }
      }

      if(!remoteCertificateFile.empty())
      {
         logInfo("Set Verify");
         SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

         if(SSL_CTX_load_verify_locations(ctx, remoteCertificateFile.c_str(), NULL) != 1)
         {
            ERR_print_errors_fp(stderr);
         }
      }

      logVerbose("Exit");
      return ctx;
   }
}