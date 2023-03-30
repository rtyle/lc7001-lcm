INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Embedded/Includes)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Server/Common)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Server/LCMServer)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/external/lightingcommon/headers)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Version)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Embedded/Sources/jansson)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Embedded/Sources/flash)


# Add the  Test
ADD_EXECUTABLE(Socket_Task-test
   Socket_Task-test.cpp)

TARGET_LINK_LIBRARIES(Socket_Task-test  Common Sources LCMServerLibrary ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

ADD_TEST(NAME Socket_Task-test COMMAND Socket_Task-test)
