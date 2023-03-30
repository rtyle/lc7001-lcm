#include "ServerUtil.h"
#include <sstream>
#include <vector>
#include <iomanip>

uint64_t ServerUtil::MACAddressFromString(const std::string &macAddressString)
{
   uint64_t address = 0;

   std::istringstream ss(macAddressString);
   std::string token;
   std::vector<std::string> tokens;

   // Add all of the number elements of the MAC address to a vector
   // Each element is delimited by the ':' character
   while(std::getline(ss, token, ':'))
   {
      tokens.push_back(token);
   }

   for(size_t tokenIndex = 0; tokenIndex < tokens.size() && tokenIndex < 6; tokenIndex++)
   {
      // Parse the tokens into the address
      std::stringstream ss;
      ss << std::hex << tokens[tokenIndex];
      uint64_t macByte;
      ss >> macByte;

      // Add the element to the address
      address |= ((macByte & 0xFF) << ((5-tokenIndex)*8));
   }

   return address;
}

std::string ServerUtil::MACAddressToString(const uint64_t &macAddress)
{
   std::stringstream ss;
   ss << std::hex << std::uppercase;
   ss << std::setfill('0') << std::setw(2) << ((macAddress >> 40) & 0xFF) << ":";
   ss << std::setfill('0') << std::setw(2) << ((macAddress >> 32) & 0xFF) << ":";
   ss << std::setfill('0') << std::setw(2) << ((macAddress >> 24) & 0xFF) << ":";
   ss << std::setfill('0') << std::setw(2) << ((macAddress >> 16) & 0xFF) << ":";
   ss << std::setfill('0') << std::setw(2) << ((macAddress >> 8) & 0xFF) << ":";
   ss << std::setfill('0') << std::setw(2) << (macAddress & 0xFF);

   return(ss.str());
}
