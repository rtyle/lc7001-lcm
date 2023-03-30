# Create the version information
FIND_PACKAGE(Git)

# Set the git version to get the date information
EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} describe --tags --long OUTPUT_VARIABLE GIT_VERSION)

# Create the input file that will be used to create the header file
FILE(WRITE ${CMAKE_SOURCE_DIR}/Version/ReleaseVersion.h.in "\#ifndef _RELEASE_VERSION_H_\n\#define _RELEASE_VERSION_H_\n\n\#define SYSTEM_INFO_FIRMWARE_VERSION \"@FIRMWARE_VERSION@\"\n\#define SYSTEM_INFO_FIRMWARE_DATE \"@FIRMWARE_DATE@\"\n\n\#endif")

# Add the commands in the cmake file to generate the variables written to the header file
FILE(WRITE ${CMAKE_SOURCE_DIR}/Version/ReleaseVersion.cmake
   "EXECUTE_PROCESS(
   COMMAND ${GIT_EXECUTABLE} describe --tags --long --abbrev=4
   OUTPUT_VARIABLE FIRMWARE_VERSION
   OUTPUT_STRIP_TRAILING_WHITESPACE)\n")

FILE(APPEND ${CMAKE_SOURCE_DIR}/Version/ReleaseVersion.cmake
   "EXECUTE_PROCESS(
   COMMAND ${GIT_EXECUTABLE} show ${GIT_VERSION} -s --date=short --format=%cd
   OUTPUT_VARIABLE FIRMWARE_DATE
   OUTPUT_STRIP_TRAILING_WHITESPACE)
   CONFIGURE_FILE(\${SRC} \${DST} @ONLY)\n")

# Add the target that creates the ReleaseVersion.h file
ADD_CUSTOM_TARGET(version ${CMAKE_COMMAND}
   -D SRC=${CMAKE_SOURCE_DIR}/Version/ReleaseVersion.h.in
   -D DST=${CMAKE_SOURCE_DIR}/Version/ReleaseVersion.h
   -P ${CMAKE_SOURCE_DIR}/Version/ReleaseVersion.cmake
)
