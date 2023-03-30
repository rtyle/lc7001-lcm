#include "Publisher.h"

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

Publisher::Publisher()
{
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

Publisher::~Publisher()
{
   subscribers.clear();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void Publisher::subscribe(Subscriber *subscriber)
{
   subscribers.push_back(subscriber);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void Publisher::unsubscribe(Subscriber *subscriber)
{
   subscribers.remove(subscriber);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void Publisher::publish(std::shared_ptr<PublishObject> object, Subscriber *ignoreSubscriber)
{
   for(auto iter = subscribers.begin(); iter != subscribers.end(); ++iter)
   {
      if(*iter != ignoreSubscriber)
      {
         (*iter)->update(this, object);
      }
   }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
