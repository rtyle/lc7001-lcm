INCLUDE_DIRECTORIES(${AVAHI_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(../../Embedded/Includes)
INCLUDE_DIRECTORIES(../../Version)
INCLUDE_DIRECTORIES(../Common)
INCLUDE_DIRECTORIES(../../Embedded/Sources/jansson)
INCLUDE_DIRECTORIES(../../Embedded/Sources/flash)
INCLUDE_DIRECTORIES(../../external/lightingcommon/headers)

ADD_LIBRARY(LCMServerLibrary 
   LCMServer.cpp
   AvahiClient.cpp
   RFM100Stub.cpp
   EventTaskStub.cpp
   FlashStub.cpp)

# Add the LCM server executable
ADD_EXECUTABLE(LCMServer
   LCMServerMain.cpp)

TARGET_LINK_LIBRARIES(LCMServer LCMServerLibrary Common Sources ${AVAHI_LIBRARIES})
