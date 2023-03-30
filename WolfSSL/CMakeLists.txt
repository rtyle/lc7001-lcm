#[[ Legrand modifications:

2015-10-01: Remove two MESSAGE calls, move some .c files to the top of SET(SOURCES block
2015-10-01: Add dependencies
2015-11-03: Added compiler controls to remove unused SSL library code and added optimization for the WolfSSL library.
  Turned on SSL support and set the _DEBUG flag to 0 to remove unnecessary MQX debug information
2019-03-13: Add WolfMQTT configuration requirements

file changes:
2021-05-13: wolfssl-3.11.0-stable/woflcrypt/src/aes.c
            Remove static keyword from wc_AesEncrypt
2015-05-13: wolfssl-3.11.0-stable/woflcrypt/src/random.c
            Wrote a random seed using the time instead of using the default random seed to remove warning
2021-05-13: wolfssl-3.11.0-stable/woflcrypt/settings.h
            Comment out FREESCALE_K70_RNGA (line 844)
            No longer need to comment out WOLFSSL_USER_IO - version 3.11.0 defaults this option to false
            In the FREESCALE_COMMON section, comment out:
            #include "fsl_common.h" (line 824) - our environment does not provide that library include file.
2021-05-13: wolfssl-3.11.0-stable/woflssl/ssl.h
            Comment out unnecessary #include <stdio.h> that causes conflicting types errors
2015-05-13: wolfssl-3.11.0-stable/wolfssl/woflcrypt/tfm.h
            Added defines to determine FP_MAX_BITS - to support build process
2021-05-13: wolfssl-3.11.0-stable/.gitignore
            Remove wolfssl/options.h from .gitignore since MQTT library references it.
2021-05-19: wolfssl-3.11.0-stable/CMakeLists.txt
            Remove -DUSE_CERD_BUFFERS_2048 since the new library defines this internally.

]]


SET(LIBRARY_NAME WolfSSL)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/WolfSSL/wolfssl-3.11.0-stable)

IF(NOT ${ARCH_BUILD} STREQUAL "amd64")
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/bsp/Generated_Code)
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/bsp/Sources)
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug)
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/bsp)
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/psp)
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/mfs)
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/rtcs)
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/shell)
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/usb)
   INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/tools/toolchain/arm-none-eabi/include)
   INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/tools/toolchain/lib/gcc/arm-none-eabi/5.2.1/include)
   INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/tools/toolchain/lib/gcc/arm-none-eabi/5.2.1/include-fixed)

   SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_WOLFSSL_SERVER -DNO_DES3 -DNO_DSA -DNO_ERROR_STRINGS -DNO_MD4 -DNO_PSK -DNO_PWDBASED -DNO_RC4 -DNO_HC128 -DNO_SESSION_CACHE -DFREESCALE_MQX -DWOLFSSL_STATIC_RSA -DWOLFSSL_BASE64_ENCODE")
ENDIF()

SET(SOURCES
   wolfssl-3.11.0-stable/wolfcrypt/src/memory.c
   wolfssl-3.11.0-stable/wolfcrypt/src/integer.c
   wolfssl-3.11.0-stable/wolfcrypt/src/pwdbased.c
   wolfssl-3.11.0-stable/wolfcrypt/src/error.c
   wolfssl-3.11.0-stable/wolfcrypt/src/wc_port.c
   wolfssl-3.11.0-stable/src/crl.c
   wolfssl-3.11.0-stable/src/internal.c
   wolfssl-3.11.0-stable/src/io.c
   wolfssl-3.11.0-stable/src/keys.c
   wolfssl-3.11.0-stable/src/ocsp.c
   wolfssl-3.11.0-stable/src/sniffer.c
   wolfssl-3.11.0-stable/src/ssl.c
   wolfssl-3.11.0-stable/src/tls.c
   wolfssl-3.11.0-stable/wolfcrypt/src/logging.c
   wolfssl-3.11.0-stable/wolfcrypt/src/asn.c
   wolfssl-3.11.0-stable/wolfcrypt/src/rsa.c
   wolfssl-3.11.0-stable/wolfcrypt/src/md5.c
   wolfssl-3.11.0-stable/wolfcrypt/src/sha.c
   wolfssl-3.11.0-stable/wolfcrypt/src/sha256.c
   wolfssl-3.11.0-stable/wolfcrypt/src/random.c
   wolfssl-3.11.0-stable/wolfcrypt/src/hash.c
   wolfssl-3.11.0-stable/wolfcrypt/src/arc4.c
   wolfssl-3.11.0-stable/wolfcrypt/src/des3.c
   wolfssl-3.11.0-stable/wolfcrypt/src/aes.c
   wolfssl-3.11.0-stable/wolfcrypt/src/dh.c
   wolfssl-3.11.0-stable/wolfcrypt/src/coding.c
   wolfssl-3.11.0-stable/wolfcrypt/src/hmac.c
   wolfssl-3.11.0-stable/wolfcrypt/src/tfm.c
   wolfssl-3.11.0-stable/wolfcrypt/src/ecc.c
   wolfssl-3.11.0-stable/wolfcrypt/src/wolfmath.c)

ADD_LIBRARY(${LIBRARY_NAME} STATIC ${SOURCES})
ADD_DEPENDENCIES(${LIBRARY_NAME} version)

IF(NOT ${ARCH_BUILD} STREQUAL "amd64")
   ADD_DEPENDENCIES(${LIBRARY_NAME} shell)
   ADD_DEPENDENCIES(${LIBRARY_NAME} mfs)
   ADD_DEPENDENCIES(${LIBRARY_NAME} bsp)
   ADD_DEPENDENCIES(${LIBRARY_NAME} psp)
   ADD_DEPENDENCIES(${LIBRARY_NAME} rtcs)
   ADD_DEPENDENCIES(${LIBRARY_NAME} usbd)
   ADD_DEPENDENCIES(${LIBRARY_NAME} usbh)
ENDIF()
