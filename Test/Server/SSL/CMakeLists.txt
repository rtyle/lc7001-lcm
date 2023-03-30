INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Server/Common)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Server/SSL)

# Add the SSL Test
ADD_EXECUTABLE(SSL-test
   SSL-test.cpp)

TARGET_LINK_LIBRARIES(SSL-test SSLLibrary Common ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} pthread ${CMAKE_DL_LIBS} ssl crypto)

ADD_TEST(NAME SSL-init COMMAND ${CMAKE_SOURCE_DIR}/Test/Server/SSL/createCerts.sh)

ADD_TEST(NAME SSL-test COMMAND SSL-test --serverCert serverCert.pem --serverKey serverKey.pem --expiredCert expiredCert.pem --expiredKey expiredKey.pem --clientCert clientCert.pem --clientKey clientKey.pem)
