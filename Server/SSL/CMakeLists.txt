INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Server/Common)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/WolfSSL/wolfssl-3.6.8)

ADD_LIBRARY(SSLLibrary
   SSLServer.cpp
   SSLClient.cpp
   SSLCommon.cpp)

# Add the SSL Server executable
ADD_EXECUTABLE(SSLServer
   SSLServerMain.cpp)
TARGET_LINK_LIBRARIES(SSLServer SSLLibrary Common pthread ssl crypto)

# Add the SSL Client executable
ADD_EXECUTABLE(SSLClient
   SSLClientMain.cpp)
TARGET_LINK_LIBRARIES(SSLClient SSLLibrary Common pthread ssl crypto)

# Add the Wolf SSL Client executable
ADD_EXECUTABLE(WolfSSLClient
   WolfSSLTest.cpp)
TARGET_LINK_LIBRARIES(WolfSSLClient Common pthread WolfSSL)
