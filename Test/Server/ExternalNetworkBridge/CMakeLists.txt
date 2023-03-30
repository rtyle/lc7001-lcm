INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Server/Common)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Server/ExternalNetworkBridge)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Server/SQLite3)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Server/SSL)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Test/Common)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Embedded/Sources/jansson)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Embedded/Includes)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Version)

# Add the test client library
ADD_LIBRARY(TestClientLibrary
   TestClient.cpp
   LCMTestClient.cpp
   AppTestClient.cpp
   RegistrationTestClient.cpp)

# Add the Client Handler Test
ADD_EXECUTABLE(ClientHandler-test
   ClientHandler-test.cpp)
TARGET_LINK_LIBRARIES(ClientHandler-test TestClientLibrary Common TestCommon ExternalNetworkBridgeLibrary Sources SSLLibrary ssl crypto ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${CMAKE_DL_LIBS})
ADD_TEST(NAME ClientHandler-test COMMAND ClientHandler-test)

# Add the SSL Client Handler Test
ADD_EXECUTABLE(SSLClientHandler-test
   SSLClientHandler-test.cpp)
TARGET_LINK_LIBRARIES(SSLClientHandler-test TestClientLibrary Common TestCommon ExternalNetworkBridgeLibrary Sources SSLLibrary ssl crypto ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${CMAKE_DL_LIBS})
ADD_TEST(NAME SSLClientHandler-test COMMAND SSLClientHandler-test --serverCert ${CMAKE_SOURCE_DIR}/Server/SSL/sslCertificates/serverCert.pem --serverKey ${CMAKE_SOURCE_DIR}/Server/SSL/sslCertificates/serverKey.pem --clientCert ${CMAKE_SOURCE_DIR}/Server/SSL/sslCertificates/clientCert.pem --clientKey ${CMAKE_SOURCE_DIR}/Server/SSL/sslCertificates/clientKey.pem)

# Create the initialize db command
ADD_TEST(NAME ExternalNetworkBridge-init COMMAND ${CMAKE_SOURCE_DIR}/Test/Server/SQLite3/db_setup.sh -f test.db ${CMAKE_SOURCE_DIR}/Web/schema.sql)

# Add the External Network Bridge Test
ADD_EXECUTABLE(ExternalNetworkBridge-test
   ExternalNetworkBridge-test.cpp)
TARGET_LINK_LIBRARIES(ExternalNetworkBridge-test TestClientLibrary Common TestCommon ExternalNetworkBridgeLibrary Sources SSLLibrary ssl crypto ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} SQLite3 ${CMAKE_DL_LIBS})
ADD_TEST(NAME ExternalNetworkBridge-test COMMAND ExternalNetworkBridge-test)

# Add the SSL External Network Bridge Test
ADD_EXECUTABLE(SSLExternalNetworkBridge-test
   SSLExternalNetworkBridge-test.cpp)
TARGET_LINK_LIBRARIES(SSLExternalNetworkBridge-test TestClientLibrary Common TestCommon ExternalNetworkBridgeLibrary Sources SSLLibrary ssl crypto ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} SQLite3 ${CMAKE_DL_LIBS})
ADD_TEST(NAME SSLExternalNetworkBridge-test COMMAND SSLExternalNetworkBridge-test --serverCert ${CMAKE_SOURCE_DIR}/Server/SSL/sslCertificates/serverCert.pem --serverKey ${CMAKE_SOURCE_DIR}/Server/SSL/sslCertificates/serverKey.pem --clientCert ${CMAKE_SOURCE_DIR}/Server/SSL/sslCertificates/clientCert.pem --clientKey ${CMAKE_SOURCE_DIR}/Server/SSL/sslCertificates/clientKey.pem)

