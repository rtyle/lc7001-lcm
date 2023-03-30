PROJECT(LCM C CXX ASM)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

IF(NOT ARCH_BUILD)
   SET(ARCH_BUILD "amd64")
ENDIF(NOT ARCH_BUILD)
SET(ARCH_BUILD ${ARCH_BUILD} CACHE STRING "Default architecture (amd64, armel)")

ADD_SUBDIRECTORY(Version)

IF(${ARCH_BUILD} STREQUAL "amd64")
   MESSAGE("BUILDING AMD")
   ADD_SUBDIRECTORY(WolfSSL)

   # Set the C++ Compiler flags
   SET(CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_NANOSLEEP -Wall -std=c++11 -fPIC -g")
   SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
   ADD_DEFINITIONS(-DAMDBUILD -DUSE_SSL)

   # Add the subdirectories
   ADD_SUBDIRECTORY(Server)
   ADD_SUBDIRECTORY(Embedded)

   # Enable testing
   enable_testing()

   # Add the BOOST unit tests
   FIND_PACKAGE(Boost COMPONENTS unit_test_framework)
   IF(Boost_FOUND)
      enable_testing()
      ADD_SUBDIRECTORY(Test)
      find_program(CTEST_MEMORYCHECK_COMMAND valgrind)
      SET(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full")
      SET(MEMORYCHECK_SUPPRESSIONS_FILE "${CMAKE_SOURCE_DIR}/Test/.valgrindSuppressions")
      include(CTest)
   ENDIF()

ELSE()
   IF(DEFINED USE_SSL)
      ADD_SUBDIRECTORY(WolfSSL)
   ENDIF()

   ADD_SUBDIRECTORY(WolfMQTT)

   SET(ASM_OPTIONS "${CMAKE_C_FLAGS} -x assembler-with-cpp -D_EWL_C99=1 -D_DEBUG=0 -std=gnu99")
   SET(CMAKE_ASM_FLAGS "${ASM_OPTIONS}")

   MESSAGE("BUILDING ARMEL")
   ADD_SUBDIRECTORY(MQX)
   ADD_SUBDIRECTORY(Embedded)
ENDIF(${ARCH_BUILD} STREQUAL "amd64")
