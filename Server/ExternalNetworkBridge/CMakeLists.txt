INCLUDE_DIRECTORIES(../../Version)
INCLUDE_DIRECTORIES(../../Embedded/Includes)
INCLUDE_DIRECTORIES(../Common)
INCLUDE_DIRECTORIES(../../Embedded/Sources/jansson)
INCLUDE_DIRECTORIES(../SQLite3)
INCLUDE_DIRECTORIES(../SSL)

ADD_LIBRARY(ExternalNetworkBridgeLibrary 
   ExternalNetworkBridge.cpp
   LCMSocketHandler.cpp
   AppSocketHandler.cpp
   RegistrationSocketHandler.cpp
   ClientHandler.cpp)

# Add the External Network Bridge Executable
ADD_EXECUTABLE(ExternalNetworkBridge
   ExternalNetworkBridgeMain.cpp)
TARGET_LINK_LIBRARIES(ExternalNetworkBridge ExternalNetworkBridgeLibrary SQLite3 Common Sources pthread SSLLibrary ssl crypto ${CMAKE_DL_LIBS}) 
