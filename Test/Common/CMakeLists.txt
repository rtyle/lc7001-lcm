INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Server/Common)

SET(LIBRARY_NAME TestCommon)
SET(SOURCES
    TestCommon.cpp)

ADD_LIBRARY(${LIBRARY_NAME} ${SOURCES})
