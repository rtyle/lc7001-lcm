#ifndef _REGISTRATION_TEST_CLIENT_H_
#define _REGISTRATION_TEST_CLIENT_H_

#include "TestClient.h"

class RegistrationTestClient : public TestClient
{
   public:
      bool sendRegistrationMessage(const uint64_t &appId, const std::string &lcmId);
};

#endif
