#ifndef _LCM_TEST_CLIENT_H_
#define _LCM_TEST_CLIENT_H_

#include "TestClient.h"

class LCMTestClient : public TestClient
{
   public:
      LCMTestClient(const uint64_t &deviceId);

   protected:
      virtual bool handleSystemInfo(json_t * systemInfoObject);
};

#endif
