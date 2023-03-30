#ifndef _PUBLISH_OBJECT_H_
#define _PUBLISH_OBJECT_H_

class PublishObject
{
   public:
      virtual ~PublishObject() = 0;
};

inline PublishObject::~PublishObject()
{
}

#endif
