#ifndef _APP_TEST_CLIENT_H_
#define _APP_TEST_CLIENT_H_

#include "TestClient.h"

class AppTestClient : public TestClient
{
   public:
      AppTestClient(const uint64_t &deviceId);

   protected:
      virtual bool handleReportAppID(json_t * appIdObject);
};

#endif
