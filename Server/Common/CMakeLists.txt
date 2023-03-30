INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Embedded/Includes)

SET(LIBRARY_NAME Common)
SET(SOURCES 
    Debug.cpp
    MQXWrapper.cpp
    Publisher.cpp
    ServerUtil.cpp)

ADD_LIBRARY(${LIBRARY_NAME} STATIC ${SOURCES})
