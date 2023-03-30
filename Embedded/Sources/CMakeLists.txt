INCLUDE_DIRECTORIES(./jansson)
INCLUDE_DIRECTORIES(../Includes)
INCLUDE_DIRECTORIES(../../external/lightingcommon/headers)
INCLUDE_DIRECTORIES(../../Version)
INCLUDE_DIRECTORIES(./flash)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/WolfSSL/wolfssl-3.11.0-stable)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/WolfMQTT)

IF(${ARCH_BUILD} STREQUAL "amd64")
   INCLUDE_DIRECTORIES(../../Server/Common/)
   SET(LIBRARY_NAME Sources)
   SET(FIRMWARE_SOURCES
      jansson/dump.c
      jansson/error.c
      jansson/hashtable.c
      jansson/load.c
      jansson/memory.c
      jansson/pack_unpack.c
      jansson/strbuffer.c
      jansson/strconv.c
      jansson/value.c
      jansson/utf.c
      Crc.c
      Zones.c
      Scenes.c
      SceneController.c
      Socket_Task.c
      RFM100_Tasks.c
      Event_Task.c)
   ADD_LIBRARY(${LIBRARY_NAME} STATIC ${FIRMWARE_SOURCES})
   TARGET_LINK_LIBRARIES(${LIBRARY_NAME} Common pthread WolfSSL)
   ADD_DEPENDENCIES(${LIBRARY_NAME} version)
ELSE()
   SET(EXECUTABLE_NAME "LCM1-K64.elf")
   IF(DEFINED USE_SSL)
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_SSL -DFREESCALE_MQX -DONQ_CONFIG_PLATFORM_K60 -D__VFPV4__=1 -D_EWL_C99=1 -D_DEBUG=0 -std=gnu99 -DENABLE_MQTT_TLS -DWOLFMQTT_NO_TIMEOUT -DWOLFMQTT_NO_STDIO")
   ELSE()
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFREESCALE_MQX -DONQ_CONFIG_PLATFORM_K60 -D__VFPV4__=1 -D_EWL_C99=1 -D_DEBUG=0 -std=gnu99")
   ENDIF()
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/bsp/Generated_Code)
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/bsp/Sources)
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug)
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/bsp)
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/psp)
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/mfs)
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/rtcs)
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/shell)
   INCLUDE_DIRECTORIES(${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/usb)
   INCLUDE_DIRECTORIES(./flash)
   INCLUDE_DIRECTORIES(../../tools/toolchain/arm-none-eabi/include)
   INCLUDE_DIRECTORIES(../../tools/toolchain/lib/gcc/arm-none-eabi/5.2.1/include)
   INCLUDE_DIRECTORIES(../../tools/toolchain/lib/gcc/arm-none-eabi/5.2.1/include-fixed)
   IF(DEFINED USE_SSL)
      INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/WolfSSL/wolfssl-3.11.0-stable)
   ENDIF()

   SET(SOURCES
      jansson/dump.c
      jansson/error.c
      jansson/hashtable.c
      jansson/load.c
      jansson/memory.c
      jansson/pack_unpack.c
      jansson/strbuffer.c
      jansson/strconv.c
      jansson/value.c
      jansson/utf.c
      flash/DEFlashPartition.c
      flash/DFlashGetProtection.c
      flash/DFlashSetProtection.c
      flash/EEEWrite.c
      flash/EERAMGetProtection.c
      flash/EERAMSetProtection.c
      flash/FlashCheckSum.c
      flash/FlashCommandSequence.c
      flash/FlashEraseAllBlock.c
      flash/FlashEraseBlock.c
      flash/FlashEraseResume.c
      flash/FlashEraseSector.c
      flash/FlashEraseSuspend.c
      flash/FlashGetInterruptEnable.c
      flash/FlashGetSecurityState.c
      flash/FlashInit.c
      flash/FlashProgramCheck.c
      flash/FlashProgramLongword.c
      flash/FlashProgramOnce.c
      flash/FlashProgramPhrase.c
      flash/FlashProgramSection.c
      flash/FlashReadOnce.c
      flash/FlashReadResource.c
      flash/FlashSecurityBypass.c
      flash/FlashSetInterruptEnable.c
      flash/FlashVerifyAllBlock.c
      flash/FlashVerifyBlock.c
      flash/FlashVerifySection.c
      flash/PFlashGetProtection.c
      flash/PFlashGetSwapStatus.c
      flash/PFlashSetProtection.c
      flash/PFlashSwap.c
      flash/SetEEEEnable.c
      Command_Lists.c
      I2C.c
      jsr.c
      Crc.c
      Logging_Task.c
      RFM100_Tasks.c
      RTCS.c
      Shell_Task.c
      Zones.c
      Scenes.c
      SceneController.c
      Flash.c
      Socket_Task.c
      Event_Task.c
      Tasks.c
      cgi_hvac.c
      cgi_index.c
      cgi_netstat.c
      html_util.c
      tfs_data.c
      CC1110_Programmer.c
      Eliot_REST_Task.c
      Eliot_MQTT_Task.c
      Eliot_Common.c
      Mqtt.c
      String_Sanitizer.c)
      
   ADD_EXECUTABLE(${EXECUTABLE_NAME} ${SOURCES})
   ADD_DEPENDENCIES(${EXECUTABLE_NAME} version)

   SET(CUSTOM_FLAGS "-T ${MQX_ROOTDIR}/lib/twrk64f120m.cmake/debug/bsp/intflash.ld -nostartfiles -nodefaultlibs -nostdlib -Xlinker --gc-sections -Wl,-Map,\"LCM1-K64.map\" -Xlinker --cref -v -Xlinker -z -Xlinker muldefs -static ../../MQX/mqx/libpsp.a ../../MQX/mqx/libbsp.a ../../MQX/rtcs/librtcs.a")
   SET(CMAKE_EXE_LINKER_FLAGS ${CUSTOM_FLAGS})
   IF(DEFINED USE_SSL)
	   TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} -Wl,--start-group bsp psp rtcs shell WolfSSL WolfMQTT -Wl,--end-group)
   ELSE()
      TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} -Wl,--start-group bsp psp rtcs shell -Wl,--end-group)
   ENDIF()

   SET(MY_TARGETLIB_PREFIX "")
   TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} c m gcc)

   ADD_CUSTOM_COMMAND(TARGET ${EXECUTABLE_NAME}
      POST_BUILD
      COMMAND ${MQX_ROOTDIR}/../Embedded/creates19.bat ${EXECUTABLE_NAME} ${CMAKE_CURRENT_BINARY_DIR} LCM.s19
      COMMENT "Creating LCM.s19 in ${CMAKE_CURRENT_BINARY_DIR}")

   # Add the LCM.s19 file to the clean command
   SET(CLEANFILES
      LCM.s19
      LCM1-K64.map)
   SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CLEANFILES}")
ENDIF(${ARCH_BUILD} STREQUAL "amd64")
