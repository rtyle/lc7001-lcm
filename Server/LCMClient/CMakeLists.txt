#set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
#find_package(Avahi)
#INCLUDE_DIRECTORIES(${AVAHI_INCLUDE_DIRS})

# Add the LCM server executable
ADD_EXECUTABLE(LCMClient
      LCMClientMain.cpp)
