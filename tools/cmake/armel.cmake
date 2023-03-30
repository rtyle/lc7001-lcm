# This is the toolchain file for compiling the LCM for armel

include(CMakeForceCompiler)
SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)

# Uncomment to use SSL
SET(USE_SSL 1)

SET(SYSROOT ${CMAKE_SOURCE_DIR}/tools/toolchain)
SET(CMAKE_SYSROOT ${SYSROOT})

CMAKE_FORCE_C_COMPILER(arm-none-eabi-gcc GNU)
CMAKE_FORCE_CXX_COMPILER(arm-none-eabi-g++ GNU)

SET(MQX_ROOTDIR "${CMAKE_SOURCE_DIR}/MQX")
SET(CMAKE_C_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wno-missing-braces -g3 -gdwarf-2 -gstrict-dwarf -gstrict-dwarf -DMQX_DISABLE_CONFIG_CHECK=1 -D_AEABI_LC_CTYPE=C -D__VFPV4__=1 -D__STRICT_ANSI__=1 -D_DEBUG=0 -fno-strict-aliasing -Wno-switch -Wno-unused-value -Wno-unused-variable -Wno-unused-but-set-variable -Wno-pointer-to-int-cast -Wno-unused-function -Wno-unused-label -Wno-char-subscripts -Wno-int-to-pointer-cast -nostdinc")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "C flags" FORCE)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "C++ flags" FORCE)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# set the SDK architecture
SET(ARCH_BUILD armel)
