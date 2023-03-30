#ifndef _SERVER_UTIL_H_
#define _SERVER_UTIL_H_

#include <cstdint>
#include <string>

namespace ServerUtil
{
   uint64_t MACAddressFromString(const std::string &macAddressString);
   std::string MACAddressToString(const uint64_t &macAddress);
};
#endif
