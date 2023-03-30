#ifndef _SUBSCRIBER_H_
#define _SUBSCRIBER_H_

#include "Publisher.h"
#include "PublishObject.h"
#include <vector>
#include <cstdint>
#include <memory>

class Publisher;  // forward reference

class Subscriber
{
   public:
      virtual ~Subscriber() {};
      virtual void update(Publisher *who, std::shared_ptr<PublishObject> object) = 0;
};

#endif
