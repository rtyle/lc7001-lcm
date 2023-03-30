#include "Debug.h"
#include "SSLServer.h"
#include <getopt.h>
#include <sstream>

#define DEFAULT_PORT 1979

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void printUsage()
{
   std::cout << "Usage: ./SSLServer -p [port] -c [cert.pem] -k [key.pem] -r [remoteCert.pem]" << std::endl;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
   int port = DEFAULT_PORT;
   char option = 0;
   std::string serverCertificateFile;
   std::string serverKeyFile;
   std::string remoteCertificateFile;

   while ((option = getopt(argc, argv, "p:c:k:r:")) != -1)
   {
      switch (option)
      {
         case 'p':
            {
               std::istringstream ss(optarg);
               if(!(ss >> port))
               {
                  logError("Invalid port number %s. Using default %d", optarg, DEFAULT_PORT);
                  port = DEFAULT_PORT;
               }
               break;
            }
         case 'c':
            serverCertificateFile = optarg;
            break;
         case 'k':
            serverKeyFile = optarg;
            break;
         case 'r':
            remoteCertificateFile = optarg;
            break;
         default:
            logError("Invalid Argument %c", option);
            printUsage();
            exit(-1);
      }
   }

   logInfo("Creating the Server listening on port %d", port);
   SSLServer server(port, serverCertificateFile, serverKeyFile, remoteCertificateFile);
   server.start();

   bool done = false;
   while(!done)
   {
      std::string cmd;
      std::cout << "SSL Server: " << std::flush;
      std::getline(std::cin, cmd);

      if(!cmd.empty())
      {
         if(cmd == "quit")
         {
            server.stop();
            done = true;
         }
         else if((cmd == "?") || (cmd == "help"))
         {
            std::cout << "Available commands: quit, help, ?" << std::endl;
         }
         else
         {
            std::cout << "Unrecognized command: " << cmd << std::endl;
         }
      }
   }

   return 0;
}
