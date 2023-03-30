#ifndef _PUBLISHER_H_
#define _PUBLISHER_H_

#include "Subscriber.h"
#include "PublishObject.h"
#include <list>
#include <vector>
#include <cstdint>
#include <memory>

class Publisher
{
   public:
      Publisher();
      virtual ~Publisher();

      void subscribe(Subscriber *subscriber);
      void unsubscribe(Subscriber *subscriber);
      void publish(std::shared_ptr<PublishObject> object, Subscriber *ignoreSubscriber = nullptr);
      
   private:
      std::list<Subscriber*> subscribers;
};

#endif
