/**HEADER********************************************************************
 *
 * Copyright (c) 2008 Freescale Semiconductor;
 * All Rights Reserved
 *
 * Copyright (c) 2004-2008 Embedded Access Inc.;
 * All Rights Reserved
 *
 ***************************************************************************
 *
 * THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESSED OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 **************************************************************************
 *
 * $FileName: Socket_Task.c$
 * $Version : 3.8.24.0$
 * $Date    : Sep-20-2011$
 *
 * Comments:
 *
 *
 *
 *END************************************************************************/

#include "includes.h"
#include "Eliot_REST_Task.h"
#include "Eliot_Common.h"

// For factory password and key generation
#include "../../WolfSSL/wolfssl-3.11.0-stable/wolfssl/wolfcrypt/md5.h"
// For socket authentication
#include "../../WolfSSL/wolfssl-3.11.0-stable/wolfssl/wolfcrypt/aes.h"
// For key changes (wc_AesCbcDecryptWithKey())
#include "../../WolfSSL/wolfssl-3.11.0-stable/wolfcrypt/src/wc_encrypt.c"
// For signed firmware update checking
#include "../../WolfSSL/wolfssl-3.11.0-stable/wolfssl/wolfcrypt/sha256.h"


// wc_AesEncrypt lives in wolfssl/Sources/wolfcrypt_src/aes.c
// and used to have a static declaration.
void wc_AesEncrypt(Aes* aes, const byte* inBlock, byte* outBlock);

#ifndef AMDBUILD

#include <stdlib.h>
#include <errno.h>

#include <ipcfg.h>
#include "common.h"
#include "swap_demo.h"
#include "jansson_private.h"

#ifdef USE_SSL
#include <wolfssl/ssl.h>
#include "SSLCertificates.h"
WOLFSSL *ssl = NULL;
#endif

//===============
// Debug settings
//===============
#define DEBUG_SOCKETTASK_ANY                 0 // check this in as 0
#define DEBUG_SOCKETTASK                     (1 && DEBUG_SOCKETTASK_ANY)
#define DEBUG_SOCKETTASK_AUTH                0 // Insecure
#define DEBUG_SOCKETTASK_JSONPROC            (0 && DEBUG_SOCKETTASK_ANY) // don't enable this even in a debug release - it hurts discoverability
#define DEBUG_SOCKETTASK_ERROR               (1 && DEBUG_SOCKETTASK_ANY)
#define DEBUG_SOCKETTASK_QMOTION             (0 && DEBUG_SOCKETTASK_ANY)  
#define DEBUG_SOCKETTASK_QMOTION_DETAIL      (0 && DEBUG_SOCKETTASK_ANY)
#define DEBUG_SOCKETTASK_QMOTION_STATUS_RESPONSE_VERBOSE  (0 && DEBUG_SOCKETTASK_ANY && DEBUG_SOCKETTASK_QMOTION)
#define DEBUG_SOCKETTASK_QMOTION_ERROR       (1 && DEBUG_SOCKETTASK_ANY)
#define DEBUG_SOCKETTASK_SOCKET_SEND_FAIL    (0 && DEBUG_SOCKETTASK_ANY) // this runs us into trouble when debugging, better to leave it off
#define DEBUG_QUBE_MUTEX                     (0 && DEBUG_SOCKETTASK_ANY)
#define DEBUG_ZONEARRAY_MUTEX                (0 && DEBUG_SOCKETTASK_ANY)
#define DEBUG_UPDATE_HOST_RESOLVE            (0 && DEBUG_SOCKETTASK_ANY)
#define DEBUG_QUBE_MUTEX_MAX_MS          10
#define DEBUG_ZONEARRAY_MUTEX_MAX_MS     10

#define PASSWORD_ALPHABET_SIZE 32  // Printable character count - must be a power of 2
#define LOWERCASE_CHARACTERS_ALLOWED_ON_LABEL 1

#if(LOWERCASE_CHARACTERS_ALLOWED_ON_LABEL)
	const char PASSWORD_ALPHABET[] = "BFGHLNRTVX346789bdfghjmnpqrtvwxy";
#else
	const char PASSWORD_ALPHABET[] = "BCDFGHJKLMNPQRTVWXY346789*+#?%&-";
#endif


#if DEBUG_SOCKETTASK_ANY //-----------------------------------------------------
#define DEBUGBUF                              debugBuf
#define SIZEOF_DEBUGBUF                       sizeof(debugBuf)
#define DEBUG_BEGIN_TICK         debugLocalStartTick
#define DEBUG_END_TICK           debugLocalEndTick
#define DEBUG_TICK_OVERFLOW      debugLocalOverflow
#define DEBUG_TICK_OVERFLOW_MS   debugLocalOverflowMs
// declare these locally: MQX_TICK_STRUCT            debugLocalStartTick;
// declare these locally: MQX_TICK_STRUCT            debugLocalEndTick;
// declare these locally: bool                       debugLocalOverflow;
// declare these locally: uint32_t                   debugLocalOverflowMs;
#else //-------------------------------------------------------------------------
// dummy variables will be optimized away, needed for compilation
char                             nonDebugDummyBuf[2];
MQX_TICK_STRUCT                  nonDebugStartTick;
MQX_TICK_STRUCT                  nonDebugEndTick;
bool                             nonDebugOverflow;
uint32_t                         nonDebugOverflowMs;
#define DEBUGBUF         nonDebugDummyBuf
#define SIZEOF_DEBUGBUF  sizeof(nonDebugDummyBuf)
#define DEBUG_BEGIN_TICK         nonDebugStartTick
#define DEBUG_END_TICK           nonDebugEndTick
#define DEBUG_TICK_OVERFLOW      nonDebugOverflow
#define DEBUG_TICK_OVERFLOW_MS   nonDebugOverflowMs
#endif //-------------------------------------------------------------------------


#define DBGPRINT_INTSTRSTR if (DEBUG_SOCKETTASK) broadcastDebug
#define DBGPRINT_INTSTRSTR_JSONPROC if (DEBUG_SOCKETTASK_JSONPROC) broadcastDebug
#define DBGPRINT_INTSTRSTR_ERROR if (DEBUG_SOCKETTASK_ERROR) broadcastDebug
#define DBGSNPRINTF if (DEBUG_SOCKETTASK) snprintf
#define DBGPRINT_INTSTRSTR_QMOTION_DETAIL if (DEBUG_SOCKETTASK_QMOTION_DETAIL) broadcastDebug
#define DBGSNPRINTF_QMOTION_DETAIL if (DEBUG_SOCKETTASK_QMOTION_DETAIL) snprintf
#define DBGPRINT_INTSTRSTR_QMOTION if (DEBUG_SOCKETTASK_QMOTION) broadcastDebug
#define DBGPRINT_INTSTRSTR_QMOTION_ERROR if (DEBUG_SOCKETTASK_QMOTION_ERROR) broadcastDebug
#define DBGSNPRINTF_QMOTION if (DEBUG_SOCKETTASK_QMOTION) snprintf
#define DBGPRINT_INTSTRSTR_SOCKET_SEND_ERROR  if (DEBUG_SOCKETTASK_SOCKET_SEND_FAIL) broadcastDebug
#define DBGPRINT_INTSTRSTR_ZONEARRAY_MUTEX if (DEBUG_ZONEARRAY_MUTEX) broadcastDebug
#define DBGPRINT_INTSTRSTR_QUBE_MUTEX if (DEBUG_QUBE_MUTEX) broadcastDebug

#define DEBUG_QUBE_MUTEX_START if (DEBUG_QUBE_MUTEX) _time_get_ticks(&DEBUG_BEGIN_TICK) 
#define DEBUG_QUBE_MUTEX_GOT   if (DEBUG_QUBE_MUTEX) _time_get_ticks(&DEBUG_END_TICK) 
#define DEBUG_QUBE_MUTEX_IS_OVERFLOW (DEBUG_QUBE_MUTEX && (DEBUG_QUBE_MUTEX_MAX_MS < (DEBUG_TICK_OVERFLOW_MS = (uint32_t) _time_diff_milliseconds(&DEBUG_END_TICK, &DEBUG_BEGIN_TICK, &DEBUG_TICK_OVERFLOW))))   
#define DBGSNPRINT_MS_STR_IF_QUBE_MUTEX_OVERFLOW if (DEBUG_QUBE_MUTEX && DEBUG_QUBE_MUTEX_IS_OVERFLOW) DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"qubeSocketMutex %dms at %s:%d",DEBUG_TICK_OVERFLOW_MS, __PRETTY_FUNCTION__, __LINE__); else DEBUGBUF[0] = 0; 
#if DEBUG_QUBE_MUTEX
#define DBGPRINT_INTSTRSTR_IF_QUBE_MUTEX_OVERFLOW  DEBUG_QUBE_MUTEX_GOT; DBGSNPRINT_MS_STR_IF_QUBE_MUTEX_OVERFLOW; if (DEBUG_QUBE_MUTEX && DEBUG_TICK_OVERFLOW) broadcastDebug 
#else
#define DBGPRINT_INTSTRSTR_IF_QUBE_MUTEX_OVERFLOW  if (DEBUG_QUBE_MUTEX) broadcastDebug // optimizes away, no warnings 
#endif

#define DEBUG_ZONEARRAY_MUTEX_START if (DEBUG_ZONEARRAY_MUTEX) _time_get_ticks(&DEBUG_BEGIN_TICK) 
#define DEBUG_ZONEARRAY_MUTEX_GOT   if (DEBUG_ZONEARRAY_MUTEX) _time_get_ticks(&DEBUG_END_TICK) 
#define DEBUG_ZONEARRAY_MUTEX_IS_OVERFLOW (DEBUG_ZONEARRAY_MUTEX && (DEBUG_ZONEARRAY_MUTEX_MAX_MS < (DEBUG_TICK_OVERFLOW_MS = (uint32_t) _time_diff_milliseconds(&DEBUG_END_TICK, &DEBUG_BEGIN_TICK, &DEBUG_TICK_OVERFLOW))))   
#define DBGSNPRINT_MS_STR_IF_ZONEARRAY_MUTEX_OVERFLOW if (DEBUG_ZONEARRAY_MUTEX && DEBUG_ZONEARRAY_MUTEX_IS_OVERFLOW) DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"zoneArrayMutex %dms at %s:%d",DEBUG_TICK_OVERFLOW_MS, __PRETTY_FUNCTION__, __LINE__); else DEBUGBUF[0] = 0; 
#if DEBUG_ZONEARRAY_MUTEX
#define DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW  DEBUG_ZONEARRAY_MUTEX_GOT; DBGSNPRINT_MS_STR_IF_ZONEARRAY_MUTEX_OVERFLOW; if (DEBUG_ZONEARRAY_MUTEX_IS_OVERFLOW) broadcastDebug 
#else
#define DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW  if (DEBUG_ZONEARRAY_MUTEX) broadcastDebug 
#endif

//==================================================================================================


// QMotion shades communication tuning parameters
#define QMOTION_HANDLE_DEVICES_INFO_RESPONSE_DELAY_MS    15 // new, 10 helped with DEBUG_SOCKETTASK_QMOTION_DETAIL enabled, not enough without
#define QMOTION_POST_REQUEST_DISCOVERY_DELAY_MS    10 // orig=10
#define QMOTION_RECV_RESPONSE_MAX_RETRIES          3  // orig=2
#define QMOTION_RECV_RESPONSE_RETRY_DELAY_MS       10 // orig=10
#define QMOTION_PRE_RECV_DISCOVERY_PAYLOAD_DELAY_MS 3 // added
#define QMOTION_RECV_DISCOVERY_MAX_RETRIES         10 // orig=10
#define QMOTION_RECV_DISCOVERY_RETRY_DELAY_MS      10 // orig=10
#define QMOTION_PRE_RECV_PAYLOAD_DELAY_MS          3 // orig=10
#define QMOTION_RECV_JSON_PAYLOAD_DELAY_MS         10 // orig=10
#define QMOTION_RECV_JSON_PAYLOAD_MAX_RETRIES      10 // orig=10
#define SOCKETTASK_RECV_PAYLOAD_DELAY_MS           10 // orig=10
#define SOCKETTASK_RECV_PAYLOAD_MAX_RETRIES        10 // orig=10

#define QMOTION_UDP_PORT 30122

// Zone Bit Arrays
extern byte_t   lastLowBatteryBitArray[ZONE_BIT_ARRAY_BYTES];
extern byte_t   zoneChangedThisMinuteBitArray[ZONE_BIT_ARRAY_BYTES];
extern unsigned long malloc_count;
extern unsigned long free_count;
extern unsigned long malloc_fail_count;

IPCFG_IP_ADDRESS_DATA current_ip_data;
uint32_t MDNSlistensock = 0;
static int32_t socketPingSeqno[MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS];

// Within Socket_Task(), socketIndexGlobalRx is set to socketIndex prior
// to calling ProcessJsonPacket().  This allows command handlers to see which
// socket the command came from, which is needed by JSA_HandleKeysProperty().
byte_t socketIndexGlobalRx = 0;


#else
#include "jansson.h"
#include <string.h>
#include <stdio.h>
#endif

#define PASSWORD_LABEL_LENGTH 8
#define MAX_WATCH_TIME_DIFF 5

json_socket_key json_socket_factory_key;  // Hash of password_label
char password_label[PASSWORD_LABEL_LENGTH+1];  // Derived from unique IDs in the K64

char jsonErrorString[50];

char * UpdateStateStrings[4] = 
{
   "None",
   "Downloading",
   "Ready",
   "Installing"
};
  
firmware_info  updateFirmwareInfo;     // contains information on the downloaded (or downloading) firmware.

#define MAX_QMOTION_BUFFER 3000 // orig 3000

uint8_t        qmotionBuffer[MAX_QMOTION_BUFFER];
uint_32        QSock = 0;

#ifndef AMDBUILD
// Begin MQX Only Build Section



volatile byte_t globalJsonSocketIndex;
volatile word_t globalCount = 0;

#ifdef SOCKET_MONITOR_TASK
// each json transmit thread has a queue
JSON_transmit_message_queue   JSONTransmitMessageQueue[MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS];
#endif
// worst case, a different message for each slot in each transmit thread's queue
JSON_transmit_message         JSONTransmitMessage[MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS * NUMBER_OF_JSON_TRANSMIT_MESSAGES_IN_QUEUE];
word_t                        currentMessageIndex;


static MUTEX_STRUCT JSONTransmitMutex;

char     updateBuf[680];
uint_32  updateSock = 0;
char     updateServerHostname[MAX_HOSTNAMESIZE];

#ifdef MEMORY_DIAGNOSTIC_PRINTF
dword_t globalShowMemoryFlag = 0;
char diagnosticsBuf[100];
#endif

mtm_input_sense_properties  MTMInputSense[NUMBER_OF_MTM_INPUTS];

const char * MTMInputStrings[NUMBER_OF_MTM_INPUTS] = 
{
   "PUSH BUTTON S2 (Factory Reset)",
   "PUSH BUTTON S3 (Spare PB Left)",
   "PUSH BUTTON S4 (Spare PB Right)",
   "IN0",
   "IN1"
};

LWGPIO_STRUCT * MTMInputs[NUMBER_OF_MTM_INPUTS] = 
{
   &factoryResetInput,
   &spareS3Input,
   &spareS4Input,
   &IN0Input,
   &IN1Input
};

const char * MTMOutputStrings[NUMBER_OF_MTM_OUTPUTS] = 
{
   "LED1-GR",
   "LED1-RED",
   "LED2-GR",
   "LED2-RED",
   "OUT0",
   "OUT1"
};

LWGPIO_STRUCT * MTMOutputs[NUMBER_OF_MTM_OUTPUTS] = 
{
   &led1Green,
   &led1Red,
   &led2Green,
   &led2Red,
   &OUT0Output,
   &OUT1Output
};

// APPLE Watch buffers
char httpBuf[450];

char httpStr[4500]; 
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

byte_t ReadMTMInput(byte_t whichInput)
{
   byte_t inputSense = false;
   
   if (whichInput < NUMBER_OF_MTM_INPUTS)
   {
      inputSense = !lwgpio_get_value(MTMInputs[whichInput]);
   }
   
   return inputSense;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// JSA-prefixed functions are related to JSON socket authentication.


// JSA_SendMAC prints the LCM's MAC address in JSON format, directly to a
// socket.  This is useful where broadcastDebug output is not available.
uint32_t JSA_SendMAC(int sockfd)
{
	char buf[24];

	snprintf(buf, sizeof(buf), "{\"MAC\":\"%02X%02X%02X%02X%02X%02X\"}",
		myMACAddress[0], myMACAddress[1], myMACAddress[2],
		myMACAddress[3], myMACAddress[4], myMACAddress[5]);

	return Socket_send(sockfd, buf, _strnlen(buf, sizeof(buf)), 0, NULL, __PRETTY_FUNCTION__);
}


// This debug function prints 16-byte blocks as ASCII hex.
void JSA_PrintHex(uint8_t* b, char *description)
{
#if DEBUG_SOCKETTASK_AUTH
	int x=0;
	char dbg[40];

	for(x=0; x<16; x++)
	{
		sprintf(&dbg[x*2], "%02X", b[x]);
	}

	DBGPRINT_INTSTRSTR(0, description, dbg);
#endif
}


// Generates a 128-bit private factory key from the password/label.
// GenerateFactoryPassword() must be called before GenerateFactoryKey().
void JSA_GenerateFactoryKey(void)
{
	struct Md5 md5;
	wc_InitMd5(&md5);
	wc_Md5Update(&md5, (const byte*)&password_label, PASSWORD_LABEL_LENGTH);
	wc_Md5Final(&md5, (byte*)&json_socket_factory_key.content);
}


// JSA_ClearFactoryAuthInit() sets the factory_auth_init value to the bitwise
// NOT of its magic value, unsetting the factoryAuthInit "flag".  The debug
// jumper must be installed for it to work.  This is the only method besides
// a programmer that can unset the flag since it's stored in non-resettable
// flash.
uint8_t JSA_ClearFactoryAuthInit()
{
	json_t *broadcastObject = json_object();

	if(DEBUG_JUMPER)
	{
		extendedSystemProperties.factory_auth_init = (uint16_t)~FACTORY_AUTH_INIT_ENABLED;
		SaveSystemPropertiesToFlash();

		if(broadcastObject)
		{
			json_object_set_new(broadcastObject, "ClearFactoryAuthInitResponse", json_string("OK"));
			SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
			json_decref(broadcastObject);
		}
		return 0;
	}
	else
	{
		if(broadcastObject)
		{
			json_object_set_new(broadcastObject, "ClearFactoryAuthInitResponse", json_string("Denied"));
			SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
			json_decref(broadcastObject);
		}
		return -1;
	}
}


// JSA_GetFactoryPassword() prints the factory password string to the
// JSON socket in response to a factory query if either condition is true:
//   1.  The debug jumper is installed
//   2.  The MAC address has not been changed from its default
// The factory_auth_init signature value will be written to flash.
// This forces a challenge using the factory key if the user key is undefined.
uint8_t JSA_GetFactoryPassword()
{
	uint8_t default_mac[] = DEFAULT_MAC_ADDRESS;
	json_t *broadcastObject = json_object();
	uint8_t ret = 0;

	if (broadcastObject)
	{
		if( memcmp(&default_mac, myMACAddress, sizeof(default_mac)) && !DEBUG_JUMPER )
		{
			json_object_set_new(broadcastObject, "GetFactoryPasswordFailure", json_string("MacSet"));
			ret = -1;
		}
		else
		{
			json_object_set_new(broadcastObject, "GetFactoryPasswordResponse", json_string(password_label));

			// Set the factory_auth_init signature value in flash.
			if(extendedSystemProperties.factory_auth_init != FACTORY_AUTH_INIT_ENABLED)
			{
				extendedSystemProperties.factory_auth_init = FACTORY_AUTH_INIT_ENABLED;
				SaveSystemPropertiesToFlash();
			}
		}
		SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
		json_decref(broadcastObject);
	}
	else
	{
		ret = -2;
	}

	return ret;
}


// Return if the user has set a socket key.  Verify the key
// content against its checksum and return true on a match.
bool_t JSA_UserKeyIsSet()
{
	uint8_t idx = 0;
	json_socket_key* user = (json_socket_key*)FBK_Address(&eliot_fbk, LCM_DATA_JSON_SOCKET_USER_KEY_OFFSET);
	uint16_t acc = FLASH_CHECKSUM_INIT;

	for(idx=0; idx<MD5_DIGEST_SIZE>>1; idx++)
	{
		acc += user->content.w[idx];
	}

	return user->checksum == acc;
}


// JSA_WriteUserKeyToFlash() accepts a pointer to a json_socket_key
// struct, containing both a 16 byte key and a 2 byte checksum.  It
// calculates a checksum, writes it to the struct and writes all 18
// bytes to flash.
void JSA_WriteUserKeyToFlash(json_socket_key *new)
{
	uint8_t idx = 0;
	uint16_t acc = FLASH_CHECKSUM_INIT;

	for(idx=0; idx<sizeof(new->content.b)>>1; idx++)
	{
		acc += new->content.w[idx];
	}
	new->checksum = acc;

	FBK_WriteFlash(&eliot_fbk, (char*)new, LCM_DATA_JSON_SOCKET_USER_KEY_OFFSET, sizeof(json_socket_key));
	DBGPRINT_INTSTRSTR(LCM_DATA_JSON_SOCKET_USER_KEY_OFFSET, "JSA_WriteUserKeyToFlash() - Wrote key to flash", "");
}


// JSA_ClearUserKeyInFlash() zeros the region in flash where the
// user key is stored, rendering it invalid.
void JSA_ClearUserKeyInFlash()
{
	uint8_t set[sizeof(json_socket_key)] = {0};
	FBK_WriteFlash(&eliot_fbk, (char*)&set, LCM_DATA_JSON_SOCKET_USER_KEY_OFFSET, sizeof(json_socket_key));
	DBGPRINT_INTSTRSTR(LCM_DATA_JSON_SOCKET_USER_KEY_OFFSET, "JSA_ClearUserKeyInFlash() - User key cleared", "");
}


// JSA_HandleKeysProperty() receives a 64 character string from
// HandleIndividualSystemProperty() containing the current/old key
// and the new key, both encrypted using the current/old key which
// can be a unique factory-enabled key, a user defined key or the
// MD5 sum of an empty string in the case of LCMs unsecured from
// the factory.  If the decrypted old key matches the LCMs local
// key, the decrypted new key is adopted as the new local key
// and written to flash.
uint8_t JSA_HandleKeysProperty(char *key_set_string, json_t *responseObject)
{
	uint8_t x = 0;
	uint8_t str_idx = 0;
	uint8_t nibble = 0;
	uint8_t zero_iv[16]={0};

	// This will point to one of three possible keys
	json_socket_key *local = 0;

	// Becomes MD5 sum of an empty data set: D41D8CD98F00B204E9800998ECF8427E
	json_socket_key empty = {0};

	// The encrypted old password as received from the app.
	json_socket_key old_encrypted = {0};

	// The encrypted new password as received from the app.
	json_socket_key new_encrypted = {0};

	// The app's old password decrypted with the LCM's local key.
	json_socket_key old_decrypted = {0};

	// The app's new password decrypted with the LCM's local key.  This will
	// become the new local key if old_decrypted matches *local.
	json_socket_key new_decrypted = {0};

	DBGPRINT_INTSTRSTR(0, "JSA_HandleKeysProperty() - Received string:", key_set_string);

	if( (x=strlen(key_set_string)) != MD5_DIGEST_SIZE * 4)
	{
		if(responseObject)
		{
			json_object_set_new(responseObject, "Status", json_string("Error"));
			json_object_set_new(responseObject, "ErrorText", json_string("Bad length"));
			json_object_set_new(responseObject, "ErrorCode", json_integer(-2));
		}

		DBGPRINT_INTSTRSTR(x, "JSA_HandleKeysProperty() - Bad argument length", "");
		return -1;
	}

	// Parse the incoming ASCII hex
	for(str_idx=0; str_idx<32; str_idx++)
	{
		x = key_set_string[str_idx + 0];
		nibble = x < 'A' ? (x-'0') : ((0x20|x)-87);  // ASCII hex to nibble conversion (upper or lower case)
		old_encrypted.content.b[(str_idx>>1) & (MD5_DIGEST_SIZE-1)] |= nibble << (str_idx&1 ? 0:4);

		x = key_set_string[str_idx + 32];
		nibble = x < 'A' ? (x-'0') : ((0x20|x)-87);  // ASCII hex to nibble conversion (upper or lower case)
		new_encrypted.content.b[(str_idx>>1) & (MD5_DIGEST_SIZE-1)] |= nibble << (str_idx&1 ? 0:4);
	}

	// Decide which is the current local key:
	//  1.  User key in flash
	//  2.  Factory key in RAM
	//  3.  Empty key (MD5 hash of an empty string)

	// Has the user set a key?
	if(JSA_UserKeyIsSet())
	{
		// User key is set, use it.
		DBGPRINT_INTSTRSTR(0, "JSA_HandleKeysProperty() - Using user key", "");
		local = (json_socket_key*)FBK_Address(&eliot_fbk, LCM_DATA_JSON_SOCKET_USER_KEY_OFFSET);
	}
	else
	{
		// The user has not set a key.  Depending on factory_auth_init, fall
		// back to Md5(label password) or Md5("").
		if(extendedSystemProperties.factory_auth_init == FACTORY_AUTH_INIT_ENABLED)
		{
			// Use the factory key
			DBGPRINT_INTSTRSTR(0, "JSA_HandleKeysProperty() - Using factory key", "");
			local = &json_socket_factory_key;
		}
		else
		{
			// Build and use the empty key

			struct Md5 md5;
			wc_InitMd5(&md5);
			wc_Md5Update(&md5, 0, 0);
			wc_Md5Final(&md5, (byte*)&empty.content);
			local = &empty;

			DBGPRINT_INTSTRSTR(0, "JSA_HandleKeysProperty() - Using empty key", "");
		}
	}

	JSA_PrintHex((uint8_t*)local, "JSA_HandleKeysProperty() - Local key content:");

	// Decrypt the key pair we've received with our local key and check if the decrypted
	// old key matches it.

	wc_AesCbcDecryptWithKey(&old_decrypted, &old_encrypted, 16, local, 16, zero_iv);
	JSA_PrintHex((uint8_t*)&old_decrypted, "JSA_HandleKeysProperty() - Decrypted old key content:");

	if(memcmp(&old_decrypted, local, sizeof(local->content)))
	{
		if(responseObject)
		{
			json_object_set_new(responseObject, "Status", json_string("Error"));
			json_object_set_new(responseObject, "ErrorText", json_string("Old key mismatch"));
			json_object_set_new(responseObject, "ErrorCode", json_integer(-1));
		}

		DBGPRINT_INTSTRSTR(0, "JSA_HandleKeysProperty() - Old key mismatch, exiting.", "");
		return -1;
	}

	// Old key is valid.  Decrypt the new key and write it to flash.

	wc_AesCbcDecryptWithKey(&new_decrypted, &new_encrypted, 16, local, 16, zero_iv);
	JSA_PrintHex((uint8_t*)&new_decrypted, "JSA_HandleKeysProperty() - Old key is valid - Decrypted new key content:");
	JSA_WriteUserKeyToFlash(&new_decrypted);

	if(responseObject)
	{
		json_object_set_new(responseObject, "Status", json_string("Success"));
		json_object_set_new(responseObject, "ErrorCode", json_integer(0));
	}

	// If this socket is in a limited set-key mode, enable full communications.
	if(jsonSockets[socketIndexGlobalRx].jsa_state & JSA_MASK_JSON_LIMITED)
	{
		jsonSockets[socketIndexGlobalRx].jsa_state = JSA_STATE_AUTH_DEFEATED;
	}

	// Shut down all sockets except socketIndexGlobalRx
	for(x=0; x<MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS; x++)
	{
		if(x != socketIndexGlobalRx)
		{
			ShutdownSocket(x, 0);
		}
	}

	return 0;
}


// This function generates the default eight character password
// printed on the LCM during manufacture.  The factory key is
// derived from this password.
void JSA_GenerateFactoryPassword(void)
{
	union
	{
		uint32_t u32[3];
		uint8_t md5[16];
	} packed_password;

	uint8_t char_idx=0;
	struct Md5 md5;

	memset(&packed_password, 0, sizeof(packed_password));
	memset(&password_label, 0, sizeof(password_label));

	// Unique hardware ID
	packed_password.u32[0] = SIM_UIDL;    // 32 effective bits
	packed_password.u32[1] = SIM_UIDML;   // 32 effective bits
	packed_password.u32[2] = SIM_UIDMH;   // 16 effective bits

	wc_InitMd5(&md5);
	wc_Md5Update(&md5, (const byte*)&packed_password, sizeof(packed_password.u32));
	wc_Md5Final(&md5, (byte*)&packed_password);

	for(char_idx=0; char_idx<PASSWORD_LABEL_LENGTH; char_idx++)
	{
		password_label[char_idx] = PASSWORD_ALPHABET[packed_password.md5[char_idx] & (PASSWORD_ALPHABET_SIZE-1)];
	}
}


// JSA_AuthExemptTask() is called within the main socket task loop and sets
// the auth_exempt value in non-resettable flash.
void JSA_SetAuthExemptTask()
{
	// Is auth_exempt undefined?
	if(extendedSystemProperties.auth_exempt != AUTH_EXEMPT && extendedSystemProperties.auth_exempt != AUTH_NOT_EXEMPT)
	{
		if(AnyZonesInArray()) // Do we have at least one zone?
		{
			DBGPRINT_ERRORS_INTSTRSTR(__LINE__, "JSA_AuthExemptTask()","Setting AUTH_EXEMPT");
			extendedSystemProperties.auth_exempt = AUTH_EXEMPT;
		}
		else // No zones added
		{
			DBGPRINT_ERRORS_INTSTRSTR(__LINE__, "JSA_AuthExemptTask()","Setting AUTH_NOT_EXEMPT");
			extendedSystemProperties.auth_exempt = AUTH_NOT_EXEMPT;
		}
		SaveSystemPropertiesToFlash();
	}
}


// JSA_FirstTimeRecordedIsValid() returns true if the Unix time in
// extendedSystemProperties.first_time_recorded is valid.
bool_t JSA_FirstTimeRecordedIsValid()
{
	return (extendedSystemProperties.first_time_recorded == ~extendedSystemProperties.first_time_recorded_complement);
}


// JSA_SetFirstTimeRecordedTask() is called within the main socket task loop and
// sets extendedSystemProperties.first_time_recorded and its complement to the
// current Unix time, if current Unix time is valid.  This value is written only
// once in non-resettable memory through the LCM's lifetime as a record of when the
// LCM was first placed into service.  The AnyZonesInArray() check prevents the
// recording of a time during manufacture.
void JSA_SetFirstTimeRecordedTask()
{
	if(!JSA_FirstTimeRecordedIsValid())
	{
		uint32_t time_now = Eliot_TimeNow();
		if(time_now > JSA_VALID_TIME_MIN)
		{
			if(AnyZonesInArray()) // Do we have at least one zone?
			{
				DBGPRINT_ERRORS_INTSTRSTR(time_now, "JSA_FirstTimeRecordedTask()","Setting first time recorded");
				extendedSystemProperties.first_time_recorded = time_now;
				extendedSystemProperties.first_time_recorded_complement = ~extendedSystemProperties.first_time_recorded;
				SaveSystemPropertiesToFlash();
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool_t TestWatchAuth(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr)
{
	json_t *jsonObj;
	int messageTime, delta;
	const char *challengeStr;
	bool_t authPass = true;

	#ifdef _WATCH_NO_AUTH
		return true;
	#endif

	//if authentication is disabled, return true
	if((extendedSystemProperties.factory_auth_init != FACTORY_AUTH_INIT_ENABLED) && !JSA_UserKeyIsSet()) return true;

	//Get Current Time
	TIME_STRUCT time;
	_time_get(&time);

	//Get message time
	jsonObj = json_object_get(root, "currTime");
	if (!jsonObj) authPass = false;
	if (json_is_integer(jsonObj) && authPass)
	{
		messageTime = (json_integer_value(jsonObj));
		delta = (time.SECONDS - systemProperties.effectiveGmtOffsetSeconds) - messageTime;
		if (delta < -MAX_WATCH_TIME_DIFF || delta > MAX_WATCH_TIME_DIFF) authPass=false;
	}
	else authPass = false;

	//Get Challenge time
	if (authPass)
	{
		jsonObj = NULL;
		jsonObj = json_object_get(root, "auth");
		if (!jsonObj) authPass = false;
		if (json_is_string(jsonObj) && authPass)
		{
			challengeStr = (json_string_value(jsonObj));
			//Make sure string is 32 Chars long
			if((strlen(challengeStr)) != MD5_DIGEST_SIZE * 2) authPass = false;
		}
		else authPass = false;
	}

	//Decrypt Challenge
	if (authPass)
	{
		// The current crypto key
		json_socket_key *local = 0;

		// The encrypted Challenge String
		json_socket_key str_encrypted = {0};

		// The Challenge String Decrypted
		char str_decrypted[32];

		Aes aes;
		uint8_t nibble;
		uint8_t x = 0;
		uint8_t zero_iv[16]={0};

		//Get Key
		if(JSA_UserKeyIsSet())
		{
			// User key is set, use it.
			DBGPRINT_INTSTRSTR(0, "JSA_HandleKeysProperty() - Using user key", "");
			local = (json_socket_key*)FBK_Address(&eliot_fbk, LCM_DATA_JSON_SOCKET_USER_KEY_OFFSET);
		}
		else
		{
			DBGPRINT_INTSTRSTR(0, "JSA_HandleKeysProperty() - Using factory key", "");
			local = &json_socket_factory_key;
		}
		wc_AesSetKey(&aes, (byte*)&local->content, sizeof(local->content), 0, AES_ENCRYPTION);
		
		// Parse the ASCII hex
		for(int str_idx=0; str_idx<MD5_DIGEST_SIZE * 2; str_idx++)
		{
			x = challengeStr[str_idx];
			nibble = x < 'A' ? (x-'0') : ((0x20|x)-87);  // ASCII hex to nibble conversion (upper or lower case)
			str_encrypted.content.b[(str_idx>>1) & (MD5_DIGEST_SIZE-1)] |= nibble << (str_idx&1 ? 0:4);
		}

		//Decrypt
		wc_AesCbcDecryptWithKey(&str_decrypted, &str_encrypted, 16, local, 16, zero_iv);
		int decryptTime = atoi(str_decrypted);

		authPass = (messageTime == decryptTime);
	}

	//Return true if passed, else send failure message and return false
	if (authPass) return true;
	else
	{
		json_t *broadcastObject = json_object();

		if (broadcastObject)
		{
			json_object_set_new(broadcastObject, "AuthFailure", json_string("True"));
			SendJSONPacket(socketIndexGlobalRx, broadcastObject);
			json_decref(broadcastObject);
		}
		return false;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void InitializeMTMInputSense(void)
{
   byte_t index;
   
   for (index = 0; index < NUMBER_OF_MTM_INPUTS; index++)
   {
      MTMInputSense[index].inputSenseFlag = ReadMTMInput(index);
      MTMInputSense[index].inputSenseDebounceCount = 0;
      MTMInputSense[index].inputSenseTimerTicks = 0;
   }

}   
   
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void SendBroadcastMessageForMTMInputSense(byte_t inputIndex, bool_t stateFlag)
{

   if (inputIndex < NUMBER_OF_MTM_INPUTS)
   {
      json_t * inputSenseStateChangeObject;

      inputSenseStateChangeObject = json_object();
      if (inputSenseStateChangeObject)
      {
         json_object_set_new(inputSenseStateChangeObject, "ID", json_integer(0));
         json_object_set_new(inputSenseStateChangeObject, "Service", json_string("InputSenseStateChange"));
         json_object_set_new(inputSenseStateChangeObject, "Input", json_string(MTMInputStrings[inputIndex]));
   
         if (stateFlag)
         {
            json_object_set_new(inputSenseStateChangeObject, "State", json_true());
         }
         else
         {
            json_object_set_new(inputSenseStateChangeObject, "State", json_false());
         }
   
         json_object_set_new(inputSenseStateChangeObject, "Status", json_string("Success"));
   
         SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, inputSenseStateChangeObject);
         json_decref(inputSenseStateChangeObject);
      }
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleMTMInputSense(onqtimer_t tickCount)
{
   byte_t   inputSenseFlag;
   byte_t   inputIndex;

   for (inputIndex = 0; inputIndex < NUMBER_OF_MTM_INPUTS; inputIndex++)
   {
      MTMInputSense[inputIndex].inputSenseTimerTicks += tickCount;
      inputSenseFlag = ReadMTMInput(inputIndex);

      if (MTMInputSense[inputIndex].inputSenseFlag)
      {  // we were already active
         if (!inputSenseFlag)
         {  // and it's now showing inactive
            MTMInputSense[inputIndex].inputSenseDebounceCount += MTMInputSense[inputIndex].inputSenseTimerTicks;
            if (MTMInputSense[inputIndex].inputSenseDebounceCount > MTM_INPUT_INACTIVE_DEBOUNCE_TIME)
            {
               MTMInputSense[inputIndex].inputSenseDebounceCount = 0;
               MTMInputSense[inputIndex].inputSenseFlag = false;

               SendBroadcastMessageForMTMInputSense(inputIndex, false);
            }
         }
         else
         {  // still active - reset debounce count
            MTMInputSense[inputIndex].inputSenseDebounceCount = 0;
         }
      }
      else
      {  // we were inactive
         if (inputSenseFlag)
         {  // and now we're active
            MTMInputSense[inputIndex].inputSenseDebounceCount += MTMInputSense[inputIndex].inputSenseTimerTicks;
            if (MTMInputSense[inputIndex].inputSenseDebounceCount > MTM_INPUT_ACTIVE_DEBOUNCE_TIME)
            {
               MTMInputSense[inputIndex].inputSenseDebounceCount = 0;
               MTMInputSense[inputIndex].inputSenseFlag = true;
               
               SendBroadcastMessageForMTMInputSense(inputIndex, true);
            }
         }
         else
         {  // still inactive - reset debounce count
            MTMInputSense[inputIndex].inputSenseDebounceCount = 0;
         }
      }
      MTMInputSense[inputIndex].inputSenseTimerTicks = 0;
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleManufacturingTestMode(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr)
{
   json_t * stateObject;

   stateObject = json_object_get(root, "State");
   if (!json_is_boolean(stateObject))
   {
      BuildErrorResponse(responseObject, "'State' field not a boolean.", MTM_STATE_FIELD_NOT_BOOLEAN);
      return;
   }

   manufacturingTestModeFlag = json_is_true(stateObject);
   if (manufacturingTestModeFlag)
   {
      SetGPIOOutput(led1Green, LED_OFF);
      SetGPIOOutput(led1Red, LED_OFF);
      SetGPIOOutput(led2Green, LED_OFF);
      SetGPIOOutput(led2Red, LED_OFF);
      InitializeMTMInputSense(); // initialize the input sense
   }
   
   // if we don't have an error, then prepare the asynchronous message
   // to all json sockets.
   *jsonBroadcastObjectPtr = json_copy(root);
   json_object_set_new(*jsonBroadcastObjectPtr, "ID", json_integer(0));
   json_object_set_new(*jsonBroadcastObjectPtr, "Status", json_string("Success"));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleMTMSendRFPacket(json_t * root, json_t * responseObject)
{
   json_t *   localObject;
   json_t *   localArrayElementObject;
   json_int_t localInt;
   size_t     arrayIndex;
   byte_t     xmitBuffer[32];
   byte_t     xmitBufferIndex = 0;
   byte_t     arraySize;
   byte_t     crcValue;

   xmitBuffer[xmitBufferIndex++] = 0x86;  // Command 0x86: set bit TxF and TxB in cmd reg 0 
   xmitBuffer[xmitBufferIndex++] = 0xb4;  // buffer header 
   xmitBufferIndex++;  // Length Byte - skip for now, but go back when we have the final buffer size
         
   // fill in the header
   localObject = json_object_get(root, "Header");
   if (!json_is_integer(localObject))
   {
      BuildErrorResponse(responseObject, "'Header' field not an integer.", MTM_HEADER_FIELD_NOT_AN_INTEGER);
      return;
   }
   localInt = json_integer_value(localObject);
   if ((localInt < 0) || (localInt > 0xFF))
   {
      BuildErrorResponse(responseObject, "'Header' field must be 0 through 255.", MTM_HEADER_FIELD_MUST_BE_BETWEEN);
      return;
   }
   xmitBuffer[xmitBufferIndex++] = (byte_t) localInt;
         
   // fill in the destination address
   localObject = json_object_get(root, "Destination");
   if (!json_is_array(localObject))
   {
      BuildErrorResponse(responseObject, "'Destination' not an array.", MTM_DESTINATION_NOT_AN_ARRAY);
      return;
   }

   if (json_array_size(localObject) != 3)
   {
      BuildErrorResponse(responseObject, "'Destination' array does not contain 3 bytes.", MTM_DESTINATION_ARRAY_INCORRECT_BYTE_COUNT);
      return;
   }
   for (arrayIndex = 0; arrayIndex < 3; arrayIndex++)
   {
      localArrayElementObject = json_array_get(localObject, arrayIndex);
      if (!json_is_integer(localArrayElementObject))
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "Destination byte %d not an integer.", arrayIndex);
         BuildErrorResponse(responseObject, jsonErrorString, MTM_DESTINATION_BYTE_NOT_AN_INTEGER);
         return;
      }
      localInt = json_integer_value(localArrayElementObject);
      if ((localInt < 0) || (localInt > 0xFF))
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "Destination byte %d invalid: must be 0-255.", arrayIndex);
         BuildErrorResponse(responseObject, jsonErrorString, MTM_DESTINATION_BYTE_IS_INVALID);
         return;
      }
      xmitBuffer[xmitBufferIndex++] = (byte_t) localInt;
   }
      
   // fill in the payload
   localObject = json_object_get(root, "Payload");
   if (!json_is_array(localObject))
   {
      BuildErrorResponse(responseObject, "'Payload' not an array.", MTM_PAYLOAD_NOT_AN_ARRAY);
      return;
   }

   arraySize = json_array_size(localObject);
   switch (arraySize)
   {
      case 1:
         xmitBuffer[xmitBufferIndex] = 0x00;
         break;
         
      case 2:
         xmitBuffer[xmitBufferIndex] = 0x40;
         break;

      case 4:
         xmitBuffer[xmitBufferIndex] = 0x80;
         break;

      case 6:
         xmitBuffer[xmitBufferIndex] = 0xC0;
         break;

      default:
         BuildErrorResponse(responseObject, "'Payload' array must contain 1, 2, 4 or 6 bytes.", MTM_PAYLOAD_ARRAY_INVALID);
         return;
   }
   for (arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
   {
      localArrayElementObject = json_array_get(localObject, arrayIndex);
      if (!json_is_integer(localArrayElementObject))
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "Payload byte %d not an integer.", arrayIndex);
         BuildErrorResponse(responseObject, jsonErrorString, MTM_PAYLOAD_BYTE_NOT_AN_INTEGER);
         return;
      }
      localInt = json_integer_value(localArrayElementObject);
      if (!arrayIndex)
      {  // special case for the first payload byte - must be less than 0x40
         if ((localInt < 0) || (localInt > 0x3F))
         {
            BuildErrorResponse(responseObject, "Payload byte 1 invalid: must be 0 - 63.", MTM_PAYLOAD_BYTE_1_INVALID_SHOULD_BE_0_63);
            return;
         }
         xmitBuffer[xmitBufferIndex++] |= (byte_t) localInt;
      }
      else
      {
         if ((localInt < 0) || (localInt > 0xFF))
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "Payload byte %d invalid: must be 0-255.", arrayIndex);
            BuildErrorResponse(responseObject, jsonErrorString, MTM_PAYLOAD_BYTE_X_INVALID_SHOULD_BE_0_255);
            return;
         }
         xmitBuffer[xmitBufferIndex++] = (byte_t) localInt;
      }
   }
      
   // check for optional second address
   localObject = json_object_get(root, "SecondAddress");
   if (localObject)
   {
      if (!json_is_array(localObject))
      {
         BuildErrorResponse(responseObject, "'Destination' not an array.", MTM_DESTINATION_NOT_AN_ARRAY);
         return;
      }
   
      if (json_array_size(localObject) != 3)
      {
         BuildErrorResponse(responseObject, "'Destination' array does not contain 3 bytes.", MTM_DESTINATION_ARRAY_INCORRECT_BYTE_COUNT);
         return;
      }
      for (arrayIndex = 0; arrayIndex < 3; arrayIndex++)
      {
         localArrayElementObject = json_array_get(localObject, arrayIndex);
         if (!json_is_integer(localArrayElementObject))
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "Destination byte %d not an integer.", arrayIndex);
            BuildErrorResponse(responseObject, jsonErrorString, MTM_DESTINATION_BYTE_NOT_AN_INTEGER);
            return;
         }
         localInt = json_integer_value(localArrayElementObject);
         if ((localInt < 0) || (localInt > 0xFF))
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "Destination byte %d invalid: must be 0-255.", arrayIndex);
            BuildErrorResponse(responseObject, jsonErrorString, MTM_DESTINATION_BYTE_IS_INVALID);
            return;
         }
         xmitBuffer[xmitBufferIndex++] = (byte_t) localInt;
      }
   }
   
   crcValue = UpdateCRC8(0, &xmitBuffer[3], (xmitBufferIndex - 3));

   xmitBuffer[xmitBufferIndex++] = crcValue;
   xmitBuffer[xmitBufferIndex++] = ~crcValue;

   // now, add the length into the packet, don't include the first 3 bytes
   xmitBuffer[2] = xmitBufferIndex - 3;
   
   (void) RFM100_Send(xmitBuffer, xmitBufferIndex);

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

byte_t MatchStringList(const char ** stringListPtr, char * stringPtr, byte_t startIndex, byte_t endIndex)
{
   byte_t index;
   
   index = startIndex;
   while (index <= endIndex)
   {
      if (!strcmp(stringPtr, stringListPtr[index]))
      {  // found a match
         break;
      }
      index++;
   }
   
   return index;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleMTMSetOutput(json_t * root, json_t * responseObject)
{
   json_t *       outputObject;
   json_t *       stateObject;
   const char *   outputString;
   byte_t         outputIndex;

   outputObject = json_object_get(root, "Output");
   if (!json_is_string(outputObject))
   {
      BuildErrorResponse(responseObject, "'Output' field not a string.", MTM_OUTPUT_FIELD_NOT_A_STRING);
      return;
   }
   outputString = json_string_value(outputObject);
   outputIndex = MatchStringList(MTMOutputStrings, (char *) outputString, 0, (NUMBER_OF_MTM_OUTPUTS - 1));
   if (outputIndex >= NUMBER_OF_MTM_OUTPUTS)
   {
      snprintf(jsonErrorString, sizeof(jsonErrorString), "Unknown 'Output' %s.", outputString);
      BuildErrorResponse(responseObject, jsonErrorString, MTM_UNKNOWN_OUTPUT_STRING);
      return;
   }

   stateObject = json_object_get(root, "State");
   if (!json_is_boolean(stateObject))
   {
      BuildErrorResponse(responseObject, "'State' field not a boolean.", MTM_STATE_FIELD_NOT_BOOLEAN);
      return;
   }

   if (json_is_true(stateObject))
   {
      lwgpio_set_value(MTMOutputs[outputIndex], LWGPIO_VALUE_LOW);
   }
   else
   {
      lwgpio_set_value(MTMOutputs[outputIndex], LWGPIO_VALUE_HIGH);
   }
   
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleMTMSendRS485String(json_t * root, json_t * responseObject)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleMTMSetMACAddress(json_t * root, json_t * responseObject)
{
   json_t * MACAddressObject;
   json_t * MACAddressByteObject;
   json_int_t MACAddressByte;
   byte_t MACAddressArray[3];
   byte_t index;
   uint_32 returnCode;

   MACAddressObject = json_object_get(root, "MACAddress");
   if (!json_is_array(MACAddressObject))
   {
      BuildErrorResponse(responseObject, "'MACAddress' not an array.", MTM_MAC_ADDRESS_NOT_AN_ARRAY);
      return;
   }

   if (json_array_size(MACAddressObject) != 3)
   {
      DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleMTMSetMACAddress fails - array does not contain 3 bytes","");
      BuildErrorResponse(responseObject, "'MACAddress' array does not contain 3 bytes.", MTM_MAC_ADDRESS_ARRAY_DOES_NOT_CONTAIN_3_BYTES);
      return;
   }

   for (index = 0; index < 3; index++)
   {
      MACAddressByteObject = json_array_get(MACAddressObject, index);
      if (!json_is_integer(MACAddressByteObject))
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "MAC address byte %d not an integer.", index);
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleMTMSetMACAddress fails",jsonErrorString);
         BuildErrorResponse(responseObject, jsonErrorString, MTM_MAC_ADDRESS_BYTE_NOT_AN_INTEGER);
         return;
      }
      MACAddressByte = json_integer_value(MACAddressByteObject);
      if ((MACAddressByte < 0) || (MACAddressByte > 0xFF))
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "MAC address byte %02x invalid.", MACAddressByte);
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleMTMSetMACAddress fails",jsonErrorString);
         BuildErrorResponse(responseObject, jsonErrorString, MTM_MAC_ADDRESS_BYTE_INVALID);
         return;
      }
      MACAddressArray[index] = (unsigned char) MACAddressByte;
   }
   for (index = 0; index < 3; index++)
   {
      myMACAddress[index + 3] = MACAddressArray[index];
   }

   returnCode = SaveSystemPropertiesToFlash();
   if (returnCode != FTFx_OK)
   {
      snprintf(jsonErrorString, sizeof(jsonErrorString), "Failure erasing/saving FLASH.  Code=%d", returnCode);
      DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleMTMSetMACAddress fails",jsonErrorString);
      BuildErrorResponse(responseObject, jsonErrorString, MTM_FAILURE_ERASING_SAVING_FLASH);
      return;
   }

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void UnicastSystemInfo(byte_t socketIndex)
{
   json_t * broadcastObject;

   broadcastObject = json_object();
   if (broadcastObject)
   {
      json_object_set_new(broadcastObject, "ID", json_integer(0));
      json_object_set_new(broadcastObject, "Service", json_string("SystemInfo"));
   
      HandleSystemInfo(broadcastObject);
   
      json_object_set_new(broadcastObject, "Status", json_string("Success"));
      SendJSONPacket(socketIndex, broadcastObject);
      json_decref(broadcastObject);
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleCheckForUpdate(void)
{
   checkForUpdatesTimer = 1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleInstallUpdate(json_t * responseObject)
{
   if (firmwareUpdateState != UPDATE_STATE_READY)
   {
      DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleInstallUpdate fails: Firmware image is not ready to be installed","");
      BuildErrorResponse(responseObject, "Firmware image is not ready to be installed.", USR_FIRMWARE_IMAGE_IS_NOT_READY_TO_BE_INSTALLED);
      return;
   }

   SendUpdateCommand(UPDATE_COMMAND_INSTALL_FIRMWARE, NULL, NULL);

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleDiscardUpdate()
{
   SendUpdateCommand(UPDATE_COMMAND_DISCARD_UPDATE, NULL, NULL);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandlePing(void)
{
   // Ping received from the cloud server. Reset the ping timeout value
   timeSincePingReceived = 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

byte_t S19ASCIIToBinary(const char * pS19ASCII, byte_t * pReturnByte)
{
   byte_t index = 2;

   *pReturnByte = 0;
   while (index--)
   {
      *pReturnByte <<= 4;
      if ((*pS19ASCII >= '0') && (*pS19ASCII <= '9'))
      {
         *pReturnByte += (*pS19ASCII - '0');
      }
      else if ((*pS19ASCII >= 'a') && (*pS19ASCII <= 'f'))
      {
         *pReturnByte += (*pS19ASCII - 'a' + 10);
      }
      else if ((*pS19ASCII >= 'A') && (*pS19ASCII <= 'F'))
      {
         *pReturnByte += (*pS19ASCII - 'A' + 10);
      }
      else
      {
         return false;
      }
      pS19ASCII++;
   }

   return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

byte_t ValidateS19Record(const char * pS19ASCII, byte_t * pS19Binary, byte_t maxSizeBinary, byte_t * pS19BinaryLength)
{
   byte_t byteValidFlag;
   byte_t S19ByteCount;
   byte_t S19CalculatedChecksum;
   byte_t S19RecordChecksum;

   pS19ASCII += +2;
   byteValidFlag = S19ASCIIToBinary(pS19ASCII, &S19ByteCount);
   if (!byteValidFlag)
   {  // invalid length
      return false;
   }

   S19CalculatedChecksum = S19ByteCount;

   S19ByteCount--;
   *pS19BinaryLength = S19ByteCount;

   if (S19ByteCount > maxSizeBinary)
   {  // length too long
      return false;
   }

   while (S19ByteCount--)
   {
      pS19ASCII += 2;
      byteValidFlag = S19ASCIIToBinary(pS19ASCII, pS19Binary);
      if (!byteValidFlag)
      {  // invalid byte
         return false;
      }
      S19CalculatedChecksum += *pS19Binary;
      pS19Binary++;
   }

   // we've converted the ascii string, but need to add in the final checksum to validate
   pS19ASCII += 2;
   byteValidFlag = S19ASCIIToBinary(pS19ASCII, &S19RecordChecksum);
   if (!byteValidFlag)
   {  // invalid checksum byte
      return false;
   }

   S19CalculatedChecksum += S19RecordChecksum;
   if (S19CalculatedChecksum != 0xFF)
   {  // invalid checksum
      return false;
   }

   return true;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleFirmwareUpdate(json_t * root, json_t * responseObject)
{
   json_t * hostObj;
   json_t * branchObj;

   hostObj = json_object_get(root, "Host");
   if (!json_is_string(hostObj))
   {
      DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleFirmwareUpdate fails: 'Host' not a string","");
      BuildErrorResponse(responseObject, "'Host' not a string.", SW_HOST_NOT_A_STRING);
      return;
   }

   branchObj = json_object_get(root, "Branch");
   if (!json_is_string(branchObj))
   {
      DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleFirmwareUpdate fails: 'Branch' not a string","");
      BuildErrorResponse(responseObject, "'Branch' not a string.", SW_BRANCH_NOT_STRING);
      return;
   }

   // indicate that we are going to continue using these objects and will free them later
   json_incref(hostObj);
   json_incref(branchObj);

   SendUpdateCommand(UPDATE_COMMAND_DOWNLOAD_FIRMWARE, hostObj, branchObj);

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleSystemRestart(void)
{
   restartSystemTimer = 1000;  // alert the qlink task to restart the system
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void ShutdownSocket(byte_t socketIndex, byte_t blinkCountIndex)
{
   word_t index;
   dword_t bitmask;
   dword_t invertedBitmask;

   bitmask = (0x00000001 << socketIndex);
   invertedBitmask = (~bitmask);

   if(socketIndex == CLOUD_SERVER_SOCKET_INDEX)
   {
      // Cloud Socket Disconnected. Turn off the Cloud Socket LED
      if (!manufacturingTestModeFlag)
      {
         if (led1Green_function == LED1_GREEN_FUNCTION_DEFAULT)
         {
            led1Green_default_state = LED_OFF;
            SetGPIOOutput(led1Green, led1Green_default_state);
         }
#ifdef USE_SSL
         if(ssl)
         {
            // Shutdown the SSL connection
            wolfSSL_shutdown(ssl);
            wolfSSL_free(ssl);
            ssl = NULL;
         }
#endif
      }
   }

   socketPingSeqno[socketIndex] = 0;
   shutdown(jsonSockets[socketIndex].JSONSocketConnection, FLAG_ABORT_CONNECTION);

   if (jsonSockets[socketIndex].jsonReceiveMessage)
   {
      _mem_free(jsonSockets[socketIndex].jsonReceiveMessage);
      free_count++;
      jsonSockets[socketIndex].jsonReceiveMessage = NULL;
      jsonSockets[socketIndex].jsonReceiveMessageAllocSize = 0;
      jsonSockets[socketIndex].httpSocket = FALSE;  // set to no HTTP when closing
      jsonSockets[socketIndex].WatchSocket = FALSE; 
   }

   //if (blinkCountIndex == 2)
   //{
   //   BlinkDisplayFlag[blinkCountIndex] = true;
   //   BlinkDisplayCount[blinkCountIndex]++;
   //}

   // and go through all message packets to clear our bitmask
   for (index = 0; index < (MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS * NUMBER_OF_JSON_TRANSMIT_MESSAGES_IN_QUEUE); index++)
   {
      if (JSONTransmitMessage[index].socketBitmask & bitmask)
      {
         JSONTransmitMessage[index].socketBitmask &= invertedBitmask;
         if (!JSONTransmitMessage[index].socketBitmask)
         {  // this was the last socket for this message
            if (JSONTransmitMessage[index].messageStringPtr)
            {
               jsonp_free(JSONTransmitMessage[index].messageStringPtr);
               globalCount--;
               JSONTransmitMessage[index].messageStringPtr = NULL;
            }
         }
      }
   }

   socketPingSeqno[socketIndex] = 0;
   jsonSockets[socketIndex].JSONSocketConnection = 0;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool_t GetNextCurrentMessageSlot(void)
{
   // will return true, and a correct currentMessageIndex if a slot is found.
   // returns false if no slot is available.
   word_t localMessageIndex;

   localMessageIndex = currentMessageIndex;

   // first, make sure this slot is available
   if (JSONTransmitMessage[currentMessageIndex].socketBitmask)
   {
      currentMessageIndex++;
      if (currentMessageIndex >= (MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS * NUMBER_OF_JSON_TRANSMIT_MESSAGES_IN_QUEUE))
      {
         currentMessageIndex = 0;
      }
      while ((JSONTransmitMessage[currentMessageIndex].socketBitmask) && (localMessageIndex != currentMessageIndex))
      {
         currentMessageIndex++;
         if (currentMessageIndex >= (MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS * NUMBER_OF_JSON_TRANSMIT_MESSAGES_IN_QUEUE))
         {
            currentMessageIndex = 0;
         }
      }
      if (localMessageIndex != currentMessageIndex)
      {
         return true;
      }
   }
   else
   {
      return true;
   }

   no_json_msg_slot_err++;
   return false;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void BuildMessagePacket(char * messageString, dword_t messageLength, dword_t socketBitmask)
{
   JSONTransmitMessage[currentMessageIndex].messageStringPtr = messageString;
   JSONTransmitMessage[currentMessageIndex].messageLength = messageLength;
   JSONTransmitMessage[currentMessageIndex].socketBitmask = socketBitmask;
}

bool_t AddToTransmitSocketQueue(byte_t socketIndex)
{
   dword_t bitmask;
   word_t messageIndex;
   SOCKET_MONITOR_MESSAGE_PTR socketMonitorMsgPtr;
   onqtimer_t socketTimer;
   bool_t errorFlag = true;

   _mutex_lock(&JSONTransmitTimerMutex);
   socketTimer = jsonSocketTimer[socketIndex];
   _mutex_unlock(&JSONTransmitTimerMutex);

   if (socketTimer > 10000)
   {  // it's been too long since we were able to send a packet - force close the socket
      ShutdownSocket(socketIndex, LED_NUMBER_3);
   }

   socketMonitorMsgPtr = (SOCKET_MONITOR_MESSAGE_PTR) _msg_alloc(socketMonitorMessagePoolID); // this can be exhausted with too many message in short time
   if (socketMonitorMsgPtr)
   {
      socketMonitorMsgPtr->HEADER.SOURCE_QID = 0;  //not relevant
      socketMonitorMsgPtr->HEADER.TARGET_QID = _msgq_get_id(0, (SOCKET_MONITOR_MESSAGE_SERVER_QUEUE_BASE + socketIndex));
      socketMonitorMsgPtr->HEADER.SIZE = sizeof(SOCKET_MONITOR_MESSAGE);

      socketMonitorMsgPtr->messageIndex = currentMessageIndex;
      errorFlag = !_msgq_send(socketMonitorMsgPtr);
   }
   else
   {
      socketMonMsgAllocErr++;
   }

   return errorFlag;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool_t JSONtransmitQueueIsEmpty(byte_t socketIndex)
{
   bool_t returnFlag = true;
   word_t numberOfMessagesInQueue;

   numberOfMessagesInQueue = _msgq_get_count(_msgq_get_id(0, (SOCKET_MONITOR_MESSAGE_SERVER_QUEUE_BASE + socketIndex)));
   if (numberOfMessagesInQueue)
   {
      returnFlag = false;
   }

   return returnFlag;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void SendJSONPacket(byte_t socketIndex, json_t * packetToSend)
{
   char * localString;
   volatile uint_16 messageLength;
   dword_t socketBitmask = 0x00000000;
   dword_t tempBitmask;
   bool_t foundSlotFlag;
   bool_t errorFlag;
   char test[60];

   if (packetToSend)
   {
      localString = json_dumps(packetToSend, (JSON_COMPACT | JSON_PRESERVE_ORDER));

      if (localString)
      {
         globalCount++;
         messageLength = strlen((const char *) localString) + 1;
         jsonActivityTimer = 100;

         _mutex_lock(&JSONTransmitMutex);
         foundSlotFlag = GetNextCurrentMessageSlot();
         if (foundSlotFlag)
         {
            if (socketIndex < MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS)
            {
               // make sure this socket is active and authorized
               if ((jsonSockets[socketIndex].JSONSocketConnection) &&
                   ((jsonSockets[socketIndex].jsa_state == JSA_STATE_AUTH_DEFEATED) ||
                    (jsonSockets[socketIndex].jsa_state == JSA_STATE_AUTH_VALID)))
               {
                  socketBitmask = (0x00000001 << socketIndex);
                  // fill in the message slot
                  BuildMessagePacket((char *) localString, messageLength, socketBitmask);
                  // and add it to the appropriate send task's queue
                  errorFlag = AddToTransmitSocketQueue(socketIndex);
                  if (errorFlag)
                  {
                     JSONTransmitMessage[currentMessageIndex].socketBitmask = 0;
                     JSONTransmitMessage[currentMessageIndex].messageStringPtr = NULL;
                     jsonp_free(localString);
                     globalCount--;
                  }
               }
               else
               {
                  jsonp_free(localString);
                  globalCount--;
               }
            }
            else
            {
               tempBitmask = 0x00000001;
               for (socketIndex = 0; socketIndex < MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS; socketIndex++)
               {
                  // make sure this socket is active and authorized - and not http or a watch
                  if ((jsonSockets[socketIndex].JSONSocketConnection) &&
                      (jsonSockets[socketIndex].httpSocket == FALSE) &&
                      (jsonSockets[socketIndex].WatchSocket == FALSE) &&
                      ((jsonSockets[socketIndex].jsa_state == JSA_STATE_AUTH_DEFEATED) ||
                       (jsonSockets[socketIndex].jsa_state == JSA_STATE_AUTH_VALID)))
                  {
                     socketBitmask |= tempBitmask;
                  }
                  tempBitmask <<= 1;
               }
               if (socketBitmask)
               {
                  // fill in the message slot
                  BuildMessagePacket((char *) localString, messageLength, socketBitmask);
                  for (socketIndex = 0; socketIndex < MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS; socketIndex++)
                  {
                     if (jsonSockets[socketIndex].JSONSocketConnection)
                     {
                        // and add it to the appropriate send task's queue
                        // Suppress broadcastDebug output if we're forcing the key to be set or it is a Watch Connection unless we are in manufacturing test mode
                        if( ( (jsonSockets[socketIndex].jsa_state & JSA_MASK_JSON_LIMITED) || jsonSockets[socketIndex].WatchSocket ) && !manufacturingTestModeFlag)
                        {
                           errorFlag = 0;
                        }
                        else
                        {
                           errorFlag = AddToTransmitSocketQueue(socketIndex);
                        }
                        if (errorFlag)
                        {
                           // we do hit this when we try to send too much too fast
                           JSONTransmitMessage[currentMessageIndex].socketBitmask &= (~(((uint_32) 0x00000001) << socketIndex));
                        }
                     }
                  }
                  if (!JSONTransmitMessage[currentMessageIndex].socketBitmask)
                  {
                     JSONTransmitMessage[currentMessageIndex].messageStringPtr = NULL;
                     jsonp_free(localString);
                     globalCount--;
                  }
               }
               else
               {
                  jsonp_free(localString);
                  globalCount--;
               }
            }
         }
         else
         {
            jsonp_free(localString);
            globalCount--;
         }

         _mutex_unlock(&JSONTransmitMutex);

      }
      //else
      //{
      //   BlinkDisplayFlag[1] = true;
      //   BlinkDisplayCount[1]++;
      //}

#ifdef MEMORY_DIAGNOSTIC_PRINTF
      snprintf(test, sizeof(test), "Static RAM Usage:   %d Bytes\r\n", (uint32_t)_mqx_get_initialization()->START_OF_KERNEL_MEMORY - 0x1FFF0000);
      snprintf(test, sizeof(test), "  Peak RAM Usage:   %d Bytes(%d%%)\r\n", (uint32_t)_mem_get_highwater() - 0x1FFF0000, ((uint32_t)_mem_get_highwater() - 0x1FFF0000)/0x51E);
      snprintf(test, sizeof(test), "Current RAM Free:   %d Bytes(%d%%)\r\n", _mem_get_free(), _mem_get_free()/0x51E);
#endif

   }

}

int_32 Socket_send(int sockfd, void *buf, size_t len, int flags, unsigned long* errorCounter, const char *caller)
{
    errno = 0;
    int_32 result = send(sockfd, buf, len, flags);
    
    if (result < 0)
    {
        DBGPRINT_INTSTRSTR_SOCKET_SEND_ERROR(errno, "Socket_send fail", caller); // this can makes matters worse, leading to infinite loop
        last_send_err = errno;
        
        if (!errorCounter)
        {
            errorCounter = &other_send_err_count;
        }
        
        (*errorCounter)++;
    }
    
    return result;
}

int_32 Socket_sendto(int sockfd, void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen, unsigned long* errorCounter, const char *caller)
{
    errno = 0;
    int_32 result = sendto(sockfd, buf, len, flags, dest_addr, addrlen);
    
    if (result < 0)
    {
        broadcastDebug(errno, "Socket_sendto fail", caller);
        last_send_err = errno;
        
        if (!errorCounter)
        {
            errorCounter = &other_send_err_count;
        }
        
        (*errorCounter)++;
    }
    
    return result;
}

int_32 Socket_recv(int sockfd, void *buf, size_t len, int flags, unsigned long* errorCounter, const char *caller)
{
    errno = 0;
    int_32 result = recv(sockfd, buf, len, flags);
    
    if (result < 0)
    {
        broadcastDebug(result, "Socket_recv fail", caller);
        last_recv_err = errno;
        
        if (!errorCounter)
        {
            errorCounter = &other_recv_err_count;
        }
        
        (*errorCounter)++;
    }
    
    return result;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void JSONTransmit_Task(uint_32 socketIndex)
{
   dword_t myBitPattern;
   word_t messageIndex;
   SOCKET_MONITOR_MESSAGE_PTR socketMonitorMsgPtr;
   boolean msgSendSuccessfulFlag;
   byte_t localEmptyIndex;
   int_32 result;
   int_32 retryResult;
   _queue_id socketMonitorMessageQID;
   word_t numberOfMessagesInQueue;

   myBitPattern = (0x00000001 << socketIndex);
   myBitPattern = ~myBitPattern;

   /* Open a message queue: */
   socketMonitorMessageQID = _msgq_open(SOCKET_MONITOR_MESSAGE_SERVER_QUEUE_BASE + socketIndex, 0);

   numberOfMessagesInQueue = 0;
   for (;;)
   {
      // wait forever for a message to send
      socketMonitorMsgPtr = _msgq_receive(socketMonitorMessageQID, 0);
      messageIndex = socketMonitorMsgPtr->messageIndex;
      numberOfMessagesInQueue++;
      _msg_free(socketMonitorMsgPtr);

//      numberOfMessagesInQueue = _msgq_get_count(_msgq_get_id(0, (SOCKET_MONITOR_MESSAGE_SERVER_QUEUE_BASE + socketIndex)));
//      if (numberOfMessagesInQueue < (NUMBER_OF_JSON_TRANSMIT_MESSAGES_IN_QUEUE - 1))
      {
         if (jsonSockets[socketIndex].JSONSocketConnection)
         {
            //send the message
#ifdef USE_SSL
            if((socketIndex == CLOUD_SERVER_SOCKET_INDEX) && 
               (ssl != NULL))
            {
               result = wolfSSL_write(ssl, 
                     JSONTransmitMessage[messageIndex].messageStringPtr, 
                     JSONTransmitMessage[messageIndex].messageLength);
            }
            else
#endif
            {
                // if we have a HTTP socket, all HTTP header stuff
                if (jsonSockets[socketIndex].httpSocket == TRUE)
                {
                   memset(httpStr, 0, sizeof(httpStr));
                   snprintf(httpStr, sizeof(httpStr), 
                         "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: %d\r\n\r\n%s",
                         (JSONTransmitMessage[messageIndex].messageLength),
                         JSONTransmitMessage[messageIndex].messageStringPtr);
                   JSONTransmitMessage[messageIndex].messageLength += 92;  // add header length to message total
                }
                else
                {
                   snprintf(httpStr, sizeof(httpStr), 
                         "%s",
                         JSONTransmitMessage[messageIndex].messageStringPtr);
                
                }
                if(jsonSockets[socketIndex].jsa_state & JSA_MASK_JSON_ENABLED)
                {
                   result = Socket_send(jsonSockets[socketIndex].JSONSocketConnection,
                                 httpStr,
                                 JSONTransmitMessage[messageIndex].messageLength,
                                 0, NULL, __PRETTY_FUNCTION__);
                }
                else
                {
                   result = JSONTransmitMessage[messageIndex].messageLength;
                }
            }

            if (result < 0)
            {
                // Socket_send() returned an error so we got 0 bytes.
                // We will retry next.
                result = 0;
            }
            
            if (result < JSONTransmitMessage[messageIndex].messageLength)
            {
                
               _time_delay(100);
#ifdef USE_SSL
               if((socketIndex == CLOUD_SERVER_SOCKET_INDEX) && 
                  (ssl != NULL))
               {
                  retryResult = wolfSSL_write(ssl, 
                        JSONTransmitMessage[messageIndex].messageStringPtr + result, 
                        JSONTransmitMessage[messageIndex].messageLength - result);
               }
               else
#endif
               {
                  retryResult = Socket_send(jsonSockets[socketIndex].JSONSocketConnection,
                        httpStr + result,
                        JSONTransmitMessage[messageIndex].messageLength - result,
                        0, NULL, __PRETTY_FUNCTION__);
               }

               if (retryResult < 0)
               {
                   // Socket_send() returned an error so we got 0 bytes.
                   // Make sure result has the correct number of bytes.
                   retryResult = 0;
               }

               result += retryResult;
            }
            
            if (result == JSONTransmitMessage[messageIndex].messageLength)
            {  // only clear the timer if we were actually successful in delivering this to the stack.
               // if we were not successful, it's most likely because the destianation device has
               // disappeared without disconnecting.
               _mutex_lock(&JSONTransmitTimerMutex);
               jsonSocketTimer[socketIndex] = 0;
               _mutex_unlock(&JSONTransmitTimerMutex);
               if (jsonSockets[socketIndex].httpSocket == TRUE)
               {
                  // if we sent our response, open this socket back up for regular comm
                  jsonSockets[socketIndex].httpSocket = FALSE;  // set to no HTTP when closing
                  jsonSockets[socketIndex].WatchSocket = FALSE; //Set to No Apple Watch
                  // shut down the socket if HTTP after request-response
               }  // if (jsonSockets[socketIndex].httpSocket == TRUE)
            }
            else
            {
                // DMC: Do something with the error?
            }
         }
      }

      // clear our bit from the message
      _mutex_lock(&JSONTransmitMutex);
      if (JSONTransmitMessage[messageIndex].socketBitmask)
      {
         JSONTransmitMessage[messageIndex].socketBitmask &= myBitPattern;
         if (!JSONTransmitMessage[messageIndex].socketBitmask)
         {  // we were the last ones acting on this message
            if (JSONTransmitMessage[messageIndex].messageStringPtr)
            {
               jsonp_free(JSONTransmitMessage[messageIndex].messageStringPtr);
               globalCount--;
               JSONTransmitMessage[messageIndex].messageStringPtr = NULL;
            }
         }
      }
      _mutex_unlock(&JSONTransmitMutex);

      if (numberOfMessagesInQueue >= NUMBER_OF_JSON_PACKETS_TO_SEND_PER_CYCLE)
      {
         _sched_yield();  // nothing to do, yield to the next transmit packet
         numberOfMessagesInQueue = 0;
      }

   }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void InitializeJSONThreads(void)
{
   word_t socketIndex;
   byte_t queueIndex;

   for (socketIndex = 0; socketIndex < (MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS * NUMBER_OF_JSON_TRANSMIT_MESSAGES_IN_QUEUE); socketIndex++)
   {
      JSONTransmitMessage[socketIndex].messageStringPtr = NULL;
      JSONTransmitMessage[socketIndex].messageLength = 0;
      JSONTransmitMessage[socketIndex].socketBitmask = 0;
   }

   /* Create a message pool: */
   socketMonitorMessagePoolID = _msgpool_create(sizeof(SOCKET_MONITOR_MESSAGE),
         (MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS * NUMBER_OF_JSON_TRANSMIT_MESSAGES_IN_QUEUE), 0, 0);

   for (socketIndex = 0; socketIndex < MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS; socketIndex++)
   {
      _task_create(0, JSON_TRANSMIT_TASK, socketIndex);
   }

   currentMessageIndex = 0;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void JSONAccept_Task(uint_32 param)
{
   dword_t socketRequest;
   uint_32 opt_value;
   byte_t socketIndex;
   byte_t foundSlotFlag;
   uint_32 error;
   sockaddr_in remote_addr = { 0 };
   uint_16 remote_addr_len;
   sockaddr_in laddr;
   int_32 result;
   char test[60];

   dword_t sock;

   if (param == 0)
   {
      _mutex_lock(&JSONWatchMutex);
      for (socketIndex = 0; socketIndex < MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS; socketIndex++)
      {
         socketPingSeqno[socketIndex] = 0;
         jsonSockets[socketIndex].JSONSocketConnection = 0;
         jsonSockets[socketIndex].jsonReceiveMessage = NULL;
         jsonSockets[socketIndex].jsonReceiveMessageAllocSize = 0;
         jsonSockets[socketIndex].httpSocket = FALSE;
         jsonSockets[socketIndex].WatchSocket = FALSE;
      }
      _mutex_unlock(&JSONWatchMutex);
   }
   
   sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sock == RTCS_SOCKET_ERROR)
   {
      return;
   }

   laddr.sin_family = AF_INET;
   laddr.sin_addr.s_addr = INADDR_ANY;
   if (param == 0) laddr.sin_port = LISTEN_PORT;
   else laddr.sin_port = WATCH_PORT;

   // DMC: Error not handled
   result = bind(sock, &laddr, sizeof(laddr));
   
   // DMC: Error not handled
   result = listen(sock, 0);

   for (;;)
   {
      socketRequest = accept(sock, &remote_addr, &remote_addr_len);
      if (socketRequest == RTCS_SOCKET_ERROR)
      {
         shutdown(sock, FLAG_ABORT_CONNECTION);
         _time_delay(1000);

         sock = socket(AF_INET, SOCK_STREAM, 0);
         if(sock != RTCS_SOCKET_ERROR)
         {
            laddr.sin_family = AF_INET;
            laddr.sin_port = LISTEN_PORT;
            laddr.sin_addr.s_addr = INADDR_ANY;

            // DMC: Error not handled
            result = bind(sock, &laddr, sizeof(laddr));
            
            // DMC: Error not handled
            result = listen(sock, 0);
         }
      }
      else
      {
         // someone wants to connect, see if we have a slot left for this
         socketIndex = 0;
         foundSlotFlag = false;
         _mutex_lock(&JSONWatchMutex);
         while ((socketIndex < MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS) && (!foundSlotFlag))
         {
            if ((socketIndex != CLOUD_SERVER_SOCKET_INDEX) &&
                (!jsonSockets[socketIndex].JSONSocketConnection))
            {  // found one
               foundSlotFlag = true;
            }
            else
            {
               socketIndex++;
            }
         }

         if (foundSlotFlag)
         {
            opt_value = TRUE;
            error = setsockopt(socketRequest, SOL_TCP, OPT_RECEIVE_NOWAIT, &opt_value, sizeof(opt_value));
            opt_value = TRUE;
            error = setsockopt(socketRequest, SOL_TCP, OPT_SEND_NOWAIT, &opt_value, sizeof(opt_value));

            jsonSocketTimer[socketIndex] = 0;
            jsonSockets[socketIndex].jsa_state = JSA_STATE_START;
            jsonSockets[socketIndex].JSONSocketConnection = socketRequest;
            jsonSockets[socketIndex].jsonReceiveMessageIndex = 0;
            jsonSockets[socketIndex].jsonReceiveMessageOverrunFlag = false;
            
            if (param == 0)
            {
               jsonSockets[socketIndex].WatchSocket = FALSE;
               jsonSockets[socketIndex].httpSocket = FALSE;
            }
            else
            {
               jsonSockets[socketIndex].WatchSocket = TRUE;
            }

            RTCS_detachsock(jsonSockets[socketIndex].JSONSocketConnection);
            jsonSockets[socketIndex].needsAttachSock = true;

#ifdef MEMORY_DIAGNOSTIC_PRINTF
            snprintf(test, sizeof(test), "Static RAM Usage:   %d Bytes\r\n", (uint32_t)_mqx_get_initialization()->START_OF_KERNEL_MEMORY - 0x1FFF0000);
            snprintf(test, sizeof(test), "  Peak RAM Usage:   %d Bytes(%d%%)\r\n", (uint32_t)_mem_get_highwater() - 0x1FFF0000, ((uint32_t)_mem_get_highwater() - 0x1FFF0000)/0x51E);
            snprintf(test, sizeof(test), "Current RAM Free:   %d Bytes(%d%%)\r\n", _mem_get_free(), _mem_get_free()/0x51E);
#endif

         }
         else
         {
            shutdown(socketRequest, FLAG_ABORT_CONNECTION);
         }
         _mutex_unlock(&JSONWatchMutex);
      }
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void Socket_Task(uint_32 param)
{
   MUTEX_ATTR_STRUCT mutexattr;
   json_t * root = NULL;
   json_t * jsonResponseObject = NULL;
   json_t * jsonResponseStatusObject = NULL;
   json_t * jsonBroadcastObject = NULL;
   json_t * reportPropertiesObject = NULL;
   json_t * volumeObj = NULL;
   dword_t zoneBitmask;
   IPCFG_IP_ADDRESS_DATA latest_ip_data;
   json_error_t jsonError;
   int_32 result;
   uchar txbuf[1] = { 0 };
   uchar rxbuf[1] = { 0 };
   uint_32 error;
   uchar jsonReceiveMessageOverrunFlag = false;
   int option;
   byte_t socketIndex;
   char ZIDString[MAX_SIZE_ZID_STRING];
   word_t tempIndex;
   bool_t doneFlag;
   volatile uint32 responseLength;
   byte_t * newReceiveBufferPtr = NULL;
   short  temp_active_incoming_json_connections;
#if DEBUG_SOCKETTASK_ANY
   char                       debugBuf[100];
   MQX_TICK_STRUCT            debugLocalStartTick;
   MQX_TICK_STRUCT            debugLocalEndTick;
   bool                       debugLocalOverflow;
   uint32_t                   debugLocalOverflowMs;
   _time_delay(8000);
#endif
   
   _time_delay(1000);

   LCM1_initialize_networking(myMACAddress);
   memset(&latest_ip_data, 0, sizeof(latest_ip_data));
   memset(&current_ip_data, 0, sizeof(current_ip_data));
   memset(socketPingSeqno,0,sizeof(int32_t) * MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS);
   
   memset(lastLowBatteryBitArray,0,sizeof(lastLowBatteryBitArray));
   memset(zoneChangedThisMinuteBitArray,0,sizeof(zoneChangedThisMinuteBitArray));

   _mutatr_init(&mutexattr);
   _mutex_init(&JSONTransmitMutex, &mutexattr);
   _mutex_init(&JSONWatchMutex, &mutexattr);

   JSA_GenerateFactoryPassword();
   JSA_GenerateFactoryKey();
   InitializeJSONThreads();

   _task_create(0, MDNS_TASK, 0);  // start up MDNS_TASK for handling discovery messages

   _task_create(0, JSON_ACCEPT_TASK, 0);  // start up JSON_ACCEPT_TASK for handling incoming json connect requests

   _task_create(0, WATCH_ACCEPT_TASK, 1);  // start up WATCH_ACCEPT_TASK for handling incoming Watch requests

   _task_create(0, FIRMWARE_UPDATE_TASK, 0);  // start up firmware update task

   _task_create(0, EVENT_TIMER_TASK, 0);    // start up event timer task

   _task_create(0, ELIOT_REST_TASK, 0);   // start up the Eliot REST task

   _task_create(0, ELIOT_MQTT_TASK, 0);   // start up the Eliot MQTT task

#ifdef USE_SSL
   WOLFSSL_CTX *ctx = NULL;
#endif
   
#ifdef SHADE_CONTROL_ADDED
   DEBUG_QUBE_MUTEX_START;
   _mutex_lock(&qubeSocketMutex);
   DBGPRINT_INTSTRSTR_IF_QUBE_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW Socket_Task before qubeSocketMutex",DEBUGBUF);
   DEBUG_QUBE_MUTEX_START;
   if (qubeAddress)
   {
      QueryQmotionStatus();
   }
   DBGPRINT_INTSTRSTR_IF_QUBE_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW Socket_Task after QueryQmotionStatus",DEBUGBUF);
   _mutex_unlock(&qubeSocketMutex);
#if 0
   if (qubeAddress)
   {
      GetQmotionDevicesInfo();
   }
#endif
#endif
 
   for (;;)
   {
      RTCS_rand();
      JSA_SetAuthExemptTask();
      JSA_SetFirstTimeRecordedTask();
      if(!clearQubeAddressTimer)
      {
         _mutex_lock(&qubeSocketMutex);
         TryToFindQmotion();
         _mutex_unlock(&qubeSocketMutex);
       }

       _mutex_lock(&TaskHeartbeatMutex);
       task_heartbeat_bitmask |= Socket_Task_HEARTBEAT;
       _mutex_unlock(&TaskHeartbeatMutex);
       
#ifdef MEMORY_DIAGNOSTIC_PRINTF
      if (globalShowMemoryFlag)
      {
         snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "Static RAM Usage:   %d Bytes\r\n", (uint32_t)_mqx_get_initialization()->START_OF_KERNEL_MEMORY - 0x1FFF0000);
         snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "  Peak RAM Usage:   %d Bytes(%d%%)\r\n", (uint32_t)_mem_get_highwater() - 0x1FFF0000, ((uint32_t)_mem_get_highwater() - 0x1FFF0000)/0x51E);
         snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "Current RAM Free:   %d Bytes(%d%%)\r\n", _mem_get_free(), _mem_get_free()/0x51E);
         globalShowMemoryFlag = 0;
      }
#endif

      // these next announcements will only be posted if the individual transmit queues are empty
      doneFlag = true;
      socketIndex = 0;
      while (socketIndex < MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS)
      {
         if (jsonSockets[socketIndex].JSONSocketConnection)
         {
            if (!JSONtransmitQueueIsEmpty(socketIndex))
            {
               doneFlag = false;
            }
         }
         socketIndex++;
      }

      socketIndex = 0;
      temp_active_incoming_json_connections = 0;
      while (socketIndex < MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS)
      {
         if((socketIndex == CLOUD_SERVER_SOCKET_INDEX) && 
            (!jsonSockets[socketIndex].JSONSocketConnection))
         {
            // Can only connect to the cloud server when the system time is valid
            // because SSL needs to use the time to validate the certificates
#ifdef USE_SSL
            if(systemTimeValid)
            {
               if(ctx == NULL)
               {
                  // Initialize and create the SSL Context
                  wolfSSL_Init();
                  ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());

                  // Load the server certificate to validate
                  if(wolfSSL_CTX_load_verify_buffer(ctx, serverCert, sizeof(serverCert)/sizeof(uint8_t), SSL_FILETYPE_ASN1) == SSL_SUCCESS)
                  {
                     // Load the client certificate to give to the server
                     if(wolfSSL_CTX_use_certificate_buffer(ctx, clientCert, sizeof(clientCert)/sizeof(uint8_t), SSL_FILETYPE_ASN1) == SSL_SUCCESS)
                     {
                        // Load the private key for the client
                        if(wolfSSL_CTX_use_PrivateKey_buffer(ctx, clientKey, sizeof(clientKey)/sizeof(uint8_t), SSL_FILETYPE_ASN1) != SSL_SUCCESS)
                        {
                           // Failed to use the private key
                           wolfSSL_CTX_free(ctx);
                           ctx = NULL;
                           wolfSSL_Cleanup();
                        }
                     }
                     else
                     {
                        // Failed to load the client certificate
                        wolfSSL_CTX_free(ctx);
                        ctx = NULL;
                        wolfSSL_Cleanup();
                     }
                  }
                  else
                  {
                     // Failed to load the server certificate
                     wolfSSL_CTX_free(ctx);
                     ctx = NULL;
                     wolfSSL_Cleanup();
                  }
               }

               if(ssl == NULL && ctx != NULL)
               {
                  // Something happened to the cloud server connection. Attempt to
                  // connect again
                  sockaddr_in cloudServerAddr;
                  uint_32 cloudSocket;
                  uint_32 opt_value;

                  memset((char *)&cloudServerAddr, 0, sizeof(sockaddr_in));
                  cloudServerAddr.sin_family = AF_INET;
                  cloudServerAddr.sin_port = CLOUD_SERVER_PORT;
                  cloudServerAddr.sin_addr.s_addr = CLOUD_SERVER_IP_ADDRESS;

                  cloudSocket = socket(AF_INET, SOCK_STREAM, 0);

                  result = connect(cloudSocket, &cloudServerAddr, sizeof(sockaddr_in));

                  opt_value = TRUE;
                  error = setsockopt(cloudSocket, SOL_TCP, OPT_RECEIVE_NOWAIT, &opt_value, sizeof(opt_value));
                  opt_value = TRUE;
                  error = setsockopt(cloudSocket, SOL_TCP, OPT_SEND_NOWAIT, &opt_value, sizeof(opt_value));
                  
                  if(result == RTCS_OK)
                  {
                     ssl = wolfSSL_new(ctx);

                     if(ssl != NULL)
                     {
                        // Set the socket handle for the SSL connection
                        wolfSSL_set_fd(ssl, cloudSocket);
                     }
                     else
                     {
                        // Close the socket and clean up
                        wolfSSL_CTX_free(ctx);
                        ctx = NULL;
                        wolfSSL_Cleanup();
                        shutdown(cloudSocket, FLAG_ABORT_CONNECTION);
                     }
                  }
                  else
                  {
                     shutdown(cloudSocket, FLAG_ABORT_CONNECTION);
                  }
               }

               if((ssl != NULL) &&
                  (jsonSockets[socketIndex].JSONSocketConnection == 0))
               {
                  // Attempt to connect securely to the socket
                  int connectStatus = wolfSSL_connect(ssl);
                  if(connectStatus == SSL_SUCCESS)
                  {

                     // Cloud Socket Connected. Turn on the Cloud Socket LED
                     if (!manufacturingTestModeFlag)
                     {
                        if (led1Green_function == LED1_GREEN_FUNCTION_DEFAULT)
                        {
                           led1Green_default_state = LED_ON;
                           SetGPIOOutput(led1Green, led1Green_default_state);
                        }
                     }

                     jsonSocketTimer[socketIndex] = 0;
                     jsonSockets[socketIndex].JSONSocketConnection = wolfSSL_get_fd(ssl);
                     jsonSockets[socketIndex].jsonReceiveMessageIndex = 0;
                     jsonSockets[socketIndex].jsonReceiveMessageOverrunFlag = false;
                     RTCS_detachsock(jsonSockets[socketIndex].JSONSocketConnection);
                     jsonSockets[socketIndex].needsAttachSock = true;
                     timeSincePingReceived = 0;
                  }
                  else
                  {
                     int connectError = wolfSSL_get_error(ssl, connectStatus);

                     if((connectError == SSL_ERROR_WANT_READ) ||
                        (connectError == SSL_ERROR_WANT_WRITE))
                     {
                        // Continue on and try to connect the next time through
                     }
                     else
                     {
                        // Failed to connect to the SSL Socket
                        wolfSSL_shutdown(ssl);
                        wolfSSL_free(ssl);
                        wolfSSL_CTX_free(ctx);
                        wolfSSL_Cleanup();
                        ctx = NULL;
                        ssl = NULL;
                        shutdown(wolfSSL_get_fd(ssl), FLAG_ABORT_CONNECTION);
                     }
                  }
               }
            }
#else
            // Something happened to the cloud server connection. Attempt to
            // connect again
            sockaddr_in cloudServerAddr;
            uint_32 cloudSocket;
            uint_32 opt_value;

            memset((char *)&cloudServerAddr, 0, sizeof(sockaddr_in));
            cloudServerAddr.sin_family = AF_INET;
            cloudServerAddr.sin_port = CLOUD_SERVER_PORT;
            cloudServerAddr.sin_addr.s_addr = CLOUD_SERVER_IP_ADDRESS;

            cloudSocket = socket(AF_INET, SOCK_STREAM, 0);

            result = connect(cloudSocket, &cloudServerAddr, sizeof(sockaddr_in));

            if(result == RTCS_OK)
            {
               // Connect to the cloud socket
               // Cloud Socket Connected. Turn on the Cloud Socket LED
               if (!manufacturingTestModeFlag)
               {
                  if (led1Green_function == LED1_GREEN_FUNCTION_DEFAULT)
                  {
                     led1Green_default_state = LED_ON;
                     SetGPIOOutput(led1Green, led1Green_default_state);
                  }
               }

               opt_value = TRUE;
               error = setsockopt(cloudSocket, SOL_TCP, OPT_RECEIVE_NOWAIT, &opt_value, sizeof(opt_value));
               opt_value = TRUE;
               error = setsockopt(cloudSocket, SOL_TCP, OPT_SEND_NOWAIT, &opt_value, sizeof(opt_value));

               jsonSocketTimer[socketIndex] = 0;
               jsonSockets[socketIndex].JSONSocketConnection = cloudSocket;
               jsonSockets[socketIndex].jsonReceiveMessageIndex = 0;
               jsonSockets[socketIndex].jsonReceiveMessageOverrunFlag = false;
               RTCS_detachsock(jsonSockets[socketIndex].JSONSocketConnection);
               jsonSockets[socketIndex].needsAttachSock = true;
               timeSincePingReceived = 0;

            }
            else
            {
               shutdown(cloudSocket, FLAG_ABORT_CONNECTION);
            }
#endif
         }
         else if((socketIndex == CLOUD_SERVER_SOCKET_INDEX) && 
                 (jsonSockets[socketIndex].JSONSocketConnection))
         {
            // Cloud Socket is connected. Make sure that a ping was received in the last 12 seconds
            if (timeSincePingReceived > (CLOUD_SERVER_PING_TIMEOUT_SECONDS * 1000))
            {  // it's been too long since we received a ping from the cloud socket. Shutdown the socket
               ShutdownSocket(socketIndex, LED_NUMBER_2);
            }
         }


         if (jsonSockets[socketIndex].JSONSocketConnection)
         {
            temp_active_incoming_json_connections++;
            if (jsonSockets[socketIndex].needsAttachSock)
            {
               jsonSockets[socketIndex].needsAttachSock = false;
               jsonSockets[socketIndex].JSONSocketConnection = RTCS_attachsock(jsonSockets[socketIndex].JSONSocketConnection);
               if (jsonSockets[socketIndex].JSONSocketConnection == RTCS_SOCKET_ERROR)
               {
                  socketIndex = MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS;
                  continue;
               }
               else
               {

                  jsonSockets[socketIndex].jsonReceiveMessageAllocSize = 0;  // make sure we require an immediate alloc on the first character received.

                  if (firmwareUpdateState == UPDATE_STATE_READY)
                  {
                     updateAlertPingCounter[socketIndex] = NUMBER_OF_PINGS_BEFORE_UPDATE_READY_ALERT;
                  }
                  else
                  {
                     updateAlertPingCounter[socketIndex] = 0;
                  }
               }

#ifdef MEMORY_DIAGNOSTIC_PRINTF
               if (globalShowMemoryFlag)
               {
                  snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "Static RAM Usage:   %d Bytes\r\n", (uint32_t)_mqx_get_initialization()->START_OF_KERNEL_MEMORY - 0x1FFF0000);
                  snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "  Peak RAM Usage:   %d Bytes(%d%%)\r\n", (uint32_t)_mem_get_highwater() - 0x1FFF0000, ((uint32_t)_mem_get_highwater() - 0x1FFF0000)/0x51E);
                  snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "Current RAM Free:   %d Bytes(%d%%)\r\n", _mem_get_free(), _mem_get_free()/0x51E);
                  globalShowMemoryFlag = 0;
               }
#endif
            }

            if ((restartSystemTimer) && (restartSystemTimer < 1000))
            {
                broadcastDebug(restartSystemTimer, "Socket_Task", "SYSTEM RESTART");
            }

			//========= JSON Socket Auth Begin  ===============================
			else if(!(jsonSockets[socketIndex].jsa_state & JSA_MASK_JSON_ENABLED))
			{
				uint8_t idx=0;
				uint8_t x=0;

				// If the socket is unauthenticated, use the socket struct's receive buffer
				// pointer for the memory we'll need for authentication.  Afterward, the
				// socket task will free or resize it as needed.

				// Is enough memory already allocated?
				if(!jsonSockets[socketIndex].jsonReceiveMessageAllocSize < sizeof(json_socket_auth))
				{	// Is any memory allocated?
					if(jsonSockets[socketIndex].jsonReceiveMessageAllocSize)
					{
						free(jsonSockets[socketIndex].jsonReceiveMessage);
					}
					jsonSockets[socketIndex].jsonReceiveMessage = malloc(sizeof(json_socket_auth));
					if(jsonSockets[socketIndex].jsonReceiveMessage)
					{	// malloc() success - record the buffer size
						jsonSockets[socketIndex].jsonReceiveMessageAllocSize = sizeof(json_socket_auth);
					}
					else
					{
						// malloc() failure - Print the out-of-memory message and let the socket time out.
						if(jsonSockets[socketIndex].jsa_state != JSA_STATE_LOW_MEM)
						{
							Socket_send(jsonSockets[socketIndex].JSONSocketConnection, JSA_OUT_OF_MEMORY_MESSAGE, sizeof(JSA_OUT_OF_MEMORY_MESSAGE),  0, NULL, __PRETTY_FUNCTION__);
							jsonSockets[socketIndex].jsa_state = JSA_STATE_LOW_MEM;
						}
					}
				}
				json_socket_auth* jsa = (json_socket_auth*)jsonSockets[socketIndex].jsonReceiveMessage;

				//-------------------------------------------------------------

				if(jsonSockets[socketIndex].jsa_state == JSA_STATE_START)
				{
#if DEBUG_SOCKETTASK_AUTH
					// DEBUG ONLY - Disable challenge on the first socket
					if(!socketIndex)
					{
						char dbg[128];
						uint8_t *flash_key = (uint8_t*)FBK_Address(&eliot_fbk, LCM_DATA_JSON_SOCKET_USER_KEY_OFFSET);

						int x=0;

						snprintf(dbg, sizeof(dbg), "============= SOCKET AUTH INFO =============\r\n");
						Socket_send(jsonSockets[socketIndex].JSONSocketConnection, dbg, strlen(dbg),  0, NULL, __PRETTY_FUNCTION__);
						snprintf(dbg, sizeof(dbg), "  Factory label: %s\r\n", password_label);
						Socket_send(jsonSockets[socketIndex].JSONSocketConnection, dbg, strlen(dbg),  0, NULL, __PRETTY_FUNCTION__);

						snprintf(dbg, sizeof(dbg), "    Factory key: ");
						Socket_send(jsonSockets[socketIndex].JSONSocketConnection, dbg, strlen(dbg),  0, NULL, __PRETTY_FUNCTION__);
						for(x=0; x<16; x++)
						{
							sprintf(&dbg[x*2], "%02X", json_socket_factory_key.content.b[x]);
						}
						Socket_send(jsonSockets[socketIndex].JSONSocketConnection, dbg, strlen(dbg),  0, NULL, __PRETTY_FUNCTION__);

						snprintf(dbg, sizeof(dbg), "\r\nFactoryAuthInit: %d", extendedSystemProperties.factory_auth_init == FACTORY_AUTH_INIT_ENABLED);
						Socket_send(jsonSockets[socketIndex].JSONSocketConnection, dbg, strlen(dbg),  0, NULL, __PRETTY_FUNCTION__);

						snprintf(dbg, sizeof(dbg), "\r\n      Flash key: ");
						Socket_send(jsonSockets[socketIndex].JSONSocketConnection, dbg, strlen(dbg),  0, NULL, __PRETTY_FUNCTION__);
						for(x=0; x<16; x++)
						{
							sprintf(&dbg[x*2], "%02X", flash_key[x]);
						}
						Socket_send(jsonSockets[socketIndex].JSONSocketConnection, dbg, strlen(dbg),  0, NULL, __PRETTY_FUNCTION__);

						snprintf(dbg, sizeof(dbg), "\r\n         K64 ID: %04X:%08X:%08X\r\n\r\n", (uint16_t)SIM_UIDMH,SIM_UIDML,SIM_UIDL);
						Socket_send(jsonSockets[socketIndex].JSONSocketConnection, dbg, strlen(dbg),  0, NULL, __PRETTY_FUNCTION__);

						jsonSockets[socketIndex].jsa_state = JSA_STATE_AUTH_DEFEATED;
						socketIndex++;
						continue;
					}
#endif

					// If Apple Watch port, Ignore regular authentication
					if(jsonSockets[socketIndex].WatchSocket)
					{
						//Do not send greeting for Apple Watch
						DBGPRINT_INTSTRSTR(socketIndex, "Socket_Task/JSA - Socket opened without challenge:", "Apple Watch");
						jsonSockets[socketIndex].jsa_state = JSA_STATE_AUTH_DEFEATED;

						socketIndex++;
						continue;
					}

					// If neither the factory key nor user key are enabled, don't issue a challenge.
					if((extendedSystemProperties.factory_auth_init != FACTORY_AUTH_INIT_ENABLED) && !JSA_UserKeyIsSet())
					{
						// Depending on the state of extendedSystemProperties.auth_exempt, the LCM communication can enter one of two modes:
						// 1.  Normal JSON communications
						// 2.  Limited JSON communications where only the key may be changed
						if(extendedSystemProperties.auth_exempt == AUTH_NOT_EXEMPT)
						{
							Socket_send(jsonSockets[socketIndex].JSONSocketConnection, JSA_AUTH_SET_KEY_MESSAGE, sizeof(JSA_AUTH_SET_KEY_MESSAGE),  0, NULL, __PRETTY_FUNCTION__);
							JSA_SendMAC(jsonSockets[socketIndex].JSONSocketConnection);
                     Socket_send(jsonSockets[socketIndex].JSONSocketConnection, JSA_AUTH_SET_KEY_PADDING_MESSAGE, sizeof(JSA_AUTH_SET_KEY_PADDING_MESSAGE),  0, NULL, __PRETTY_FUNCTION__);
							DBGPRINT_INTSTRSTR(socketIndex, "Socket_Task/JSA - Socket opened in key-set mode:", "");
							jsonSockets[socketIndex].jsa_state = JSA_STATE_REQUIRE_SET_KEY;
						}
						else
						{
							JSA_SendMAC(jsonSockets[socketIndex].JSONSocketConnection);
							DBGPRINT_INTSTRSTR(socketIndex, "Socket_Task/JSA - Socket opened without challenge:", "Auth Defeated");
							jsonSockets[socketIndex].jsa_state = JSA_STATE_AUTH_DEFEATED;
						}
						socketIndex++;
						continue;
					}

					// At this point, a challenge will be issued against either the user or factory key.
					Socket_send(jsonSockets[socketIndex].JSONSocketConnection, JSA_GREETING, sizeof(JSA_GREETING),  0, NULL, __PRETTY_FUNCTION__);

					// Generate a random challenge phrase
					for(x=0; x<sizeof(jsa->challenge_phrase); x++)
					{
						jsa->challenge_phrase[x] = socketIndex ^ (uint8_t)RTCS_rand() ^ _time_get_nanoseconds();

						// For testing AES, the first secured socket's challenge is a constant
#if DEBUG_SOCKETTASK_AUTH
						if(socketIndex == 1)
						{
							jsa->challenge_phrase[x] = x | x<<4;
						}
#endif
					}

					// Using the challenge_response receive buffer as temporary storage, convert
					// the challenge_phrase to ASCII hex and send it in two parts.
					for(x=0; x<2; x++)
					{
						for(idx=0; idx<sizeof(jsa->challenge_phrase)>>1; idx++)
						{
							sprintf(&jsa->challenge_response[(idx<<1)], "%02X", jsa->challenge_phrase[idx + x*(sizeof(jsa->challenge_phrase)>>1) ]);
						}
						Socket_send(jsonSockets[socketIndex].JSONSocketConnection, jsa->challenge_response, sizeof(jsa->challenge_response),  0, NULL, __PRETTY_FUNCTION__);
					}

					// Print a space, followed by the MAC address
					jsa->challenge_response[0]=' ';
					for(x=0; x<sizeof(myMACAddress); x++)
					{
						sprintf(&jsa->challenge_response[1+(x<<1)], "%02X", myMACAddress[x]);
					}
					Socket_send(jsonSockets[socketIndex].JSONSocketConnection, jsa->challenge_response, strlen(jsa->challenge_response),  0, NULL, __PRETTY_FUNCTION__);

					jsa->rx_count = 0;
					memset(jsa->challenge_response, 0, sizeof(jsa->challenge_response));
					jsonSockets[socketIndex].jsa_state = JSA_STATE_CHALLENGE_SENT;
				}

				//-------------------------------------------------------------

				if(jsonSockets[socketIndex].jsa_state == JSA_STATE_CHALLENGE_SENT)
				{
					uint8_t nibble;
					int32_t recv;

					while((recv=Socket_recv(jsonSockets[socketIndex].JSONSocketConnection, (void *) &x, 1, 0, NULL, "JSA_STATE_CHALLENGE_SENT")) > 0)
					{
						nibble = x < 'A' ? (x-'0') : ((0x20|x)-87);  // ASCII hex to nibble conversion (upper or lower case)
						jsa->challenge_response[(jsa->rx_count>>1) & (sizeof(jsa->challenge_response)-1)] |= nibble << (jsa->rx_count&1 ? 0:4);
						jsa->rx_count++;
					}

					if(recv<0)
					{
						broadcastDebug(socketIndex, "Socket_Task/JSA - Socket closed before receiving challenge response:", "");
						jsonSockets[socketIndex].jsa_state = JSA_STATE_IDLE;
						socketIndex++;
						continue;
					}

					if(jsa->rx_count >= sizeof(jsa->challenge_response)<<1)
					{
						jsonSockets[socketIndex].jsa_state = JSA_STATE_AUTH_RECEIVED;
						jsa->event_timer = Eliot_TimeNow()+JSA_EVAL_WAIT_SEC;
					}
				}

				//-------------------------------------------------------------

				if(jsonSockets[socketIndex].jsa_state == JSA_STATE_AUTH_RECEIVED)
				{
					if(Eliot_TimeNow() > jsa->event_timer)
					{
						json_socket_key* local=0;
						uint8_t encrypted_challenge[sizeof(jsa->challenge_phrase)];
						Aes aes;

						if(JSA_UserKeyIsSet())
						{
							// User key is set, use it.
							DBGPRINT_INTSTRSTR(0, "JSA_HandleKeysProperty() - Using user key", "");
							local = (json_socket_key*)FBK_Address(&eliot_fbk, LCM_DATA_JSON_SOCKET_USER_KEY_OFFSET);
						}
						else
						{
							DBGPRINT_INTSTRSTR(0, "JSA_HandleKeysProperty() - Using factory key", "");
							local = &json_socket_factory_key;
						}

						wc_AesSetKey(&aes, (byte*)&local->content, sizeof(local->content), 0, AES_ENCRYPTION);
						wc_AesEncrypt(&aes, jsa->challenge_phrase, encrypted_challenge);

#if DEBUG_SOCKETTASK_AUTH
						char dbg[3*sizeof(jsa->challenge_response)+2];
						for(x=0; x<16; x++)
						{
							sprintf(&dbg[x*3], "%02X ", encrypted_challenge[x]);
						}
						broadcastDebug(strlen(dbg), "Socket_Task/JSA - Expected challenge response:", dbg);
#endif
						// Compare the challenge response with the encrypted challenge.
						if(memcmp(encrypted_challenge, jsa->challenge_response, sizeof(jsa->challenge_phrase)))
						{
							jsa->event_timer = Eliot_TimeNow()+JSA_EVAL_WAIT_SEC;
							Socket_send(jsonSockets[socketIndex].JSONSocketConnection, JSA_AUTH_DENIED_MESSAGE, sizeof(JSA_AUTH_DENIED_MESSAGE),  0, NULL, __PRETTY_FUNCTION__);
							jsonSockets[socketIndex].jsa_state = JSA_STATE_AUTH_DENIED;
						}
						else
						{
							Socket_send(jsonSockets[socketIndex].JSONSocketConnection, JSA_AUTH_OK_MESSAGE, sizeof(JSA_AUTH_OK_MESSAGE),  0, NULL, __PRETTY_FUNCTION__);
							jsonSockets[socketIndex].jsa_state = JSA_STATE_AUTH_VALID;
						}

#if DEBUG_SOCKETTASK_AUTH
						char dbg[3*sizeof(jsa->challenge_response)+2];
						for(x=0; x<16; x++)
						{
							sprintf(&dbg[x*3], "%02X ", jsa->challenge_response[x]);
						}
						if(jsonSockets[socketIndex].jsa_state == JSA_STATE_AUTH_VALID)
						{
							broadcastDebug(0, "Socket_Task/JSA - Auth accepted:", dbg);
						}
						else
						{
							broadcastDebug(0, "Socket_Task/JSA - Auth rejected:", dbg);
						}
#endif
					}
				}

				//-------------------------------------------------------------

				if(jsonSockets[socketIndex].jsa_state == JSA_STATE_AUTH_DENIED)
				{
					if(Eliot_TimeNow() > jsa->event_timer)
					{
						ShutdownSocket(socketIndex, 0);
					}
				}

				//-------------------------------------------------------------

				// No further processing for this socket until authenticated
				socketIndex++;
				continue;
			}

            //========= JSON Socket Auth End ==================================

            else if (JSONtransmitQueueIsEmpty(socketIndex))
            {  // don't allow any further packet reception if we're sending a json message.
               doneFlag = false;
               while (!doneFlag)
               {
#ifdef USE_SSL
                  if((socketIndex == CLOUD_SERVER_SOCKET_INDEX) && 
                     (ssl != NULL))
                  {
                     result = wolfSSL_read(ssl, rxbuf, sizeof(rxbuf));

                     int err = wolfSSL_get_error(ssl, result);
                     if(err == SSL_ERROR_WANT_READ)
                     {
                        char errorString[80];
                        wolfSSL_ERR_error_string(err, errorString);

                        result = 0;
                     }
                  }
                  else
#endif

                  {
                     result = Socket_recv(jsonSockets[socketIndex].JSONSocketConnection, (void *) rxbuf, sizeof(rxbuf), 0, NULL, __PRETTY_FUNCTION__);
                  }

                  if (result == sizeof(rxbuf))
                  {  // we received a byte
                     if (jsonSockets[socketIndex].jsonReceiveMessageIndex >= jsonSockets[socketIndex].jsonReceiveMessageAllocSize)
                     {  // don't have a place to put the newly received byte.  See if we can get (or expand) the buffer.
                        if (!jsonSockets[socketIndex].jsonReceiveMessageOverrunFlag)
                        {  // don't keep trying to alloc more memory if we've already triggered an overrun
                           if (jsonSockets[socketIndex].jsonReceiveMessageAllocSize < SOCKET_RECEIVE_BUFFER_MAX_SIZE)
                           {  // only allow the buffer to expand to a limit
                              // try to get the new buffer, expanded by SOCKET_RECEIVE_BUFFER_ALLOC_CHUNK size
                              newReceiveBufferPtr = _mem_alloc_system(jsonSockets[socketIndex].jsonReceiveMessageAllocSize + SOCKET_RECEIVE_BUFFER_ALLOC_CHUNK);
                              if (!newReceiveBufferPtr)
                              {  // couldn't get memory for the expanded receive buffer
                                 malloc_fail_count++;
                                 jsonSockets[socketIndex].jsonReceiveMessageIndex = 0;
                                 jsonSockets[socketIndex].jsonReceiveMessageOverrunFlag = true;
                                 // free the existing receive buffer - since we can't use its contents
                                 if (jsonSockets[socketIndex].jsonReceiveMessage)
                                 {
                                    _mem_free(jsonSockets[socketIndex].jsonReceiveMessage);
                                    free_count++;
                                    jsonSockets[socketIndex].jsonReceiveMessage = NULL;
                                 }
                              }
                              else
                              {
                                 malloc_count++;
                                 if (jsonSockets[socketIndex].jsonReceiveMessageAllocSize)
                                 {  // only need to copy if we had a buffer already
                                    memcpy(newReceiveBufferPtr, jsonSockets[socketIndex].jsonReceiveMessage,
                                          jsonSockets[socketIndex].jsonReceiveMessageAllocSize);
                                    if (jsonSockets[socketIndex].jsonReceiveMessage)
                                    {
                                       _mem_free(jsonSockets[socketIndex].jsonReceiveMessage);
                                       free_count++;
                                       jsonSockets[socketIndex].jsonReceiveMessage = NULL;
                                    }
                                 }
                                 // point to our new buffer, and increase our jsonReceiveMessageAllocSize to the new limit.
                                 jsonSockets[socketIndex].jsonReceiveMessage = newReceiveBufferPtr;
                                 jsonSockets[socketIndex].jsonReceiveMessageAllocSize += SOCKET_RECEIVE_BUFFER_ALLOC_CHUNK;

                                 // finally, save the received byte in the new buffer
                                 jsonSockets[socketIndex].jsonReceiveMessage[jsonSockets[socketIndex].jsonReceiveMessageIndex] = *rxbuf;
                                 jsonSockets[socketIndex].jsonReceiveMessageIndex++;
                              }
                           }
                        }
                     }
                     else
                     {  // didn't need to worry about expanding the buffer - simply save the received byte
                        jsonSockets[socketIndex].jsonReceiveMessage[jsonSockets[socketIndex].jsonReceiveMessageIndex] = *rxbuf;
                        jsonSockets[socketIndex].jsonReceiveMessageIndex++;

                     }

                     if (*rxbuf == 0x00)
                     {  // null terminator signifies end of json packet
                        lwgpio_set_value(&jsonActivityLED, LWGPIO_VALUE_LOW);
                        jsonActivityTimer = 100;
                        jsonBroadcastObject = NULL;
                        if (jsonSockets[socketIndex].jsonReceiveMessageOverrunFlag)
                        {
                           jsonResponseObject = json_object();
                           if (jsonResponseObject)
                           {
                              snprintf(jsonErrorString, sizeof(jsonErrorString), "JSON packet exceeds max buffer size of %d.",
                                    jsonSockets[socketIndex].jsonReceiveMessageAllocSize);
                              DBGPRINT_INTSTRSTR_ERROR(__LINE__,"Socket_Task fails",jsonErrorString);
                              BuildErrorResponseNoID(jsonResponseObject, jsonErrorString, USR_JSON_PACKET_EXCEEDS_MAX_BUFFER_SIZE);
                           }
                           // already released this buffer's memory, but still need to reset its jsonReceiveMessageAllocSize
                           jsonSockets[socketIndex].jsonReceiveMessageAllocSize = 0;
                           root = NULL;  // tag the root as null so we don't try to dereference it later
                        }
                        else
                        {
                           memcpy(httpBuf, jsonSockets[socketIndex].jsonReceiveMessage, sizeof(httpBuf));
                           if (strstr(httpBuf,"HTTP") != NULL)
                           {
                              jsonSockets[socketIndex].httpSocket = TRUE;
                                  
                           }
                           // if httpSocket is TRUE, strip the http stuff off of the message
                           //     then handle the json
                           if (jsonSockets[socketIndex].httpSocket == TRUE)
                           {
                               int ix = 0;
                               while ((ix < sizeof(httpBuf)) && (httpBuf[ix] != '{'))
                               {
                                  ix++;
                               }
                               if (httpBuf[ix] == '{') // we found the json data
                               {
                                  memcpy(&jsonSockets[socketIndex].jsonReceiveMessage[0], &httpBuf[ix], sizeof(httpBuf));
                               }
                               else  // is the packet broke up into multiple packets?? 
                               {
                                  jsonSockets[socketIndex].jsonReceiveMessageAllocSize = 0;
                                  root = NULL;  // tag the root as null so we don't try to dereference it later
                                  jsonSockets[socketIndex].jsonReceiveMessage = 0; // discard this message
                               }
                           }
                           if (jsonSockets[socketIndex].jsonReceiveMessage)
                           {
                              root = json_loads((const char *) jsonSockets[socketIndex].jsonReceiveMessage, 0, &jsonError);
                              // now that we've built the json object, immediately release the buffer's memory.
                              _mem_free(jsonSockets[socketIndex].jsonReceiveMessage);
                              free_count++;
                              jsonSockets[socketIndex].jsonReceiveMessage = NULL;
                              jsonSockets[socketIndex].jsonReceiveMessageAllocSize = 0;
                           }
                           else
                           {
                              root = NULL;
                           }

                           if (!root && (jsonSockets[socketIndex].httpSocket == FALSE))
                           {
                              DBGPRINT_INTSTRSTR_ERROR(__LINE__,"Socket_Task fails: BadJsonFormat","");
                              jsonResponseObject = BuildErrorResponseBadJsonFormat(&jsonError);
                           }
                           else
                           {
#ifdef MEMORY_DIAGNOSTIC_PRINTF
                              if (globalShowMemoryFlag)
                              {
                                 snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "Static RAM Usage:   %d Bytes\r\n", (uint32_t)_mqx_get_initialization()->START_OF_KERNEL_MEMORY - 0x1FFF0000);
                                 snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "  Peak RAM Usage:   %d Bytes(%d%%)\r\n", (uint32_t)_mem_get_highwater() - 0x1FFF0000, ((uint32_t)_mem_get_highwater() - 0x1FFF0000)/0x51E);
                                 snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "Current RAM Free:   %d Bytes(%d%%)\r\n", _mem_get_free(), _mem_get_free()/0x51E);
                                 globalShowMemoryFlag = 0;
                              }
#endif
                              socketIndexGlobalRx = socketIndex;
                              jsonResponseObject = ProcessJsonPacket(root, &jsonBroadcastObject);
                           }
                        }
                        if (root)
                        {
                           json_decref(root);
                        }
                        if (jsonResponseObject)
                        {
                           jsonResponseStatusObject = json_object_get(jsonResponseObject, "Status");
                           if (!jsonResponseStatusObject)
                           {  // if we haven't already tagged the message with an error, add a "Status": "Success" tag.
                              json_object_set_new(jsonResponseObject, "Status", json_string("Success"));
#ifdef MEMORY_DIAGNOSTIC_PRINTF
                              if (globalShowMemoryFlag)
                              {
                                 snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "Static RAM Usage:   %d Bytes\r\n", (uint32_t)_mqx_get_initialization()->START_OF_KERNEL_MEMORY - 0x1FFF0000);
                                 snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "  Peak RAM Usage:   %d Bytes(%d%%)\r\n", (uint32_t)_mem_get_highwater() - 0x1FFF0000, ((uint32_t)_mem_get_highwater() - 0x1FFF0000)/0x51E);
                                 snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "Current RAM Free:   %d Bytes(%d%%)\r\n", _mem_get_free(), _mem_get_free()/0x51E);
                                 globalShowMemoryFlag = 0;
                              }
#endif
                              SendJSONPacket(socketIndex, jsonResponseObject);
#ifdef MEMORY_DIAGNOSTIC_PRINTF
                              if (globalShowMemoryFlag)
                              {
                                 snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "Static RAM Usage:   %d Bytes\r\n", (uint32_t)_mqx_get_initialization()->START_OF_KERNEL_MEMORY - 0x1FFF0000);
                                 snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "  Peak RAM Usage:   %d Bytes(%d%%)\r\n", (uint32_t)_mem_get_highwater() - 0x1FFF0000, ((uint32_t)_mem_get_highwater() - 0x1FFF0000)/0x51E);
                                 snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "Current RAM Free:   %d Bytes(%d%%)\r\n", _mem_get_free(), _mem_get_free()/0x51E);
                                 globalShowMemoryFlag = 0;
                              }
#endif
                              json_decref(jsonResponseObject);
#ifdef MEMORY_DIAGNOSTIC_PRINTF
                              if (globalShowMemoryFlag)
                              {
                                 snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "Static RAM Usage:   %d Bytes\r\n", (uint32_t)_mqx_get_initialization()->START_OF_KERNEL_MEMORY - 0x1FFF0000);
                                 snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "  Peak RAM Usage:   %d Bytes(%d%%)\r\n", (uint32_t)_mem_get_highwater() - 0x1FFF0000, ((uint32_t)_mem_get_highwater() - 0x1FFF0000)/0x51E);
                                 snprintf(diagnosticsBuf, sizeof(diagnosticsBuf), "Current RAM Free:   %d Bytes(%d%%)\r\n", _mem_get_free(), _mem_get_free()/0x51E);
                                 globalShowMemoryFlag = 0;
                              }
#endif
                              jsonResponseObject = NULL;
                              if (jsonBroadcastObject)
                              {  // if we've got a broadcast message, send it
                                 SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, jsonBroadcastObject);
                                 json_decref(jsonBroadcastObject);
                                 jsonBroadcastObject = NULL;
                              }
                           }
                           else
                           {
                              SendJSONPacket(socketIndex, jsonResponseObject);
                              json_decref(jsonResponseObject);
                           }
                        }
                        if (jsonBroadcastObject)
                        {
                           json_decref(jsonBroadcastObject);
                        }

                        jsonSockets[socketIndex].jsonReceiveMessageIndex = 0;
                        jsonSockets[socketIndex].jsonReceiveMessageOverrunFlag = false;

                        doneFlag = true;
                     }
                  }
                  else if (result == RTCS_ERROR)
                  {
                     if (jsonSockets[socketIndex].JSONSocketConnection)
                     {
                        error = RTCS_geterror(jsonSockets[socketIndex].JSONSocketConnection);
                        _time_delay(100);
                        _mutex_lock(&JSONTransmitMutex);
                        ShutdownSocket(socketIndex, LED_NUMBER_2);
                        _mutex_unlock(&JSONTransmitMutex);
                        _time_delay(100);
                     }
                     doneFlag = true;
                  }
                  else
                  {
                     doneFlag = true;
                     // check if enough time has expired from our last transmission to this socket
                     //Do Not Ping WatchSockets
                     if (jsonSocketTimer[socketIndex] > MAX_SOCKET_QUIET_TIME && !jsonSockets[socketIndex].WatchSocket)
                     {
                        jsonResponseObject = json_object();
                        if (jsonResponseObject)
                        {
                           TIME_STRUCT mqxTime;
                           _time_get(&mqxTime);

                           json_object_set_new(jsonResponseObject, "ID", json_integer(0));
                           json_object_set_new(jsonResponseObject, "Service", json_string("ping"));
                           // Need to subtract out the effective GMT offset so the 
                           // time display looks correct in the app
                           json_object_set_new(jsonResponseObject, "CurrentTime", json_integer(mqxTime.SECONDS - systemProperties.effectiveGmtOffsetSeconds));
                           json_object_set_new(jsonResponseObject, "PingSeq", json_integer((int) ++socketPingSeqno[socketIndex]));
                           json_object_set_new(jsonResponseObject, "Status", json_string("Success"));
                           SendJSONPacket(socketIndex, jsonResponseObject);
                           json_decref(jsonResponseObject);
                        }

                        if (firmwareUpdateState == UPDATE_STATE_READY)
                        {
                           if (updateAlertPingCounter[socketIndex])
                           {
                              updateAlertPingCounter[socketIndex]--;
                              if (!updateAlertPingCounter[socketIndex])
                              {
                                 firmwareUpdateState = UPDATE_STATE_DOWNLOADING;
                                 UnicastSystemInfo(socketIndex);
                                 
                                 firmwareUpdateState = UPDATE_STATE_READY;
                                 UnicastSystemInfo(socketIndex);
                              }
                           }
                        }
                        else
                        {
                           updateAlertPingCounter[socketIndex] = 0;
                        }
                     }

                  }  // if (result == sizeof(rxbuf))

               }  // while (!doneFlag)

            }  // else if ((!QLink_IsTransmittingPacket()) && (JSONtransmitQueueIsEmpty(socketIndex)))

         }  // if (jsonSockets[socketIndex].JSONSocketConnection)

         _sched_yield();  // nothing to do, yield to the next transmit task
         socketIndex++;

      }  // while (socketIndex < MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS)

      active_incoming_json_connections = temp_active_incoming_json_connections;

      ipcfg_get_ip(BSP_DEFAULT_ENET_DEVICE, &latest_ip_data);
      if (latest_ip_data.ip != current_ip_data.ip)
      {
         if (MDNSlistensock)
         {
            shutdown(MDNSlistensock, FLAG_ABORT_CONNECTION);
         }
      }
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void SendBroadcastMessageForMTMCheckFlashResults(bool_t errorFlag)
{
   json_t * flashCheckObject;

   flashCheckObject = json_object();
   if (flashCheckObject)
   {
      json_object_set_new(flashCheckObject, "ID", json_integer(0));
      json_object_set_new(flashCheckObject, "Service", json_string("CheckFlashResults"));
   
      if (errorFlag)
      {
         json_object_set_new(flashCheckObject, "Flash", json_string("Bad"));
      }
      else
      {
         json_object_set_new(flashCheckObject, "Flash", json_string("Good"));
      }
   
      json_object_set_new(flashCheckObject, "Status", json_string("Success"));
   
      SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, flashCheckObject);
      json_decref(flashCheckObject);
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

word_t GetPayload(word_t lineNumber, char * UDPBuf, word_t UDPBufSize)
// return 0 = timeout
// return non-0 = payload length (including payload start)
{
   volatile int_32 result;
   word_t timeoutCount = 0;
   word_t currentLoadIndex = 0;
   char payloadStartString[5] = { 0x0D, 0x0A, 0x0D, 0x0A, 0x00 };
   volatile char * payloadStartPtr;
   volatile char * payloadEndPtr;
   volatile int receivedLineNumber;
   volatile int payloadLength;
   
   while (timeoutCount < SOCKETTASK_RECV_PAYLOAD_MAX_RETRIES)
   {
      result = Socket_recv(updateSock, (void *) (&UDPBuf[currentLoadIndex]), (UDPBufSize - currentLoadIndex - 1), 0, NULL, __PRETTY_FUNCTION__);

      if ((result > 0) && (result <= (UDPBufSize - currentLoadIndex - 1)))
      {
         // force a zero at the end of the received buffer to make sure we don't run past our buffer
         UDPBuf[currentLoadIndex + result] = 0;
         // check if we have the start of the payload
         payloadStartPtr = strstr(UDPBuf, payloadStartString);
         if (payloadStartPtr)
         {
            // we've got the start of the payload - let's see if we've got the end.
            payloadEndPtr = payloadStartPtr;
            if (payloadStartPtr != UDPBuf)
            {  // compress the received buffer
               payloadLength = strlen((char *) payloadStartPtr);
               memcpy(UDPBuf, (char *) payloadStartPtr, (payloadLength + 1));
               currentLoadIndex = payloadLength;
            }
            payloadEndPtr = &UDPBuf[4];
            while ((*payloadEndPtr != 0) && (*payloadEndPtr != 0x1E))
            {
               payloadEndPtr++;
            }
            if (*payloadEndPtr)
            {  // we found the end of the payload
               // check if it's the line we want
               receivedLineNumber = atoi(UDPBuf);
               if (receivedLineNumber == lineNumber)
               {  // success
                  return (payloadEndPtr - UDPBuf);
               }
               else
               {
                  // continue looking.  compress the buffer to move the payload start bytes out
                  if (currentLoadIndex > 4)
                  {
                     memcpy(UDPBuf, &UDPBuf[4], (UDPBufSize - 4));
                     currentLoadIndex -= 4;
                  }
               }
            }
            else if (payloadStartPtr == UDPBuf)
            {
               currentLoadIndex += result;
            }
         }
         else
         {
            // didn't find the payload start indicator, so compress the buffer
            currentLoadIndex += result;
            if (currentLoadIndex > 4)
            {
               // stage the last 4 bytes of the current received buffer as the first 4 bytes in the buffer.
               memcpy(UDPBuf, &UDPBuf[currentLoadIndex - 4], 4);
               currentLoadIndex = 4;
            }
         }
      }
      else
      {
         _time_delay(SOCKETTASK_RECV_PAYLOAD_DELAY_MS);
         timeoutCount++;
      }
   }

   return 0;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void ConnectToUpdateServer(char * updateHostString)
{
   sockaddr_in remote_sin;
   int_32 result;
   uint_32 opt_value;
   _ip_address hostaddr = 0;
   byte_t retryCount = 0;
#if DEBUG_UPDATE_HOST_RESOLVE
   char ipAddressStringForLogging[MAX_SIZE_IP_ADDRESS_STRING];
#endif

   while ((hostaddr == 0) && (retryCount < MAX_NUMBER_OF_IP_ADDRESS_RESOLUTION_ATTEMPTS))
   {
      RTCS_resolve_ip_address(updateHostString, &hostaddr, updateServerHostname, MAX_HOSTNAMESIZE);
      retryCount++;
   }

   if (hostaddr == 0)
   {
      printf("\nError, RTCS_resolve_ip_address() failed.");
      return;
   }

#if DEBUG_UPDATE_HOST_RESOLVE
   broadcastDebug(0, "updateHostString", updateHostString);
   broadcastDebug(0, "updateServerHostname", updateServerHostname);
   snprintf(ipAddressStringForLogging
             MAX_SIZE_IP_ADDRESS_STRING,
              "%d.%d.%d.%d",
               HighByteOfDword(hostaddr),
                UpperMiddleByteOfDword(hostaddr),
                 LowerMiddleByteOfDword(hostaddr),
                  LowByteOfDword(hostaddr));
   broadcastDebug(0, "updateServerAddress", ipAddressStringForLogging);
#endif

   updateSock = socket(AF_INET, SOCK_STREAM, 0);
   if (updateSock == RTCS_SOCKET_ERROR)
   {
      printf("\nError, socket() failed.");
      return;
   }

   opt_value = 180000;  // change the connect timeout to 3 minutes 
   result = setsockopt(updateSock, SOL_TCP, OPT_CONNECT_TIMEOUT, &opt_value, sizeof(opt_value));

   memset((char *) &remote_sin, 0, sizeof(sockaddr_in));
   remote_sin.sin_family = AF_INET;
   remote_sin.sin_port = 80;
   remote_sin.sin_addr.s_addr = hostaddr;
   result = connect(updateSock, (struct sockaddr *) &remote_sin, sizeof(sockaddr_in));
   if (result != RTCS_OK)
   {
      printf("\nError--connect() failed with error code %lx.", result);
      return;
   }

   opt_value = true;
   result = setsockopt(updateSock, SOL_SOCKET, OPT_RECEIVE_NOWAIT, &opt_value, sizeof(opt_value));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool_t CheckForNewerFirmware(word_t * currentFirmwareVersion, word_t * newFirmwareInfo)
{
   if (newFirmwareInfo[0] > currentFirmwareVersion[0])
   {
      return true;
   }
   else if (newFirmwareInfo[0] == currentFirmwareVersion[0])
   {
      if (newFirmwareInfo[1] > currentFirmwareVersion[1])
      {
         return true;
      }
      else if (newFirmwareInfo[1] == currentFirmwareVersion[1])
      {
         if (newFirmwareInfo[2] > currentFirmwareVersion[2])
         {
            return true;
         }
         else if (newFirmwareInfo[2] == currentFirmwareVersion[2])
         {
            if (newFirmwareInfo[3] > currentFirmwareVersion[3])
            {
               return true;
            }
         }
      }
   }
   return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void sha256BinaryToString(byte_t * binaryPtr, char * stringPtr, uint16_t maxStringLength)
{
   byte_t byteIndex;

   // check for null pointers and that we have enough space to store our resulting string
   if ((binaryPtr) && (stringPtr) && (maxStringLength >= ((SHA256_DIGEST_SIZE * 2) + 1)))
   {
      for (byteIndex = 0; byteIndex < SHA256_DIGEST_SIZE; byteIndex++)
      {
        sprintf(stringPtr, "%02X", binaryPtr[byteIndex]);
        stringPtr += 2;
      }
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

word_t GetAndStorePayload(void)
// return 0 = error
// return 1 = firmware downloaded
{
   int_32 result;
   word_t timeoutCount = 0;
   word_t currentLoadIndex = 0;
   char payloadStartString[5] = { 0x0D, 0x0A, 0x0D, 0x0A, 0x00 };
   char * payloadStartPtr;
   char * payloadEndPtr;
   int receivedLineNumber;
   int payloadLength;
   bool_t foundPayloadFlag = false;
   bool_t doneParsingFlag;
   byte_t S19RecordBytes[0x51];
   byte_t S19RecordLength;
   byte_t S19RecordValid;
   json_t * updateMessageObject;
   uint_32 percentageDone;
   uint_32 lastPercentageDone = 0;
   bool_t errorFlag;
   json_t * errorString;
   Sha256 sha256;
   byte_t sha256BinaryArray[SHA256_DIGEST_SIZE];
   int sha256Error = 0;

   while (timeoutCount < 200)
   {
      result = Socket_recv(updateSock, (void *) (&updateBuf[currentLoadIndex]), (sizeof(updateBuf) - currentLoadIndex - 1), 0, NULL, __PRETTY_FUNCTION__);

      if ((result > 0) && (result <= (sizeof(updateBuf) - currentLoadIndex - 1)))
      {
         timeoutCount = 0;
         // force a zero at the end of the received buffer to make sure we don't run past our buffer
         updateBuf[currentLoadIndex + result] = 0;
         // check if we have the start of the payload
         if (!foundPayloadFlag)
         {
            payloadStartPtr = strstr(updateBuf, payloadStartString);
            if (payloadStartPtr)
            {  // we've got the start of the payload - shift the payload into the beginning of the buffer
               payloadStartPtr += 4;  // skip the payload indicator bytes
               foundPayloadFlag = true;
               payloadLength = strlen(payloadStartPtr);
               memcpy(updateBuf, payloadStartPtr, (payloadLength + 1));
               currentLoadIndex = payloadLength;
               firmwareUpdateLineNumber = 1;
               // initialize sha256 calculation
               sha256Error = wc_InitSha256(&sha256);
               if (sha256Error != 0) {
                  broadcastDebug(sha256Error, "sha256 init failed", "");
               }
               else
               {  // if we initialized properly, use this first received buffer to start the sha256 calculation
                  sha256Error = wc_Sha256Update(&sha256, updateBuf, payloadLength);
                  if (sha256Error != 0) {
                     broadcastDebug(sha256Error, "sha256 update failed", "");
                  }
               }
            }
         }
         else if (!sha256Error)
         {
            // before we parse the buffer for the individual s19 lines.
            // run the raw bytes through the sha256 calculator to make sure we use the actual bytes that were received.
            sha256Error = wc_Sha256Update(&sha256, &updateBuf[currentLoadIndex], result);
            if (sha256Error != 0) {
               broadcastDebug(sha256Error, "sha256 update failed", "");
            }
         }

         if (foundPayloadFlag)
         {  // we've got a payload and it's left justified to the start of the buffer.
            // parse any complete records.
            payloadStartPtr = updateBuf;
            doneParsingFlag = false;
            while (!doneParsingFlag)
            {
               while ((*payloadStartPtr) && (*payloadStartPtr != 'S'))
               {
                  payloadStartPtr++;
               }
               if (*payloadStartPtr == 'S')
               {  // we've got the start of a record - see if we have the end
                  payloadEndPtr = payloadStartPtr + 1;
                  while ((*payloadEndPtr) && (*payloadEndPtr != 0x0A))
                  {
                     payloadEndPtr++;
                  }

                  if (*payloadEndPtr)
                  {

                     S19RecordValid = ValidateS19Record(payloadStartPtr, S19RecordBytes, sizeof(S19RecordBytes), &S19RecordLength);
                     if (S19RecordValid)
                     {
                        errorFlag = HandleS19RecordString(payloadStartPtr, S19RecordBytes, S19RecordLength, jsonErrorString, sizeof(jsonErrorString));

                        if (errorFlag)
                        {
                           errorString = json_string(jsonErrorString);
                           updateMessageObject = json_object();
                           if (updateMessageObject)
                           {
                              json_object_set_new(updateMessageObject, "ID", json_integer(0));
                              json_object_set_new(updateMessageObject, "Service", json_string("FirmwareUpdate"));
                              json_object_set_new(updateMessageObject, "Error", errorString);
                              SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, updateMessageObject);
                              json_decref(updateMessageObject);
                           }

                           firmwareUpdateState = UPDATE_STATE_NONE;
                           BroadcastSystemInfo();
                           return 0;
                        }
                        else
                        {
                           timeoutCount = 0;
                           firmwareUpdateLineNumber++;

                           percentageDone = firmwareUpdateLineNumber;
                           percentageDone *= 100;
                           percentageDone /= updateFirmwareInfo.S19LineCount;
                           if (percentageDone != lastPercentageDone)
                           {
                              lastPercentageDone = percentageDone;
                              BroadcastSystemInfo();
                           }
                        }
                     }
                     else
                     {
                        return 0;  // failed s-record
                     }

                     payloadStartPtr++;
                     if (*payloadStartPtr == '7')  // "S7" is the final record.
                     {
                        // assume an error - overwrite if actually valid
                        snprintf(updateFirmwareInfo.sha256HashString, sizeof(updateFirmwareInfo.sha256HashString), "SHA256 CALC ERROR");
                        if (!sha256Error)
                        {  // finalize the sha256
                           sha256Error = wc_Sha256Final(&sha256, sha256BinaryArray);
                           if (!sha256Error) {
                              sha256BinaryToString(sha256BinaryArray, updateFirmwareInfo.sha256HashString, sizeof(updateFirmwareInfo.sha256HashString));
                           }
                           else
                           {
                              broadcastDebug(sha256Error, "sha256 final failed", "");
                           }
                        }
                        updateFirmwareInfo.reportSha256Flag = true;
                        // broadcast the system info so we can include the newly calculated sha256 value.
                        // this is to allow the app to use that sha256 prior to receiving the follow-up
                        // broadcast that reports the firmware is ready to use.
                        BroadcastSystemInfo();
                        return 1;
                     }
                     else
                     {
                        payloadStartPtr = payloadEndPtr + 1;
                     }
                  }
                  else
                  {
                     payloadLength = strlen(payloadStartPtr);
                     memcpy(updateBuf, payloadStartPtr, (payloadLength + 1));
                     currentLoadIndex = payloadLength;
                     doneParsingFlag = true;
                  }
               }
               else
               {
                  currentLoadIndex = 0;
                  doneParsingFlag = true;
               }
            }
         }
      }
      else
      {
         _time_delay(50);
         timeoutCount++;
      }
   }

   return 0;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void FirmwareUpdate_Task(uint_32 param)
{
   int_32 result;
   uint_32 returnCode;
   json_t * updateMessageObject;
   word_t payloadLength;
   byte_t timeoutCount = 0;
   firmware_info newFirmwareInfo;
   _queue_id updateTaskMessageQID;
   UPDATE_TASK_MESSAGE_PTR updateTaskMessagePtr;
   char updateServerString[MAX_SIZE_OF_UPDATE_SERVER_STRING];
   char updateVersionBranchString[MAX_SIZE_OF_UPDATE_VERSION_BRANCH_STRING];
   char sscanfFormatString[MAX_SIZE_OF_UPDATE_SSCANF_FORMAT_STRING];
   bool_t checkForUpdateFlag = false;
   bool_t useNewFirmwareFlag;
   json_t * updateHostObj = NULL;
   json_t * updateVersionBranchObj = NULL;

   firmwareUpdateState = UPDATE_STATE_NONE;
   updateFirmwareInfo.reportSha256Flag = false;   // tag the previous hash information as invalid

   /* Open a message queue: */
   updateTaskMessageQID = _msgq_open(UPDATE_TASK_MESSAGE_QUEUE, 0);

   /* Create a message pool: */
   updateTaskMessagePoolID = _msgpool_create(sizeof(UPDATE_TASK_MESSAGE), NUMBER_OF_UPDATE_MESSAGES_IN_POOL, 0, 0);

   for (;;)
   {
      // first, check if we've got an incoming request
      if ((checkForUpdateFlag) || (firmwareUpdateState == UPDATE_STATE_DOWNLOADING))
      {  // we're in the middle of update operations - just look to see if anyone has sent us a new command
         _sched_yield();  // let the other tasks at this priority run
         updateTaskMessagePtr = _msgq_poll(updateTaskMessageQID);
         Eliot_SendFirmwareInfo(0);
      }
      else
      {  // we need to wait for someone else to tell us to continue
         updateTaskMessagePtr = _msgq_receive(updateTaskMessageQID, 0);
      }

      if (updateTaskMessagePtr)
      {
         switch (updateTaskMessagePtr->updateCommand)
         {
            case UPDATE_COMMAND_CHECK_FOR_UPDATE:
               // we're being asked to check for a normal update.  If we had previously been given a directed update,
               // we need to clear it out.
               if (updateHostObj)
               {
                  firmwareUpdateState = UPDATE_STATE_NONE;
                  json_decref(updateHostObj);
                  updateHostObj = NULL;
               }
               if (updateVersionBranchObj)
               {
                  firmwareUpdateState = UPDATE_STATE_NONE;
                  json_decref(updateVersionBranchObj);
                  updateVersionBranchObj = NULL;
               }

               if (updateSock)
               {
                  result = shutdown(updateSock, FLAG_ABORT_CONNECTION);
                  if (result != RTCS_OK)
                  {
                     printf("\nError, shutdown() failed with error code %lx", result);
                  }
                  updateSock = 0;
               }

               checkForUpdateFlag = true;
               break;

            case UPDATE_COMMAND_DOWNLOAD_FIRMWARE:
               // we're being commanded to do a directed update, where the app specifies the update host and the version-branch.
               // first check that we were not already in-process with an earlier directed update.
               if (updateHostObj)
               {
                  json_decref(updateHostObj);
                  updateHostObj = NULL;
               }
               if (updateVersionBranchObj)
               {
                  json_decref(updateVersionBranchObj);
                  updateVersionBranchObj = NULL;
               }
               firmwareUpdateState = UPDATE_STATE_NONE;
               updateFirmwareInfo.reportSha256Flag = false;   // tag the previous hash information as invalid

               if (updateSock)
               {
                  result = shutdown(updateSock, FLAG_ABORT_CONNECTION);
                  if (result != RTCS_OK)
                  {
                     printf("\nError, shutdown() failed with error code %lx", result);
                  }
                  updateSock = 0;
               }

               updateHostObj = updateTaskMessagePtr->serverStringObj;
               updateVersionBranchObj = updateTaskMessagePtr->versionBranchStringObj;
               checkForUpdateFlag = true;
               break;

            case UPDATE_COMMAND_INSTALL_FIRMWARE:
               broadcastDebug(restartSystemTimer, "UPDATE_COMMAND_INSTALL_FIRMWARE", "Entering InstallFirmwareToFlash()");
               returnCode = InstallFirmwareToFlash();
               broadcastDebug(returnCode, "UPDATE_COMMAND_INSTALL_FIRMWARE", "InstallFirmwareToFlash() returned code");

               if (returnCode != FTFx_OK)
               {
                  updateMessageObject = json_object();
                  if (updateMessageObject)
                  {
                     json_object_set_new(updateMessageObject, "ID", json_integer(0));
                     json_object_set_new(updateMessageObject, "Service", json_string("FirmwareUpdate"));
                     json_object_set_new(updateMessageObject, "Error", json_string("Swap Failed!"));
                     json_object_set_new(updateMessageObject, "returnCode", json_integer(returnCode));
                     SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, updateMessageObject);
                     json_decref(updateMessageObject);
                  }
               }
               else
               {
                  firmwareUpdateState = UPDATE_STATE_INSTALLING;
                  BroadcastSystemInfo();
               }

               restartSystemTimer = 2000;  // Set the restart countdown now
               _time_delay(3000);
               broadcastDebug(0, "restartSystemTimer failed - Forcing restart...", "");
               _time_delay(1000);
               SCB_AIRCR = SCB_AIRCR_VECTKEY(0x5FA) | SCB_AIRCR_SYSRESETREQ_MASK;

               break;

            case UPDATE_COMMAND_DISCARD_UPDATE:
               // we're being commanded to ignore our currently downaloded firmware, and immediately re-check for updates.
               // we will retain any updateHostObj and updateVersionBranchObj info that was used previously.
               firmwareUpdateState = UPDATE_STATE_NONE;
               updateFirmwareInfo.reportSha256Flag = false;   // tag the previous hash information as invalid

               // first check that we were not already in-process with an earlier directed update.
               if (updateSock)
               {
                  result = shutdown(updateSock, FLAG_ABORT_CONNECTION);
                  if (result != RTCS_OK)
                  {
                     printf("\nError, shutdown() failed with error code %lx", result);
                  }
                  updateSock = 0;
               }

               checkForUpdateFlag = true;
               break;

         }
         _msg_free(updateTaskMessagePtr);
      }  // if (UpdateTaskMessagePtr)

      if (checkForUpdateFlag)
      {
         if (!updateSock)
         {
            if (updateHostObj)
            {
               ConnectToUpdateServer((char *) json_string_value(updateHostObj));
            }
            else
            {
               ConnectToUpdateServer(SYSTEM_INFO_DEFAULT_UPDATE_HOST);
            }
            _time_delay(100);
         }
         if (updateSock)
         {  // we're connected to the update server, let's ask for the latest info for our branch
            if (updateHostObj)
            {
               snprintf(updateBuf, sizeof(updateBuf), "GET /%s/info HTTP/1.1\r\nHost: %s\r\n\r\n", json_string_value(updateVersionBranchObj),
                     updateServerHostname);
            }
            else
            {
               snprintf(updateBuf, sizeof(updateBuf), "GET /latest-%s/info HTTP/1.1\r\nHost: %s\r\n\r\n", SYSTEM_INFO_FIRMWARE_BRANCH,
                     updateServerHostname);
            }

            // DMC: Error not handled
            result = Socket_send(updateSock,
                  (void *)updateBuf,
                  strlen((const char *)updateBuf),
                  0, NULL, __PRETTY_FUNCTION__);
            jsonActivityTimer = 100;

            _time_delay(10);

            payloadLength = GetPayload(0, updateBuf, sizeof(updateBuf));  // 0 indicates an info line, rather than an actual s19 line
            if (payloadLength)
            {
               // sscanf will overrun string buffers unless we use a field width for strings in the format string
               // In the below scanf call we use the buffer size minus one as the field width to leave room for the null terminator that sscanf automatically adds.

               // Construct the format string for sscanf based on actual buffer sizes
               sprintf(sscanfFormatString, "L(%%d) V(%%d.%%d.%%d-%%d-%%%ds) D(%%%ds) B(%%%ds) N(%%%ds)",
                     sizeof(newFirmwareInfo.versionHash) - 1,
                     sizeof(newFirmwareInfo.dateString) - 1,
                     sizeof(newFirmwareInfo.branchString) - 1,
                     sizeof(newFirmwareInfo.releaseNotes) - 1);

               result = sscanf(&updateBuf[4], sscanfFormatString, &newFirmwareInfo.S19LineCount, &newFirmwareInfo.firmwareVersion[0],
                     &newFirmwareInfo.firmwareVersion[1], &newFirmwareInfo.firmwareVersion[2], &newFirmwareInfo.firmwareVersion[3], newFirmwareInfo.versionHash, newFirmwareInfo.dateString, newFirmwareInfo.branchString,
                     newFirmwareInfo.releaseNotes);

               if (result != 9)
               {  // something was wrong with the data returned - don't use it
                  useNewFirmwareFlag = false;
               }
               else
               {
                  if (updateHostObj)
                  {  // this was a directed update, so there is no need to check if the version is newer.  We know we need to download this.
                     useNewFirmwareFlag = true;
                  }
                  else if (firmwareUpdateState != UPDATE_STATE_NONE)
                  {  // we either already had a version, or we are currently downloading one.  In either case, compare the "latest" to this in-process version
                     useNewFirmwareFlag = CheckForNewerFirmware(updateFirmwareInfo.firmwareVersion, newFirmwareInfo.firmwareVersion);
                  }
                  else
                  {  // otherwise, simply compare to the currently running firmware's version.
                     useNewFirmwareFlag = CheckForNewerFirmware(currentFirmwareVersion, newFirmwareInfo.firmwareVersion);  // otherwise, use our current firmware info 
                  }
               }
               if (useNewFirmwareFlag)
               {
                  memcpy(&updateFirmwareInfo, &newFirmwareInfo, sizeof(firmware_info));
                  updateFirmwareInfo.reportSha256Flag = false;   // tag the hash information as invalid
                  firmwareUpdateLineNumber = 1;
                  firmwareUpdateState = UPDATE_STATE_DOWNLOADING;
                  BroadcastSystemInfo();
               }
               else if (updateTaskMessagePtr->updateCommand == UPDATE_COMMAND_DISCARD_UPDATE)
               {  // if we were looking for an update as the result of a DISCARD_UPDATE command,
                  // send the system info even if we don't find anything to update, so that our
                  // status is presented as UPDATE_STATE_NONE.
                  BroadcastSystemInfo();
               }
               else
               {  // we didn't find a newer version, but if we already have one "Ready", toggle the state to let the app know.
                  if (firmwareUpdateState == UPDATE_STATE_READY)
                  {
                     firmwareUpdateState = UPDATE_STATE_DOWNLOADING;
                     BroadcastSystemInfo();

                     firmwareUpdateState = UPDATE_STATE_READY;
                     BroadcastSystemInfo();
                  }
               }
               checkForUpdateFlag = false;
               checkForUpdatesTimer = SUBSEQUENT_UPDATE_CHECK_TIME;  // set the timer for our next automatic check
            }
            else
            {
               timeoutCount++;
               if (timeoutCount > MAX_NUMBER_OF_TIMEOUTS_FOR_UPDATE_RESPONSE)
               {
                  timeoutCount = 0;
                  result = shutdown(updateSock, FLAG_ABORT_CONNECTION);
                  if (result != RTCS_OK)
                  {
                     printf("\nError, shutdown() failed with error code %lx", result);
                  }

                  updateSock = 0;
                  checkForUpdateFlag = false;
                  checkForUpdatesTimer = SUBSEQUENT_UPDATE_CHECK_TIME;  // set the timer for our next automatic check
               }

            }
         }
      }

      else if (firmwareUpdateState == UPDATE_STATE_DOWNLOADING)
      {  // we're downloading - get the next line
         if (!updateSock)
         {
            if (updateHostObj)
            {
               ConnectToUpdateServer((char *) json_string_value(updateHostObj));
            }
            else
            {
               ConnectToUpdateServer(SYSTEM_INFO_DEFAULT_UPDATE_HOST);
            }
            _time_delay(100);
         }
         else
         {
            snprintf(updateFirmwareInfo.updateHost, sizeof(updateFirmwareInfo.updateHost), "%s", updateServerHostname);
            if (updateHostObj)
            {
               snprintf(updateFirmwareInfo.updatePath, sizeof(updateFirmwareInfo.updatePath), "%s", json_string_value(updateVersionBranchObj));
            }
            else
            {
               snprintf(updateFirmwareInfo.updatePath, sizeof(updateFirmwareInfo.updatePath), "%d-%d-%d-%d-%s-%s",
                    updateFirmwareInfo.firmwareVersion[0],
                    updateFirmwareInfo.firmwareVersion[1],
                    updateFirmwareInfo.firmwareVersion[2],
                    updateFirmwareInfo.firmwareVersion[3],
                    updateFirmwareInfo.versionHash,
                    SYSTEM_INFO_FIRMWARE_BRANCH);
            }

            // tag the current sha256 string as invalid
            updateFirmwareInfo.reportSha256Flag = false;

            snprintf(updateBuf, sizeof(updateBuf), "GET /%s/whole HTTP/1.1\r\nHost: %s\r\n\r\n", updateFirmwareInfo.updatePath, updateFirmwareInfo.updateHost);

            // DMC: Error not handled
            result = Socket_send(updateSock,
                  (void *)updateBuf,
                  strlen((const char *)updateBuf),
                  0, NULL, __PRETTY_FUNCTION__)
            ;
            jsonActivityTimer = 100;

            _time_delay(1000);

            payloadLength = GetAndStorePayload();
            if (payloadLength)
            {
               firmwareUpdateState = UPDATE_STATE_READY;

               BroadcastSystemInfo();

               if (updateSock)
               {
                  result = shutdown(updateSock, FLAG_ABORT_CONNECTION);
                  if (result != RTCS_OK)
                  {
                     printf("\nError, shutdown() failed with error code %lx", result);
                  }
                  updateSock = 0;
               }
            }
         }
      }
   }

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool_t SendUpdateCommand(byte_t commandIndex, json_t * hostObj, json_t * branchObj)
{
   UPDATE_TASK_MESSAGE_PTR UpdateTaskMessagePtr;

   if (updateTaskMessagePoolID)
   {
      UpdateTaskMessagePtr = (UPDATE_TASK_MESSAGE_PTR) _msg_alloc(updateTaskMessagePoolID);
      if (UpdateTaskMessagePtr)
      {
         UpdateTaskMessagePtr->HEADER.SOURCE_QID = 0;  //not relevant
         UpdateTaskMessagePtr->HEADER.TARGET_QID = _msgq_get_id(0, UPDATE_TASK_MESSAGE_QUEUE);
         UpdateTaskMessagePtr->HEADER.SIZE = sizeof(UPDATE_TASK_MESSAGE);

         UpdateTaskMessagePtr->updateCommand = commandIndex;
         UpdateTaskMessagePtr->serverStringObj = hostObj;
         UpdateTaskMessagePtr->versionBranchStringObj = branchObj;

         _msgq_send(UpdateTaskMessagePtr);
         return false;
      }
      else
      {
         updateTaskMsgAllocErr++;
      }
   }

   return true;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool_t strncmpi(char * string1Ptr, char * string2Ptr, uint8_t length)
{  // return false if strings match, not considering case.
   uint8_t char1;
   uint8_t char2;

   while (length)
   {
      // first, make sure the characters we're comparing are forced to lower case
      char1 = *string1Ptr;
      if ((char1 >= 'A') && (char1 <= 'Z'))
      {
         char1 += ('a' - 'A');
      }

      char2 = *string2Ptr;
      if ((char2 >= 'A') && (char2 <= 'Z'))
      {
         char2 += ('a' - 'A');
      }
      
      // then, check if the characters match.
      if (char1 != char2)
      {
         return true;   // true indicates that the strings do NOT match.
      }
      
      string1Ptr++;
      string2Ptr++;
      length--;
   }
   
   return false;  // all characters (in length) matched.
   
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void MDNS_Task(uint_32 param)
{
   sockaddr_in laddr;
   int_32 result;
   sockaddr_in remote_addr;
   uint_16 remote_addr_len = sizeof(remote_addr);
   uchar UDPBuf[100];
   uint_32 sock;
   struct ip_mreq group;
   uint_32 opt_value = TRUE;
   json_t * jsonResponseObject;
   uint_32 error;
   uchar * pchar;
   uchar index;
   int_32 socklist[1];
   uint_8 new_opt_value = 255;
   int_8 LCMStringLength;
   char LCMString[4];

#define SIZE_OF_MDNS_RX_BUFFER     32
#define SIZE_OF_MDNS_TX_BUFFER     86    // doesn't include LCMStringLength (the extra length for the LCM1-xxx option)
   // request "LCM1.local":
   //       <0x00> <0x00> <0x00> <0x00> <0x00> <0x01> <0x00> <0x00> <0x00> <0x00> 
   //      |     ID      |    Flags    |   QDCOUNT   |   ANCOUNT   |   NSCOUNT   |
   // byte:    0      1      2      3      4      5      6      7      8      9

   //       <0x00> <0x00> <0x04> <0x4c> <0x43> <0x4d> <0x31> <0x05> <0x6c> <0x6f>  
   //      |  ARCOUNT    | len  | 'L'  | 'C'  | 'M'  | '1'  | len  | 'l'  | 'o'  |
   // byte:   10     11     12     13     14     15     16     17     18     19

   //       <0x63> <0x61> <0x6c> <0x00> <0x00> <0x01> <0x00> <0x01> 
   //      | 'c'  | 'a'  | 'l'  | null |    QTYPE    |    QFLAG    |
   // byte:   20     21     22     23     24     25     26     27

   // response "LCM1.local":
   //       <0x00> <0x00> <0x84> <0x00> <0x00> <0x00> <0x00> <0x01> <0x00> <0x00> 
   //      |     ID      |    Flags    |   QDCOUNT   |   ANCOUNT   |   NSCOUNT   |
   // byte:    0      1      2      3      4      5      6      7      8      9

   //       <0x00> <0x02> <0x04> <0x4c> <0x43> <0x4d> <0x31> <0x05> <0x6c> <0x6f>  
   //      |  ARCOUNT    | len  | 'L'  | 'C'  | 'M'  | '1'  | len  | 'l'  | 'o'  |
   // byte:   10     11     12     13     14     15     16     17     18     19

   //       <0x63> <0x61> <0x6c> <0x00> <0x00> <0x01> <0x80> <0x01> <0x00> <0x00>
   //      | 'c'  | 'a'  | 'l'  | null |  A/IP4 code | IPv4 code   |  IPv4 TTL ...
   // byte:   20     21     22     23     24     25     26     27     28     29

   //       <0x78> <0x00> <0x00> <0x04> <0x--> <0x--> <0x--> <0x--> <0xc0> <0x0c>
   //      ... IPv4 TTL  |  IPv4 len   |    IPv4 address bytes     | FQDN offset |
   // byte:   30     31     32     33     34     35     36     37     38     39

   //       <0x00> <0x1c> <0x80> <0x01> <0x00> <0x00> <0x78> <0x00> <0x00> <0x10>
   //      |AAAA/IPv6 cod|  IPv6 code  |          IPv6 TTL         |  IPv6 len   |
   // byte:   40     41     42     43     44     45     46     47     48     49

   //       <0x00> <0x00> <0x00> <0x00> <0x00> <0x00> <0x00> <0x00> <0x00> <0x00>
   //      |   IPv6 address (16 bytes: 10 0x00's, 2 0xFF's, then IPv4 address) ...
   // byte:   50     51     52     53     54     55     56     57     58     59

   //       <0xFF> <0xFF> <0x--> <0x--> <0x--> <0x--> <0xc0> <0x0c> <0x00> <0x2f>
   //      ...      IPv6 address (continued)         | FQDN offset |NSEC typ code|
   // byte:   60     61     62     63     64     65     66     67     68     69

   //       <0x80> <0x01> <0x00> <0x00> <0x78> <0x00> <0x00> <0x08> <0xc0> <0x0c>
   //      |NSEC cls code|          NSEC TTL         |  NSEC len   | NSEC block...
   // byte:   70     71     72     73     74     75     76     77     78     79

   //       <0x00> <0x04> <0x40> <0x00> <0x00> <0x08>
   //      ...NSEC block and bitmap bytes (continued)|
   // byte:   80     81     82     83     84     85

   //-----------------------------------------------------------------------------------

   // request "LCM1-123.local": (123 = last mac octet)
   //       <0x00> <0x00> <0x00> <0x00> <0x00> <0x01> <0x00> <0x00> <0x00> <0x00> 
   //      |     ID      |    Flags    |   QDCOUNT   |   ANCOUNT   |   NSCOUNT   |
   // byte:    0      1      2      3      4      5      6      7      8      9

   //       <0x00> <0x00> <0x08> <0x4c> <0x43> <0x4d> <0x31> <0x45> <0x31> <0x32>  
   //      |  ARCOUNT    | len  | 'L'  | 'C'  | 'M'  | '1'  | '-'  | '1'  | '2'  |
   // byte:   10     11     12     13     14     15     16     17     18     19

   //       <0x33> <0x05> <0x6c> <0x6f> <0x63> <0x61> <0x6c> <0x00> <0x00> <0x01> 
   //      | '3'  | len  | 'l'  | 'o'  | 'c'  | 'a'  | 'l'  | null |    QTYPE    |
   // byte:   20     21     22     23     24     25     26     27     28     29

   //       <0x00> <0x01> 
   //      |    QFLAG    |
   // byte:   30     31

   // reply: same as before, replacing LCM1 with LCM1-123

   //-----------------------------------------------------------------------------------

   MDNSlistensock = 0;
   sock = RTCS_SOCKET_ERROR; // seed it with an error to force the initial socket creation

   for (;;)
   {  // listen forever
      if (sock == RTCS_SOCKET_ERROR)
      {
         // create the socket for listening for mdns requests
         MDNSlistensock = socket(AF_INET, SOCK_DGRAM, 0);
         if (MDNSlistensock != RTCS_SOCKET_ERROR)
         {
            // add the socket to the 224.0.0.251 (mdns) group
            group.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
            ipcfg_get_ip(BSP_DEFAULT_ENET_DEVICE, &current_ip_data);
            group.imr_interface.s_addr = current_ip_data.ip;

            result = setsockopt(MDNSlistensock, SOL_IGMP, RTCS_SO_IGMP_ADD_MEMBERSHIP, &group, sizeof(group));

            // bind the socket to the 5353 (mdns) port
            laddr.sin_family = AF_INET;
            laddr.sin_port = 5353;
            laddr.sin_addr.s_addr = INADDR_ANY;

            result = bind(MDNSlistensock, &laddr, sizeof(laddr));

            socklist[0] = MDNSlistensock;
         }
         else
         {  // wait a bit and try again
            _time_delay(100);
            MDNSlistensock = 0;
         }
      }

      if (MDNSlistensock)
      {
         sock = RTCS_selectset(socklist, 1, 0);

         if(sock == MDNSlistensock)
         {
            /* Datagram socket received data. */
            memset(&remote_addr, 0, sizeof(remote_addr));  // clean out ip info
            memset(UDPBuf, 0, sizeof(UDPBuf));
            result = recvfrom(sock, (void *) UDPBuf, SIZE_OF_MDNS_RX_BUFFER, 0, &remote_addr, &remote_addr_len);
            
            if (result > 0)
            {
               // we need to check if someone is asking about LCM1.local or LCM1-xxx.local, where xxx is our last mac octet
               if ((UDPBuf[2] == 0x00) &&  // check MDNS packet
                     (!strncmpi((char *)&UDPBuf[13], "lcm1", 4)))  // don't include null terminator in comparison
               {  // may be for us - keep checking
                  if ((UDPBuf[17] == '-') && (UDPBuf[12] == 0x08))  // check "LCM1-xxx" length
                  {
                     snprintf(LCMString, sizeof(LCMString), "%03d", myMACAddress[5]);
                     if (!memcmp(&UDPBuf[18], LCMString, 3))
                     {
                        LCMStringLength = 4;
                     }
                     else
                     {
                        LCMStringLength = -1;  // invalid - don't continue
                     }
                  }
                  else if (UDPBuf[12] == 0x04)  // check "LCM1" length
                  {
                     LCMStringLength = 0;
                  }
                  else
                  {
                     LCMStringLength = -1;  // invalid - don't continue
                  }

                  if (LCMStringLength >= 0)
                  {
                     if ((UDPBuf[17 + LCMStringLength] == 0x05) &&  // check "local" length
                           (!strncmpi((char*)&UDPBuf[18 + LCMStringLength], "local", 6)))  // include null terminator in comparison
                     {  // request is for us.  build the reply
                        pchar = UDPBuf;

                        *pchar++ = 0x00;  // UDPBuf[0]
                        *pchar++ = 0x00;  // UDPBuf[1]

                        *pchar++ = 0x84;  // UDPBuf[2]
                        *pchar++ = 0x00;  // UDPBuf[3]

                        *pchar++ = 0x00;  // UDPBuf[4]
                        *pchar++ = 0x00;  // UDPBuf[5]

                        *pchar++ = 0x00;  // UDPBuf[6]
                        *pchar++ = 0x01;  // UDPBuf[7]

                        *pchar++ = 0x00;  // UDPBuf[8]
                        *pchar++ = 0x00;  // UDPBuf[9]

                        *pchar++ = 0x00;  // UDPBuf[10]
                        *pchar++ = 0x02;  // UDPBuf[11]

                        // skip the LCM1.local or LCM1-xxx.local and their length bytes - they are already in the re-used buffer
                        pchar++;  // skip "LCM1" length  // UDPBuf[12]
                        pchar += (4 + LCMStringLength);  // skip "LCM1-xxx"          // UDPBuf[13-16] or UDPBuf[13-20] 

                        pchar++;  // skip "local" length  // UDPBuf[17] or UDPBuf[21]  
                        pchar += 5;  // skip "local"         // UDPBuf[18-22] or UDPBuf[22-26]
                        pchar++;  // skip null terminator // UDPBuf[23] or UDPBuf[27]

                        // the A/IPv4 address type code (hex 00 01),
                        *pchar++ = 0x00;  // UDPBuf[24] or UDPBuf[28]
                        *pchar++ = 0x01;  // UDPBuf[25] or UDPBuf[29]

                        // the IPv4 class code (hex 80 01),
                        *pchar++ = 0x80;  // UDPBuf[26] or UDPBuf[30]
                        *pchar++ = 0x01;  // UDPBuf[27] or UDPBuf[31]

                        // the IPv4 TTL (hex 00 00 78 00 for 30720 seconds),
                        *pchar++ = 0x00;  // UDPBuf[28] or UDPBuf[32]
                        *pchar++ = 0x00;  // UDPBuf[29] or UDPBuf[33]
                        *pchar++ = 0x78;  // UDPBuf[30] or UDPBuf[34]
                        *pchar++ = 0x00;  // UDPBuf[31] or UDPBuf[35]

                        // the IPv4 length (hex 00 04),
                        *pchar++ = 0x00;  // UDPBuf[32] or UDPBuf[36]
                        *pchar++ = 0x04;  // UDPBuf[33] or UDPBuf[37]

                        // the four IPv4 address bytes (hex xx xx xx xx)
                        *pchar++ = (current_ip_data.ip >> 24) & 0x000000FF;  // UDPBuf[34] or UDPBuf[38]
                        *pchar++ = (current_ip_data.ip >> 16) & 0x000000FF;  // UDPBuf[35] or UDPBuf[39]
                        *pchar++ = (current_ip_data.ip >> 8) & 0x000000FF;  // UDPBuf[36] or UDPBuf[40]
                        *pchar++ = (current_ip_data.ip >> 0) & 0x000000FF;  // UDPBuf[37] or UDPBuf[41]

                        // the FQDN offset (hex C0 0C for byte 12),
                        *pchar++ = 0xC0;  // UDPBuf[38] or UDPBuf[42]
                        *pchar++ = 0x0C;  // UDPBuf[39] or UDPBuf[43]

                        // the AAAA/IPv6 address type code (hex 00 1C),
                        *pchar++ = 0x00;  // UDPBuf[40] or UDPBuf[44]
                        *pchar++ = 0x1C;  // UDPBuf[41] or UDPBuf[45]

                        // the IPv6 class code (hex 80 01),
                        *pchar++ = 0x80;  // UDPBuf[42] or UDPBuf[46]
                        *pchar++ = 0x01;  // UDPBuf[43] or UDPBuf[47]

                        // the IPv6 TTL (again hex 00 00 78 00),
                        *pchar++ = 0x00;  // UDPBuf[44] or UDPBuf[48]
                        *pchar++ = 0x00;  // UDPBuf[45] or UDPBuf[49]
                        *pchar++ = 0x78;  // UDPBuf[46] or UDPBuf[50]
                        *pchar++ = 0x00;  // UDPBuf[47] or UDPBuf[51]

                        // the IPv6 length (hex 00 10),
                        *pchar++ = 0x00;  // UDPBuf[48] or UDPBuf[52]
                        *pchar++ = 0x10;  // UDPBuf[49] or UDPBuf[53]

                        // the 16 IPv6 address bytes (hex 00 00 00 00 00 00 00 00 00 00 FF FF xx xx xx xx),
                        for (index = 0; index < 10; index++)
                        {  // UDPBuf[50-59] or UDPBuf[54-63]
                           *pchar++ = 0x00;
                        }
                        *pchar++ = 0xFF;  // UDPBuf[60] or UDPBuf[64]
                        *pchar++ = 0xFF;  // UDPBuf[61] or UDPBuf[65]
                        *pchar++ = (current_ip_data.ip >> 24) & 0x000000FF;  // UDPBuf[62] or UDPBuf[66]
                        *pchar++ = (current_ip_data.ip >> 16) & 0x000000FF;  // UDPBuf[63] or UDPBuf[67]
                        *pchar++ = (current_ip_data.ip >> 8) & 0x000000FF;  // UDPBuf[64] or UDPBuf[68]
                        *pchar++ = (current_ip_data.ip >> 0) & 0x000000FF;  // UDPBuf[65] or UDPBuf[69]

                        // the FQDN offset (hex C0 0C for byte 12),
                        *pchar++ = 0xC0;  // UDPBuf[66] or UDPBuf[70]
                        *pchar++ = 0x0C;  // UDPBuf[67] or UDPBuf[71]

                        // the NSEC type code (hex 00 2F),
                        *pchar++ = 0x00;  // UDPBuf[68] or UDPBuf[72]
                        *pchar++ = 0x2F;  // UDPBuf[69] or UDPBuf[73]

                        // the NSEC class code (hex 80 01 for Ethernet),
                        *pchar++ = 0x80;  // UDPBuf[70] or UDPBuf[74]
                        *pchar++ = 0x01;  // UDPBuf[71] or UDPBuf[75]

                        // the NSEC TTL (again hex 00 00 78 00),
                        *pchar++ = 0x00;  // UDPBuf[72] or UDPBuf[76]
                        *pchar++ = 0x00;  // UDPBuf[73] or UDPBuf[77]
                        *pchar++ = 0x78;  // UDPBuf[74] or UDPBuf[78]
                        *pchar++ = 0x00;  // UDPBuf[75] or UDPBuf[79]

                        // the NSEC length (hex 00 08, for an 8-byte name section record), and
                        *pchar++ = 0x00;  // UDPBuf[76] or UDPBuf[80]
                        *pchar++ = 0x08;  // UDPBuf[77] or UDPBuf[81]

                        // the 8 NSEC block and bitmap bytes (hex C0 0C 00 04 40 00 00 08)
                        *pchar++ = 0xC0;  // UDPBuf[78] or UDPBuf[82]
                        *pchar++ = 0x0C;  // UDPBuf[79] or UDPBuf[83]
                        *pchar++ = 0x00;  // UDPBuf[80] or UDPBuf[84]
                        *pchar++ = 0x04;  // UDPBuf[81] or UDPBuf[85]
                        *pchar++ = 0x40;  // UDPBuf[82] or UDPBuf[86]
                        *pchar++ = 0x00;  // UDPBuf[83] or UDPBuf[87]
                        *pchar++ = 0x00;  // UDPBuf[84] or UDPBuf[88]
                        *pchar++ = 0x08;  // UDPBuf[85] or UDPBuf[89]

                        lwgpio_set_value(&jsonActivityLED, LWGPIO_VALUE_LOW);
                        jsonActivityTimer = 500;

                        _time_delay(10);
                        result = sendto(sock,
                              (void *) UDPBuf,
                              (SIZE_OF_MDNS_TX_BUFFER + LCMStringLength),
                              0,
                              &remote_addr,
                              remote_addr_len);

                        // send to the mdns ip address and port
                        remote_addr.sin_addr.s_addr = inet_addr("224.0.0.251");
                        remote_addr.sin_family = AF_INET;
                        remote_addr.sin_port = 5353;

                        _time_delay(10);
                        result = sendto(sock, (void *)UDPBuf, (SIZE_OF_MDNS_TX_BUFFER + LCMStringLength), 0, &remote_addr, remote_addr_len);

                        if (result == RTCS_ERROR)
                        {
                           error = RTCS_geterror(sock);
                           // can't do anything with the error, so jsut go back to listening.
                        }
                     }
                  }
               }
            }
            else
            {
                // DMC: Do something with the error?
            }
         }
      }

   }

}
#else

// Begin AMD Only Build Section
void NotDefined(json_t *root, json_t *responseObject)
{
   if(responseObject)
   {
      DBGPRINT_INTSTRSTR_ERROR(__LINE__,"MDNS_Task fails: 'Service' not handled for this interface","");
      BuildErrorResponse(responseObject,
            "'Service' not handled for this interface.", SW_SERVICE_NOT_HANDLED_FOR_THIS_INTERFACE);
   }
}


#endif
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void BroadcastSystemInfo(void)
{
   json_t * broadcastObject;

   broadcastObject = json_object();
   if (broadcastObject)
   {
      json_object_set_new(broadcastObject, "ID", json_integer(0));
      json_object_set_new(broadcastObject, "Service", json_string("SystemInfo"));
   
      HandleSystemInfo(broadcastObject);
   
      json_object_set_new(broadcastObject, "Status", json_string("Success"));
      SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
      json_decref(broadcastObject);
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleClearFlash(void)
{
   uint_32 returnCode = ClearFlash();

   // Set the restart system timer to restart the system in 1000 ticks
   restartSystemTimer = 1000;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleCloudDisconnect(void)
{
   // force the ping timer to its timeout value
   timeSincePingReceived = SERVER_PING_TIMEOUT_SECONDS * 1000;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-------------------------------------------------------------------
// this goes thru the zone list array of the set scene - 
// array can be 100 elements of up to 4 key/value pairs
//------------------------------------------------------------------

bool_t VerifySceneZoneList(json_t * valueObject, uint_8 setFlag, json_t * responseObject, scene_properties * scenePropertiesPtr)
{

   zone_properties zoneProperties;
   json_t * zoneArrayElement;
   json_t * zoneListObject;
   json_int_t zoneArrayIndex;
   json_int_t zoneArraySize;
   byte_t arrayZoneCount;
   byte_t zoneId;
#if DEBUG_SOCKETTASK_ANY
char debugBuf[100];
#endif

   zoneArrayIndex = 0;
   zoneArraySize = json_array_size(valueObject);
   if (!zoneArraySize)
   {
      DBGPRINT_INTSTRSTR_ERROR(__LINE__,"VerifySceneZoneList fails: No elements in 'ZoneList' array.","");
      BuildErrorResponse(responseObject, "No elements in 'ZoneList' array.", USR_NO_ELEMENTS_IN_ZoneList_ARRAY);
      return TRUE ;
   }
   else if (zoneArraySize > MAX_ZONES_IN_SCENE)
   {
      DBGPRINT_INTSTRSTR_ERROR(zoneArraySize,"VerifySceneZoneList fails: Too many zones in the scene.","");
      BuildErrorResponse(responseObject, "Too many zones in the scene.", USR_TOO_MANY_ZONES_IN_THE_SCENE);
      return TRUE ;
   }
   else if (!setFlag)  // we need to check for duplicate Zone IDs in the array
   {
      // check for dups
      for (int zoneIndex = 0; zoneIndex < zoneArraySize; zoneIndex++)
      {
         zoneArrayElement = json_array_get(valueObject, zoneIndex);

         zoneListObject = json_object_get(zoneArrayElement, "ZID");
         if (json_is_integer(zoneListObject))
         {
            zoneId = json_integer_value(zoneListObject);
         }
         else
         {
            BuildErrorResponse(responseObject, "ZoneList, Zone ID must be an integer.", SW_ZONE_LIST_ZONE_ID_MUST_BE_INTEGER);
            return TRUE ;
         }

         // check to make sure that zone wanted for the scene exists in the global zone list
         if (GetZoneProperties(zoneId, &zoneProperties))
         {
            if (zoneProperties.ZoneNameString[0] == 0)
            {
               snprintf(jsonErrorString, sizeof(jsonErrorString), "ZoneList, Zone ID %d does not exist.", zoneId);
               DBGPRINT_INTSTRSTR_ERROR(__LINE__,"VerifySceneZoneList fails",jsonErrorString);
               BuildErrorResponse(responseObject, jsonErrorString, SW_ZONE_LIST_ZONE_ID_DOES_NOT_EXIST);
               return TRUE ;
            }
         }
         else
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "ZoneList, Could not get properties for zone id %d.", zoneId);
            DBGPRINT_INTSTRSTR_ERROR(__LINE__,"VerifySceneZoneList fails",jsonErrorString);
            BuildErrorResponse(responseObject, jsonErrorString, SW_ZONE_LIST_COULD_NOT_GET_PROPERTIES_FOR_ZONE_ID);
            return TRUE;
         }

         for (int innerZoneIndex = zoneIndex + 1; innerZoneIndex < zoneArraySize; innerZoneIndex++)
         {
            zoneArrayElement = json_array_get(valueObject, innerZoneIndex);  // get next zone id

            zoneListObject = json_object_get(zoneArrayElement, "ZID");
            if (json_is_integer(zoneListObject))
            {
               if (zoneId == json_integer_value(zoneListObject))
               {
                  snprintf(jsonErrorString, sizeof(jsonErrorString), "ZoneList, Zone ID duplicate %d.", zoneId);
                  DBGPRINT_INTSTRSTR_ERROR(__LINE__,"VerifySceneZoneList fails",jsonErrorString);
                  BuildErrorResponse(responseObject, jsonErrorString, SW_ZONE_LIST_ZONE_ID_DUPLICATE);
                  return TRUE ;
               }
            }
            else
            {
               BuildErrorResponse(responseObject, "ZoneList, Zone ID must be an integer.", SW_ZONE_LIST_ZONE_ID_MUST_BE_INTEGER);
               return TRUE ;
            }
         }  // end inner for loop
      }  // end for loop

   }  // end of else not setflag

   if (setFlag)
   {
      // clear out array first
      for (int zoneIndex = 0; zoneIndex < MAX_ZONES_IN_SCENE; zoneIndex++)
      {
         scenePropertiesPtr->zoneData[zoneIndex].zoneId = 0xff;
         scenePropertiesPtr->zoneData[zoneIndex].powerLevel = 0;
         scenePropertiesPtr->zoneData[zoneIndex].rampRate = 50;
         scenePropertiesPtr->zoneData[zoneIndex].state = false;

      }
      scenePropertiesPtr->numZonesInScene = zoneArraySize;
   }

   while (zoneArrayIndex < zoneArraySize)
   {
      // get the element, which may contain up to 4 key/value pairs
      zoneArrayElement = json_array_get(valueObject, zoneArrayIndex);

      zoneListObject = json_object_get(zoneArrayElement, "ZID");
      if (!json_is_integer(zoneListObject))
      {
         // error
         BuildErrorResponse(responseObject, "ZoneList, Zone ID must be an integer.", SW_ZONE_LIST_ZONE_ID_MUST_BE_INTEGER);
         return TRUE ;
      }
      else
      {
         if ((json_integer_value(zoneListObject) >= MAX_NUMBER_OF_ZONES))
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "ZoneList, Zone ID must be less then %d.", MAX_NUMBER_OF_ZONES);
            DBGPRINT_INTSTRSTR_ERROR(__LINE__,"VerifySceneZoneList fails",jsonErrorString);
            BuildErrorResponse(responseObject, jsonErrorString, SW_ZONE_LIST_ZONE_ID_MUST_BE_LESS_THEN);
            return TRUE ;
         }
         else if (setFlag)
         {
            scenePropertiesPtr->zoneData[zoneArrayIndex].zoneId = json_integer_value(zoneListObject);
         }
      }

      zoneListObject = json_object_get(zoneArrayElement, "Lvl");

      if (zoneListObject)  // if it is there
      {
         if (!json_is_integer(zoneListObject))  // check to see if we got integer
         {
            // error
            BuildErrorResponse(responseObject, "ZoneList, Power Level must be an integer.", SW_ZONE_LIST_POWER_LEVEL_MUST_BE_AN_INTEGER);
            return TRUE ;
         }
         else
         {
            if ((json_integer_value(zoneListObject) > MAX_POWER_LEVEL) || (json_integer_value(zoneListObject) == 0))
            {
               snprintf(jsonErrorString, sizeof(jsonErrorString), "ZoneList, Power Level must be 1 - %d.", MAX_POWER_LEVEL);
               DBGPRINT_INTSTRSTR_ERROR(__LINE__,"VerifySceneZoneList fails",jsonErrorString);
               BuildErrorResponse(responseObject, jsonErrorString, SW_ZONE_LIST_POWER_LEVEL_MUST_BE_BETWEEN);
               return TRUE ;
            }
            else if (setFlag)
            {
               DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"VerifySceneZoneList for zoneIndex %d changed from %d to %d", zoneArrayIndex, scenePropertiesPtr->zoneData[zoneArrayIndex].powerLevel, json_integer_value(zoneListObject));
               DBGPRINT_INTSTRSTR(__LINE__,"VerifySceneZoneList",DEBUGBUF);
               scenePropertiesPtr->zoneData[zoneArrayIndex].powerLevel = json_integer_value(zoneListObject);
            }
         }
      }

      zoneListObject = json_object_get(zoneArrayElement, "RR");
      if (zoneListObject)  // if it is there
      {
         if (!json_is_integer(zoneListObject))  // check to see if we got integer
         {
            // error
            BuildErrorResponse(responseObject, "ZoneList, RampRate must be an integer.", SW_ZONE_LIST_RAMP_RATE_MUST_BE_AN_INTEGER);
            return TRUE ;
         }
         else
         {
            if ((json_integer_value(zoneListObject) > MAX_RAMP_RATE) || (json_integer_value(zoneListObject) < 0))
            {
               snprintf(jsonErrorString, sizeof(jsonErrorString), "ZoneList, Ramp Rate must be 0 - %d.", MAX_RAMP_RATE);
               DBGPRINT_INTSTRSTR_ERROR(__LINE__,"VerifySceneZoneList fails",jsonErrorString);
               BuildErrorResponse(responseObject, jsonErrorString, SW_ZONE_LIST_RAMP_RATE_MUST_BE_BETWEEN);
               return TRUE ;
            }
            else if (setFlag)
            {
               scenePropertiesPtr->zoneData[zoneArrayIndex].rampRate = json_integer_value(zoneListObject);
            }
         }
      }

      zoneListObject = json_object_get(zoneArrayElement, "St");
      if (zoneListObject)  // if it is there
      {
         if (!json_is_boolean(zoneListObject))
         {
            // error
            BuildErrorResponse(responseObject, "ZoneList, State must be boolean.", SW_ZONE_LIST_STATE_MUST_BE_BOOLEAN);
            return TRUE ;
         }
         else if (setFlag)
         {
            scenePropertiesPtr->zoneData[zoneArrayIndex].state = json_is_true(zoneListObject);
         }
      }
      zoneArrayIndex++;

   }  // end of while

   return FALSE ;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

json_t * BuildErrorResponseBadJsonFormat(json_error_t * error)
{
   json_t * errorObj;

   errorObj = json_pack("{sss{sssisisiss}}", "Status", "ErrorReport", "ErrorDetails", "Text", error->text, "Line", error->line, "Column", error->column,
         "Position", error->position, "Source", error->source);

   return (errorObj);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void BuildErrorResponse(json_t * responseObject, const char * errorText, uint32_t errorCode)
{
   json_object_set_new(responseObject, "Status", json_string("Error"));
   json_object_set_new(responseObject, "ErrorText", json_string(errorText));
   json_object_set_new(responseObject, "ErrorCode", json_integer(errorCode));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void BuildErrorResponseNoID(json_t * responseObject, const char * errorText, uint32_t errorCode)
{
   json_object_set_new(responseObject, "ID", json_integer(0));
   BuildErrorResponse(responseObject, errorText, errorCode);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void ProcessJsonService(json_t * root, json_t * service, json_t * responseObject, json_t ** jsonBroadcastObjectPtr)
{
   const char * serviceText;
#if DEBUG_SOCKETTASK_ANY
   char debugBuf[100];
   char *debugLocalString = NULL;
   MQX_TICK_STRUCT            debugLocalStartTick;
   MQX_TICK_STRUCT            debugLocalEndTick;
   bool                       debugLocalOverflow;
   uint32_t                   debugLocalOverflowMs;
#endif

   serviceText = json_string_value(service);
   if (serviceText)
   {
      DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"Entering ProcessJsonService %s", serviceText);
      DBGPRINT_INTSTRSTR(__LINE__,"ProcessJsonService",DEBUGBUF);
   }
   //Do Not Allow ManufacturingTestMode on WatchSocket!
   if (!strcmp("ManufacturingTestMode", serviceText) && !jsonSockets[socketIndexGlobalRx].WatchSocket)
   {
      HandleManufacturingTestMode(root, responseObject, jsonBroadcastObjectPtr);
   }
   else
   {
      if (manufacturingTestModeFlag)
      {
         if ((!strcmp("MTMSendRFPacket", serviceText)) && (manufacturingTestModeFlag))
         {
            HandleMTMSendRFPacket(root, responseObject);
         }
         else if (!strcmp("SetOutput", serviceText))
         {
            HandleMTMSetOutput(root, responseObject);
         }
         else if (!strcmp("SendRS485String", serviceText))
         {
            HandleMTMSendRS485String(root, responseObject);
         }
         else if (!strcmp("SetMACAddress", serviceText))
         {
            HandleMTMSetMACAddress(root, responseObject);
         }
         else
         {
            DBGPRINT_INTSTRSTR_ERROR(__LINE__,"ProcessJsonService fails: Unknown 'Service'.",jsonErrorString);
            BuildErrorResponse(responseObject, "Unknown 'Service'.", MTM_UNKNOWN_SERVICE);
         }
      }
      else
      {
         //Limit WatchSocket connections to the following commands
         if(jsonSockets[socketIndexGlobalRx].WatchSocket)
         {
            if (TestWatchAuth(root, responseObject, jsonBroadcastObjectPtr))
            {
               if (!strcmp("SetZoneProperties", serviceText))
               {
                  DEBUG_ZONEARRAY_MUTEX_START;
                  _mutex_lock(&ZoneArrayMutex);
                  DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ProcessJsonService before ZoneArrayMutex",DEBUGBUF);
                  DEBUG_ZONEARRAY_MUTEX_START;
                  HandleSetZoneProperties(root, responseObject, jsonBroadcastObjectPtr);
                  DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ProcessJsonService after HandleSetZoneProperties",DEBUGBUF);
                  _mutex_unlock(&ZoneArrayMutex);
               }
               
               else if (!strcmp("SetSceneProperties", serviceText))
               {
                  _mutex_lock(&ScenePropMutex);
                  HandleSetSceneProperties(root, responseObject, jsonBroadcastObjectPtr);
                  _mutex_unlock(&ScenePropMutex);
               }

               else if (!strcmp("RunScene", serviceText))
               {
                   _mutex_lock(&ScenePropMutex);
                   HandleRunScene(root, responseObject, jsonBroadcastObjectPtr);
                   _mutex_unlock(&ScenePropMutex);
               }
               
               else if (!strcmp("TriggerRampAllCommand", serviceText))
               {
                  HandleTriggerRampAllCommand(root, responseObject);
               }

               else if (!strcmp("ListScenesExpanded", serviceText))
               {
                  _mutex_lock(&ScenePropMutex);
                  HandleListScenesExpanded(responseObject); 
                  _mutex_unlock(&ScenePropMutex);
   #if DEBUG_SOCKETTASK_JSONPROC
                  if (responseObject)
                  {
                     debugLocalString = json_dumps(responseObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
                     DBGPRINT_INTSTRSTR_JSONPROC(__LINE__,"HandleListScenesExpanded generated response",debugLocalString);
                     jsonp_free(debugLocalString);
                  }
   #endif
               }
               else if (!strcmp("ListZonesExpanded", serviceText))
               {
                  DEBUG_ZONEARRAY_MUTEX_START;          
                  _mutex_lock(&ZoneArrayMutex);       
                  DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ProcessJsonService before ZoneArrayMutex",DEBUGBUF);
                  DEBUG_ZONEARRAY_MUTEX_START;
                  
                  HandleListZonesExpanded(responseObject);
                  
                  DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ProcessJsonService after HandleListZonesExpanded",DEBUGBUF);
                  _mutex_unlock(&ZoneArrayMutex);
   #if DEBUG_SOCKETTASK_JSONPROC
                  if (responseObject)
                  {
                     debugLocalString = json_dumps(responseObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
                     DBGPRINT_INTSTRSTR_JSONPROC(__LINE__,"HandleListZonesExpanded generated response",debugLocalString);
                     jsonp_free(debugLocalString);
                  }
   #endif
               }
            }
            else
            {
               DBGPRINT_INTSTRSTR_ERROR(socketIndexGlobalRx,"ProcessJsonService fails: Watch fails authentication.  Socket:",jsonErrorString);
            }
         }
         else if (!strcmp("SetSystemProperties", serviceText))
         {
            HandleSetSystemProperties(root, responseObject, jsonBroadcastObjectPtr);
         }
         else if(jsonSockets[socketIndexGlobalRx].jsa_state & JSA_MASK_JSON_LIMITED)
         {
            // If we're forcing the client to set a key, disable all commands except SetSystemProperies/Keys
            // Because broadcastDebug is disabled in [SETKEY] mode, the socket which caused this condition will not see this message:
            DBGPRINT_INTSTRSTR_ERROR(socketIndexGlobalRx,"ProcessJsonService fails: Only SetSystemProperies/Keys is allowed in [SETKEY] mode.  Socket:",jsonErrorString);
            BuildErrorResponse(responseObject, "Unknown 'Service'.", SW_UNKNOWN_SERVICE);
         }
         else if (!strcmp("SetZoneProperties", serviceText))
         {
            // _mutex_lock(&qubeSocketMutex); // need to lock out QueryQmotionStatus 
            DEBUG_ZONEARRAY_MUTEX_START;
            _mutex_lock(&ZoneArrayMutex);
            DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ProcessJsonService before ZoneArrayMutex",DEBUGBUF);
            DEBUG_ZONEARRAY_MUTEX_START;
            HandleSetZoneProperties(root, responseObject, jsonBroadcastObjectPtr);
            DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ProcessJsonService after HandleSetZoneProperties",DEBUGBUF);
            _mutex_unlock(&ZoneArrayMutex);
            // _mutex_unlock(&qubeSocketMutex);
         }
         else if (!strcmp("GetFactoryPassword", serviceText))
         {
            JSA_GetFactoryPassword();
         }
         else if (!strcmp("ClearFactoryAuthInit", serviceText))
         {
            JSA_ClearFactoryAuthInit();
         }
         else if (!strcmp("ListZones", serviceText))
         {
            DEBUG_ZONEARRAY_MUTEX_START;
            _mutex_lock(&ZoneArrayMutex);
            DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ProcessJsonService before ZoneArrayMutex",DEBUGBUF);
            DEBUG_ZONEARRAY_MUTEX_START;
            HandleListZones(responseObject);
            DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ProcessJsonService after HandleListZones",DEBUGBUF);
            _mutex_unlock(&ZoneArrayMutex);
#if DEBUG_SOCKETTASK_JSONPROC
            if (responseObject)
            {
               debugLocalString = json_dumps(responseObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
               DBGPRINT_INTSTRSTR_JSONPROC(__LINE__,"HandleListZones generated response",debugLocalString);
               jsonp_free(debugLocalString);
            }
#endif
         }
         else if (!strcmp("ReportZoneProperties", serviceText))
         {
            DEBUG_ZONEARRAY_MUTEX_START;
            _mutex_lock(&ZoneArrayMutex);
            DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ProcessJsonService before ZoneArrayMutex",DEBUGBUF);
            DEBUG_ZONEARRAY_MUTEX_START;
            HandleReportZoneProperties(root, responseObject);
            DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ProcessJsonService after HandleReportZoneProperties",DEBUGBUF);
            _mutex_unlock(&ZoneArrayMutex);
#if DEBUG_SOCKETTASK_JSONPROC
            if (responseObject)
            {
               debugLocalString = json_dumps(responseObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
               DBGPRINT_INTSTRSTR_JSONPROC(__LINE__,"HandleReportZoneProperties generated response",debugLocalString);
               jsonp_free(debugLocalString);
            }
#endif
         }
         else if (!strcmp("DeleteZone", serviceText))
         {
            DEBUG_ZONEARRAY_MUTEX_START;
            _mutex_lock(&ZoneArrayMutex);
            DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ProcessJsonService before ZoneArrayMutex",DEBUGBUF);
            DEBUG_ZONEARRAY_MUTEX_START;
            HandleDeleteZone(root, responseObject, jsonBroadcastObjectPtr);
            DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ProcessJsonService after HandleDeleteZone",DEBUGBUF);
            _mutex_unlock(&ZoneArrayMutex);
         }
         else if (!strcmp("SystemInfo", serviceText))
         {
            HandleSystemInfo(responseObject);
#if DEBUG_SOCKETTASK_JSONPROC
            if (responseObject)
            {
               debugLocalString = json_dumps(responseObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
               DBGPRINT_INTSTRSTR_JSONPROC(__LINE__,"HandleSystemInfo generated response",debugLocalString);
               jsonp_free(debugLocalString);
            }
#endif
         }
         else if (!strcmp("CheckForUpdate", serviceText))
         {
            HandleCheckForUpdate();
         }
         else if (!strcmp("InstallUpdate", serviceText))
         {
            HandleInstallUpdate(responseObject);
         }
         else if (!strcmp("DiscardUpdate", serviceText))
         {
            HandleDiscardUpdate();
         }
         else if (!strcmp("ClearFlash", serviceText))
         {
            HandleClearFlash();
         }
         else if (!strcmp("CloudDisconnect", serviceText))
         {
            HandleCloudDisconnect();
         }
         else if (!strcmp("FirmwareUpdate", serviceText))
         {
            HandleFirmwareUpdate(root, responseObject);
#if DEBUG_SOCKETTASK_JSONPROC
            if (responseObject)
            {
               debugLocalString = json_dumps(responseObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
               DBGPRINT_INTSTRSTR_JSONPROC(__LINE__,"HandleFirmwareUpdate generated response",debugLocalString);
               jsonp_free(debugLocalString);
            }
#endif
         }
         else if (!strcmp("SystemRestart", serviceText))
         {
            HandleSystemRestart();
         }
         else if (!strcmp("ping", serviceText))
         {
            HandlePing();
         }
         else if (!strcmp("ListScenes", serviceText))
         {
            _mutex_lock(&ScenePropMutex);
            HandleListScenes(responseObject);
            _mutex_unlock(&ScenePropMutex);
#if DEBUG_SOCKETTASK_JSONPROC
            if (responseObject)
            {
               debugLocalString = json_dumps(responseObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
               DBGPRINT_INTSTRSTR_JSONPROC(__LINE__,"HandleListScenes generated response",debugLocalString);
               jsonp_free(debugLocalString);
            }
#endif
         }
         else if (!strcmp("ReportSceneProperties", serviceText))
         {
            _mutex_lock(&ScenePropMutex);
            HandleReportSceneProperties(root, responseObject);
            _mutex_unlock(&ScenePropMutex);
#if DEBUG_SOCKETTASK_JSONPROC
            if (responseObject)
            {
               debugLocalString = json_dumps(responseObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
               DBGPRINT_INTSTRSTR_JSONPROC(__LINE__,"HandleReportSceneProperties generated response",debugLocalString);
               jsonp_free(debugLocalString);
            }
#endif
         }
         else if (!strcmp("SetSceneProperties", serviceText))
         {
            _mutex_lock(&ScenePropMutex);
            HandleSetSceneProperties(root, responseObject, jsonBroadcastObjectPtr);
            _mutex_unlock(&ScenePropMutex);
         }
         else if (!strcmp("CreateScene", serviceText))
         {
            _mutex_lock(&ScenePropMutex);
            HandleCreateScene(root, responseObject, jsonBroadcastObjectPtr);
            _mutex_unlock(&ScenePropMutex);
         }
         else if (!strcmp("DeleteScene", serviceText))
         {
            _mutex_lock(&ScenePropMutex);
            HandleDeleteScene(root, responseObject, jsonBroadcastObjectPtr);
            _mutex_unlock(&ScenePropMutex);
         }
         else if (!strcmp("RunScene", serviceText))
         {
            _mutex_lock(&ScenePropMutex);
            HandleRunScene(root, responseObject, jsonBroadcastObjectPtr);
            _mutex_unlock(&ScenePropMutex);
         }
         else if (!strcmp("ReportSystemProperties", serviceText))
         {
            HandleReportSystemProperties(responseObject);
#if DEBUG_SOCKETTASK_JSONPROC
            if (responseObject)
            {
               debugLocalString = json_dumps(responseObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
               DBGPRINT_INTSTRSTR_JSONPROC(__LINE__,"HandleReportSystemProperties generated response",debugLocalString);
               jsonp_free(debugLocalString);
            }
#endif
         }
         else if (!strcmp("ListSceneControllers", serviceText))
         {
            HandleListSceneControllers(responseObject);
#if DEBUG_SOCKETTASK_JSONPROC
            if (responseObject)
            {
               debugLocalString = json_dumps(responseObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
               DBGPRINT_INTSTRSTR_JSONPROC(__LINE__,"HandleListSceneControllers generated response",debugLocalString);
               jsonp_free(debugLocalString);
            }
#endif
         }
         else if (!strcmp("ReportSceneControllerProperties", serviceText))
         {
            HandleReportSceneControllerProperties(root, responseObject);
#if DEBUG_SOCKETTASK_JSONPROC
            if (responseObject)
            {
               debugLocalString = json_dumps(responseObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
               DBGPRINT_INTSTRSTR_JSONPROC(__LINE__,"HandleReportSceneControllerProperties generated response",debugLocalString);
               jsonp_free(debugLocalString);
            }
#endif
         }
         else if (!strcmp("SetSceneControllerProperties", serviceText))
         {
            HandleSetSceneControllerProperties(root, responseObject, jsonBroadcastObjectPtr);
         }
         else if (!strcmp("DeleteSceneController", serviceText))
         {
            HandleDeleteSceneController(root, responseObject, jsonBroadcastObjectPtr);
         }
         else if (!strcmp("TriggerRampCommand", serviceText))
         {
            HandleTriggerRampCommand(root, responseObject);
         }
         else if (!strcmp("TriggerRampAllCommand", serviceText))
         {
			HandleTriggerRampAllCommand(root, responseObject);
         }
         else if (!strcmp("TriggerSceneControllerCommand", serviceText))
         {
            HandleTriggerSceneControllerCommand(root, responseObject);
         }
         else if (!strcmp("ListScenesExpanded", serviceText))
         {
            _mutex_lock(&ScenePropMutex);
            HandleListScenesExpanded(responseObject); 
            _mutex_unlock(&ScenePropMutex);
#if DEBUG_SOCKETTASK_JSONPROC
            if (responseObject)
            {
               debugLocalString = json_dumps(responseObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
               DBGPRINT_INTSTRSTR_JSONPROC(__LINE__,"HandleListScenesExpanded generated response",debugLocalString);
               jsonp_free(debugLocalString);
            }
#endif
         }
         else if (!strcmp("ListZonesExpanded", serviceText))
         {
            DEBUG_ZONEARRAY_MUTEX_START;          
            _mutex_lock(&ZoneArrayMutex);       
            DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ProcessJsonService before ZoneArrayMutex",DEBUGBUF);
            DEBUG_ZONEARRAY_MUTEX_START;
            
            HandleListZonesExpanded(responseObject);
            
            DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ProcessJsonService after HandleListZonesExpanded",DEBUGBUF);
            _mutex_unlock(&ZoneArrayMutex);
#if DEBUG_SOCKETTASK_JSONPROC
            if (responseObject)
            {
               debugLocalString = json_dumps(responseObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
               DBGPRINT_INTSTRSTR_JSONPROC(__LINE__,"HandleListZonesExpanded generated response",debugLocalString);
               jsonp_free(debugLocalString);
            }
#endif
         }
         else
         {
            DBGPRINT_INTSTRSTR_ERROR(__LINE__,"ProcessJsonService fails: Unknown 'Service'.",jsonErrorString);
            BuildErrorResponse(responseObject, "Unknown 'Service'.", SW_UNKNOWN_SERVICE);
         }
      }
   }
   if (serviceText)
   {
      DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"Leaving ProcessJsonService %s", serviceText);
      DBGPRINT_INTSTRSTR_JSONPROC(__LINE__,"ProcessJsonService",DEBUGBUF);
   }
}

//-----------------------------------------------------------------------------
//   Used by get a list of all of zones and all of the associated
//  properties for each of the zones
//  NOTE: only the following properties are sent back for each zone:
//      Name
//      ZID
//      DeviceType
//      Power
//      PowerLevel
//
//    2/9/17 - limit number of zones to 20 that can be sent
//
//-----------------------------------------------------------------------------
void HandleListZonesExpanded(json_t * responseObject)
{
   json_t * zoneListArray;
   json_t * zoneArrayElement = NULL;
   zone_properties zoneProperties;
   int      zoneCount = 0;

   if (responseObject)
   {
      zoneListArray = json_array();
      if (zoneListArray)
      {
         // build zoneListObject
         for (int zoneIdx = 0; zoneIdx < MAX_NUMBER_OF_ZONES; zoneIdx++)
         {
            // fill in the zone ID and name if it exists
            if (GetZoneProperties(zoneIdx, &zoneProperties))
            {
               if (zoneProperties.ZoneNameString[0])
               {
                  zoneArrayElement = json_object();
                  if (zoneArrayElement)
                  {
                     json_object_set_new(zoneArrayElement, "ZID", json_integer(zoneIdx));
                     //  add property list of the zone  
                     //      ALL_Properties does not include ramp rate
                     BuildZonePropertiesObject(zoneArrayElement, zoneIdx, ALL_PROPERTIES_BITMASK);
                     json_array_append_new(zoneListArray, zoneArrayElement);
                  }
               }
               zoneCount++;
            }
            
            if (zoneCount > 19)
            {
               break;   // if we have filled in 20 zones, break out of the loop and send the packet
            }     
            
         }  // end of for ix loop
      
         json_object_set_new(responseObject, "ZoneList", zoneListArray);
      }
   }
}

//-----------------------------------------------------------------------------
//   Used by get list of all of the scenes and all of the associated
//  properties for each of the scenes
//  NOTE: only the following properties are sent back for each scene:
//      Name
//      SID
//      TriggerTime
//      Frequency
//      TriggerType
//      Skip
//
//    2/9/17 - limit number of scenes to 20 that can be sent
//
//-----------------------------------------------------------------------------
void HandleListScenesExpanded(json_t * responseObject)
{
   json_t * sceneListArray;
   json_t * sceneArrayElement;
   json_t * propObj;
   scene_properties sceneProperties;
   int      sceneCount = 0;

   if (responseObject)
   {
      sceneListArray = json_array();
      if (sceneListArray)
      {
         for (int sceneIdx = 0; sceneIdx < MAX_NUMBER_OF_SCENES; sceneIdx++)
         {
            // fill in the scene ID and name if it exists
            if (GetSceneProperties(sceneIdx, &sceneProperties))
            {
               if (sceneProperties.SceneNameString[0])
               {
                  sceneArrayElement = json_object();
                  if (sceneArrayElement)
                  {
                     json_object_set_new(sceneArrayElement, "SID", json_integer(sceneIdx));
                     // add properies
                     
                     // create an object to store the property list    
                     propObj = json_object();
                     if (propObj)
                     {
                        json_object_set_new(propObj, "Name", json_string(sceneProperties.SceneNameString));
            
                        // add the array to the json report output
            
                        json_object_set_new(propObj, "TriggerTime", json_integer(sceneProperties.nextTriggerTime));
            
                        json_object_set_new(propObj, "Frequency", json_integer(sceneProperties.freq));
            
                        json_object_set_new(propObj, "TriggerType", json_integer(sceneProperties.trigger));
             
                        if (sceneProperties.skipNext)
                        {
                           json_object_set_new(propObj, "Skip", json_true());
                        }
                        else
                        {
                           json_object_set_new(propObj, "Skip", json_false());
                        }
            
                        // store the property list into the response object
                        json_object_set_new(sceneArrayElement, "PropertyList", propObj);
                     }
                     
                     json_array_append_new(sceneListArray, sceneArrayElement);
                  }
               }
               sceneCount++;  // we filled in a json scene, so count it
            }
            
            if (sceneCount > 19)
            {
               break;   // if we have filled in 20 scenes, break out of the loop and send the packet
            }
         }  // end of for  loop
      
         json_object_set_new(responseObject, "SceneList", sceneListArray);
      }
   }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

json_t * ProcessJsonPacket(json_t * root, json_t ** jsonBroadcastObjectPtr)
{
   json_t * serviceJsonObject;
   json_t * idJsonObject;
   json_t * responseObject = NULL;

   if (root)
   {
      // start building our response
      responseObject = json_object();
      if (responseObject)
      {
         idJsonObject = json_object_get(root, "ID");
         if (!idJsonObject)
         {
            BuildErrorResponseNoID(responseObject, "No 'ID' field.", SW_NO_ID_FIELD);
         }
         else if (!json_is_integer(idJsonObject))
         {
            BuildErrorResponseNoID(responseObject, "'ID' field not an integer.", SW_ID_FIELD_IS_NOT_AN_INTEGER);
         }
         else
         {
      
            json_object_set(responseObject, "ID", idJsonObject);
      
            serviceJsonObject = json_object_get(root, "Service");
            if (!serviceJsonObject)
            {
               BuildErrorResponse(responseObject, "No 'Service' field.", SW_NO_SERVICE_FIELD);
            }
            else if (!json_is_string(serviceJsonObject))
            {
               BuildErrorResponse(responseObject, "'Service' field not a string.", SW_SERVICE_FIELD_IS_NOT_A_STRING);
            }
            else
            {
               json_object_set(responseObject, "Service", serviceJsonObject);
               ProcessJsonService(root, serviceJsonObject, responseObject, jsonBroadcastObjectPtr);
            }
         }
      }
   }

   return (responseObject);

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleListSceneControllers(json_t * responseObject)
{
   json_t * sceneControllerListArray;
   json_t * sceneControllerArrayElement;
   byte_t sceneControllerIndex;
   scene_controller_properties sctlProperties;
   
   if (responseObject)
   {
      sceneControllerListArray = json_array();
      if (sceneControllerListArray)
      {
         for(sceneControllerIndex = 0; sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS; sceneControllerIndex++)
         {
            if (GetSceneControllerProperties(sceneControllerIndex, &sctlProperties))
            {
               // fill in the scene controller ID and name if it exists
               if (sctlProperties.SceneControllerNameString[0])
               {
                  sceneControllerArrayElement = json_object();
                  if (sceneControllerArrayElement)
                  {
                     json_object_set_new(sceneControllerArrayElement, "SCID", json_integer(sceneControllerIndex));
                     json_array_append_new(sceneControllerListArray, sceneControllerArrayElement);
                  }
                }
            }
         }
         json_object_set_new(responseObject, "SceneControllerList", sceneControllerListArray);
      }
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleReportSceneControllerProperties(json_t * root, json_t * responseObject)
{
   json_t * sceneControllerObject;
   json_t * propObj;
   json_t * sceneListArray;
   json_t * sceneListArrayElement;
   byte_t   sceneControllerIndex;
   scene_controller_properties sceneControllerProperties;

   if (root)
   {
      sceneControllerObject = json_object_get(root, "SCID");
      if (!sceneControllerObject)
      {
         BuildErrorResponse(responseObject, "No SCID field.", SW_NO_SCID_FIELD);
         return;
      }
   
      if (json_is_integer(sceneControllerObject))
      {
         // get scene requested by the GUI and set array value
         sceneControllerIndex = json_integer_value(sceneControllerObject);
      }
      else
      {
         BuildErrorResponse(responseObject, "SCID must be an integer.", SW_SCID_MUST_BE_AN_INTEGER);
         return;
      }
   
      // check to see if the scene controller id is invalid
      if (sceneControllerIndex >= MAX_NUMBER_OF_SCENE_CONTROLLERS)
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "SCID is invalid. %d", sceneControllerIndex);
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleReportSceneControllerProperties fails",jsonErrorString);
         BuildErrorResponse(responseObject, jsonErrorString, SW_SCID_IS_INVALID);
         return;
      }
   
      if (GetSceneControllerProperties(sceneControllerIndex, &sceneControllerProperties))
      {
      
         // check to see if the scene controller has a name, if not, it isn't populated, so return
         if (sceneControllerProperties.SceneControllerNameString[0] == 0)
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "SCID %d does not exist.", sceneControllerIndex);
            DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleReportSceneControllerProperties fails",jsonErrorString);
            BuildErrorResponse(responseObject, jsonErrorString, SW_SCENE_ID_DOES_NOT_EXIST);
            return;
         }
     
         // fill in the scene controller ID that we are returning
         json_object_set_new(responseObject, "SCID", json_integer(sceneControllerIndex));
      
         // create an object to store the property list    
         propObj = json_object();
         if (propObj)
         {
            json_object_set_new(propObj, "Name", json_string(sceneControllerProperties.SceneControllerNameString));        
            sceneListArray = json_array();
            if (sceneListArray)
            {
               if (sceneControllerProperties.nightMode)
               {
                  json_object_set_new(propObj, "NightMode", json_true());
               }
               else
               {
                  json_object_set_new(propObj, "NightMode", json_false());
               }  
               
               for (int bankIndex = 0; bankIndex < MAX_SCENES_IN_SCENE_CONTROLLER; bankIndex++)
               {
                  sceneListArrayElement = json_object();  // create one element for the array
                  if (sceneListArrayElement)
                  {
                     // fill the element with values
                     json_object_set_new(sceneListArrayElement, "SID", json_integer(sceneControllerProperties.bank[bankIndex].sceneId ));
               
                     if (sceneControllerProperties.bank[bankIndex].toggleable)
                     {
                        json_object_set_new(sceneListArrayElement, "Toggleable", json_true());
                     }
                     else
                     {
                        json_object_set_new(sceneListArrayElement, "Toggleable", json_false());
                     }
               
                     if (sceneControllerProperties.bank[bankIndex].toggled)
                     {
                        json_object_set_new(sceneListArrayElement, "Toggled", json_true());
                     }
                     else
                     {
                        json_object_set_new(sceneListArrayElement, "Toggled", json_false());
                     }
               
                     json_array_append_new(sceneListArray, sceneListArrayElement);
                  }
               }
            
               // add the array to the json report output
               json_object_set_new(propObj, "Banks", sceneListArray);
            
               // store the property list into the response object
               json_object_set_new(responseObject, "PropertyList", propObj);
            }
            else
            {
               json_decref(propObj);
            }
         }
      }
      else
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "SCID %d does not exist.", sceneControllerIndex);
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleReportSceneControllerProperties fails",jsonErrorString);
         BuildErrorResponse(responseObject, jsonErrorString, SW_SCENE_ID_DOES_NOT_EXIST);
      }
      // we have filled the response in, it will now be transmitted via socket to app
   }
}

//-------------------------------------------------------------------
//------------------------------------------------------------------
//------------------------------------------------------------------

bool_t HandleIndividualBankListProperty(const char * keyPtr, json_t * valueObject, 
                                       scene_controller_properties *sceneControllerProperties, byte_t bankArrayIndex, json_t * responseObject, uint_8 setFlag)
{
   signed char sceneId; 
   scene_properties sceneProperties;
 
   if(!strcmp(keyPtr, "SID"))
   {
      if(!json_is_integer(valueObject))
      {
         BuildErrorResponse(responseObject, "Bank, SID must be an integer.", SW_BANK_SID_MUST_BE_INTEGER);
         return TRUE;
      }
      else
      {
         sceneId = json_integer_value(valueObject);
         // APP will send -1 for scene id when no scene is to be associated with the button
         //    OR
         // APP can send MASTER_OFF Scene to tell SCTL to turn off all other scenes in the sctl
         if((sceneId != -1) && (sceneId != MASTER_OFF_FOR_SCENE_CONTROLLER))
         {
             
            // Scene ID is valid. Make sure it exists
            if(GetSceneProperties(sceneId, &sceneProperties))
            {
               if(sceneProperties.SceneNameString[0] == 0)
               {
                  snprintf(jsonErrorString, sizeof(jsonErrorString), "Bank, SID %d does not exist.", sceneId);
                  DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleIndividualBankListProperty fails",jsonErrorString);
                  BuildErrorResponse(responseObject, jsonErrorString, SW_BANK_SID_DOES_NOT_EXIST);
                  return TRUE;
               }
               else if(setFlag)
               {
                  sceneControllerProperties->bank[bankArrayIndex].sceneId = sceneId;
               }
            }
            else
            {
               snprintf(jsonErrorString, sizeof(jsonErrorString), "Bank, Could not get properties for scene %d", sceneId);
               DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleIndividualBankListProperty fails",jsonErrorString);
               BuildErrorResponse(responseObject, jsonErrorString, SW_BANK_COULD_NOT_GET_PROPERTIES_FOR_SCENE);
               return TRUE;
            }
         }
         else if(sceneId == -1) // -- user may be changing the button to have no scene attached
         {
            // if scene associated with button is not equal to -1 and app just sent in -1, we are 
            //    clearing the scene associated with this button
            if(sceneControllerProperties->bank[bankArrayIndex].sceneId != -1)
            {
               sceneControllerProperties->bank[bankArrayIndex].sceneId = sceneId;
            }
         } // else if(sceneId == -1)
         else if (sceneId == MASTER_OFF_FOR_SCENE_CONTROLLER)
         {
            sceneControllerProperties->bank[bankArrayIndex].sceneId = sceneId;
            sceneControllerProperties->bank[bankArrayIndex].toggleable = false;
         }
         
            
      } // else sceneId is an integer
   }
   else if(!strcmp(keyPtr, "Toggleable"))
   {
      if(!json_is_boolean(valueObject))
      {
         BuildErrorResponse(responseObject, "Bank, Toggleable must be a boolean.", SW_BANK_TOGGLEABLE_MUST_BE_BOOLEAN);
         return TRUE;
      }
      else if(setFlag)
      {
          if(json_is_true(valueObject))
          {
             // can't set toggleable if scene ID is MASTER OFF
             if (sceneControllerProperties->bank[bankArrayIndex].sceneId != MASTER_OFF_FOR_SCENE_CONTROLLER)
             {
                sceneControllerProperties->bank[bankArrayIndex].toggleable = true;
             }
             else // error - scene is MASTER OFF, must not be toggleable, send back an error
             {
                DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleIndividualBankListProperty fails: Bank, Toggleable must be a False for Master Off","");
                BuildErrorResponse(responseObject, "Bank, Toggleable must be a False for Master Off.", SW_BANK_TOGGLEABLE_MUST_BE_FALSE_FOR_MASTER_OFF);
                return TRUE;
             }
          }
          else
          {
             sceneControllerProperties->bank[bankArrayIndex].toggleable = false;
          }
  
      } // end else if(setFlag)
   }
   else if(!strcmp(keyPtr, "Toggled"))
   {
      if(!json_is_boolean(valueObject))
      {
         BuildErrorResponse(responseObject, "Bank, Toggled must be an boolean.", SW_BANK_TOGGLED_MUST_BE_BOOLEAN);
         return TRUE ;
      }
      else if(setFlag)
      {
          if (json_is_true(valueObject))
          {
             sceneControllerProperties->bank[bankArrayIndex].toggled = true;
          }
          else
          {
             sceneControllerProperties->bank[bankArrayIndex].toggled = false;
          }
      }
   }
   else
   {
      BuildErrorResponse(responseObject, "Invalid Property in List.", SW_INVALID_PROPERTY_IN_LIST);
      return TRUE;
   }

   return FALSE ;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool_t HandleIndividualSceneControllerProperty(const char * keyPtr, json_t * valueObject, 
                                                scene_controller_properties *sceneControllerProperties, json_t * responseObject, uint_8 setFlag)
{
   bool_t returnFlag = FALSE;
   json_t * bankArrayElement;
   const char * bankKeyPtr;
   json_t * bankValueObject;
   byte_t bankArrayIndex;
   bool_t originalNightMode;
    
   if (!strcmp(keyPtr, "Name"))  // check for name
   {
      if (!json_is_string(valueObject))  // check to see if we got string
      {
         // error
         BuildErrorResponse(responseObject, "PropertyList, Name must be string.", SW_PROPERTY_LIST_NAME_MUST_BE_A_STRING);
         returnFlag = TRUE;
      }
      else if (setFlag)
      {
         SanitizeJsonStringObject(valueObject);  // sanitize this string object, since it will be used in our subsequent broadcast.
         snprintf(sceneControllerProperties->SceneControllerNameString, sizeof(sceneControllerProperties->SceneControllerNameString), "%s", json_string_value(valueObject));
         sceneControllerProperties->SceneControllerNameString[sizeof(sceneControllerProperties->SceneControllerNameString) - 1] = 0;   // force null terminator
      }
   }  // end of else if we got name

   else if (!strcmp(keyPtr, "NightMode"))  // check for night mode
   {
      if (!json_is_boolean(valueObject))  // check to see if we got boolean
      {
         // error
         BuildErrorResponse(responseObject, "PropertyList, NightMode must be boolean.", SW_PROPERTY_LIST_NIGHTMODE_MUST_BE_BOOLEAN);
         returnFlag = TRUE;
      }
      else if (setFlag)
      {
         originalNightMode = sceneControllerProperties->nightMode;
         if (json_is_true(valueObject))
         {
            sceneControllerProperties->nightMode = true;
         }
         else
         {
            sceneControllerProperties->nightMode = false;
         }
         if (sceneControllerProperties->nightMode != originalNightMode)
         {
            if (SendSceneSettingsCommandToTransmitTask(sceneControllerProperties, sceneControllerProperties->nightMode, FALSE) )
            {
                DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleIndividualSceneControllerProperty fails: Could not transmit on queue.","");
                BuildErrorResponse(responseObject, "Could not transmit on queue.", SW_COULD_NOT_TRANSMIT_ON_QUEUE);
                returnFlag = FALSE;
            }
         }
      }
   }  // end of else if we got night mode

   else if (!strcmp(keyPtr, "Banks"))  // check for scenes associated with the buttons
   {
      if (!json_is_array(valueObject))  // check to see if we got an array
      {
         // error
         BuildErrorResponse(responseObject, "Banks has to be an array.", SW_BANK_HAS_TO_BE_AN_ARRAY);
         returnFlag = TRUE;
      }
      else  // else, it is an array, so check 
      {
         if (json_array_size(valueObject) > MAX_SCENES_IN_SCENE_CONTROLLER)
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "Banks has an invalid array size:%d", (int)json_array_size(valueObject));
            DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleIndividualSceneControllerProperty fails:",jsonErrorString);
            BuildErrorResponse(responseObject, jsonErrorString, SW_BANK_ARRAY_HAS_AN_INVALID_SIZE);
            return TRUE;
         }
         else
         {
            // Iterate over the banks array and make sure each bank has valid data
            for(bankArrayIndex = 0; bankArrayIndex < json_array_size(valueObject); bankArrayIndex++)
            {
               bankArrayElement = json_array_get(valueObject, bankArrayIndex);

               // Go through each element of the banklist to validate and/or set
               json_object_foreach(bankArrayElement, bankKeyPtr, bankValueObject)
               {
                  if (HandleIndividualBankListProperty(bankKeyPtr, bankValueObject, sceneControllerProperties, bankArrayIndex, responseObject, setFlag))
                  {
                     return TRUE;
                  }
               }
            }
         }
      }
   }
   else
   {
      BuildErrorResponse(responseObject, "Invalid Property in List.", SW_INVALID_PROPERTY_IN_LIST);
      returnFlag = TRUE;
   }
  
   return returnFlag;
}


//-----------------------------------------------------------------------------
// CalculateAvgPowerLevelOfScene()
//   for each scene controller bank/scene, calculate the average power level
//  of all of the zones that are non zero
//  Avg Power Level = sum of all zones power levels that are not zero div
//                          number of zones with power levels that are not zero 
//  added fromZones Boolean Value -- true - use power levels from zones in the scene
//                                   false - use levels from scene 
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
uint8_t CalculateAvgPowerLevelOfScene(uint8_t sceneId, bool_t fromZones)
{
   uint8_t  avgPowerCtr;
   uint8_t  avgLevel = 0;
   uint16_t avgPowerSum;
   scene_properties sceneProperties;
   zone_properties zoneProperties;

   avgPowerCtr = 0;
   avgPowerSum = 0;
      
   if (sceneId != -1)
   {
      if (GetSceneProperties(sceneId, &sceneProperties))
      {
         for (uint8_t zoneIdx = 0; zoneIdx < sceneProperties.numZonesInScene; zoneIdx++)
         {
            if (fromZones) 
            {
               if (GetZoneProperties(sceneProperties.zoneData[zoneIdx].zoneId, &zoneProperties))
               {
                  if (zoneProperties.state)
                  {
                     avgPowerSum += zoneProperties.powerLevel;
                     avgPowerCtr += 1;
                  }
               }
            } //  end if (fromZones)
            else
            {
               if (sceneProperties.zoneData[zoneIdx].state != 0)
               {
                  avgPowerSum += sceneProperties.zoneData[zoneIdx].powerLevel;
                  avgPowerCtr += 1;
               }
            } // end else Not (fromZones)
               
         } // end for zone index loop
              
         if (avgPowerCtr > 0)  // we found at least one power level that is On
         {
            avgLevel = (uint8_t)(avgPowerSum / avgPowerCtr);
         }
         else
         {
            avgLevel = 0;
         }
      } // if (GetSceneProperties())
   } // end if (sceneId != -1)
  
  return avgLevel;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleSetSceneControllerProperties(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr)
{
   json_t * propertyListObject;
   const char * keyPtr;
   json_t * valueObject;
   json_t * sceneControllerObject;
   byte_t  sceneControllerIndex;
   byte_t  avgLevel;
   scene_controller_properties sceneControllerProperties;

   // get the zone id objects
   sceneControllerObject = json_object_get(root, "SCID");
   if (!sceneControllerObject)
   {
      BuildErrorResponse(responseObject, "No SCID field.", SW_NO_SCID_FIELD);
   }
   else if (!json_is_integer(sceneControllerObject))
   {
      BuildErrorResponse(responseObject, "SCID must be an integer", SW_SCID_MUST_BE_AN_INTEGER);
   }
   else
   {
      sceneControllerIndex = (byte_t) json_integer_value(sceneControllerObject);

      if (sceneControllerIndex >= MAX_NUMBER_OF_SCENE_CONTROLLERS)
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "SCID %d must be less then %d.", sceneControllerIndex, MAX_NUMBER_OF_SCENE_CONTROLLERS);
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleSetSceneControllerProperties fails:",jsonErrorString);
         BuildErrorResponse(responseObject, jsonErrorString, SW_SCENE_ID_MUST_BE_LESS_THEN);
         return;
      }
      
      if(GetSceneControllerProperties(sceneControllerIndex, &sceneControllerProperties))
      {
         if (sceneControllerProperties.SceneControllerNameString[0] == 0)
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "SCID %d does not exist.", sceneControllerIndex);
            DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleSetSceneControllerProperties fails:",jsonErrorString);
            BuildErrorResponse(responseObject, jsonErrorString, SW_SCENE_ID_DOES_NOT_EXIST);
         }
         else
         {
            // Get the property list object
            propertyListObject = json_object_get(root, "PropertyList");
            if (!json_is_object(propertyListObject))
            {
               BuildErrorResponse(responseObject, "Invalid 'PropertyList'.", SW_INVALID_PROPERTY_LIST);
               return;
            }
         
            // first, go through the entire property list and make sure we don't have ANY errors.
            json_object_foreach(propertyListObject, keyPtr, valueObject)
            {
               if (HandleIndividualSceneControllerProperty(keyPtr, valueObject, &sceneControllerProperties, responseObject, FALSE))
               {
                  return;
               }
            }
         
            // if everything was ok, go through the property list again and execute.
            json_object_foreach(propertyListObject, keyPtr, valueObject)
            {
               if (HandleIndividualSceneControllerProperty(keyPtr, valueObject, &sceneControllerProperties, responseObject, TRUE))
               {
                  return;
               }
            }
            
            // Because we got here, we have no errors in the data passed in, so store into NVRAM
            SetSceneControllerProperties(sceneControllerIndex, &sceneControllerProperties);
           
            // send assign scene command to the Scene Controller for each bank
            for (byte_t bankArrayIndex = 0; bankArrayIndex < MAX_SCENES_IN_SCENE_CONTROLLER; bankArrayIndex++)
            {
               // calculate the average levle of the loads for this scene
               avgLevel = CalculateAvgPowerLevelOfScene(sceneControllerProperties.bank[bankArrayIndex].sceneId, FALSE);
                  
               if (SendAssignSceneCommandToTransmitTask(&sceneControllerProperties, bankArrayIndex, avgLevel))
               {
                  DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleSetSceneControllerProperties fails: Could not transmit on queue.","");
                  BuildErrorResponse(responseObject, "Could not transmit on queue.", SW_COULD_NOT_TRANSMIT_ON_QUEUE);
                  return;
               }
            }
         
            // if we don't have an error, then prepare the asynchronous message
            // to all json sockets.
            *jsonBroadcastObjectPtr = json_copy(root);
            json_object_set_new(*jsonBroadcastObjectPtr, "ID", json_integer(0));
            json_object_set_new(*jsonBroadcastObjectPtr, "Service", json_string("SceneControllerPropertiesChanged"));
            json_object_set_new(*jsonBroadcastObjectPtr, "Status", json_string("Success"));
         
           
         } // else we have a name
      } // if(GetSctlProperties(sceneControllerIndex, &sctlProperties))
   } // else we have a SCID: index into scene controllers  

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleDeleteSceneController(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr)
{
   json_t * sceneControllerObject;
   json_t * broadcastObject;
   bool_t   errorFlag = false;
   byte_t   sceneControllerIndex;
   scene_controller_properties sceneControllerProperties;
  
   // get the zone id object
   sceneControllerObject = json_object_get(root, "SCID");
   if (!sceneControllerObject)
   {
      BuildErrorResponse(responseObject, "No SCID field.", SW_NO_SCID_FIELD);
   }
   else
   {
      if (!json_is_integer(sceneControllerObject))
      {
         BuildErrorResponse(responseObject, "SCID must be an integer", SW_SCID_MUST_BE_AN_INTEGER);
      }
      else
      {
         sceneControllerIndex = json_integer_value(sceneControllerObject);
         if (sceneControllerIndex >= MAX_NUMBER_OF_SCENE_CONTROLLERS)
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "SCID %d must be less then %d.", sceneControllerIndex, MAX_NUMBER_OF_SCENE_CONTROLLERS);
            BuildErrorResponse(responseObject, jsonErrorString, SW_SCENE_ID_MUST_BE_LESS_THEN);
         }
         
         else if(GetSceneControllerProperties(sceneControllerIndex, &sceneControllerProperties)) 
         {
            if (sceneControllerProperties.SceneControllerNameString[0] == 0)
            {
               snprintf(jsonErrorString, sizeof(jsonErrorString), "SCID %d does not exist.", sceneControllerIndex);
               BuildErrorResponse(responseObject, jsonErrorString, SW_SCENE_ID_DOES_NOT_EXIST);
            }
            else
            {
               memset(sceneControllerProperties.SceneControllerNameString, 0, sizeof(sceneControllerProperties.SceneControllerNameString));

               // initialize each scene id to NO_SCENE and 4th to MASTER_OFF
               for (byte_t bankArrayIndex = 0; bankArrayIndex < MAX_SCENES_IN_SCENE_CONTROLLER; bankArrayIndex++)
               {
                  sceneControllerProperties.bank[bankArrayIndex].sceneId = -1;
               }
               sceneControllerProperties.bank[3].sceneId = MASTER_OFF_FOR_SCENE_CONTROLLER;
               
               
               // send the default scenes to the scene controller, to reset it 
               for (byte_t bankArrayIndex = 0; bankArrayIndex < MAX_SCENES_IN_SCENE_CONTROLLER; bankArrayIndex++)
               {
                   
                  if (SendAssignSceneCommandToTransmitTask(&sceneControllerProperties, bankArrayIndex, 0))
                  {
                     BuildErrorResponse(responseObject, "Could not transmit on queue.", SW_COULD_NOT_TRANSMIT_ON_QUEUE);
                     return;
                  }
               }
               
               // Save the property for the scene controller
               SetSceneControllerProperties(sceneControllerIndex, &sceneControllerProperties);
               
               *jsonBroadcastObjectPtr = json_object();
               if (*jsonBroadcastObjectPtr)
               {
                  json_object_set_new(*jsonBroadcastObjectPtr, "ID", json_integer(0));
                  json_object_set_new(*jsonBroadcastObjectPtr, "SCID", json_integer(sceneControllerIndex));
                  json_object_set_new(*jsonBroadcastObjectPtr, "Service", json_string("SceneControllerDeleted"));
                  json_object_set_new(*jsonBroadcastObjectPtr, "Status", json_string("Success"));
               }
            }
            
         } // else if(GetSctlProperties(sceneControllerIndex, &sctlProperties)) 
      }
   }
   
   if((!AnyZonesInArray()) &&
      (!AnySceneControllersInArray()))
   {
      // Clear the house ID and report the information
      global_houseID = 0;
      BroadcastSystemInfo();

      // Turn on the house ID not set LED because all devices have been removed
      SetGPIOOutput(HOUSE_ID_NOT_SET, LED_ON);

      // If we have no zones, set systemProperties.configured false and broadcast
      systemProperties.configured = FALSE;

      // Set the flash save timer to make sure this value is saved
      flashSaveTimer = FLASH_SAVE_TIMER;

      // Allocate memory for the json object
      broadcastObject = json_object();
      if (broadcastObject)
      {
         // Build the json object
         json_object_set_new(broadcastObject, "ID", json_integer(0));
         json_object_set_new(broadcastObject, "Service", json_string("SystemPropertiesChanged"));
         BuildSystemPropertiesObject(broadcastObject, SYSTEM_PROPERTY_CONFIGURED_BITMASK);
         json_object_set_new(broadcastObject, "Status", json_string("Success"));
   
         // Broadcast SystemPropertiesChanged for configured property
         SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
   
         // Free the JSON memory
         json_decref(broadcastObject);
      }
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleListZones(json_t * responseObject)
{
   json_t * zoneListArray;
   json_t * zoneArrayElement;
   zone_properties zoneProperties;
   
   zoneListArray = json_array();
   if (zoneListArray)
   {
      // build zoneListObject
      for (int zoneIdx = 0; zoneIdx < MAX_NUMBER_OF_ZONES; zoneIdx++)
      {
         // fill in the zone ID and name if it exists
         if (GetZoneProperties(zoneIdx, &zoneProperties))
         {
            if (zoneProperties.ZoneNameString[0])
            {
               zoneArrayElement = json_object();
               if (zoneArrayElement)
               {
                  json_object_set_new(zoneArrayElement, "ZID", json_integer(zoneIdx));
                  json_array_append_new(zoneListArray, zoneArrayElement);
               }
            }
         }
      }  // end of for ix loop
   
      json_object_set_new(responseObject, "ZoneList", zoneListArray);
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleReportZoneProperties(json_t * root, json_t * responseObject)
{
   byte_t zoneIndex;
   json_t *zoneObject;
   zone_properties zoneProperties;

   // get zone index from root
   zoneIndex = 0;
   // get the zone id object
   zoneObject = json_object_get(root, "ZID");
   if (!zoneObject)
   {
      BuildErrorResponse(responseObject, "No Zone ID field.", SW_NO_ZONE_ID_FIELD);
      return;
   }

   if (json_is_integer(zoneObject))
   {
      zoneIndex = json_integer_value(zoneObject);
      if (zoneIndex < MAX_NUMBER_OF_ZONES)
      {
         if (GetZoneProperties(zoneIndex, &zoneProperties))
         {
            if (zoneProperties.ZoneNameString[0] == 0)
            {
               snprintf(jsonErrorString, sizeof(jsonErrorString), "Zone ID %d doesn't exist.", zoneIndex);
               DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleReportZoneProperties fails: ",jsonErrorString);
               BuildErrorResponse(responseObject, jsonErrorString, SW_ZONE_ID_DOES_NOT_EXIST);
               return;

            }
            else
            {
               BuildZonePropertiesObject(responseObject, zoneIndex, ALL_PROPERTIES_BITMASK);
            }
         }
         else
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "Could not get properties for zone %d.", zoneIndex);
            DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleReportZoneProperties fails: ",jsonErrorString);
            BuildErrorResponse(responseObject, jsonErrorString, SW_COULD_NOT_GET_PROPERTIES_FOR_ZONE);
            return;
         }
      }
      else
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "Zone ID (%d) must be less then %d.", zoneIndex, MAX_NUMBER_OF_ZONES);
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleReportZoneProperties fails: ",jsonErrorString);
         BuildErrorResponse(responseObject, jsonErrorString, SW_ZONE_ID_MUST_BE_LESS_THEN);
         return;
      }

   }
   else
   {
      BuildErrorResponse(responseObject, "Zone ID must be an integer", SW_ZONE_ID_MUST_BE_AN_INTEGER);
      return;
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleSetZoneProperties(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr)
{
   zone_properties   zoneProperties;
   json_t            * propertyListObject;
   json_t            * appContextObj;
   json_t            * valueObject;
   json_t            * zoneObject;
   const char        * keyPtr;
   dword_t           propertyIndex = 0;
   byte_t            zoneIndex;
   bool_t            sendToTransmitQueueFlag = false;
   bool_t            broadcastFlag = false;
   bool_t            nameChangeFlag = false;
   bool_t            errorFlag;

   // get the zone id objects
   zoneObject = json_object_get(root, "ZID");
   if (!zoneObject)
   {
      BuildErrorResponse(responseObject, "No Zone ID field.", SW_NO_ZONE_ID_FIELD);
      DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleSetZoneProperties fails: No Zone ID field.","");
   }
   else if (!json_is_integer(zoneObject))
   {
      BuildErrorResponse(responseObject, "Zone ID must be an integer", SW_ZONE_ID_MUST_BE_AN_INTEGER);
      DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleSetZoneProperties fails: Zone ID must be an integer","");
   }
   else
   {
      zoneIndex = json_integer_value(zoneObject);

      if (zoneIndex >= MAX_NUMBER_OF_ZONES)
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "Zone ID (%d) must be less then %d.", (int) json_integer_value(zoneObject), MAX_NUMBER_OF_ZONES);
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleSetZoneProperties fails: ",jsonErrorString);
         BuildErrorResponse(responseObject, jsonErrorString, SW_ZONE_ID_MUST_BE_LESS_THEN);
      }
      else if (!GetZoneProperties(zoneIndex, &zoneProperties))
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "Could not get zone properties for zone %d.", zoneIndex);
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleSetZoneProperties fails: ",jsonErrorString);
         BuildErrorResponse(responseObject, jsonErrorString, SW_COULD_NOT_GET_ZONE_PROPERTIES_FOR_ZONE);
      }
      else if (zoneProperties.ZoneNameString[0] == 0)
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "Zone ID (%d) not found.", (int) json_integer_value(zoneObject));
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleSetZoneProperties fails: ",jsonErrorString);
         BuildErrorResponse(responseObject, jsonErrorString, SW_ZONE_ID_NOT_FOUND);
      }
      else
      {
         propertyListObject = json_object_get(root, "PropertyList");
         if (!json_is_object(propertyListObject))
         {
            BuildErrorResponse(responseObject, "Invalid 'PropertyList'.", SW_INVALID_PROPERTY_LIST);
         }
         else
         {
            // first, go through the entire property list and make sure we don't have ANY errors.
            DBGPRINT_INTSTRSTR_ERROR(zoneIndex,"HandleSetZoneProperties validating HandleIndividualProperty for zoneIdx","");
            json_object_foreach(propertyListObject, keyPtr, valueObject)
            {
               errorFlag = HandleIndividualProperty(keyPtr,
                                                     valueObject,
                                                      &zoneProperties,
                                                       responseObject,
                                                        &propertyIndex,
                                                         &sendToTransmitQueueFlag,
                                                          &broadcastFlag,
                                                           &nameChangeFlag,
                                                            FALSE);
               if (errorFlag)
               {
                  break;
               }
            }

            if (!errorFlag)
            {  // if everything was ok, go through the property list again and execute.
               
               // initialize RampRate to 50,  it will be overwritten if App ever sends rampRate
               zoneProperties.rampRate = 50;
               
               appContextObj = json_object_get(root, "AppContextId");
               // if we have an AppContextID, save it to send back, Not an error if not in msg
               if (appContextObj)
               {
                  if (!json_is_integer(appContextObj))
                  {
                     DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleSetZoneProperties fails: App Context ID error","");
                     BuildErrorResponse(responseObject, "App Context ID error", SW_APP_CONTEXT_ID_ERROR);
                     errorFlag = true;
                  }
                  else
                  {
                     zoneProperties.AppContextId = json_integer_value(appContextObj);
                  }
               }  // if (appContextObj)
               else
               {
                  // Clear the app context ID for the zone 
                  zoneProperties.AppContextId = 0;
               }
               
               if (!errorFlag)
               {
                  DBGPRINT_INTSTRSTR_ERROR(zoneIndex,"HandleSetZoneProperties calling HandleIndividualProperty for zoneIdx","");
                  json_object_foreach(propertyListObject, keyPtr, valueObject)
                  {
                     errorFlag = HandleIndividualProperty(keyPtr,
                                                           valueObject,
                                                            &zoneProperties,
                                                             responseObject,
                                                              &propertyIndex,
                                                               &sendToTransmitQueueFlag,
                                                                &broadcastFlag,
                                                                 &nameChangeFlag,
                                                                  TRUE);
                     if (errorFlag)
                     {
                        break;
                     }
                  }
               }
            }

            if (!errorFlag)
            {  // if we don't have an error, see if we need to send an rf message
               if (sendToTransmitQueueFlag)
               {
                  // Provide an invalid scene ID so the scene properties do
                  // not get broadcast when it is only an individual zone 
                  // property that is changing
                  SendRampCommandToTransmitTask(zoneIndex, (MAX_NUMBER_OF_SCENES + 1), 0, RFM100_MESSAGE_HIGH_PRIORITY);
               }
               
               // Update the zone properties array with any changes
               SetZoneProperties(zoneIndex, &zoneProperties);

               if (nameChangeFlag)
               {
                  // Set the name for this device ID in the Eliot cloud
                  Eliot_QueueRenameZone(zoneIndex);
               }

               if (broadcastFlag)
               {
                  *jsonBroadcastObjectPtr = json_copy(root);
                  json_t *propertyListObject = json_object_get(root, "PropertyList");
                  
                  // if we have Power Level or Power State, remove from packet and then send
                  // unless it is a switch type and the message was not sent to the transmit queue.
                  // A switch type needs to report the power level as 100% so the app correctly
                  // shows the level when using the master dimmer slider. Switches cannot be 
                  // dimmed to a level other than 100%
                  if(!sendToTransmitQueueFlag && zoneProperties.deviceType == SWITCH_TYPE)
                  {
                     json_object_set_new(propertyListObject, "PowerLevel", json_integer(MAX_POWER_LEVEL));
                  }
                  else if (json_object_get(propertyListObject, "PowerLevel"))
                  {
                     json_object_del(propertyListObject, "PowerLevel");
                  }

                  if (json_object_get(propertyListObject, "Power"))
                  {
                     json_object_del(propertyListObject, "Power");
                  }
                  
                  json_object_set_new(*jsonBroadcastObjectPtr, "ID", json_integer(0));
                  json_object_set_new(*jsonBroadcastObjectPtr, "Status", json_string("Success"));
                  json_object_set_new(*jsonBroadcastObjectPtr, "Service", json_string("ZonePropertiesChanged"));

               }
            }  // if (!errorFlag)
         }  // else
      }  // else zone exists
   }  // else zone id is an integer
}

//-----------------------------------------------------------------------------
//----------set zone----
//  if setFlag is False, just check parameter
//  else set
//
//  also, any errors should set returnFlag to TRUE
//-----------------------------------------------------------------------------

bool_t HandleIndividualProperty(const char * keyPtr,
                                 json_t * valueObject,
                                  zone_properties * zoneProperties,
                                   json_t * responseObject,
                                    dword_t * propertyIndexPtr,
                                     bool_t * sendToTransmitQueueFlagPtr,
                                      bool_t * broadcastFlagPtr,
                                       bool_t * nameChangeFlagPtr,
                                        uint_8 setFlag)
{
   bool_t returnFlag = FALSE;
   
   
   if (keyPtr && json_is_integer(valueObject))  // check to see if we got integer
   {
      DBGPRINT_INTSTRSTR(json_integer_value(valueObject),"HandleIndividualProperty:",keyPtr);
   }
   if (!strcmp(keyPtr, "PowerLevel"))  // check to see if we got Power Level
   {
      if (!json_is_integer(valueObject))  // check to see if we got integer
      {
         // error
         BuildErrorResponse(responseObject, "PropertyList, Power Level must be integer.", SW_PROPERTY_LIST_POWER_LEVEL_MUST_BE_INTEGER);
         returnFlag = TRUE;
      }
      else
      {
         if ((json_integer_value(valueObject) > MAX_POWER_LEVEL) || (json_integer_value(valueObject) == 0))
         {
            if ((zoneProperties->deviceType == SHADE_TYPE  || zoneProperties->deviceType == SHADE_GROUP_TYPE) && (json_integer_value(valueObject) == 0) )
            {
               // this is OK -- for a shade to go to 0 
               returnFlag = FALSE;
               if (setFlag)
               {
                  zoneProperties->targetPowerLevel = json_integer_value(valueObject);
                  *sendToTransmitQueueFlagPtr = true;
                  DBGPRINT_INTSTRSTR(zoneProperties->targetPowerLevel,"HandleIndividualProperty set shade to 0 targetPowerLevel","");
               }
            }
            else
            {
               snprintf(jsonErrorString, sizeof(jsonErrorString), "PropertyList, Power Lvl must be 1 - %d.", MAX_POWER_LEVEL);
               DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleIndividualProperty fails",jsonErrorString);
               BuildErrorResponse(responseObject, jsonErrorString, SW_PROPERTY_LIST_POWER_LEVEL_MUST_BE_BETWEEN);
               returnFlag = TRUE;
            }
         }
         else if (setFlag)
         {
            if(zoneProperties->deviceType == SWITCH_TYPE)
            {
               // Always set the power level to 100% for switch types. 
               zoneProperties->targetPowerLevel = MAX_POWER_LEVEL;
               DBGPRINT_INTSTRSTR(zoneProperties->targetPowerLevel,"HandleIndividualProperty switch targetPowerLevel MAX_POWER_LEVEL","");
               
               // No need to send it to the transmit queue because it
               // should only ever be set to 100%. However we do need to 
               // broadcast that the level is set to 100% so the app correctly
               // displays the current value
               *broadcastFlagPtr = true;
            }
            else
            {
               zoneProperties->targetPowerLevel = json_integer_value(valueObject);
               DBGPRINT_INTSTRSTR(zoneProperties->targetPowerLevel,"HandleIndividualProperty setFlag targetPowerLevel","");
               *sendToTransmitQueueFlagPtr = true;
            }
         }
      }
   }  // end of if we got Power Level

   else if (!strcmp(keyPtr, "RampRate"))  // check for ramp rate
   {
      if (!json_is_integer(valueObject))  // check to see if we got integer
      {
         // error
         BuildErrorResponse(responseObject, "PropertyList, RampRate must be integer.", SW_PROPERTY_LIST_RAMP_RATE_MUST_BE_AN_INTEGER);
         returnFlag = TRUE;
      }
      else
      {
         if ((json_integer_value(valueObject) > MAX_RAMP_RATE) || (json_integer_value(valueObject) < 0)) 
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "PropertyList, Ramp Rate must be 0 - %d.", MAX_RAMP_RATE);
            DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleIndividualProperty fails",jsonErrorString);
            BuildErrorResponse(responseObject, jsonErrorString, SW_PROPERTY_LIST_RAMP_RATE_MUST_BE_BETWEEN);
            returnFlag = TRUE;
         }
         else if (setFlag)
         {
            zoneProperties->rampRate = json_integer_value(valueObject);
            *broadcastFlagPtr = true;
         }
      }
   }  // end of else if we got ramp rate

   else if (!strcmp(keyPtr, "Name"))  // check for name
   {
      if (!json_is_string(valueObject))  // check to see if we got string
      {
         // error
         BuildErrorResponse(responseObject, "PropertyList, Name must be string.", SW_PROPERTY_LIST_NAME_MUST_BE_A_STRING);
         returnFlag = TRUE;
      }
      else if (setFlag)
      {
         SanitizeJsonStringObject(valueObject);  // sanitize this string object, since it will be used in our subsequent broadcast.
         snprintf(zoneProperties->ZoneNameString, sizeof(zoneProperties->ZoneNameString), "%s", json_string_value(valueObject));
         zoneProperties->ZoneNameString[sizeof(zoneProperties->ZoneNameString) - 1] = 0;  // force null terminator

         *broadcastFlagPtr = true;
         *nameChangeFlagPtr = true;
      }
   }  // end of else if we got name

   else if (!strcmp(keyPtr, "Power"))  // check for name
   {
      if (!json_is_boolean(valueObject))  // check to see if we got boolean
      {
         // error
         BuildErrorResponse(responseObject, "PropertyList, Power must be boolean.", SW_PROPERTY_LIST_POWER_MUST_BE_BOOLEAN);
         returnFlag = TRUE;
      }
      else if (setFlag)
      {
         zoneProperties->targetState = json_is_true(valueObject);
         *sendToTransmitQueueFlagPtr = true;
      }
   }  // end of else if we got power state

   else if (!strcmp(keyPtr, "DeviceType"))  // check for device type
   {
      if (!json_is_string(valueObject))
      {
         // error
         BuildErrorResponse(responseObject, "PropertyList, DeviceType must be a string.", SW_PROPERTY_LIST_DEVICE_TYPE_MUST_BE_A_STRING);
         returnFlag = TRUE;
      }
      else 
      {
         const char * deviceTypeString = json_string_value(valueObject);
         Dev_Type deviceType = NO_TYPE;

         // Convert the string to a device type
         if(!strcmp(deviceTypeString, "Dimmer"))
         {
            deviceType = DIMMER_TYPE;
         }
         if(!strcmp(deviceTypeString, "Fan"))
         {
            deviceType = FAN_CONTROLLER_TYPE;
         }
         else if(!strcmp(deviceTypeString, "Switch"))
         {
            deviceType = SWITCH_TYPE;
         }
         else if(!strcmp(deviceTypeString, "Shade"))
         {
            deviceType = SHADE_TYPE;
         }
         else if(!strcmp(deviceTypeString, "ShadeGroup"))
         {
            deviceType = SHADE_GROUP_TYPE;
         }
         
         // Check the parameters
         if ((deviceType != DIMMER_TYPE)
             && (deviceType != FAN_CONTROLLER_TYPE)
             && (deviceType != SWITCH_TYPE)
             && (deviceType != SHADE_TYPE)
             && (deviceType != SHADE_GROUP_TYPE))
         {
            DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleIndividualProperty fails: PropertyList, DeviceType must be \"Dimmer\" or \"Switch\" or \"Fan\" or \"Shade\" or \"ShadeGroup\".","");
            BuildErrorResponse(responseObject, "PropertyList, DeviceType must be \"Dimmer\" or \"Switch\" or \"Fan\" or \"Shade\" or \"ShadeGroup\".", SW_PROPERTY_LIST_DEVICE_TYPE_MUST_BE_DIM_OR_SW);
            returnFlag = TRUE;
         }
         else if ((zoneProperties->deviceType != NETWORK_REMOTE_TYPE) &&
                  (zoneProperties->deviceType != NO_TYPE))
         {
            BuildErrorResponse(responseObject, "PropertyList, Can only change device type when not already set.", SW_PROPERTY_LIST_CAN_ONLY_CHANGE_DEVICE_TYPE_IF_NOT);
            returnFlag = TRUE;
         }
         else if (setFlag)
         {
            zoneProperties->deviceType = deviceType;
            *broadcastFlagPtr = true;
         }
      }
   }  // end of else if we got a device type

   else  // we got a invalid property value
   {
      // error
      DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleIndividualProperty fails: Invalid Property' in List.","");
      BuildErrorResponse(responseObject, "Invalid Property' in List.",  SW_INVALID_PROPERTY_IN_LIST);
      returnFlag = TRUE;
   }

   return returnFlag;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void BuildZonePropertiesObject(json_t * reportPropertiesObject, byte_t zoneIdxWanted, dword_t propertyBitmask)
{

   zone_properties zoneProperties;
   json_t *listObj;

   if (zoneIdxWanted < MAX_NUMBER_OF_ZONES)
   {
      if (GetZoneProperties(zoneIdxWanted, &zoneProperties))
      {
         json_object_set_new(reportPropertiesObject, "ZID", json_integer(zoneIdxWanted));

         listObj = json_object();
         if (listObj)
         {
            if (propertyBitmask & NAME_PROPERTIES_BITMASK)
            {
               json_object_set_new(listObj, "Name", Sanitized_json_string(zoneProperties.ZoneNameString));
            }
   
            if (propertyBitmask & DEV_PROPERTIES_BITMASK)
            {
               switch (zoneProperties.deviceType)
               {
                  case DIMMER_TYPE:
                     json_object_set_new(listObj, "DeviceType", json_string("Dimmer"));
                     break;
                  
                  case SWITCH_TYPE:
                     json_object_set_new(listObj, "DeviceType", json_string("Switch"));
                     break;
                  
                  case FAN_CONTROLLER_TYPE:
                     json_object_set_new(listObj, "DeviceType", json_string("Fan"));
                     break;
   
                  case SHADE_TYPE:
                     json_object_set_new(listObj, "DeviceType", json_string("Shade"));
                     break;
                     
                  case SHADE_GROUP_TYPE:
                     json_object_set_new(listObj, "DeviceType", json_string("ShadeGroup"));
                     break;
                  
                  case NETWORK_REMOTE_TYPE:
                  case NO_TYPE:
                     json_object_set_new(listObj, "DeviceType", json_string("Unknown"));
                     break;
   
                  default:
                     // shouldn't happen
                     json_object_set_new(listObj, "DeviceType", json_string("???"));
                     break;
               }
            }
   
            if (propertyBitmask & PWR_LVL_PROPERTIES_BITMASK)
            {
               json_object_set_new(listObj, "PowerLevel", json_integer(zoneProperties.powerLevel));
            }
   
            if (propertyBitmask & RAMP_RATE_PROPERTIES_BITMASK)
            {
               json_object_set_new(listObj, "RampRate", json_integer(zoneProperties.rampRate));
            }
   
            if (propertyBitmask & PWR_BOOL_PROPERTIES_BITMASK)
            {
               if (zoneProperties.state)
               {
                  json_object_set_new(listObj, "Power", json_true());
               }
               else
               {
                  json_object_set_new(listObj, "Power", json_false());
               }
            }
            json_object_set_new(reportPropertiesObject, "PropertyList", listObj);
         }
      }
      else
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "Could not get zone properties for zone %d.", zoneIdxWanted);
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"BuildZonePropertiesObject fails",jsonErrorString);
         BuildErrorResponse(reportPropertiesObject, jsonErrorString, SW_COULD_NOT_GET_ZONE_PROPERTIES_FOR_ZONE);
      }
   }
   else
   {
      DBGPRINT_INTSTRSTR_ERROR(__LINE__,"BuildZonePropertiesObject fails: LIGHT REPORT, no zone found","");
      BuildErrorResponse(reportPropertiesObject, "LIGHT REPORT, no zone found", SW_LIGHT_REPORT_NO_ZONE_FOUND);
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleDeleteZone(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr)
{
   json_t   *zoneObject;
   json_t   *appContextObj;
   json_t   *broadcastObject;
   bool_t   errorFlag = false;
   //bool_t   emptyZoneList = true;
   uint32_t appContextId = 0;
   byte_t   zoneIndex;
   zone_properties zoneProperties;
   
   // get the zone id object
   zoneObject = json_object_get(root, "ZID");
   if (!zoneObject)
   {
      BuildErrorResponse(responseObject, "No Zone ID field.", SW_NO_ZONE_ID_FIELD);
   }
   else
   {
      if (!json_is_integer(zoneObject))
      {
         BuildErrorResponse(responseObject, "Zone ID must be an integer", SW_ZONE_ID_MUST_BE_AN_INTEGER);
      }
      else
      {
         zoneIndex = json_integer_value(zoneObject);
         if (zoneIndex >= MAX_NUMBER_OF_ZONES)
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "Zone ID must be less then %d.", MAX_NUMBER_OF_ZONES);
            BuildErrorResponse(responseObject, jsonErrorString, SW_ZONE_ID_MUST_BE_LESS_THEN);
         }
         else if (!GetZoneProperties(zoneIndex, &zoneProperties))
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "Could not get zone properties for zone %d.", zoneIndex);
            BuildErrorResponse(responseObject, jsonErrorString, SW_COULD_NOT_GET_ZONE_PROPERTIES_FOR_ZONE);
         }
         else if (zoneProperties.ZoneNameString[0] == 0)
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "Zone ID %d doesn't exist.", zoneIndex);
            BuildErrorResponse(responseObject, jsonErrorString, SW_ZONE_ID_DOES_NOT_EXIST);
         }
         else
         {
            appContextObj = json_object_get(root, "AppContextId");
            if (appContextObj)
            {
               if (!json_is_integer(appContextObj))
               {
                  BuildErrorResponse(responseObject, "App Context ID error", SW_APP_CONTEXT_ID_ERROR);
               }
               else
               {
                  appContextId = json_integer_value(appContextObj);
               }
            }

            if(zoneProperties.EliotDeviceId[0])
            {
               Eliot_QueueDeleteDevice(zoneProperties.EliotDeviceId);
            }

            // delete the zone by clearing the name string
            memset(zoneProperties.ZoneNameString, 0, sizeof(zoneProperties.ZoneNameString));
            memset(zoneProperties.EliotDeviceId, 0, sizeof(zoneProperties.EliotDeviceId));
            // check if a shade, if so, then delete the shade ID from shade list
            // Set the properties for the zone
            SetZoneProperties(zoneIndex, &zoneProperties);
#ifdef SHADE_CONTROL_ADDED
            if (zoneProperties.deviceType == SHADE_TYPE || zoneProperties.deviceType == SHADE_GROUP_TYPE)
            {
               memset(shadeInfoStruct.shadeInfo[zoneIndex].idString, 0, sizeof(shadeInfoStruct.shadeInfo[zoneIndex].idString));
               WriteAllShadeIdsToFlash();
            }
#endif

            // fill in the rest of the properties
            *jsonBroadcastObjectPtr = json_object();
            if (*jsonBroadcastObjectPtr)
            {
               json_object_set_new(*jsonBroadcastObjectPtr, "ID", json_integer(0));
               json_object_set_new(*jsonBroadcastObjectPtr, "ZID", json_integer(zoneIndex));
               if (appContextId)  // if we got an appContextId, send it back
               {
                  json_object_set_new(*jsonBroadcastObjectPtr, "AppContextId", json_integer(appContextId));
                }
               json_object_set_new(*jsonBroadcastObjectPtr, "Service", json_string("ZoneDeleted"));
               json_object_set_new(*jsonBroadcastObjectPtr, "Status", json_string("Success"));
            }
         }
      }
   }

   if((!AnyZonesInArray()) &&
      (!AnySceneControllersInArray()))
   {
      // Clear the house ID and report the information
      global_houseID = 0;
      BroadcastSystemInfo();

      // Turn on the house ID not set LED because all devices have been removed
      SetGPIOOutput(HOUSE_ID_NOT_SET, LED_ON);

      // If we have no zones, set systemProperties.configured false and broadcast
      systemProperties.configured = FALSE;

      // Set the flash save timer to make sure this value is saved
      flashSaveTimer = FLASH_SAVE_TIMER;

      // Allocate memory for the json object
      broadcastObject = json_object();
      if (broadcastObject)
      {
         // Build the json object
         json_object_set_new(broadcastObject, "ID", json_integer(0));
         json_object_set_new(broadcastObject, "Service", json_string("SystemPropertiesChanged"));
         BuildSystemPropertiesObject(broadcastObject, SYSTEM_PROPERTY_CONFIGURED_BITMASK);
         json_object_set_new(broadcastObject, "Status", json_string("Success"));
   
         // Broadcast SystemPropertiesChanged for configured property
         SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
   
         // Free the JSON memory
         json_decref(broadcastObject);
      }
   }
}
   
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleListScenes(json_t * responseObject)
{
   json_t * sceneListArray;
   json_t * sceneArrayElement;
   scene_properties sceneProperties;
   
   if (responseObject)
   {
      sceneListArray = json_array();
      if (sceneListArray)
      {
         for (int sceneIdx = 0; sceneIdx < MAX_NUMBER_OF_SCENES; sceneIdx++)
         {
            // fill in the scene ID and name if it exists
            if (GetSceneProperties(sceneIdx, &sceneProperties))
            {
               if (sceneProperties.SceneNameString[0])
               {
                  sceneArrayElement = json_object();
                  if (sceneArrayElement)
                  {
                     json_object_set_new(sceneArrayElement, "SID", json_integer(sceneIdx));
                     json_array_append_new(sceneListArray, sceneArrayElement);
                  }
               }
            }
         }  // end of for  loop
      
         json_object_set_new(responseObject, "SceneList", sceneListArray);
      }
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleReportSceneProperties(json_t * root, json_t * responseObject)
{

   json_t *sceneObject;
   byte_t sceneIndex = 0;
   byte_t zidx = 0;
   json_t *propObj;
   json_t *sceneListArray;
   json_t *sceneListArrayElement;
   json_t *freqObject;
   json_t *freqValuesArray;
   json_t *trigObject;
   json_t *trigValuesArray;
   scene_properties sceneProperties;

   if (responseObject)
   {
      sceneObject = json_object_get(root, "SID");
      if (!sceneObject)
      {
         BuildErrorResponse(responseObject, "No Scene ID field.", SW_NO_SCENE_ID_FIELD);
         return;
      }
   
      if (json_is_integer(sceneObject))  // else, is it an integer
      {
         // get scene requested by the GUI and set array value
         sceneIndex = json_integer_value(sceneObject);
      }
      else
      {
         BuildErrorResponse(responseObject, "Scene ID must be an integer.", SW_SCENE_ID_MUST_BE_AN_INTEGER);
         return;
      }
   
      // check to see if the scene id is invalid
      if (json_integer_value(sceneObject) >= MAX_NUMBER_OF_SCENES)
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "Scene ID is invalid. %d", (int) json_integer_value(sceneObject));
         BuildErrorResponse(responseObject, jsonErrorString, SW_SCENE_ID_IS_INVALID);
         return;
      }
   
      // check to see if the scene has a name, if not, it isn't populated, so return
      if (GetSceneProperties(sceneIndex, &sceneProperties))
      {
         if (sceneProperties.SceneNameString[0] == 0)
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "Scene %d does not exist.", sceneIndex);
            BuildErrorResponse(responseObject, jsonErrorString, SW_SCENE_DOES_NOT_EXIST);
            return;
   
         }
      }
      else
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "Could not get properties for scene %d.", sceneIndex);
         BuildErrorResponse(responseObject, jsonErrorString, SW_COULD_NOT_GET_PROPERTIES_FOR_SCENE);
         return;
      }
   
      // fill in the scene ID that we are returning
      json_object_set_new(responseObject, "SID", json_integer(sceneIndex));
   
      // create an object to store the property list    
      propObj = json_object();
      if (propObj)
      {
         json_object_set_new(propObj, "Name", Sanitized_json_string(sceneProperties.SceneNameString));
         // create an array for the zone list
         sceneListArray = json_array();
         if (sceneListArray)
         {
            for (int zoneDataIdx = 0; zoneDataIdx < sceneProperties.numZonesInScene; zoneDataIdx++)
            {
               sceneListArrayElement = json_object();  // create one element for the array
               if (sceneListArrayElement)
               {
                  // fill the element with values
                  json_object_set_new(sceneListArrayElement, "ZID", json_integer(sceneProperties.zoneData[zoneDataIdx].zoneId));
                  json_object_set_new(sceneListArrayElement, "Lvl", json_integer(sceneProperties.zoneData[zoneDataIdx].powerLevel));
                  json_object_set_new(sceneListArrayElement, "RR", json_integer(sceneProperties.zoneData[zoneDataIdx].rampRate));
                  if (sceneProperties.zoneData[zoneDataIdx].state)
                  {
                     json_object_set_new(sceneListArrayElement, "St", json_true());
                  }
                  else
                  {
                     json_object_set_new(sceneListArrayElement, "St", json_false());
                  }
                  // append the element to the array
                  json_array_append_new(sceneListArray, sceneListArrayElement);
               }
            }
            // add the array to the json report output
            json_object_set_new(propObj, "ZoneList", sceneListArray);
         }
      
         json_object_set_new(propObj, "TriggerTime", json_integer(sceneProperties.nextTriggerTime));
      
         json_object_set_new(propObj, "Frequency", json_integer(sceneProperties.freq));
      
         json_object_set_new(propObj, "TriggerType", json_integer(sceneProperties.trigger));
      
         json_object_set_new(propObj, "DayBits", json_integer((int) sceneProperties.dayBitMask.days));
        
         json_object_set_new(propObj, "Delta", json_integer((int) sceneProperties.delta));
        
         if (sceneProperties.running)
         {
            json_object_set_new(propObj, "Running", json_true());
         }
         else
         {
            json_object_set_new(propObj, "Running", json_false());
         }
      
         if (sceneProperties.skipNext)
         {
            json_object_set_new(propObj, "Skip", json_true());
         }
         else
         {
            json_object_set_new(propObj, "Skip", json_false());
         }
      
         // store the property list into the response object
         json_object_set_new(responseObject, "PropertyList", propObj);
      }
      
      // we have filled the response in, it will now be transmitted via socket to app
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool_t CheckFreqDaybits(json_t * propertyListObject, json_t * responseObject)
{
    FrequencyType freq = NONE;
    DailySchedule dayBits;
    bool_t daysExist = FALSE;
    const char * keyPtr;
    json_t     * valueObject;
  
    valueObject = json_object_get(propertyListObject, "Frequency");
    if (valueObject)
    {
       if (!json_is_integer(valueObject))
       {
          // error
          BuildErrorResponse(responseObject, "Frequency must be a integer.", SW_FREQUENCY_MUST_BE_AN_INTEGER);
          return TRUE;
       }
       else 
       {
          freq = json_integer_value(valueObject);
           
          if (freq > EVERY_WEEK)
          {
             BuildErrorResponse(responseObject, "Frequency index too great", SW_FREQUENCY_INDEX_TOO_GREAT);
             return TRUE;
          }
       }
    }
    
    valueObject = json_object_get(propertyListObject, "DayBits");
    if (valueObject)
    {
       if (!json_is_integer(valueObject))
        {
           // error
           BuildErrorResponse(responseObject, "DayBits must be a integer.", SW_DAYBITS_MUST_BE_AN_INTEGER);
           return TRUE;
        }
        else  
        {
           dayBits.days = (byte_t) json_integer_value(valueObject);
            
           if ((dayBits.DayBits.spare7 >= 0x80) || (dayBits.days == 0)) 
           {
              BuildErrorResponse(responseObject, "DayBits invalid", SW_DAYBITS_INVALID);
              return TRUE;
           }
           daysExist = TRUE;
        }
    }
    
    // check for the following rule: if freq is EVERY_WEEK, we must have Days Bits, else we must not
    //   have day bits sent in
    if (freq == EVERY_WEEK)
    {
       if (!daysExist)
       {
          //error, we should have got day bits with freq of Every Week
          BuildErrorResponse(responseObject, "Need Days Bits when Freq:EVERY WEEK", SW_NEED_DAYBITS_WHEN_FREQ_IS_EVERY_WEEK);
          return TRUE;
       }
    }
    // else freq is not Every Week, so we can't have day bits
    else if (daysExist)
    {
       // error, we got Freq of Once or None and we got day bits
       BuildErrorResponse(responseObject, "Need Freq:EVERY WEEK with Day Bits", SW_NEED_FREQ_EVERY_WEEK_WITH_DAYBITS);
       return TRUE;
    }
    
    return FALSE;
    
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleSetSceneProperties(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr)
{
   scene_properties  sceneProperties;
   json_t            * propertyListObject;
   json_t            * valueObject;
   json_t            * sceneObject;
   json_t            * appContextObj;
   const char        * keyPtr;
   uint32_t          appContextId = 0;
   byte_t            sceneIndex;
   bool_t            nameChangeFlag = false;
   bool_t            errorFlag;
   
   // get the scene id object
   sceneObject = json_object_get(root, "SID");
   if (!sceneObject)
   {
      BuildErrorResponse(responseObject, "No Scene ID field.", SW_NO_SCENE_ID_FIELD);
      return;
   }

   if (json_is_integer(sceneObject))
   {
      if ((json_integer_value(sceneObject) >= MAX_NUMBER_OF_SCENES))
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "Scene ID must be less then %d.", MAX_NUMBER_OF_SCENES);
         BuildErrorResponse(responseObject, jsonErrorString, SW_SCENE_ID_MUST_BE_LESS_THEN);
         return;
      }
      else
      {
         sceneIndex = json_integer_value(sceneObject);
      }
   }
   else
   {
      BuildErrorResponse(responseObject, "Scene ID must be an integer", SW_SCENE_ID_MUST_BE_AN_INTEGER);
      return;
   }

   if (!GetSceneProperties(sceneIndex, &sceneProperties))
   {
      snprintf(jsonErrorString, sizeof(jsonErrorString), "Could not get scene properties for scene index %d.", sceneIndex);
      BuildErrorResponse(responseObject, jsonErrorString, SW_COULD_NOT_GET_SCENE_PROPERTIES_FOR_SCENE_INDEX);
      return;
   }

   propertyListObject = json_object_get(root, "PropertyList");
   if (!json_is_object(propertyListObject))
   {
      BuildErrorResponse(responseObject, "Invalid 'PropertyList'.", SW_INVALID_PROPERTY_LIST);
      return;
   }

   // check for DayBits and Freq
   errorFlag = CheckFreqDaybits(propertyListObject, responseObject);
   if (errorFlag)
   {
      return;
   }   
   
   // first, go through the entire property list and make sure we don't have ANY errors.
   json_object_foreach(propertyListObject, keyPtr, valueObject)
   {
      errorFlag = HandleIndividualSceneProperty(keyPtr, valueObject, &sceneProperties, responseObject, &nameChangeFlag, FALSE);
      if (errorFlag)
      {
         return;
      }
   }
   
   // if everything was ok, go through the property list again and execute.
   json_object_foreach(propertyListObject, keyPtr, valueObject)
   {
      errorFlag = HandleIndividualSceneProperty(keyPtr, valueObject, &sceneProperties, responseObject, &nameChangeFlag, TRUE);
      if (errorFlag)
      {
         return;
      }
   }

   // Calculate the next trigger time
   TIME_STRUCT currentTime;
   _time_get(&currentTime);
   calculateNewTrigger(&sceneProperties, currentTime);

   // Set the properties for the scene
   SetSceneProperties(sceneIndex, &sceneProperties);

   if (nameChangeFlag)
   {
      if (sceneProperties.EliotDeviceId[0])
      {
         Eliot_QueueRenameScene(sceneIndex);
      }
   }
   
   // if we don't have an error, then prepare the asynchronous message
   // to all json sockets.
   
   appContextObj = json_object_get(root, "AppContextId");
   if (appContextObj)
   {
      if (!json_is_integer(appContextObj))
      {
         BuildErrorResponse(responseObject, "App Context ID error", SW_APP_CONTEXT_ID_ERROR);
      }
      else
      {
         appContextId = json_integer_value(appContextObj);
      }
   }

   // Create an exact copy of the JSON data that was sent as input, with the exception
   // of the next TriggerTime, which could have changed due to a change in TriggerType.
   json_object_set_new(propertyListObject, "TriggerTime", json_integer(sceneProperties.nextTriggerTime));
   *jsonBroadcastObjectPtr = json_copy(root);
   
   json_object_set_new(*jsonBroadcastObjectPtr, "ID", json_integer(0));
   json_object_set_new(*jsonBroadcastObjectPtr, "Status", json_string("Success"));

   if (appContextId)
   {
      json_object_set_new(*jsonBroadcastObjectPtr, "AppContextId", json_integer(appContextId));
   }
   
   json_object_set_new(*jsonBroadcastObjectPtr, "Service", json_string("ScenePropertiesChanged"));
}

//-----------------------------------------------------------------------------
// ExecuteScene()
//  
//-----------------------------------------------------------------------------

void ExecuteScene(byte_t sceneIndex, scene_properties * scenePropertiesPtr, uint32_t appContextId, bool_t activate, byte_t value, byte_t change, byte_t rampRate)
{
   json_t * broadcastObject;
   json_t * propObject;
   byte_t scenePercent = 0;
   bool_t sendFlag = FALSE;
   zone_properties zoneProperties;
   bool_t foundIt;
   byte_t sceneControllerIndex = 0;
   byte_t avgLevel = 0;
   scene_controller_properties sceneControllerProperties; 
#if DEBUG_SOCKETTASK_ANY
   char                       debugBuf[100];
   MQX_TICK_STRUCT            debugLocalStartTick;
   MQX_TICK_STRUCT            debugLocalEndTick;
   bool                       debugLocalOverflow;
   uint32_t                   debugLocalOverflowMs;
#endif

   broadcastObject = json_object();
   if (broadcastObject == NULL)
   {
      return;
   }
   
   json_object_set_new(broadcastObject, "ID", json_integer(0));
   json_object_set_new(broadcastObject, "Service", json_string("ScenePropertiesChanged"));

   if (appContextId) // if we have a AppContextId, send it back
   {
      json_object_set_new(broadcastObject, "AppContextId", json_integer(appContextId));
   }

   // build PropertyList 
   json_object_set_new(broadcastObject, "SID", json_integer(sceneIndex));
   
   // Check to see if the scene is running, if it is, return success
   if (scenePropertiesPtr->running == true)
   {
      //broadcast scenePropertiesChanged with created PropertyList
      json_object_set_new(broadcastObject, "Status", json_string("Already Running"));
      SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
      json_decref(broadcastObject);
  
   }
   else
   {  
      propObject = json_object();
      if (propObject == NULL)
      {
         json_decref(broadcastObject);
         return;
      }
      
      scenePropertiesPtr->running = true;
      json_object_set_new(propObject, "Running", json_true());
   
      // store the property list into the broadcast object
      json_object_set_new(broadcastObject, "PropertyList", propObject);
   
      //broadcast zonePropertiesChanged with created PropertyList
      json_object_set_new(broadcastObject, "Status", json_string("Success"));
      SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
      json_decref(broadcastObject);
      
      // Send the scene property changed to the samsung task
      Eliot_QueueSceneStarted(sceneIndex);

      // If the rampRate is 0, set it to 100 so the scene runs instantly,
      // otherwise we'll have a div by zero and undefined behavior
      int rampRate = scenePropertiesPtr->zoneData[0].rampRate;
      if (rampRate == 0)
      {
        rampRate = 100;
      }

      // Queue our scene finished function after the scene should be done running.
      // sceneIndex will not be in scope when this timer fires so we cannot reference it.
      float sceneDuration = (1.00 / ((float)rampRate / 100.00)) * 1000;
      _timer_start_oneshot_after(
          (TIMER_NOTIFICATION_TIME_FPTR)Eliot_QueueSceneFinished,
          (void*)sceneIndex,     // Use the value of sceneIndex, not its location
          TIMER_KERNEL_TIME_MODE,
          (uint32_t)sceneDuration
      );

      // Set the target levels for the scene and add them to the transmit task queue
      for (int zoneIndex = 0; zoneIndex < scenePropertiesPtr->numZonesInScene; zoneIndex++)
      {
         // Calculate the scene percent
         scenePercent = (byte_t)(((zoneIndex+1) * 100) / scenePropertiesPtr->numZonesInScene);
            
         if (GetZoneProperties(scenePropertiesPtr->zoneData[zoneIndex].zoneId, &zoneProperties))
         {
            // Set the target level for the zones for the scene.
            if (activate) // if we are to activate the scene, set according to the scene
            {
               if (change == 1) // we are to increase by value (value may be 0, if not adjusting)
               {
                  // if value has a value, then we are adjusting the scene, so set the target level
                  //   to be the current power level of the zone plus the value sent in
                  if (value)
                  {
                     if (zoneProperties.state) // only adjust of state is ON
                     {
                        zoneProperties.targetPowerLevel = zoneProperties.powerLevel + value;    
                     }
                  }
                  else
                  {
                     zoneProperties.targetPowerLevel = scenePropertiesPtr->zoneData[zoneIndex].powerLevel;
                     zoneProperties.targetState = scenePropertiesPtr->zoneData[zoneIndex].state;
                     // set the ramp rate to value wanted for the scene
                     zoneProperties.rampRate = scenePropertiesPtr->zoneData[zoneIndex].rampRate;
                  }
                  if (zoneProperties.targetPowerLevel > MAX_POWER_LEVEL)
                  {
                     zoneProperties.targetPowerLevel = MAX_POWER_LEVEL;
                  }
               }
               else
               {
                  // if value has a value, then we are adjusting the scene, so set the target level
                  //   to be the current power level of the zone plus the value sent in
                  if (value)
                  { 
                     if (zoneProperties.state) // only adjust of state is ON
                     {
                        if ((signed char)(zoneProperties.powerLevel - value) > 0)
                        {
                           zoneProperties.targetPowerLevel = zoneProperties.powerLevel - value;
                        }
 
                        else
                        {
                           zoneProperties.targetPowerLevel =  0;
                        }
                     }                 
                  }
                  else
                  {
                     zoneProperties.targetPowerLevel = scenePropertiesPtr->zoneData[zoneIndex].powerLevel;
                     zoneProperties.targetState = scenePropertiesPtr->zoneData[zoneIndex].state;
                     // set the ramp rate to value wanted for the scene
                     zoneProperties.rampRate = scenePropertiesPtr->zoneData[zoneIndex].rampRate;
                  }
               }
               
            }
            else // deactivate -- turn off if scene said to go on
            {
               if (scenePropertiesPtr->zoneData[zoneIndex].state) // if scene wanted it on, to deact, turn off
               {
                  zoneProperties.targetState = FALSE;
               }
               else // just leave as is if deactivating and we are already off
               {
                  zoneProperties.targetPowerLevel = scenePropertiesPtr->zoneData[zoneIndex].powerLevel;
                  zoneProperties.targetState = scenePropertiesPtr->zoneData[zoneIndex].state;
               }
               
               // if ramp rate sent in as Slow deactivate, set the zone ramp and execute
               //   The ramp rate will be set back when scene or zone is executed again
               if (rampRate == SLOW_DEACTIVATION_RAMP_VALUE)
               {
                  zoneProperties.rampRate = SLOW_DEACTIVATION_RAMP_VALUE;
               }
            }
            
            // Set the app context id for the zone
            zoneProperties.AppContextId = appContextId;
            
            // Set the zone properties
            DEBUG_ZONEARRAY_MUTEX_START;          
            _mutex_lock(&ZoneArrayMutex);
            DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ExecuteScene before ZoneArrayMutex",DEBUGBUF);
            DEBUG_ZONEARRAY_MUTEX_START;          

            SetZoneProperties(scenePropertiesPtr->zoneData[zoneIndex].zoneId, &zoneProperties);

            DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ExecuteScene after ZoneArrayMutex",DEBUGBUF);
            _mutex_unlock(&ZoneArrayMutex);
            
            // Send the Ramp command to the RFM 100 transmit task with the zone id,
            // scene id and scene percentage so it can send the appropriate
            // broadcast messages at the time of RF transmission
            SendRampCommandToTransmitTask(scenePropertiesPtr->zoneData[zoneIndex].zoneId, sceneIndex, scenePercent, RFM100_MESSAGE_LOW_PRIORITY);
            

            
         } // if (GetZoneProperties(scenePropertiesPtr->zoneData[zoneIndex].zoneId, &zoneProperties))
         else // else, the zone does not exist anymore
         {
            if (scenePercent == 100) // if we have completed the scene, send running:false
            {
               // broadcast that running is false
                broadcastObject = json_object();
                if (broadcastObject)
                {
                   propObject = json_object();
                   if (propObject)
                   {     
                      json_object_set_new(broadcastObject, "ID", json_integer(0));
                      json_object_set_new(broadcastObject, "Service", json_string("ScenePropertiesChanged"));
         
                      if (appContextId) // if we have a AppContextId, send it back
                      {
                         json_object_set_new(broadcastObject, "AppContextId", json_integer(appContextId));
                      }
         
                      // build PropertyList 
                      json_object_set_new(broadcastObject, "SID", json_integer(sceneIndex));
         
                      scenePropertiesPtr->running = false;
                      json_object_set_new(propObject, "Running", json_false());
         
                      // store the property list into the broadcast object
                      json_object_set_new(broadcastObject, "PropertyList", propObject);
                   }
      
                   //broadcast zonePropertiesChanged with created PropertyList
                   json_object_set_new(broadcastObject, "Status", json_string("Success"));
                   SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
                   json_decref(broadcastObject);
                }
            } // if (scenePercent == 100)
         }  // else, zone does not exist anymore 
      } // for (int zoneIndex = 0; zoneIndex < scenePropertiesPtr->numZonesInScene; zoneIndex++)
 
      // now that we have set the scene, send Scene Executed to scene controllers
      // find the scene controller that contains this scene
      foundIt = FALSE;
      for (sceneControllerIndex=0; sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS; sceneControllerIndex++)
      {
         if (GetSceneControllerProperties(sceneControllerIndex, &sceneControllerProperties))
         {
            for (byte_t bankIndex=0; bankIndex < MAX_SCENES_IN_SCENE_CONTROLLER; bankIndex++)
            {
               if (sceneControllerProperties.bank[bankIndex].sceneId == sceneIndex) 
               {
                  foundIt = TRUE;
                  sceneControllerProperties.bank[bankIndex].toggled = activate;
                  break;  // break out of bankIndex for loop
               }
            }
         }
         if (foundIt)
         {
            break; // break out of sceneControllerIndex for loop
         }
      }
      // if we found a scene controller with this scene, send the scene executed to the scene controllers
      //   Note, calculate the average power of the scene
      if (foundIt)
      {
         if (value != 0) // We are adjusting the scene up or down
         {
            avgLevel = CalculateAvgPowerLevelOfScene(sceneIndex, TRUE);
         }
         else
         {
            avgLevel = CalculateAvgPowerLevelOfScene(sceneIndex, FALSE);
         }
         
         SendSceneExecutedToTransmitTask(&sceneControllerProperties, sceneIndex, avgLevel, activate);
         SetSceneControllerProperties(sceneControllerIndex, &sceneControllerProperties);
      }   
      
      
   } // end of else, this scene was not running
   
}

//-----------------------------------------------------------------------------
// HandleRunScene()
//
//  given a Scene ID by the API, this will set the existing scene to execute
//  immediately, no matter what its trigger type or time is.
//
//-----------------------------------------------------------------------------

void HandleRunScene(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr)
{
   json_t *sceneObject;
   json_t *appContextObj;
   uint32_t appContextId = 0;
   bool_t errorFlag = false;
   byte_t sceneIndex;
   scene_properties sceneProperties;
   
   // get the scene id object
   sceneObject = json_object_get(root, "SID");
   if (!sceneObject)
   {
      BuildErrorResponse(responseObject, "No Scene ID field.", SW_NO_SCENE_ID_FIELD);
      errorFlag = true;
   }

   if ((json_is_integer(sceneObject)) && (!errorFlag))
   {
      if ((json_integer_value(sceneObject) >= MAX_NUMBER_OF_SCENES))
      {
         snprintf(jsonErrorString, sizeof(jsonErrorString), "Scene ID must be less then %d.", MAX_NUMBER_OF_SCENES);
         BuildErrorResponse(responseObject, jsonErrorString, SW_SCENE_ID_MUST_BE_LESS_THEN);
         errorFlag = true;
      }
      else
      {
         sceneIndex = json_integer_value(sceneObject);

         if (GetSceneProperties(sceneIndex, &sceneProperties))
         {
            if (sceneProperties.SceneNameString[0] == 0)
            {
               snprintf(jsonErrorString, sizeof(jsonErrorString), "Scene %d does not exist.", sceneIndex);
               BuildErrorResponse(responseObject, jsonErrorString, SW_SCENE_ID_DOES_NOT_EXIST);
               errorFlag = true;
            }
         }
         else
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "Unable to get properties for scene %d", sceneIndex);
            BuildErrorResponse(responseObject, jsonErrorString, SW_UNABLE_TO_GET_PROPERTIES_FOR_SCENE);
            errorFlag = true;
         }
      }
   }
   else
   {
      BuildErrorResponse(responseObject, "Scene ID must be an integer", SW_SCENE_ID_MUST_BE_AN_INTEGER);
      errorFlag = true;
   }

   if (!errorFlag)
   {
      appContextObj = json_object_get(root, "AppContextId");
      if (appContextObj)
      {
         if (!json_is_integer(appContextObj))
         {
            BuildErrorResponse(responseObject, "App Context ID error", SW_APP_CONTEXT_ID_ERROR);
         }
         else
         {
            appContextId = json_integer_value(appContextObj);
         }
      }

      // run the scene
      //
      //  
      ExecuteScene(sceneIndex, &sceneProperties, appContextId, TRUE, 0, 1, 0);
      SetSceneProperties(sceneIndex, &sceneProperties);
   }

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool_t HandleIndividualSceneProperty(const char * keyPtr, 
                                      json_t * valueObject, 
                                       scene_properties * scenePropertiesPtr,
                                        json_t * responseObject,
                                         bool_t * nameChangeFlagPtr,
                                          uint_8 setFlag)
{
   bool_t returnFlag = FALSE;
   byte_t index;
   byte_t daybit;
   byte_t freq;
   char * stringPtr;

   if (!strcmp(keyPtr, "Name"))  // check for name
   {
      if (!json_is_string(valueObject))  // check to see if we got string
      {
         // error
         BuildErrorResponse(responseObject, "PropertyList, Name must be string.", SW_PROPERTY_LIST_NAME_MUST_BE_A_STRING);
         returnFlag = TRUE;
      }
      else
      {
         if (setFlag)
         {
            SanitizeJsonStringObject(valueObject);  // sanitize this string object, since it will be used in our subsequent broadcast.
            snprintf(scenePropertiesPtr->SceneNameString, sizeof(scenePropertiesPtr->SceneNameString), "%s", json_string_value(valueObject));
            scenePropertiesPtr->SceneNameString[sizeof(scenePropertiesPtr->SceneNameString) - 1] = 0;    // force null terminator.

            *nameChangeFlagPtr = true;
         }
      }
   }  // end of else if we got name

   else if (!strcmp(keyPtr, "ZoneList"))  // check for Zone List
   {
      if (!json_is_array(valueObject))  // check to see if we got an array
      {
         // error
         BuildErrorResponse(responseObject, "Zone List has to be an array.", SW_ZONE_LIST_HAS_TO_BE_AN_ARRAY);
         returnFlag = TRUE;
      }
      else  // else, it is an array, so check 
      {
         returnFlag = VerifySceneZoneList(valueObject, setFlag, responseObject, scenePropertiesPtr);
      }
   }  // end of check for zone list

   else if (!strcmp(keyPtr, "TriggerTime"))  // check to see if we got trigger Time
   {
      if (!json_is_integer(valueObject))  // check to see if we got integer
      {
         // error
         BuildErrorResponse(responseObject, "Trigger Time must be integer.", SW_TRIGGER_TIME_MUST_BE_INTEGER);
         returnFlag = TRUE;
      }
      else
      {
         if (setFlag)
         {
            // Store the trigger time on the minute boundary
            uint32_t triggerTime = (uint32_t)(json_integer_value(valueObject));
            scenePropertiesPtr->nextTriggerTime = triggerTime - (triggerTime % 60);
         }
      }
   }  // end of if we got trigger time

   else if (!strcmp(keyPtr, "Frequency"))  // check for frequency
   {
      if (!json_is_integer(valueObject))
      {
         // error
         BuildErrorResponse(responseObject, "Frequency must be a integer.", SW_FREQUENCY_MUST_BE_AN_INTEGER);
         returnFlag = TRUE;
      }
      else 
      {
         freq = json_integer_value(valueObject);
          
         if (freq > EVERY_WEEK)
         {
            BuildErrorResponse(responseObject, "Frequency index too great", SW_FREQUENCY_INDEX_TOO_GREAT);
            returnFlag = TRUE;
         }
         else if (setFlag) 
         {
            scenePropertiesPtr->freq = freq;
         }
      }
   }  // end of else if we got frequency      
   
   else if (!strcmp(keyPtr, "DayBits"))  // check for Day Bits
   {
      if (!json_is_integer(valueObject))
       {
          // error
          BuildErrorResponse(responseObject, "DayBits must be a integer.", SW_DAYBITS_MUST_BE_AN_INTEGER);
          returnFlag = TRUE;
       }
       else  
       {
          daybit = json_integer_value(valueObject);
           
          if ((daybit >= 0x80) || (daybit == 0)) 
          {
             BuildErrorResponse(responseObject, "DayBits invalid", SW_DAYBITS_INVALID);
             returnFlag = TRUE;
          }
          else if (setFlag == TRUE)
          {
             scenePropertiesPtr->dayBitMask.days = daybit;
          }
       }   
   }
   
   else if (!strcmp(keyPtr, "TriggerType"))  // check for triggerType
   {
      if (!json_is_integer(valueObject))  //
      {
         // error
         BuildErrorResponse(responseObject, "Trigger Type must be a integer.", SW_TRIGGER_TYPE_MUST_BE_INTEGER);
         returnFlag = TRUE;
      }
      else
      {
         index = json_integer_value(valueObject);
         if (index > SUNSET)
         {
            BuildErrorResponse(responseObject, "Trigger index too great", SW_TRIGGER_INDEX_TOO_GREAT);
            returnFlag = TRUE;
         }
         else if (setFlag)
         {
            scenePropertiesPtr->trigger = index;
         }
      }
   }  // end of else if we got triggerType

   else if (!strcmp(keyPtr, "Delta"))  // check for delta
   {
      if (!json_is_integer(valueObject))  // check to see if we got integer
      {
         // error
         BuildErrorResponse(responseObject, "Delta must be integer", SW_DELTA_MUST_BE_AN_INTEGER);
         returnFlag = TRUE;
      }
      else
      {

         if ((json_integer_value(valueObject) > MAX_DELTA_VALUE) || (json_integer_value(valueObject) < MIN_DELTA_VALUE))
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "Delta must be %d - %d.", MIN_DELTA_VALUE, MAX_DELTA_VALUE);
            BuildErrorResponse(responseObject, jsonErrorString, SW_DELTA_MUST_BE_BETWEEN);
            returnFlag = TRUE;
         }
         else if (setFlag)
         {
            scenePropertiesPtr->delta = (signed char) json_integer_value(valueObject);
         }
      }
   }  // end of else if we got delta
   
   else if (!strcmp(keyPtr, "Skip"))
   {
      if (!json_is_boolean(valueObject))
      {
         BuildErrorResponse(responseObject, "Skip must be a boolean", SW_SKIP_MUST_BE_BOOLEAN);
         returnFlag = TRUE;
      }
      else if (setFlag)
      {
         scenePropertiesPtr->skipNext = json_is_true(valueObject);
      }
   }
   
   else  // we got a invalid property value
   {
      // error
      BuildErrorResponse(responseObject, "Invalid 'Property' in List.", SW_INVALID_PROPERTY_IN_LIST);
      returnFlag = TRUE;
   }
   
   return returnFlag;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleCreateScene(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr)
{
   scene_properties  sceneProperties;
   json_t            * propertyListObject;
   json_t            * valueObject;
   json_t            * appContextObj;
   const char        * keyPtr;
   uint32_t          appContextId = 0;
   byte_t            sceneIndex = 0;
   bool_t            nameChangeFlag = false;
   bool_t            errorFlag;

   // look for an open slot in the scene array
   sceneIndex = GetAvailableSceneIndex();

   // if we have room for another scene, error check the property list and then store the values
   if (sceneIndex < MAX_NUMBER_OF_SCENES)
   {
      propertyListObject = json_object_get(root, "PropertyList");
      if (!json_is_object(propertyListObject))
      {
         BuildErrorResponse(responseObject, "Invalid 'PropertyList'.", SW_INVALID_PROPERTY_LIST);
         return;
      }

      // check for DayBits and Freq
      errorFlag = CheckFreqDaybits(propertyListObject, responseObject);
      if (errorFlag)
      {
         return;
      }   
      
      // Initialize the scene properties to defaults
      sceneProperties.running = false;
      sceneProperties.skipNext = false;
      sceneProperties.numZonesInScene = 0;
      memset(sceneProperties.SceneNameString, 0x00, sizeof(sceneProperties.SceneNameString));

      // first, go through the entire property list and make sure we don't have ANY errors.
      json_object_foreach(propertyListObject, keyPtr, valueObject)
      {
         errorFlag = HandleIndividualSceneProperty(keyPtr, valueObject, &sceneProperties, responseObject, &nameChangeFlag, FALSE);
         if (errorFlag)
         {
            return;
         }
      }
      
      // if everything was ok, go through the property list again and execute.
      json_object_foreach(propertyListObject, keyPtr, valueObject)
      {
         errorFlag = HandleIndividualSceneProperty(keyPtr, valueObject, &sceneProperties, responseObject, &nameChangeFlag, TRUE);
         if (errorFlag)
         {
            return;
         }
      }  
      
      appContextObj = json_object_get(root, "AppContextId");
      if (appContextObj)
      {
         if (!json_is_integer(appContextObj))
         {
            BuildErrorResponse(responseObject, "App Context ID error", SW_APP_CONTEXT_ID_ERROR);
         }
         else
         {
            appContextId = json_integer_value(appContextObj);
         }
      }
     
      // create broadcast json object for SceneCreated
      *jsonBroadcastObjectPtr = json_object();
      if (*jsonBroadcastObjectPtr)
      {
         json_object_set_new(*jsonBroadcastObjectPtr, "ID", json_integer(0));
         json_object_set_new(*jsonBroadcastObjectPtr, "SID", json_integer(sceneIndex));
    
         json_object_set_new(*jsonBroadcastObjectPtr, "Service", json_string("SceneCreated"));
         json_object_set_new(*jsonBroadcastObjectPtr, "Status", json_string("Success"));
         if (appContextId)
         {
            json_object_set_new(*jsonBroadcastObjectPtr, "AppContextId", json_integer(appContextId));
         }
      }

      // Calculate the next trigger time
      TIME_STRUCT currentTime;
      _time_get(&currentTime);
      calculateNewTrigger(&sceneProperties, currentTime);
      
      // Set the scene properties
      SetSceneProperties(sceneIndex, &sceneProperties);
      
      // Add the zone to the samsung cloud
      Eliot_QueueAddScene(sceneIndex);

   }
   else
   {
      DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleCreateScene fails: No Space left for Scene Addition","");
      BuildErrorResponse(responseObject, "No Space left for Scene Addition", USR_NO_SPACE_LEFT_FOR_SCENE_ADDITION);
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void DeleteSceneFromSceneController(byte_t sceneIndex)
{
   bool_t   changeFlag = false;
   byte_t   sceneControllerIndex;
   scene_controller_properties sceneControllerProperties;
  
   for (sceneControllerIndex = 0; sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS; sceneControllerIndex++)
   {
      if (GetSceneControllerProperties(sceneControllerIndex, &sceneControllerProperties)) 
      {
         if (sceneControllerProperties.SceneControllerNameString[0] != 0)
         {

            // check each scene id is the SCTL to see if it matches the scene ID sent in
            //  if a match, set it to NO_SCENE and send assigne scene to that SCTL
            for (byte_t bankArrayIndex = 0; bankArrayIndex < MAX_SCENES_IN_SCENE_CONTROLLER; bankArrayIndex++)
            {
               if (sceneControllerProperties.bank[bankArrayIndex].sceneId == sceneIndex)
               {
                  sceneControllerProperties.bank[bankArrayIndex].sceneId = -1;
                  SendAssignSceneCommandToTransmitTask(&sceneControllerProperties, bankArrayIndex, 0);
                  changeFlag = true;
               }
            }  
            
            // if we have a change, then store new scene controller property into the scene controller
            if (changeFlag)
            {
               // Save the property for the scene controller
               SetSceneControllerProperties(sceneControllerIndex, &sceneControllerProperties);
 
            }
         }
      } // else if(GetSceneControllerProperties(sceneControllerIndex, &sceneControllerProperties)
    } // end for loop
   
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleDeleteScene(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr)
{
   json_t *sceneObject;
   json_t *appContextObj;
   uint32_t appContextId = 0;
   bool_t errorFlag = false;
   byte_t sceneIndex;
   scene_properties sceneProperties;

   // get the scene id object
   sceneObject = json_object_get(root, "SID");
   if (!sceneObject)
   {
      BuildErrorResponse(responseObject, "No Scene ID field.", SW_NO_SCENE_ID_FIELD);
   }
   else
   {
      if (!json_is_integer(sceneObject))
      {
         BuildErrorResponse(responseObject, "Scene ID must be an integer", SW_SCENE_ID_MUST_BE_AN_INTEGER);
      }
      else
      {
         sceneIndex = json_integer_value(sceneObject);
         if (sceneIndex >= MAX_NUMBER_OF_SCENES)
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "Scene ID must be less then %d.", MAX_NUMBER_OF_SCENES);
            BuildErrorResponse(responseObject, jsonErrorString, SW_SCENE_ID_MUST_BE_LESS_THEN);
         }
         else if (!GetSceneProperties(sceneIndex, &sceneProperties))
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "Could not get scene properties for zone %d.", sceneIndex);
            BuildErrorResponse(responseObject, jsonErrorString, SW_COULD_NOT_GET_SCENE_PROPERTIES_FOR_ZONE);
         }
         else if (sceneProperties.SceneNameString[0] == 0)
         {
            snprintf(jsonErrorString, sizeof(jsonErrorString), "Scene %d does not exist.", sceneIndex);
            BuildErrorResponse(responseObject, jsonErrorString, SW_SCENE_ID_DOES_NOT_EXIST);
         }
         else
         {
            if(sceneProperties.EliotDeviceId[0])
            {
               Eliot_QueueDeleteDevice(sceneProperties.EliotDeviceId);
            }

            // delete the scene by clearing the name string
            memset(sceneProperties.SceneNameString, 0, sizeof(sceneProperties.SceneNameString));

            // Set the properties for the scene
            SetSceneProperties(sceneIndex, &sceneProperties);
 
            appContextObj = json_object_get(root, "AppContextId");
            if (appContextObj)
            {
               if (!json_is_integer(appContextObj))
               {
                  BuildErrorResponse(responseObject, "App Context ID error", SW_APP_CONTEXT_ID_ERROR);
               }
               else
               {
                  appContextId = json_integer_value(appContextObj);
               }
            }
            
            // check to see if the scene deleted via the App is contained in any scene controller and
            //  delete that assignment from the SCTL, if it is there
            DeleteSceneFromSceneController(sceneIndex);
            
            // fill in the rest of the properties
            *jsonBroadcastObjectPtr = json_object();
            if (*jsonBroadcastObjectPtr)
            {
               json_object_set_new(*jsonBroadcastObjectPtr, "ID", json_integer(0));
               json_object_set_new(*jsonBroadcastObjectPtr, "SID", json_integer(sceneIndex));
               json_object_set_new(*jsonBroadcastObjectPtr, "Service", json_string("SceneDeleted"));
               json_object_set_new(*jsonBroadcastObjectPtr, "Status", json_string("Success"));
               if (appContextId)
               {
                  json_object_set_new(*jsonBroadcastObjectPtr, "AppContextId", json_integer(appContextId));
               }
            }
         }
      }
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleSystemInfo(json_t * responseObject)
{
   json_t * UpdateStateObj;
   uint_32 percentageDone;
   char MACAddressString[18];
#if DEBUG_SOCKETTASK_ANY
   char debugBuf[100];
#endif

   json_object_set_new(responseObject, "Model", json_string(MODEL_STRING));

#ifdef FIRMWARE_VERSION_OVERRIDE
	json_object_set_new(responseObject, "FirmwareVersion", json_string(FIRMWARE_VERSION_OVERRIDE));
#else
	json_object_set_new(responseObject, "FirmwareVersion", json_string(SYSTEM_INFO_FIRMWARE_VERSION));
#endif

   json_object_set_new(responseObject, "FirmwareDate", json_string(SYSTEM_INFO_FIRMWARE_DATE));
   json_object_set_new(responseObject, "FirmwareBranch", json_string(SYSTEM_INFO_FIRMWARE_BRANCH));
   snprintf(MACAddressString, sizeof(MACAddressString), "%02X:%02X:%02X:%02X:%02X:%02X", myMACAddress[0], myMACAddress[1], myMACAddress[2], myMACAddress[3],
         myMACAddress[4], myMACAddress[5]);
   json_object_set_new(responseObject, "MACAddress", json_string(MACAddressString));
   json_object_set_new(responseObject, "HouseID", json_integer(global_houseID));

   UpdateStateObj = json_object();
   if (UpdateStateObj)
   {
      if (firmwareUpdateState >= (sizeof(UpdateStateStrings)/sizeof(UpdateStateStrings[0])))
      { // should not be able to happen, but make sure we bounds-check the UpdateStateStrings array
         firmwareUpdateState = UPDATE_STATE_NONE;
      }
      json_object_set_new(UpdateStateObj, "Status", json_string(UpdateStateStrings[firmwareUpdateState]));
   
      DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"ver=%s branch=%s date=%s", SYSTEM_INFO_FIRMWARE_VERSION, SYSTEM_INFO_FIRMWARE_BRANCH, SYSTEM_INFO_FIRMWARE_DATE);
      DBGPRINT_INTSTRSTR(__LINE__,"HandleSystemInfo firmware",DEBUGBUF); 
      DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"mac=%s house=%s", MACAddressString, global_houseID);
      DBGPRINT_INTSTRSTR(__LINE__,"HandleSystemInfo whoami",DEBUGBUF); 
      DBGPRINT_INTSTRSTR(__LINE__,"HandleSystemInfo: updateState",UpdateStateStrings[firmwareUpdateState]); 
      
      if (firmwareUpdateState != UPDATE_STATE_NONE)
      {
         snprintf(MACAddressString, sizeof(MACAddressString), "%d:%d:%d-%d", updateFirmwareInfo.firmwareVersion[0], updateFirmwareInfo.firmwareVersion[1],
               updateFirmwareInfo.firmwareVersion[2], updateFirmwareInfo.firmwareVersion[3]);
         json_object_set_new(UpdateStateObj, "UpdateVersion", json_string(MACAddressString));
   
         json_object_set_new(UpdateStateObj, "ReleaseNotes", json_string(updateFirmwareInfo.releaseNotes));
   
         percentageDone = firmwareUpdateLineNumber;
         percentageDone *= 100;
         percentageDone /= updateFirmwareInfo.S19LineCount;
   
         DBGPRINT_INTSTRSTR(percentageDone,"HandleSystemInfo updateVer percentageDone",MACAddressString); 
         DBGPRINT_INTSTRSTR(__LINE__,"HandleSystemInfo releaseNotes",updateFirmwareInfo.releaseNotes); 
   
         json_object_set_new(UpdateStateObj, "DownloadProgress", json_integer(percentageDone));

         // only announce the SHA256, Host and Path if we've finished the firmware download and calculated our hash
         if (updateFirmwareInfo.reportSha256Flag)
         {
            json_object_set_new(UpdateStateObj, "SHA256", json_string(updateFirmwareInfo.sha256HashString));
            json_object_set_new(UpdateStateObj, "Host", json_string(updateFirmwareInfo.updateHost));
            json_object_set_new(UpdateStateObj, "Path", json_string(updateFirmwareInfo.updatePath));
         }

         Eliot_SendFirmwareInfo(0);
      }
   
      json_object_set_new(responseObject, "UpdateState", UpdateStateObj);
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleReportSystemProperties(json_t * responseObject)
{
   BuildSystemPropertiesObject(responseObject, ALL_SYSTEM_PROPERTIES_BITMASK);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// add SupportsSceneFadeRate parameter - to indicate to App that LCM will apply ramp to scenes
//-----------------------------------------------------------------------------

void BuildSystemPropertiesObject(json_t * reportPropertiesObject, dword_t propertyBitmask)
{
   json_t *listObj;
   json_t *locationObj;
   json_t *latitudeObj;
   json_t *longitudeObj;

   locationObj = NULL;
   latitudeObj = NULL;
   longitudeObj = NULL;

   listObj = json_object();
   if(listObj)
   {
      if(propertyBitmask & SYSTEM_PROPERTY_ADD_A_SCENE_CONTROLLER_BITMASK)
      {
         if(systemProperties.addASceneController)
         {
            json_object_set_new(listObj, "AddASceneController", json_true());
         }
         else
         {
            json_object_set_new(listObj, "AddASceneController", json_false());
         }
      }
        
      if(propertyBitmask & SYSTEM_PROPERTY_ADD_A_LIGHT_BITMASK)
      {
         if(systemProperties.addALight)
         {
            json_object_set_new(listObj, "AddALight", json_true());
         }
         else
         {
            json_object_set_new(listObj, "AddALight", json_false());
         }
      }
      
      if(propertyBitmask & SYSTEM_PROPERTY_ADD_A_SHADE_BITMASK)
      {
          if(systemShadeSupportProperties.addAShade)
          {
             json_object_set_new(listObj, "AddAShade", json_true());
          }
          else
          {
             json_object_set_new(listObj, "AddAShade", json_false());
          }
      }
      
      if(propertyBitmask & SYSTEM_PROPERTY_TIME_ZONE_BITMASK)
      {
         json_object_set_new(listObj, "TimeZone", json_integer(systemProperties.gmtOffsetSeconds));
      }

      if(propertyBitmask & SYSTEM_PROPERTY_DAYLIGHT_SAVING_TIME_BITMASK)
      {
         if(systemProperties.useDaylightSaving)
         {
            json_object_set_new(listObj, "DaylightSavingTime", json_true());
         }
         else
         {
            json_object_set_new(listObj, "DaylightSavingTime", json_false());
         }
      }

      if(propertyBitmask & SYSTEM_PROPERTY_LOCATION_INFO_BITMASK)
      {
         json_object_set_new(listObj, "LocationInfo", json_string(systemProperties.locationInfoString));
      }

      if(propertyBitmask & SYSTEM_PROPERTY_LATITUDE_BITMASK)
      {
         // Create the latitude object
         latitudeObj = json_object();

         if(latitudeObj)
         {
            json_object_set_new(latitudeObj, "Deg", json_integer(systemProperties.latitude.degrees));
            json_object_set_new(latitudeObj, "Min", json_integer(systemProperties.latitude.minutes));
            json_object_set_new(latitudeObj, "Sec", json_integer(systemProperties.latitude.seconds));
         }
      }

      if(propertyBitmask & SYSTEM_PROPERTY_LONGITUDE_BITMASK)
      {
         // Create the longitude object
         longitudeObj = json_object();

         if(longitudeObj)
         {
            json_object_set_new(longitudeObj, "Deg", json_integer(systemProperties.longitude.degrees));
            json_object_set_new(longitudeObj, "Min", json_integer(systemProperties.longitude.minutes));
            json_object_set_new(longitudeObj, "Sec", json_integer(systemProperties.longitude.seconds));
         }
      }

      if(latitudeObj || longitudeObj)
      {
         // Create the location object because either a latitude or longitude
         // or both objects were created
         locationObj = json_object();
         if(locationObj)
         {
            if(latitudeObj)
            {
               // Add the latitude object to the location object
               json_object_set_new(locationObj, "Lat", latitudeObj);
            }

            if(longitudeObj)
            {
               // Add the longitude object to the location object
               json_object_set_new(locationObj, "Long", longitudeObj);
            }

            // Add the location object to the list object
            json_object_set_new(listObj, "Location", locationObj);
         }
         else
         {
            if (latitudeObj)
               json_decref(latitudeObj);
            if (longitudeObj)
               json_decref(longitudeObj);
         }
      }

      if(propertyBitmask & SYSTEM_PROPERTY_EFFECTIVE_TIME_ZONE_BITMASK)
      {
         json_object_set_new(listObj, "EffectiveTimeZone", json_integer(systemProperties.effectiveGmtOffsetSeconds));
      }

      if(propertyBitmask & SYSTEM_PROPERTY_CONFIGURED_BITMASK)
      {
         if(systemProperties.configured)
         {
            json_object_set_new(listObj, "Configured", json_true());
         }
         else
         {
            json_object_set_new(listObj, "Configured", json_false());
         }
      }
      
      if(propertyBitmask & SYSTEM_PROPERTY_NEW_TIME_BITMASK)
      {
         TIME_STRUCT mqxTime;

         _time_get(&mqxTime);

         // Need to subtract out the effective GMT offset so the 
         // time display looks correct in the app
         json_object_set_new(listObj, "CurrentTime", json_integer(mqxTime.SECONDS - systemProperties.effectiveGmtOffsetSeconds));
      }

      if (propertyBitmask & SYSTEM_PROPERTY_SUPPORT_FADE_RATE_BITMASK)
      {
         json_object_set_new(listObj, "SupportsSceneFadeRate", json_true());
      }

      if(propertyBitmask & SYSTEM_PROPERTY_MOBILE_APP_DATA_BITMASK)
      {
         json_object_set_new(listObj, "MobileAppData", json_string(extendedSystemProperties.mobileAppData));
      }

      json_object_set_new(reportPropertiesObject, "PropertyList", listObj);
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool_t ValidateLocationProperty(json_t * valueObject, json_t * responseObject, bool_t isLatitude)
{
   const char *locationKeyPtr;
   json_t *locationValueObject;
   void *locationObjectIter;

   // Initialize degrees, minutes, and seconds to invalid values so we know if they are set or not
   int16_t degrees = 181;
   uint8_t minutes = 61;
   uint8_t seconds = 61;
   bool_t returnFlag = FALSE;

   if(!json_is_object(valueObject)) 
   {
      BuildErrorResponse(responseObject, "PropertyList, Location must be an object.", SW_PROPERTY_LIST_LOCATION_MUST_BE_AN_OBJECT);
      returnFlag = TRUE;
   }
   else
   {
      locationObjectIter = json_object_iter(valueObject);

      while(locationObjectIter && (returnFlag == FALSE))
      {
         locationKeyPtr = json_object_iter_key(locationObjectIter);
         locationValueObject = json_object_iter_value(locationObjectIter);

         if(!strcmp(locationKeyPtr, "Deg"))
         {
            if(!json_is_integer(locationValueObject))
            {
               BuildErrorResponse(responseObject, "PropertyList, Deg must be an integer", SW_PROPERTY_LIST_DEG_MUST_BE_AN_INTEGER);
               returnFlag = TRUE;
            }
            else
            {
               // Check the range for degrees
               degrees = json_integer_value(locationValueObject);

               if(isLatitude &&
                  ((degrees < -90) || 
                   (degrees > 90)))
               {
                  BuildErrorResponse(responseObject, "PropertyList, Deg is out of range", SW_PROPERTY_LIST_DEG_IS_OUT_RANGE);
                  returnFlag = TRUE;
               }
               else if(!isLatitude &&
                       ((degrees < -180) ||
                        (degrees > 180)))
               {
                  BuildErrorResponse(responseObject, "PropertyList, Deg is out of range", SW_PROPERTY_LIST_DEG_IS_OUT_RANGE);
                  returnFlag = TRUE;
               }
            }
         }
         else if(!strcmp(locationKeyPtr, "Min"))
         {
            if(!json_is_integer(locationValueObject))
            {
               BuildErrorResponse(responseObject, "PropertyList, Min must be an integer", SW_PROPERTY_LIST_MIN_MUST_BE_AN_INTEGER);
               returnFlag = TRUE;
            }
            else
            {
               // Check the range for degrees
               minutes = (uint8_t)(json_integer_value(locationValueObject));

               if((minutes < 0) || 
                  (minutes > 59))
               {
                  BuildErrorResponse(responseObject, "PropertyList, Min is out of range", SW_PROPERTY_LIST_MIN_IS_OUT_OF_RANGE);
                  returnFlag = TRUE;
               }
            }
         }
         else if(!strcmp(locationKeyPtr, "Sec"))
         {
            if(!json_is_integer(locationValueObject))
            {
               BuildErrorResponse(responseObject, "PropertyList, Sec must be an integer", SW_PROPERTY_LIST_SEC_MUST_BE_AN_INTEGER);
               returnFlag = TRUE;
            }
            else
            {
               // Check the range for degrees
               seconds = (uint8_t)(json_integer_value(locationValueObject));

               if((seconds < 0) || 
                  (seconds > 59))
               {
                  BuildErrorResponse(responseObject, "PropertyList, Sec is out of range", SW_PROPERTY_LIST_SEC_IS_OUT_OF_RANGE);
                  returnFlag = TRUE;
               }
            }
         }
         else
         {
            // Invalid property found. Return an error
            BuildErrorResponse(responseObject, "PropertyList, Unknown Property not handled", SW_PROPERTY_LIST_UNKNOWN_PROPERTY_NOT_HANDLED); 
            returnFlag = TRUE;
         }

         locationObjectIter = json_object_iter_next(valueObject, locationObjectIter);
      }
   }

   // Verify that degrees, minutes, and seconds were all set properly
   if(!returnFlag &&
      ((degrees == 181) ||
       (minutes == 61) ||
       (seconds == 61)))
   {
      DBGPRINT_INTSTRSTR(__LINE__,"ValidateLocationProperty: PropertyList, Not all location objects were set",""); 
      BuildErrorResponse(responseObject, "PropertyList, Not all location objects were set", SW_PROPERTY_LIST_NOT_ALL_LOCATION_OBJECTS_WERE_SET);
      returnFlag = TRUE;
   }

   return returnFlag;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifdef SHADE_CONTROL_ADDED

int32_t receiveHTTPBytesFromQmotion(char * buf, uint32_t bufferSize, bool_t headerOnly)
{
   char     receiveBuf[100];
   char *   pReceiveBuf = NULL;
   char *   htmlContent = NULL;
   uint32_t rtcsError = 0;
   int32_t  bytesReceived = 0;
   uint32_t bytesToCopy = 0;
   uint32_t writeIndex = 0;
   uint32_t numBytesToRead = 0;
   bool_t   headerFound = false;
   uint32_t receiveBufDataSize = 0;
   uint32_t socketType = 0;
   uint32_t valueLength = sizeof(socketType);
   
   // Determine if the socket is UDP (SOCK_DGRAM) or TCP (SOCK_STREAM).
   // If the socket is UDP, read the entire buffer at once, because
   // leftover UDP data is discarded.  If the socket is TCP, read the buffer in chunks.
   getsockopt(QSock, SOL_SOCKET, OPT_SOCKET_TYPE, &socketType, &valueLength);
   
   DBGPRINT_INTSTRSTR_QMOTION_DETAIL(socketType, "receiveHTTPBytesFromQmotion socketType is", "");
   
   if (socketType == SOCK_DGRAM)
   {
       DBGPRINT_INTSTRSTR_QMOTION_DETAIL(socketType, "receiveHTTPBytesFromQmotion socketType is SOCK_DGRAM", "");
       pReceiveBuf = buf;
       receiveBufDataSize = bufferSize;
   }
   else if (socketType == SOCK_STREAM)
   {
       DBGPRINT_INTSTRSTR_QMOTION_DETAIL(socketType, "receiveHTTPBytesFromQmotion socketType is SOCK_STREAM", "");
       pReceiveBuf = receiveBuf;
       receiveBufDataSize = sizeof(receiveBuf);
   }
   else
   {
       DBGPRINT_INTSTRSTR_ERROR(socketType, "receiveHTTPBytesFromQmotion socketType fail",""); 
       return 0;
   }
      
   do
   {
       _time_delay(QMOTION_PRE_RECV_PAYLOAD_DELAY_MS);
       
      // Determine how many bytes we are able to read
      if((bufferSize - writeIndex) > receiveBufDataSize)
      {
         numBytesToRead = receiveBufDataSize;
      }
      else
      {
         numBytesToRead = bufferSize - writeIndex;
      }

      memset(pReceiveBuf, 0, receiveBufDataSize);
      bytesReceived = Socket_recv(QSock, pReceiveBuf, numBytesToRead, 0, &qmotion_recv_error_count, __PRETTY_FUNCTION__);
          
      DBGPRINT_INTSTRSTR_QMOTION_DETAIL(bytesReceived, "receiveHTTPBytesFromQmotion receive", (char *)pReceiveBuf);
      
      if(bytesReceived < 0)
      {
         // Error receiving bytes. Set the write index and break
         rtcsError = RTCS_geterror(QSock);
         if (rtcsError != RTCSERR_TCP_CONN_CLOSING)
         {
            writeIndex = -1;
            DBGPRINT_INTSTRSTR_ERROR(__LINE__,"receiveHTTPBytesFromQmotion recv failed with -1",""); 
         }
         break;
      }
      else
      {
         if(!headerFound)
         {
            // DMC: TODO: If the "\r\n\r\n" happens to straddle two Socket_recv calls, it will be missed.
            htmlContent = strstr((char *)pReceiveBuf, "\r\n\r\n");
            if(htmlContent != NULL)
            {
               headerFound = true;
               htmlContent += 4;
            }
         }
         else
         {
            htmlContent = pReceiveBuf;
         }

         if(headerFound)
         {
            // Determine the number of bytes to copy into the return buffer
            bytesToCopy = bytesReceived - (htmlContent - pReceiveBuf);
            memcpy(&buf[writeIndex], htmlContent, bytesToCopy);
            writeIndex += bytesToCopy;

            if(headerOnly)
            {
               break;
            }
         }
      }
   } while((socketType != SOCK_DGRAM) && (bytesReceived > 0) && (writeIndex < bufferSize));
   
   // Fill the rest of buf with 0
   memset((void *)(buf + writeIndex), 0, bufferSize - writeIndex);

   return writeIndex;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

json_t * GetQmotionJsonPayload(void)
{
   json_t *       returnObject = NULL;
   int32_t        byteCount;
   json_error_t   jsonError;
   uint8_t        timeoutCount = 0;

   _time_delay(QMOTION_PRE_RECV_PAYLOAD_DELAY_MS);
   
   memset((void *) qmotionBuffer, 0, sizeof(qmotionBuffer));
   
   while (timeoutCount < QMOTION_RECV_JSON_PAYLOAD_MAX_RETRIES) 
   {
      byteCount = receiveHTTPBytesFromQmotion((char *) qmotionBuffer, MAX_QMOTION_BUFFER, false);

      // determine if there is more data to be recvd before trying to 
      //      get the json out of the buffer
      
      if ((byteCount > 0) && (byteCount <= sizeof(qmotionBuffer)))
      {  // got a response get the json packet out of it
         
         DBGPRINT_INTSTRSTR_QMOTION_DETAIL(byteCount,"GetQmotionJsonPayload receive OK", (char *) qmotionBuffer);
         returnObject = json_loads((const char *) qmotionBuffer, 0, &jsonError);
         break;
      }
      else // else, we got no packets from qmotion -- try again
      {
         DBGPRINT_INTSTRSTR_QMOTION_DETAIL(byteCount,"GetQmotionJsonPayload receive timeout byteCount", (char *) qmotionBuffer);
         _time_delay(QMOTION_RECV_JSON_PAYLOAD_DELAY_MS);
         timeoutCount++;
      }
   }

   if (timeoutCount >= QMOTION_RECV_JSON_PAYLOAD_MAX_RETRIES)
   {
      qmotion_missed_packet_count++;
      DBGPRINT_INTSTRSTR_QMOTION_ERROR(timeoutCount,"GetQmotionJsonPayload receive timeout FAIL", (char *) qmotionBuffer);
   }

   return returnObject;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

uint8_t checkForQmotionIdStringMatch(const char * idString)
//-----------------------------------------------------------------------------
// returns MAX_NUMBER_OF_ZONES if no match is found, 
//   else it returns the zoneIndex that is a match
//-----------------------------------------------------------------------------
{
   uint8_t zoneIndex;
   uint8_t returnZoneIndex = MAX_NUMBER_OF_ZONES;
   
   zoneIndex = 0;
   while (zoneIndex < MAX_NUMBER_OF_ZONES)
   {
      if (strcmp(idString, shadeInfoStruct.shadeInfo[zoneIndex].idString) == 0) // will return 0 is equal
      {
          returnZoneIndex = zoneIndex;
          zoneIndex = MAX_NUMBER_OF_ZONES; // force end of loop
      }
      else
      {
         zoneIndex++;
      }
   }
   
   return returnZoneIndex;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void AddAShade(uint8_t  zoneIndex, const char * shadeName, const char * identifier, bool isGroup)
{

   zone_properties   zoneProperties;
   json_t *          broadcastObject;

   if (!GetZoneProperties(zoneIndex, &zoneProperties))
   {
      // fill in zoneArray slot with defaults
      snprintf(zoneProperties.ZoneNameString, sizeof(zoneProperties.ZoneNameString), "%s", shadeName);
      zoneProperties.ZoneNameString[sizeof(zoneProperties.ZoneNameString) - 1] = 0; // force null terminator

      memset(shadeInfoStruct.shadeInfo[zoneIndex].idString, 0, sizeof(shadeInfoStruct.shadeInfo[zoneIndex].idString));
      snprintf(shadeInfoStruct.shadeInfo[zoneIndex].idString, (sizeof(shadeInfoStruct.shadeInfo[zoneIndex].idString) - 1), "%s", identifier);

      // fill in zoneArray slot with rf packet info
      if (isGroup)
         zoneProperties.deviceType = SHADE_GROUP_TYPE;
      else
         zoneProperties.deviceType = SHADE_TYPE;
     
      zoneProperties.buildingId = 0;
      zoneProperties.groupId = 0x1000 + zoneIndex;  // give the shade a unique group id, so it can be found 
      zoneProperties.houseId = 0;
      zoneProperties.powerLevel = 100;
      zoneProperties.targetPowerLevel = 100;
      zoneProperties.state = TRUE;
      zoneProperties.targetState = TRUE;

      // json broadcast ZoneAdded
      BroadcastZoneAdded(zoneIndex);

      // Set the zone properties in the array
      SetZoneProperties(zoneIndex, &zoneProperties);
      // zoneIndex = MAX_NUMBER_OF_ZONES; // exit the loop
      
      // Add the zone to the Samsung cloud
      Eliot_QueueAddZone(zoneIndex);

   }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleDevicesInfoResponse(json_t * listArrayElementObject)
{
   json_t * identifierObject;
   json_t * deviceTypeObject;
   json_t * nameObject;
   uint8_t  zoneIndex;
#if DEBUG_SOCKETTASK_ANY
   char debugBuf[200];
#endif

   _time_delay(QMOTION_HANDLE_DEVICES_INFO_RESPONSE_DELAY_MS); // adding for debug
   
   identifierObject = json_object_get(listArrayElementObject, "identifier");
   deviceTypeObject = json_object_get(listArrayElementObject, "deviceType");
   nameObject       = json_object_get(listArrayElementObject, "name");
  
   DBGSNPRINTF_QMOTION(DEBUGBUF,SIZEOF_DEBUGBUF,"type=\"%s\" name=\"%s\" id=\"%s\"", json_string_value(deviceTypeObject), json_string_value(nameObject), json_string_value(identifierObject));
   DBGPRINT_INTSTRSTR_QMOTION(__LINE__,"HandleDevicesInfoResponse",DEBUGBUF);
   
   if (deviceTypeObject)
   {
      if (strcmp(json_string_value(deviceTypeObject), "shade") == 0) // if strings are equal and device is a shade
      {
         if (identifierObject)
         {
            zoneIndex = checkForQmotionIdStringMatch(json_string_value(identifierObject));
            if (zoneIndex >= MAX_NUMBER_OF_ZONES)
            {  
               zoneIndex = GetAvailableZoneIndex();
               if (zoneIndex < MAX_NUMBER_OF_ZONES)
               {
                  AddAShade(zoneIndex, json_string_value(nameObject), json_string_value(identifierObject), false);
               }
               else
               {
                  DBGPRINT_INTSTRSTR_ERROR(zoneIndex,"HandleDevicesInfoResponse FAIL@1 - bad zoneIndex",json_string_value(nameObject));                  
               }
            }
            else
            {
               DBGPRINT_INTSTRSTR_QMOTION_DETAIL(zoneIndex,"HandleDevicesInfoResponse already found at zoneIndex",json_string_value(nameObject));                  
            }
         }
         else
         {
            DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleDevicesInfoResponse FAIL@3 - not identified obj",json_string_value(nameObject));                              
         }
      } // if this device is a shade
      else
      {
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleDevicesInfoResponse FAIL@4 - not shade",json_string_value(nameObject));                                       
      }
   } // if (deviceTypeObject)
   else
   {
      DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleDevicesInfoResponse FAIL@5 - not device",json_string_value(nameObject));                                             
   }
   
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleQmotionGroupsResponse(json_t * listArrayElementObject)
{
   json_t * identifierObject;
   json_t * deviceTypeObject;
   json_t * nameObject;
   uint8_t  zoneIndex;
   uint32_t groupIdNumber;
   char     groupIdString[MAX_SIZE_QMOTION_DEVICE_ID_STRING] = "";
#if DEBUG_SOCKETTASK
   char debugBuf[200];
#endif

   _time_delay(QMOTION_HANDLE_DEVICES_INFO_RESPONSE_DELAY_MS); // adding for debug
   
   identifierObject = json_object_get(listArrayElementObject, "groupId");
   nameObject       = json_object_get(listArrayElementObject, "name");
   
   if (json_is_integer(identifierObject))         
   {
      groupIdNumber = json_integer_value(identifierObject);
      sprintf(groupIdString,"%d", groupIdNumber);
      DBGPRINT_INTSTRSTR_QMOTION(groupIdNumber,"HandleQmotionGroupsResponse groupIdString",groupIdString);
   }
         
   DBGSNPRINTF_QMOTION(DEBUGBUF,SIZEOF_DEBUGBUF,"name=\"%s\" id=\"%s\"", json_string_value(nameObject), groupIdString);
   DBGPRINT_INTSTRSTR_QMOTION(__LINE__,"HandleQmotionGroupsResponse",DEBUGBUF);
   
   if (identifierObject)
   {
      zoneIndex = checkForQmotionIdStringMatch(groupIdString);
      if (zoneIndex >= MAX_NUMBER_OF_ZONES)
      {  
         zoneIndex = GetAvailableZoneIndex();
         if (zoneIndex < MAX_NUMBER_OF_ZONES)
         {
            AddAShade(zoneIndex, json_string_value(nameObject), groupIdString, true);
         }
         else
         {
            DBGPRINT_INTSTRSTR(zoneIndex,"HandleQmotionGroupsResponse FAIL@1 - bad zoneIndex",json_string_value(nameObject));                  
         }
      }
      else
      {
         DBGPRINT_INTSTRSTR_QMOTION_DETAIL(zoneIndex,"HandleQmotionGroupsResponse already found at zoneIndex",json_string_value(nameObject));                  
      }
   }
   else
   {
      DBGPRINT_INTSTRSTR(__LINE__,"HandleQmotionGroupsResponse FAIL@3 - not identifier obj",json_string_value(nameObject));                              
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void GetQmotionDiscoveryPayload(void)
//-----------------------------------------------------------------------------
//  If IP address is found, fills in global qubeAddress variable
//-----------------------------------------------------------------------------
{
   int_32        result;
   uint8_t *      ipStringPtr;
   int            ipAddressOctet[4];
   byte_t         timeoutCount = 0;

   _time_delay(QMOTION_PRE_RECV_DISCOVERY_PAYLOAD_DELAY_MS);
   while (timeoutCount < QMOTION_RECV_DISCOVERY_MAX_RETRIES)
   {
      result = Socket_recv(QSock, (void *) qmotionBuffer, sizeof(qmotionBuffer), 0, &qmotion_recv_error_count, __PRETTY_FUNCTION__);

      if ((result > 0) && (result <= sizeof(qmotionBuffer)))
      {  // got a response - see if we can get an address from it
         ipStringPtr = (uint8_t *) strstr((char *) qmotionBuffer, "\"ip\":");
         if (ipStringPtr)
         {
            sscanf((char *) (ipStringPtr + 6), "%d.%d.%d.%d", &ipAddressOctet[0], &ipAddressOctet[1], &ipAddressOctet[2], &ipAddressOctet[3]);
            HighByteOfDword(qubeAddress) = ipAddressOctet[0];
            UpperMiddleByteOfDword(qubeAddress) = ipAddressOctet[1];
            LowerMiddleByteOfDword(qubeAddress) = ipAddressOctet[2];
            LowByteOfDword(qubeAddress) = ipAddressOctet[3];

            qmotion_found_count++;

            return;
         }
      }
      else
      {
         _time_delay(QMOTION_RECV_DISCOVERY_RETRY_DELAY_MS);
         timeoutCount++;
      }
   }

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void TryToFindQmotion(void)
//-----------------------------------------------------------------------------
//  Broadcast a gateway command, to find the IP address of any Qube's
//-----------------------------------------------------------------------------
{
   _ip_address    broadcastAddress = IPADDR(255,255,255,255); // Use local network broadcast address
   sockaddr_in    remote_sin;
   int_32        result;
   uint_32        opt_value;

   // Create a UDP (SOCK_DGRAM) socket to communicate with the Qube
   QSock = socket(AF_INET, SOCK_DGRAM, 0);
   if (QSock == RTCS_SOCKET_ERROR)
   {
        return;
   }

   opt_value = true;
   result = setsockopt(QSock, SOL_SOCKET, OPT_RECEIVE_NOWAIT, &opt_value, sizeof(opt_value));

   memset((char *) &remote_sin, 0, sizeof(sockaddr_in));
   remote_sin.sin_family = AF_INET;
   remote_sin.sin_port = QMOTION_UDP_PORT;
   remote_sin.sin_addr.s_addr = broadcastAddress;

   snprintf((char *) qmotionBuffer, sizeof(qmotionBuffer), "GET /gateway HTTP/1.0\r\n\r\n");
   result = sendto(QSock, (void *) qmotionBuffer, strlen((const char *) qmotionBuffer), 0, &remote_sin, sizeof(sockaddr_in));
   _time_delay(QMOTION_POST_REQUEST_DISCOVERY_DELAY_MS);
   
   qmotion_try_find_count++;

   if (result > 0)
   {
      GetQmotionDiscoveryPayload();
   }

   clearQubeAddressTimer = REFRESH_QUBE_ADDRESS_TIME;
   shutdown(QSock, FLAG_ABORT_CONNECTION);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

uint8_t GetQmotionResponse()
//-----------------------------------------------------------------------------
//  returns true if we find OK in the response string
//
//   A bad command to the Qube will return "Bad Command"
//-----------------------------------------------------------------------------
{
   int_32     result;
   word_t      timeoutCount = 0;
   uint8_t *   okStringPtr;
  
   memset((void *) qmotionBuffer, 0, sizeof(qmotionBuffer));
   _time_delay(QMOTION_PRE_RECV_PAYLOAD_DELAY_MS);
   
   while (timeoutCount < QMOTION_RECV_RESPONSE_MAX_RETRIES) 
   {
      result = Socket_recv(QSock, (void *) qmotionBuffer, sizeof(qmotionBuffer), 0, &qmotion_recv_error_count, __PRETTY_FUNCTION__);

      if ((result > 0) && (result <= sizeof(qmotionBuffer)))
      {  // got a response - see if we can get an address from it
         DBGPRINT_INTSTRSTR_QMOTION_DETAIL(result,"GetQmotionResponse recv result", (char *) qmotionBuffer);
         okStringPtr = (uint8_t *) strstr((char *) qmotionBuffer, "OK");
         if (okStringPtr)
         {
            // TODO publish ACK
           PublishAckToQueue(RFM100_ACK_BYTE);
           return TRUE;
         }
         else
         {
            PublishAckToQueue(RFM100_NAK_BYTE);
            return FALSE;  // Qube returned a failure
         }
      }
      else
      {
         _time_delay(QMOTION_RECV_RESPONSE_RETRY_DELAY_MS);
         timeoutCount++;
      }
   }

   if (timeoutCount >= QMOTION_RECV_RESPONSE_MAX_RETRIES)
   {
      qmotion_missed_packet_count++;
      DBGPRINT_INTSTRSTR_ERROR(timeoutCount,"GetQmotionResponse recv fail timeouts", (char *) qmotionBuffer);
   }

  return FALSE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void SendShadeCommand(uint8_t zoneIndex, uint8_t percentOpen)
{
   sockaddr_in    remote_sin;
   int_32        result;
   uint_32        opt_value;
   zone_properties zoneProperties;
   
   if (!GetZoneProperties(zoneIndex, &zoneProperties))
   {
       return;
   }
   
   if (zoneProperties.deviceType != SHADE_GROUP_TYPE && zoneProperties.deviceType != SHADE_TYPE)
   {
       return;
   }
  
   memset((void *) qmotionBuffer, 0, sizeof(qmotionBuffer));
  
   // Create a UDP (SOCK_DGRAM) socket to communicate with the Qube
   QSock = socket(AF_INET, SOCK_DGRAM, 0);
   if (QSock == RTCS_SOCKET_ERROR)
   {
       return;
   }
   
   // Specify that the socket should not block
   opt_value = true;
   result = setsockopt(QSock, SOL_UDP, OPT_RECEIVE_NOWAIT, &opt_value, sizeof(opt_value));
   
   memset((char *) &remote_sin, 0, sizeof(sockaddr_in));
   remote_sin.sin_family = AF_INET;
   remote_sin.sin_port = QMOTION_UDP_PORT;
   remote_sin.sin_addr.s_addr = qubeAddress;

   if (zoneProperties.deviceType == SHADE_GROUP_TYPE)
   {
      snprintf((char *) qmotionBuffer,
                sizeof(qmotionBuffer),
                 "PUT /groups/id/%s HTTP/1.0\r\nHOST: %d.%d.%d.%d\r\nContent-Type: application/json\r\n\r\n{\"level\":%d}",
                 shadeInfoStruct.shadeInfo[zoneIndex].idString,
                   HighByteOfDword(qubeAddress),
                    UpperMiddleByteOfDword(qubeAddress),
                     LowerMiddleByteOfDword(qubeAddress),
                      LowByteOfDword(qubeAddress),
                       percentOpen);
   }
   else
   {
      snprintf((char *) qmotionBuffer,
                sizeof(qmotionBuffer),
                 "PUT /devices/%s HTTP/1.0\r\nHOST: %d.%d.%d.%d\r\nContent-Type: application/json\r\n\r\n{\"level\":%d}",
                 shadeInfoStruct.shadeInfo[zoneIndex].idString,
                   HighByteOfDword(qubeAddress),
                    UpperMiddleByteOfDword(qubeAddress),
                     LowerMiddleByteOfDword(qubeAddress),
                      LowByteOfDword(qubeAddress),
                       percentOpen);
   }

   result = Socket_sendto(QSock,
                  (void *)qmotionBuffer,
                   strlen((const char *)qmotionBuffer),
                       0, &remote_sin, sizeof(sockaddr_in),
                       &qmotion_send_error_count, __PRETTY_FUNCTION__);

   DBGPRINT_INTSTRSTR_QMOTION_DETAIL(result,"SendShadeCommand send result", (char *) qmotionBuffer);

   if (result > 0)
   {
      GetQmotionResponse();
   }
   
   shutdown(QSock, FLAG_ABORT_CONNECTION);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define TRY_ABORTING_QSOCK_BEFORE_NEXT_SHADE_REQUEST 1

void GetQmotionDevicesInfo(void)
{
   sockaddr_in    remote_sin;
   json_t *       packetObject = NULL;
   json_t *       deviceInfoObject = NULL;
   json_t *       totalDevicesObject = NULL;
   json_t *       groupsObject = NULL;
   json_t *       totalGroupsObject = NULL;
   json_t *       listObject = NULL;
   json_t *       listArrayElementObject = NULL;
   int_32        result;
   uint_32        opt_value;
   volatile uint16_t       totalDevices; // volatile for DEBUG
   volatile uint16_t       deviceIndex; // volatile for DEBUG
   volatile uint16_t       totalGroups; // volatile for DEBUG
   volatile uint16_t       groupIndex; // volatile for DEBUG
   volatile int16_t        listArrayIndex; // volatile for DEBUG
   volatile uint8_t        retryGetCount; // volatile for DEBUG
   bool_t         doneFlag;
   bool_t         saveShadeIdFlag = false;
   
   // Create a UDP (SOCK_DGRAM) socket to communicate with the Qube
   QSock = socket(AF_INET, SOCK_DGRAM, 0);
   if (QSock == RTCS_SOCKET_ERROR)
   {
       return;
   }

   // Specify that the socket should not block
   opt_value = true;
   result = setsockopt(QSock, SOL_UDP, OPT_RECEIVE_NOWAIT, &opt_value, sizeof(opt_value));
   
   DBGPRINT_INTSTRSTR_QMOTION_DETAIL(result,"GetQmotionDevicesInfo setsockopt result","");
   
   memset((char *) &remote_sin, 0, sizeof(sockaddr_in));
   remote_sin.sin_family = AF_INET;
   remote_sin.sin_port = QMOTION_UDP_PORT;
   remote_sin.sin_addr.s_addr = qubeAddress;
   
   // Get individual shades
   totalDevices = 0;
   deviceIndex = 0;
     
   doneFlag = false;
   retryGetCount = 6; // bump maybe? was 3
   while ((!doneFlag) && (retryGetCount))
   {
      if (deviceIndex > 0)
      {
         DBGPRINT_INTSTRSTR_QMOTION_DETAIL(deviceIndex,"GetQmotionDevicesInfo requesting from deviceIndex","");
      }
      snprintf((char *) qmotionBuffer,
                sizeof(qmotionBuffer), 
                 "GET /devices/info?startIndex=%d HTTP/1.0\r\nHOST: %d.%d.%d.%d\r\n\r\n",
                  deviceIndex,
                   HighByteOfDword(qubeAddress),
                    UpperMiddleByteOfDword(qubeAddress),
                     LowerMiddleByteOfDword(qubeAddress),
                      LowByteOfDword(qubeAddress));
   
      result = Socket_sendto(QSock,
                     (void *)qmotionBuffer,
                      strlen((const char *)qmotionBuffer),
                       0, &remote_sin, sizeof(sockaddr_in),
                       &qmotion_send_error_count, __PRETTY_FUNCTION__);
      DBGPRINT_INTSTRSTR_QMOTION_DETAIL(result,"GetQmotionDevicesInfo send qmotionBuffer result", (char *) qmotionBuffer);

      qmotion_try_add_count++;

      retryGetCount--;
      
      if (result > 0)
      {
          packetObject = GetQmotionJsonPayload(); // this fails sometimes during GetQmotionDevicesInfo
      }
   
      if (packetObject)
      {
         DBGPRINT_INTSTRSTR_QMOTION_DETAIL(__LINE__,"GetQmotionDevicesInfo packetObject received","");
         deviceInfoObject = json_object_get(packetObject, "deviceInfo");
         if (deviceInfoObject)
         {
            DBGPRINT_INTSTRSTR_QMOTION_DETAIL(__LINE__,"GetQmotionDevicesInfo deviceInfoObject received","");
            totalDevicesObject = json_object_get(deviceInfoObject, "totalDevices");
            listObject = json_object_get(deviceInfoObject, "list");
            if ((json_is_array(listObject)) && (json_is_integer(totalDevicesObject)))         
            {
               totalDevices = json_integer_value(totalDevicesObject);
               DBGPRINT_INTSTRSTR_QMOTION_DETAIL(totalDevices,"GetQmotionDevicesInfo totalDevices","");
               DBGPRINT_INTSTRSTR_QMOTION_DETAIL((int) json_array_size(listObject), "GetQmotionDevicesInfo list size", "");
               DBGPRINT_INTSTRSTR_QMOTION_DETAIL(listArrayIndex,"GetQmotionDevicesInfo listArrayIndex", "");
               
               listArrayIndex = 0;
               listArrayElementObject = json_array_get(listObject, listArrayIndex);
               while (listArrayElementObject)
               {
                  deviceIndex++;
                  retryGetCount = 3;  // as soon as we process a device, reset our retry count since we (might) need to send a new GET
                  listArrayIndex++;
                  HandleDevicesInfoResponse(listArrayElementObject);
                  saveShadeIdFlag = true;
                  
                  DBGPRINT_INTSTRSTR_QMOTION_DETAIL(listArrayIndex,"GetQmotionDevicesInfo listArrayIndex", "");
                  listArrayElementObject = json_array_get(listObject, listArrayIndex);
				  
				      qmotion_add_shades_count++;
                  DBGPRINT_INTSTRSTR_QMOTION_DETAIL(qmotion_add_shades_count,"GetQmotionDevicesInfo qmotion_add_shades_count","");
               }
			   
			   qmotion_add_success_count++;
            }  // if ((json_is_array(listObject)) && (json_is_integer(totalDevicesObject)))
            else
            {
               DBGPRINT_INTSTRSTR_ERROR(__LINE__,"GetQmotionDevicesInfo not array or not integer","");         
            }         
         }  // if (deviceInfoObject)
         else
         {
            DBGPRINT_INTSTRSTR_ERROR(__LINE__,"GetQmotionDevicesInfo no deviceInfoObject","");         
         }
         
         json_decref(packetObject);
         packetObject = NULL;
      }  // if (packetObject)
      else
      {
         DBGPRINT_INTSTRSTR_ERROR(totalDevices,"GetQmotionDevicesInfo no packetObject from GetQmotionJsonPayload","");         
      }

      if (deviceIndex >= totalDevices)
      {
         DBGPRINT_INTSTRSTR_QMOTION_DETAIL(totalDevices,"GetQmotionDevicesInfo totalDevices OK","");
         doneFlag = true;
      }
   }

   if (deviceIndex < totalDevices)
   {
      DBGPRINT_INTSTRSTR_ERROR(totalDevices,"GetQmotionDevicesInfo totalDevices FAIL","");
   }
   
   //------------------------------------------------------------------------------
   // do the same thing for shade groups
   //------------------------------------------------------------------------------
   
   // Get shade groups
   totalGroups = 0;
   groupIndex = 0;
     
   doneFlag = false;
   retryGetCount = 6; // bump maybe? was 3
   while ((!doneFlag) && (retryGetCount))
   {
      // if (groupIndex > 0)
      {
         DBGPRINT_INTSTRSTR_QMOTION_DETAIL(groupIndex,"GetQmotionDevicesInfo requesting from groupIndex","");
      }
      snprintf((char *) qmotionBuffer,
                sizeof(qmotionBuffer), 
                 "GET /groups?startIndex=%d HTTP/1.0\r\nHOST: %d.%d.%d.%d\r\n\r\n",
                  groupIndex,
                   HighByteOfDword(qubeAddress),
                    UpperMiddleByteOfDword(qubeAddress),
                     LowerMiddleByteOfDword(qubeAddress),
                      LowByteOfDword(qubeAddress));
   
      result = Socket_sendto(QSock,
                     (void *)qmotionBuffer,
                      strlen((const char *)qmotionBuffer),
                       0, &remote_sin, sizeof(sockaddr_in),
                       &qmotion_send_error_count, __PRETTY_FUNCTION__);
      DBGPRINT_INTSTRSTR_QMOTION_DETAIL(result,"GetQmotionDevicesInfo send qmotionBuffer result", (char *) qmotionBuffer);

      qmotion_try_add_count++;

      retryGetCount--;
      
      if (result > 0)
      {
         packetObject = GetQmotionJsonPayload(); // this fails sometimes during GetQmotionDevicesInfo
      }
   
      if (packetObject)
      {
         DBGPRINT_INTSTRSTR_QMOTION_DETAIL(__LINE__,"GetQmotionDevicesInfo packetObject received","");
         groupsObject = json_object_get(packetObject, "groups");
         if (groupsObject)
         {
            DBGPRINT_INTSTRSTR_QMOTION_DETAIL(__LINE__,"GetQmotionDevicesInfo groupsObject received","");
            totalGroupsObject = json_object_get(groupsObject, "totalGroups");
            listObject = json_object_get(groupsObject, "list");
            if ((json_is_array(listObject)) && (json_is_integer(totalGroupsObject)))         
            {
               totalGroups = json_integer_value(totalGroupsObject);
               DBGPRINT_INTSTRSTR_QMOTION_DETAIL(totalGroups,"GetQmotionDevicesInfo totalGroups","");
               DBGPRINT_INTSTRSTR_QMOTION_DETAIL((int) json_array_size(listObject), "GetQmotionDevicesInfo list size", "");
               DBGPRINT_INTSTRSTR_QMOTION_DETAIL(listArrayIndex,"GetQmotionDevicesInfo listArrayIndex", "");
               
               listArrayIndex = 0;
               listArrayElementObject = json_array_get(listObject, listArrayIndex);
               while (listArrayElementObject)
               {
                  groupIndex++;
                  retryGetCount = 3;  // as soon as we process a device, reset our retry count since we (might) need to send a new GET
                  listArrayIndex++;
                  HandleQmotionGroupsResponse(listArrayElementObject);
                  saveShadeIdFlag = true;
                  
                  DBGPRINT_INTSTRSTR_QMOTION_DETAIL(listArrayIndex,"GetQmotionDevicesInfo listArrayIndex", "");
                  listArrayElementObject = json_array_get(listObject, listArrayIndex);
				  
				      qmotion_add_shades_count++;
                  DBGPRINT_INTSTRSTR_QMOTION_DETAIL(qmotion_add_shades_count,"GetQmotionDevicesInfo qmotion_add_shades_count","");
               }
			   
			   qmotion_add_success_count++;
            }  // if ((json_is_array(listObject)) && (json_is_integer(totalDevicesObject)))
            else
            {
               DBGPRINT_INTSTRSTR(__LINE__,"GetQmotionDevicesInfo not array or not integer","");         
            }
         
         }  // if (groupsObject)
         else
         {
            DBGPRINT_INTSTRSTR(__LINE__,"GetQmotionDevicesInfo no groupsObject","");         
         }
         
         json_decref(packetObject);
         packetObject = NULL;
         
      }  // if (packetObject)
      else
      {
         DBGPRINT_INTSTRSTR(totalGroups,"GetQmotionDevicesInfo no packetObject from GetQmotionJsonPayload","");         
      }

      if (groupIndex >= totalGroups)
      {
         DBGPRINT_INTSTRSTR_QMOTION_DETAIL(totalGroups,"GetQmotionDevicesInfo totalGroups OK","");
         doneFlag = true;
      }
   }

   if (groupIndex < totalGroups)
   {
      DBGPRINT_INTSTRSTR(totalGroups,"GetQmotionDevicesInfo totalGroups FAIL","");
   }
   

   if (saveShadeIdFlag)
   {
      DBGPRINT_INTSTRSTR_QMOTION_DETAIL(__LINE__,"GetQmotionDevicesInfo WriteAllShadeIdsToFlash","");
      WriteAllShadeIdsToFlash();
   }

   shutdown(QSock, FLAG_ABORT_CONNECTION);
   
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleQmotionStatusResponse(json_t * listElementObject)
{
   zone_properties   zoneProperties;
   json_t *          identifierObject;
   json_t *          levelObject;
   json_t *          batteryObject;
   json_t *          broadcastObject;
   char              localString[10];
   uint8_t           zoneIndex;
   uint8_t           zoneLevel;
   // if MAX_NUMBER_OF_ZONES changes and this breaks, update initialization array below
#if DEBUG_SOCKETTASK_ANY
   char debugBuf[100];
#endif   
   identifierObject = json_object_get(listElementObject, "identifier");
   if (identifierObject)
   {
      zoneIndex = checkForQmotionIdStringMatch(json_string_value(identifierObject));
      if (zoneIndex < MAX_NUMBER_OF_ZONES)
      {  // we found a match, get the level and store into zone properties
         levelObject = json_object_get(listElementObject, "level");
         if (levelObject)
         {
            zoneLevel = atoi(json_string_value(levelObject));
            if (GetZoneProperties(zoneIndex, &zoneProperties))
            {
               if (DEBUG_SOCKETTASK_QMOTION_STATUS_RESPONSE_VERBOSE && (zoneLevel == zoneProperties.targetPowerLevel))
               {
                  DBGSNPRINTF_QMOTION(DEBUGBUF,SIZEOF_DEBUGBUF,"powerLevel for zone %d currently %d to %d (reached target=%d)", zoneIndex, zoneProperties.powerLevel, zoneLevel, zoneProperties.targetPowerLevel);
                  DBGPRINT_INTSTRSTR_QMOTION(__LINE__,"HandleQmotionStatusResponse zoneIndex",DEBUGBUF);                  
               }
               if (zoneLevel != zoneProperties.powerLevel)
               {
                  // if this zone's powerLevel has been manually changed in the last minute, distrust the incoming setting as it may be obsolete
                  if (GetZoneBit(zoneChangedThisMinuteBitArray,zoneIndex))
                  {
                     DBGSNPRINTF_QMOTION(DEBUGBUF,SIZEOF_DEBUGBUF,"powerLevel for zone %d changed from %d to %d but this may be an obsolete value", zoneIndex, zoneProperties.powerLevel, zoneLevel);
                     DBGPRINT_INTSTRSTR_QMOTION(__LINE__,"HandleQmotionStatusResponse skip this send to app",DEBUGBUF);                     
                  }
                  else
                  {
                     // store new value and broadcast to app, Do nothing if value hasn't changed
                     // fill in zoneArray 
                     DBGSNPRINTF_QMOTION(DEBUGBUF,SIZEOF_DEBUGBUF,"powerLevel for zone %d changed from %d to %d", zoneIndex, zoneProperties.powerLevel, zoneLevel);
                     DBGPRINT_INTSTRSTR_QMOTION(__LINE__,"HandleQmotionStatusResponse send to app zoneIndex",DEBUGBUF);
   
                     zoneProperties.powerLevel = zoneLevel; // TODO: What about targetPowerLevel?
                     // Set the zone properties in the array
                     SetZoneProperties(zoneIndex, &zoneProperties);
                     
                     // Broadcast the change to the app
                     broadcastObject = json_object();
                     if (broadcastObject)
                     {
                        json_object_set_new(broadcastObject, "ID", json_integer(0));
                        json_object_set_new(broadcastObject, "Service", json_string("ZonePropertiesChanged"));
      
                        // build PropertyList based off of announcePropertyBitmask
                        BuildZonePropertiesObject(broadcastObject, zoneIndex, PWR_LVL_PROPERTIES_BITMASK);
      
                        //broadcast zonePropertiesChanged with created PropertyList
                        json_object_set_new(broadcastObject, "Status", json_string("Success"));
                        SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
                        json_decref(broadcastObject);
                     }
                  }
               } // if (zoneLevel != originalLevel)
               else if (DEBUG_SOCKETTASK_QMOTION_STATUS_RESPONSE_VERBOSE)
               {
                  DBGSNPRINTF_QMOTION(DEBUGBUF,SIZEOF_DEBUGBUF,"not sending powerLevel for zone %d unchanged from %d target=%d)", zoneIndex, zoneProperties.powerLevel, zoneProperties.targetPowerLevel);
                  DBGPRINT_INTSTRSTR_QMOTION(__LINE__,"HandleQmotionStatusResponse zoneIndex",DEBUGBUF);
               }
            } 
         } // if (levelTypeObject)
         
         // check for a battery low level indication
         batteryObject = json_object_get(listElementObject, "lowBattery");
         if (batteryObject)
         {
         
             if (json_is_string(batteryObject))
             {
            	 bool_t	currLowPowerSetting;
            	 bool_t	lastLowPowerSetting = (GetZoneBit(lastLowBatteryBitArray,zoneIndex) == 0) ? false : true;
            	 
                // true means that battery is in LOW Battery state
                snprintf(localString, sizeof(localString), "%s", json_string_value(batteryObject));
                currLowPowerSetting = (!strcmp(localString, "true")) ? true : false;  // strcmp returns 0 if strings are equal
                
                // only send ZonePropertiesChanged when the lowBattery setting for this zoneIndex has changed 
                // at startup, all batteries assumed to be good, and only lowBattery = true will be sent
                if (currLowPowerSetting != lastLowPowerSetting)
                {
                    DBGSNPRINTF_QMOTION(DEBUGBUF,SIZEOF_DEBUGBUF,"LowPower setting for zone %d changed from %d to %d", zoneIndex, lastLowPowerSetting, currLowPowerSetting);
                    DBGPRINT_INTSTRSTR_QMOTION(__LINE__,"HandleQmotionStatusResponse zoneIndex",DEBUGBUF);
                    if (GetZoneProperties(zoneIndex, &zoneProperties))
                    {
                        // update ZoneProperties so that ReportZoneProperties will show the lowBattery level in "state" in zoneProperties for shades
                        // this is a Kludge which I am told will be safe as this is an end-of-life product line that will never have a 
                        // battery-powered switch.  If this proves to be false, we will have to add a lowBattery field to zone_properties 
                        if ((zoneProperties.deviceType == SHADE_TYPE) && (currLowPowerSetting != zoneProperties.state))  
                        {  // store new lowBattery value (as zoneProperties.state) and broadcast to app, Do nothing if value hasn't changed
                           // fill in zoneArray 
                           zoneProperties.state = currLowPowerSetting;
                           // Set the zone properties in the array
                           SetZoneProperties(zoneIndex, &zoneProperties);
                        }
                          
                        // we have a low battery
                        // TODO: Broadcast to all sockets that the battery for this shade is low
                        // Broadcast the change to the app
                        broadcastObject = json_object();
                        if (broadcastObject)
                        {
                           json_object_set_new(broadcastObject, "ID", json_integer(0));
                           json_object_set_new(broadcastObject, "Service", json_string("ZonePropertiesChanged"));
                           
                           json_object_set_new(broadcastObject, "ZID", json_integer(zoneIndex));
                           if (currLowPowerSetting)
                           {
                              json_object_set_new(broadcastObject, "lowBattery", json_true());
                              SetZoneBit(lastLowBatteryBitArray,zoneIndex);
                           }
                           else
                           {
                              json_object_set_new(broadcastObject, "lowBattery", json_false());
                              ClearZoneBit(lastLowBatteryBitArray,zoneIndex);
                           }
                           
                           //broadcast zonePropertiesChanged with created PropertyList
                           json_object_set_new(broadcastObject, "Status", json_string("Success"));
                           SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
                           json_decref(broadcastObject);
                        }
                    }
                }
             }
             
         } // if batteryObject
         
      } // if (zoneIndex < MAX_NUMBER_OF_ZONES)
    } // if (identifierObject)
   
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void QueryQmotionStatus(void)
//-----------------------------------------------------------------------------
// - This will query the shade devices for level and will set the power level.
//-----------------------------------------------------------------------------
{
   sockaddr_in       remote_sin;
   json_t *          responseObject = NULL;
   json_t *          deviceStatusObject;
   json_t *          totalDevicesObject;
   json_t *          listObject;
   int_32           result;
   uint_32           opt_value;
   uint16_t          totalDevices;
   uint16_t          deviceIndex;
   uint8_t           retryGetCount;
   bool_t            doneFlag;
   
   DBGPRINT_INTSTRSTR(__LINE__,"Entering QueryQmotionStatus","");
   
   // Create a UDP (SOCK_DGRAM) socket to communicate with the Qube
   QSock = socket(AF_INET, SOCK_DGRAM, 0);
   if (QSock == RTCS_SOCKET_ERROR)
   {
      // reset this latch (helps with qmotion race conditions, when qube reports an obsolete value) 
      memset(zoneChangedThisMinuteBitArray, 0, sizeof(zoneChangedThisMinuteBitArray));
      return;
   }
   
   // Specify that the socket should not block
   opt_value = true;
   result = setsockopt(QSock, SOL_UDP, OPT_RECEIVE_NOWAIT, &opt_value, sizeof(opt_value));
   
   memset((char *) &remote_sin, 0, sizeof(sockaddr_in));
   remote_sin.sin_family = AF_INET;
   remote_sin.sin_port = QMOTION_UDP_PORT;
;
   remote_sin.sin_addr.s_addr = qubeAddress;

   totalDevices = 0;
   deviceIndex = 0;

   doneFlag = false;
   retryGetCount = 3;
   DBGPRINT_INTSTRSTR(__LINE__,"Entering QueryQmotionStatus while loop","");
   while ((!doneFlag) && (retryGetCount))
   {
   
      snprintf((char *) qmotionBuffer,
                sizeof(qmotionBuffer), 
                 "GET /devices/status?startIndex=%d HTTP/1.0\r\nHOST: %d.%d.%d.%d\r\n\r\n",
                  deviceIndex,
                   HighByteOfDword(qubeAddress),
                    UpperMiddleByteOfDword(qubeAddress),
                     LowerMiddleByteOfDword(qubeAddress),
                      LowByteOfDword(qubeAddress));
   
      result = Socket_sendto(QSock,
                     (void *)qmotionBuffer,
                      strlen((const char *)qmotionBuffer),
                       0, &remote_sin, sizeof(sockaddr_in),
                       &qmotion_send_error_count, __PRETTY_FUNCTION__);

      DBGPRINT_INTSTRSTR_QMOTION_DETAIL(result,"QueryQmotionStatus send result", (char *) qmotionBuffer);
      
      retryGetCount--;
      
      if (result > 0)
      {
         responseObject = GetQmotionJsonPayload();
      }
   
      if (responseObject)
      {
         deviceStatusObject = json_object_get(responseObject, "deviceStatus");
         if (deviceStatusObject)
         {
            totalDevicesObject = json_object_get(deviceStatusObject, "totalDevices");
            listObject = json_object_get(deviceStatusObject, "list");
            if ((json_is_array(listObject)) && (json_is_integer(totalDevicesObject)))         
            {
               totalDevices = json_integer_value(totalDevicesObject);

               int listArrayIndex = 0;
               json_t * listArrayElementObject = json_array_get(listObject, listArrayIndex);

               while (listArrayElementObject)
               {
                  deviceIndex++;
                  retryGetCount = 3;  // as soon as we process a device, reset our retry count since we (might) need to send a new GET
                  listArrayIndex++;
#if 0
                  DBGPRINT_INTSTRSTR_QMOTION(listArrayIndex,"QueryQmotionStatus HandleQmotionStatusResponse listArrayIndex","");
#endif
                  HandleQmotionStatusResponse(listArrayElementObject);
                  listArrayElementObject = json_array_get(listObject, listArrayIndex);
#if 0
                  if (listArrayElementObject)
                  {
                     DBGPRINT_INTSTRSTR_QMOTION_DETAIL(__LINE__,"QueryQmotionStatus HandleQmotionStatusResponse next listArrayElementObject OK","");
                  }
                  else
                  {
                     DBGPRINT_INTSTRSTR_QMOTION_DETAIL(__LINE__,"QueryQmotionStatus HandleQmotionStatusResponse no more listArrayElementObjects","");
                  }
#endif
               }
            }
         }
         else
         {
            DBGPRINT_INTSTRSTR_ERROR(__LINE__,"QueryQmotionStatus no deviceStatusObject","");
         }

         json_decref(responseObject);  // release the response object
         responseObject = NULL;
      
      }
      else
      {
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"QueryQmotionStatus no responseObject","");
      }
      
      if (deviceIndex >= totalDevices)
      {
         doneFlag = true;
      }

   }

   shutdown(QSock, FLAG_ABORT_CONNECTION);

   // reset this latch (helps with qmotion race conditions, when qube reports an obsolete value) 
   memset(zoneChangedThisMinuteBitArray,0,sizeof(zoneChangedThisMinuteBitArray));

   DBGPRINT_INTSTRSTR(__LINE__,"Leaving QueryQmotionStatus","");
}

#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// added validation, 6/11:
//   if want to set AddASceneController: check that we have room in array
//   if want to set AddALight: check that we have room in array
//-----------------------------------------------------------------------------

bool_t HandleIndividualSystemProperty(const char * keyPtr, json_t * valueObject, json_t * responseObject, uint_8 setFlag)
{
   const char *locationKeyPtr;
   json_t *locationValueObject;
   void *locationObjectIter;
   int32_t diffOffset;
   int32_t originalGmtOffset;
   TIME_STRUCT mqxTime;
   bool_t returnFlag = FALSE;
   bool_t gotLatOrLong = FALSE;
#if DEBUG_SOCKETTASK_ANY
   char                       debugBuf[100];
   MQX_TICK_STRUCT            debugLocalStartTick;
   MQX_TICK_STRUCT            debugLocalEndTick;
   bool                       debugLocalOverflow;
   uint32_t                   debugLocalOverflowMs;
#endif

   if(!strcmp(keyPtr, "Keys"))
   {
      if(json_is_string(valueObject))
      {
         JSA_HandleKeysProperty(json_string_value(valueObject), responseObject);
      }
      returnFlag = TRUE;
   }
   // In the case of a limited JSON state where only the user key may be set:
   else if(jsonSockets[socketIndexGlobalRx].jsa_state & JSA_MASK_JSON_LIMITED)
   {
      // Because broadcastDebug is disabled in [SETKEY] mode, the socket which caused this condition will not see this message:
      DBGPRINT_INTSTRSTR_ERROR(socketIndexGlobalRx, "HandleIndividualSystemProperty: Rejected non-Keys property on socket:", "");
      return false;
   }
   // Note: While auth exempt logic no longer relies on date, AuthExemptYear
   //  is unchanged for debug / test purposes.
   // AuthExemptYear is only for testing and will not work without the debug jumper.
   // {"AuthExemptYear" : 2020} Sets auth_exempt to AUTH_NOT_EXEMPT
   // {"AuthExemptYear" : 2019} Sets auth_exempt to AUTH_EXEMPT
   // {"AuthExemptYear" : null} Sets auth_exempt to undefined
   else if(!strcmp(keyPtr, "AuthExemptYear"))
   {
      if(!DEBUG_JUMPER)
      {
         BuildErrorResponse(responseObject, "Denied", SW_UNKNOWN_SERVICE);
         return false;
      }
      returnFlag = TRUE;
      if(!json_is_integer(valueObject))
      {
         extendedSystemProperties.auth_exempt = AUTH_EXEMPT ^ AUTH_NOT_EXEMPT;
         if(AnyZonesInArray())
         {
            BuildErrorResponse(responseObject, "auth_exempt=undefined - WARNING: nonzero zone count", 0);
         }
         else
         {
             BuildErrorResponse(responseObject, "auth_exempt=undefined", 0);
         }
      }
      else
      {
         if(json_integer_value(valueObject) >= SB327_INCEPTION_YEAR)
         {
             extendedSystemProperties.auth_exempt = AUTH_NOT_EXEMPT;
             BuildErrorResponse(responseObject, "auth_exempt=AUTH_NOT_EXEMPT", 0);
         }
         else
         {
             extendedSystemProperties.auth_exempt = AUTH_EXEMPT;
             BuildErrorResponse(responseObject, "auth_exempt=AUTH_EXEMPT", 0);
         }
      }
      SaveSystemPropertiesToFlash();
   }
   else if(!strcmp(keyPtr, "AddASceneController"))
   {
      if(!json_is_boolean(valueObject))
      {
         BuildErrorResponse(responseObject, "PropertyList, AddASceneController must be boolean.", SW_PROPERTY_LIST_ADDASCENECTL_MUST_BE_BOOLEAN);
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleIndividualSystemProperty: AddASceneController must be boolean","");
         returnFlag = TRUE;
      }
      else if(GetAvailableSceneControllerIndex() == MAX_NUMBER_OF_SCENE_CONTROLLERS)
      {
         BuildErrorResponse(responseObject, "PropertyList, AddASceneController:No slots left in array.", USR_AddASceneController_NO_SLOTS_LEFT_IN_ARRAY);
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleIndividualSystemProperty: AddASceneController:No slots left in array","");
         returnFlag = TRUE;
      }
      else if(setFlag)
      {
         systemProperties.addASceneController = json_is_true(valueObject);
      }
   }
   else if(!strcmp(keyPtr, "AddALight"))
   {
      if(!json_is_boolean(valueObject))
      {
         BuildErrorResponse(responseObject, "PropertyList, AddALight must be boolean.", SW_PROPERTY_LIST_ADDALIGHT_MUST_BE_BOOLEAN);
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleIndividualSystemProperty: AddALight must be boolean","");
         returnFlag = TRUE;
      }
      else if(GetAvailableZoneIndex() == MAX_NUMBER_OF_ZONES)
      {
         BuildErrorResponse(responseObject, "PropertyList, AddALight:No slots left in array.", USR_AddALight_NO_SLOTS_LEFT_IN_ARRAY);
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleIndividualSystemProperty: AddALight: No slots left in array","");
         returnFlag = TRUE;
      }
      else if(setFlag)
      {
         systemProperties.addALight = json_is_true(valueObject);

         // Capture the time when addALight was set to true, so we can set it back to false after a timeout period.
         if (systemProperties.addALight)
         {
             _time_get(&lastAddALightTime);
         }
      }
   }

#ifdef SHADE_CONTROL_ADDED
   else if(!strcmp(keyPtr, "AddAShade"))
   {
      if(!json_is_boolean(valueObject))
      {
         BuildErrorResponse(responseObject, "PropertyList, AddAShade must be boolean.", SW_PROPERTY_LIST_ADDASHADE_MUST_BE_BOOLEAN);
         DBGPRINT_INTSTRSTR_ERROR(__LINE__,"HandleIndividualSystemProperty: AddAShade must be boolean","");
         returnFlag = TRUE;
      }
      else if(GetAvailableZoneIndex() == MAX_NUMBER_OF_ZONES)
      {
         BuildErrorResponse(responseObject, "PropertyList, AddAShade:No slots left in array.", USR_AddAShade_NO_SLOTS_LEFT_IN_ARRAY);
         DBGPRINT_INTSTRSTR(__LINE__,"HandleIndividualSystemProperty: AddAShade: No slots left in array","");
         returnFlag = TRUE;
      }
      else if(setFlag)
      {
         if (json_is_true(valueObject))
         {
            json_t * broadcastObject;
                       
            if (systemProperties.addALight)
            {
               systemProperties.addALight = false;
               broadcastObject = json_object();
               if (broadcastObject)
               {
                  json_object_set_new(broadcastObject, "ID", json_integer(0));
                  json_object_set_new(broadcastObject, "Service", json_string("SystemPropertiesChanged"));
                  BuildSystemPropertiesObject(broadcastObject, SYSTEM_PROPERTY_ADD_A_LIGHT_BITMASK);
                  json_object_set_new(broadcastObject, "Status", json_string("Success"));
                  SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
                  json_decref(broadcastObject);
               }
            }

            DEBUG_QUBE_MUTEX_START;
            _mutex_lock(&qubeSocketMutex);
            DBGPRINT_INTSTRSTR_IF_QUBE_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW HandleIndividualSystemProperty before qubeSocketMutex",DEBUGBUF);
            DEBUG_QUBE_MUTEX_START;
            if (!clearQubeAddressTimer)
            {
               TryToFindQmotion();
            }
            if (qubeAddress)
            {
               GetQmotionDevicesInfo();  // look for the devices
            }
            DBGPRINT_INTSTRSTR_IF_QUBE_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW HandleIndividualSystemProperty after GetQmotionDevicesInfo",DEBUGBUF);
            _mutex_unlock(&qubeSocketMutex);

            systemShadeSupportProperties.addAShade = json_is_true(valueObject); 
            // systemShadeSupportProperties.addAShade = true; // SamF was false
            broadcastObject = json_object();
            if (broadcastObject)
            {
               json_object_set_new(broadcastObject, "ID", json_integer(0));
               json_object_set_new(broadcastObject, "Service", json_string("SystemPropertiesChanged"));
               BuildSystemPropertiesObject(broadcastObject, SYSTEM_PROPERTY_ADD_A_SHADE_BITMASK);
               json_object_set_new(broadcastObject, "Status", json_string("Success"));
               SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
               json_decref(broadcastObject);
            }
         }
      }
   }
#endif

   else if(!strcmp(keyPtr, "TimeZone"))
   {
      if(!json_is_integer(valueObject))
      {
         BuildErrorResponse(responseObject, "TimeZone must be integer.", SW_TIMEZONE_MUST_BE_INTEGER);
         returnFlag = TRUE;
      }
      else
      {
         if(setFlag)
         {
            originalGmtOffset = systemProperties.gmtOffsetSeconds;
            systemProperties.gmtOffsetSeconds = json_integer_value(valueObject);
            
            // get difference between old and new 
            diffOffset = originalGmtOffset - systemProperties.gmtOffsetSeconds;
            
            // adjust effective GMT offset
            systemProperties.effectiveGmtOffsetSeconds = systemProperties.effectiveGmtOffsetSeconds - diffOffset; 
            
            // adjust real-time clock to reflect new time zone
            // Get the current system time
            _time_get(&mqxTime);

            mqxTime.SECONDS -= diffOffset;

            // set the real time clock with the received time
            _time_set(&mqxTime);

            // json broadcast SystemPropertiesChanged for the effective timezone and the current time
            // since it will not be returned in the root message for setting the timezone
            json_t * broadcastObject;
            broadcastObject = json_object();
            if (broadcastObject)
            {
               json_object_set_new(broadcastObject, "ID", json_integer(0));
               json_object_set_new(broadcastObject, "Service", json_string("SystemPropertiesChanged"));
               BuildSystemPropertiesObject(broadcastObject, SYSTEM_PROPERTY_NEW_TIME_BITMASK | SYSTEM_PROPERTY_EFFECTIVE_TIME_ZONE_BITMASK);
               json_object_set_new(broadcastObject, "Status", json_string("Success"));
               SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
               json_decref(broadcastObject);
            }

            // Adjust the scene times by the diff offset so they are updated properly on a time zone change
            adjustTriggers(-diffOffset);

            // Set the flash save timer to make sure this value is saved
            flashSaveTimer = FLASH_SAVE_TIMER;
         }
      }
   }
   else if(!strcmp(keyPtr, "DaylightSavingTime"))
   {
      if(!json_is_boolean(valueObject))
      {
         BuildErrorResponse(responseObject, "PropertyList, DaylightSavingTime must be boolean.", SW_PROPERTY_LIST_DAYLIGHTSAVINGS_MUST_BE_BOOLEAN);
         returnFlag = TRUE;
      }
      else if(setFlag)
      {
         TIME_STRUCT currentTime;
         DATE_STRUCT currentDate;

         systemProperties.useDaylightSaving = json_is_true(valueObject);

         // Adjust for daylight savings time if necessary
         _time_get(&currentTime);
         _time_to_date(&currentTime, &currentDate);
         checkForDST(currentDate);

         // Save the data to flash
         flashSaveTimer = FLASH_SAVE_TIMER;
      }
   }
   else if(!strcmp(keyPtr, "LocationInfo"))
   {
      if (!json_is_string(valueObject))
      {
         BuildErrorResponse(responseObject, "PropertyList, LocationInfo must be string.", SW_PROPERTY_LIST_LOCATION_INFO_MUST_BE_A_STRING);
         returnFlag = TRUE;
      }
      else
      {
         if (setFlag)
         {
            snprintf(systemProperties.locationInfoString, sizeof(systemProperties.locationInfoString), "%s", json_string_value(valueObject));
            flashSaveTimer = FLASH_SAVE_TIMER;
         }
      }
   }
   else if(!strcmp(keyPtr, "Location"))
   {
      if(!json_is_object(valueObject)) 
      {
         BuildErrorResponse(responseObject, "PropertyList, Location must be an object.", SW_PROPERTY_LIST_LOCATION_MUST_BE_AN_OBJECT);
         returnFlag = TRUE;
      }
      else
      {
         locationObjectIter = json_object_iter(valueObject);
         
         while(locationObjectIter && (returnFlag == FALSE))
         {
            locationKeyPtr = json_object_iter_key(locationObjectIter);
            locationValueObject = json_object_iter_value(locationObjectIter);

            if(!strcmp(locationKeyPtr, "Lat"))
            {
               if(!ValidateLocationProperty(locationValueObject, responseObject, TRUE))
               {
                  if(setFlag)
                  {
                     // Fields were validated so it is safe to get them directly here
                     systemProperties.latitude.degrees = (int16_t)(json_integer_value(json_object_get(locationValueObject, "Deg")));
                     systemProperties.latitude.minutes = (uint8_t)(json_integer_value(json_object_get(locationValueObject, "Min")));
                     systemProperties.latitude.seconds = (uint8_t)(json_integer_value(json_object_get(locationValueObject, "Sec")));
                     gotLatOrLong = TRUE;
                     flashSaveTimer = FLASH_SAVE_TIMER;
                  }
               }
               else
               {
                  // Response object was filled in the ValidateLocationProperty function
                  returnFlag = TRUE;
               }
            }
            else if(!strcmp(locationKeyPtr, "Long"))
            {
               if(!ValidateLocationProperty(locationValueObject, responseObject, FALSE))
               {
                  if(setFlag)
                  {
                     // Fields were validated so it is safe to get them directly here
                     systemProperties.longitude.degrees = (int16_t)(json_integer_value(json_object_get(locationValueObject, "Deg")));
                     systemProperties.longitude.minutes = (uint8_t)(json_integer_value(json_object_get(locationValueObject, "Min")));
                     systemProperties.longitude.seconds = (uint8_t)(json_integer_value(json_object_get(locationValueObject, "Sec")));
                     gotLatOrLong = TRUE;
                     flashSaveTimer = FLASH_SAVE_TIMER;
                  }
               }
               else
               {
                  // Response object was filled in the ValidateLocationProperty function
                  returnFlag = TRUE;
               }
            }
            else
            {
               BuildErrorResponse(responseObject, "PropertyList, Location Property not handled.", SW_PROPERTY_LIST_LOCATION_PROPERTY_NOT_HANDLED);
               returnFlag = TRUE;
            }

            locationObjectIter = json_object_iter_next(valueObject, locationObjectIter);
         } // end while(locationObjectIter && (returnFlag == FALSE))
         
         // if we got a good lat or long, adjust sun times for new location
         if (gotLatOrLong == TRUE)
         {
            // Get the current system time
            _time_get(&mqxTime);
            scene_properties sceneProperties;
            _mutex_lock(&ScenePropMutex);
            for (int sceneIdx = 0; sceneIdx < MAX_NUMBER_OF_SCENES; sceneIdx++)
            {
               if (GetSceneProperties(sceneIdx, &sceneProperties))
               {
                  // if we have a scene and the trigger is not regular, get a new trigger for the sun time
                  if ((sceneProperties.SceneNameString[0] != 0) && (sceneProperties.trigger != REGULAR_TIME))
                  {
                     calculateNewTrigger(&sceneProperties, mqxTime);
                     SetSceneProperties(sceneIdx, &sceneProperties);
                     broadcastNewSceneTriggerTime(sceneIdx, &sceneProperties);
                  }
               }
            } // end for (int sceneIdx = 0; sceneIdx < MAX_NUMBER_OF_SCENES; sceneIdx++)
            _mutex_unlock(&ScenePropMutex);
         } //  end of if (gotLatOrLong == TRUE)
      }
   }
   else if(!strcmp(keyPtr, "EliotUserToken"))
   {
		if(!json_is_string(valueObject))
		{
			BuildErrorResponse(responseObject, "PropertyList, EliotUserToken must be a string", -1);
			returnFlag = true;
		}
		else
		{
			if(setFlag)
			{
			    // Clear the Eliot LCM ID.  This will turn off MQTT for the previous token (if there was one).
			    // If there is a new token (and not just an empty one) then the LCM ID will be re-generated,
			    // or re-acquired if it already exists.
			    eliot_lcm_module_id[0] = 0;

			    // Also clear the Eliot Plant ID.  A new Plant ID will be acquired when first needed.
			    eliot_lcm_plant_id[0] = 0;

				strncpy(eliot_user_token, json_string_value(valueObject), ELIOT_TOKEN_SIZE);
				eliot_user_token_modified_time = Eliot_TimeNow();
				// Prints the first 200 characters of the token and its true length.
				DBGPRINT_INTSTRSTR(strlen(eliot_user_token), "SocketTask/EliotUserToken", eliot_user_token);
			}
		}
	}
	else if(!strcmp(keyPtr, "EliotRefreshToken"))
	{
		if(!json_is_string(valueObject))
		{
			BuildErrorResponse(responseObject, "PropertyList, EliotRefreshToken must be a string", -1);
			returnFlag = true;
		}
		else
		{
			if(setFlag)
			{
				FBK_WriteFlash(&eliot_fbk, (char*)json_string_value(valueObject), LCM_DATA_REFRESH_TOKEN_OFFSET, ELIOT_TOKEN_SIZE);
				eliot_refresh_token_modified_time = Eliot_TimeNow();
				eliot_refresh_token = FBK_Address(&eliot_fbk, LCM_DATA_REFRESH_TOKEN_OFFSET);
				DBGPRINT_INTSTRSTR(eliot_fbk.active_bank, "SocketTask/EliotRefreshToken written to bank:", eliot_refresh_token);

				if(!eliot_refresh_token[0])
				{
					Eliot_LogOut();
				}
			}
		}
	}
	else if(!strcmp(keyPtr, "Configured"))
	{
		if(!json_is_boolean(valueObject))
		{
			BuildErrorResponse(responseObject, "PropertyList, Configured must be boolean.", SW_PROPERTY_LIST_CONFIGURED_MUST_BE_BOOLEAN);
			returnFlag = TRUE;
		}
		else if(setFlag)
		{
			systemProperties.configured = json_is_true(valueObject);
			DBGPRINT_INTSTRSTR(systemProperties.configured,"HandleIndividualSystemProperty: configured", "");
		}
	}
	else if(!strcmp(keyPtr, "MobileAppData"))
	{
		if(!json_is_string(valueObject))
		{
			BuildErrorResponse(responseObject, "PropertyList, MobileAppData must be a string", -1);
			returnFlag = TRUE;
		}
		else if(setFlag)
		{
			snprintf(extendedSystemProperties.mobileAppData, sizeof(extendedSystemProperties.mobileAppData), "%s", json_string_value(valueObject));
			extendedSystemProperties.mobileAppData[sizeof(extendedSystemProperties.mobileAppData) - 1] = 0;  // make sure we null terminate.
			flashSaveTimer = FLASH_SAVE_TIMER;
			DBGPRINT_INTSTRSTR(__LINE__,"HandleIndividualSystemProperty: mobileAppData", extendedSystemProperties.mobileAppData);
		}
	}
   else if (!strcmp(keyPtr, "CurrentTime"))
   {
       if(!json_is_integer(valueObject))
       {
          BuildErrorResponse(responseObject, "PropertyList, CurrentTime must be an integer", -1);
          returnFlag = TRUE;
       }
       else
       {
          TIME_STRUCT mqxTime;
          _rtc_init(NULL);
          mqxTime.SECONDS = json_integer_value(valueObject) + systemProperties.effectiveGmtOffsetSeconds;
          _time_set(&mqxTime);
       }
   }
   else
   {
      BuildErrorResponse(responseObject, "PropertyList, Property not handled.", SW_PROPERTY_LIST_PROPERTY_NOT_HANDLED);
      returnFlag = TRUE;
   }

   return returnFlag;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleSetSystemProperties(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr)
{
   json_t * propertyListObject;
   const char * keyPtr;
   json_t * valueObject;
   dword_t propertyIndex = 0;

   // Get the property list object
   propertyListObject = json_object_get(root, "PropertyList");
   if (!json_is_object(propertyListObject))
   {
      BuildErrorResponse(responseObject, "Invalid 'PropertyList'.", SW_INVALID_PROPERTY_LIST);
      return;
   }

   // first, go through the entire property list and make sure we don't have ANY errors.
   json_object_foreach(propertyListObject, keyPtr, valueObject)
   {
      if (HandleIndividualSystemProperty(keyPtr, valueObject, responseObject, FALSE))
      {
         return;
      }
   }

   // if everything was ok, go through the property list again and execute.
   json_object_foreach(propertyListObject, keyPtr, valueObject)
   {
      if (HandleIndividualSystemProperty(keyPtr, valueObject, responseObject, TRUE))
      {
         return;
      }
   }

   // if we don't have an error, then prepare the asynchronous message
   // to all json sockets.
   *jsonBroadcastObjectPtr = json_copy(root);
   json_object_set_new(*jsonBroadcastObjectPtr, "ID", json_integer(0));
   json_object_set_new(*jsonBroadcastObjectPtr, "Service", json_string("SystemPropertiesChanged"));
   json_object_set_new(*jsonBroadcastObjectPtr, "Status", json_string("Success"));

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// BroadcastZoneAdded()
//   given a zone index, this broadcasts "Zone Added" to all ethernet-connected devices
//-----------------------------------------------------------------------------

void BroadcastZoneAdded(byte_t zoneID)
{
   json_t * broadcastObject;

   broadcastObject = json_object();
   if (broadcastObject)
   {
      json_object_set_new(broadcastObject, "ID", json_integer(0));
      json_object_set_new(broadcastObject, "Service", json_string("ZoneAdded"));
   
      json_object_set_new(broadcastObject, "ZID", json_integer(zoneID));
   
      json_object_set_new(broadcastObject, "Status", json_string("Success"));
      SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
      json_decref(broadcastObject);
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void BroadcastSceneControllerAdded(byte_t sceneControllerID)
{
   json_t * broadcastObject;
   broadcastObject = json_object();
   if (broadcastObject)
   {
      json_object_set_new(broadcastObject, "ID", json_integer(0));
      json_object_set_new(broadcastObject, "Service", json_string("SceneControllerAdded"));
      json_object_set_new(broadcastObject, "SCID", json_integer(sceneControllerID));
      json_object_set_new(broadcastObject, "Status", json_string("Success"));
      SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
      json_decref(broadcastObject);
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleTargetValue(word_t groupID, byte_t targetValue, byte_t zoneIdx)
{
   json_t * broadcastObject;
   dword_t announcePropertyBitmask = PWR_BOOL_PROPERTIES_BITMASK | 
                                     PWR_LVL_PROPERTIES_BITMASK;
   zone_properties zoneProperties;
#if DEBUG_SOCKETTASK_ANY
char debugBuf[100];
#endif

   if (GetZoneProperties(zoneIdx, &zoneProperties))
   {
      if (targetValue)  // if target Value ramp destination level not zero
      {
         if (!zoneProperties.state)  // if zone state property was false
         {
            //set zone state property to true
            zoneProperties.state = TRUE;

            //set the target zone state property to true
            zoneProperties.targetState = TRUE;
         }

         if (zoneProperties.powerLevel != targetValue)  // if stored zone level  doesn't match ramp target level
         {
            DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"HandleTargetValue powerLevel changed for zoneIndex %d from %d target %d to %d", zoneIdx, zoneProperties.powerLevel, zoneProperties.targetPowerLevel, targetValue);
            DBGPRINT_INTSTRSTR(__LINE__,"HandleTargetValue",DEBUGBUF);

            //set zone level property
            zoneProperties.powerLevel = targetValue;

            //set the target level property
            zoneProperties.targetPowerLevel = targetValue;
         }
      }
      else  // else, targetValue is 0
      {
         DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"HandleTargetValue targetValue 0 for zoneIndex %d state set FALSE (powerLevel remains %d target %d)", zoneIdx, zoneProperties.powerLevel, zoneProperties.targetPowerLevel);
         DBGPRINT_INTSTRSTR(__LINE__,"HandleTargetValue",DEBUGBUF);
         if (zoneProperties.state)  //zone state was true
         {
            //set zone state property to false
            zoneProperties.state = FALSE;

            //set the target zone state property to false
            zoneProperties.targetState = FALSE;
         }
      }

      // Set the zone properties in the array
      SetZoneProperties(zoneIdx, &zoneProperties);

      if (announcePropertyBitmask)
      {
         broadcastObject = json_object();
         if (broadcastObject)
         {
   
            json_object_set_new(broadcastObject, "ID", json_integer(0));
            json_object_set_new(broadcastObject, "Service", json_string("ZonePropertiesChanged"));
   
            // build PropertyList based off of announcePropertyBitmask
            BuildZonePropertiesObject(broadcastObject, zoneIdx, announcePropertyBitmask);
   
            //broadcast zonePropertiesChanged with created PropertyList
            json_object_set_new(broadcastObject, "Status", json_string("Success"));
            SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
            json_decref(broadcastObject);
         }
         
         // Send the zone property changed to the samsung task
         Eliot_QueueZoneChanged(zoneIdx);
      }
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleSceneControllerCommandReceived( byte_t sceneControllerIndex, byte_t buildingId, byte_t houseId, word_t groupId, byte_t bankIndex)
{
   json_t * broadcastObject;
   scene_properties sceneProperties;
   scene_controller_properties sceneControllerProperties;
   
   if((!AnyZonesInArray()) &&
      (!AnySceneControllersInArray()))
   {
      // Set the system properties configured flag because we added a scene controller
      systemProperties.configured = true;

      // Set house id
      global_houseID = houseId;
      BroadcastSystemInfo();

      // Turn off the house ID not set LED
      SetGPIOOutput(HOUSE_ID_NOT_SET, LED_OFF);
   }

   // Determine if this scene controller is in our house
   if(global_houseID == houseId)
   {
      if(sceneControllerIndex >= MAX_NUMBER_OF_SCENE_CONTROLLERS)
      {
         // New scene controller with our house ID
         // Find the first available slot in our scene controller array for this new scene controller
         sceneControllerIndex = GetAvailableSceneControllerIndex();

         if(sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS)
         {
            // fill in sceneControllerArray slot with defaults
            snprintf(sceneControllerProperties.SceneControllerNameString,
                  sizeof(sceneControllerProperties.SceneControllerNameString),
                  "SC %d", sceneControllerIndex);

            for(byte_t clearBankIndex = 0; clearBankIndex < MAX_SCENES_IN_SCENE_CONTROLLER; clearBankIndex++)
            {
               sceneControllerProperties.bank[clearBankIndex].sceneId = -1;
               sceneControllerProperties.bank[clearBankIndex].toggleable = FALSE;
               sceneControllerProperties.bank[clearBankIndex].toggled = FALSE;
            }
            sceneControllerProperties.bank[3].sceneId = MASTER_OFF_FOR_SCENE_CONTROLLER;  // Default last button to be Master Off
            
            // fill in scene controller array slot with rf packet info
            sceneControllerProperties.buildingId = buildingId;
            sceneControllerProperties.groupId = groupId;
            sceneControllerProperties.houseId = houseId;
            sceneControllerProperties.nightMode = false;

            // Set the scene controller properties in the array
            SetSceneControllerProperties(sceneControllerIndex, &sceneControllerProperties);

            // Broadcast SceneControllerAdded
            BroadcastSceneControllerAdded(sceneControllerIndex);
            // Reset the addASceneController flag to false
            systemProperties.addASceneController = false;

            // Broadcast the system properties changed for the addASceneController flag
            broadcastObject = json_object();
            if (broadcastObject)
            {
               json_object_set_new(broadcastObject, "ID", json_integer(0));
               json_object_set_new(broadcastObject, "Service", json_string("SystemPropertiesChanged"));
               BuildSystemPropertiesObject(broadcastObject, SYSTEM_PROPERTY_ADD_A_SCENE_CONTROLLER_BITMASK);
               json_object_set_new(broadcastObject, "Status", json_string("Success"));
               SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
               json_decref(broadcastObject);
            }
         }  // if(sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS)
      }  // if(sceneControllerIndex >= MAX_NUMBER_OF_SCENE_CONTROLLERS)

   }  // if (global_houseID == houseId)
   
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleSceneControllerMasterOff(byte_t buildingId, byte_t houseId, word_t groupId, byte_t deviceType, byte_t rampRate)
{
   byte_t sceneControllerIndex;
   scene_properties sceneProperties;
   scene_controller_properties sceneControllerProperties;
   byte_t sceneId;
   uint8_t bankIndex = 0;
   
   // Determine if this scene controller is already known
   sceneControllerIndex = GetSceneControllerIndex(groupId, buildingId, houseId);
  
   if(systemProperties.addASceneController)
   {
      HandleSceneControllerCommandReceived(sceneControllerIndex, buildingId, houseId, groupId, bankIndex);
   }
   else
   {
       // Turn off all loads for the scene controller
       if((global_houseID == houseId) &&
          (sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS))
       {
          // The scene controller is a scene controller that was already added for our house ID
          if (GetSceneControllerProperties(sceneControllerIndex, &sceneControllerProperties))
          {
             for(bankIndex = 0; bankIndex < MAX_SCENES_IN_SCENE_CONTROLLER; bankIndex++)
             {
                sceneId = sceneControllerProperties.bank[bankIndex].sceneId;

                _mutex_lock(&ScenePropMutex);
                if((sceneId != -1) &&
                   (GetSceneProperties(sceneId, &sceneProperties)))
                {
                   ExecuteScene(sceneId, &sceneProperties, 0, false, 0, 1, rampRate);
                   SetSceneProperties(sceneId, &sceneProperties);
                }
                _mutex_unlock(&ScenePropMutex);
             }
          }
       }
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleSceneControllerSceneRequest(byte_t buildingId, byte_t houseId, word_t groupId, byte_t bankIndex, bool_t state, byte_t deviceType, byte_t rampRate)
{
   byte_t sceneControllerIndex;
   scene_properties sceneProperties;
   scene_controller_properties sceneControllerProperties;
   byte_t sceneId;
   byte_t avgLevel;
   
   // Determine if this scene controller is already known
   sceneControllerIndex = GetSceneControllerIndex(groupId, buildingId, houseId);
  
   if(systemProperties.addASceneController)
   {
      HandleSceneControllerCommandReceived(sceneControllerIndex, buildingId, houseId, groupId, bankIndex);
   }
   else
   {
       // Execute the scene request of Scene Controller
       // We are not in Add A Scene Controller Mode
       if((global_houseID == houseId) &&
          (sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS))
       {
          // The scene controller is a scene controller that was already added for our house ID
          // Execute the scene for the bank that was pushed if a valid scene is set
          if (GetSceneControllerProperties(sceneControllerIndex, &sceneControllerProperties))
          {
             if(bankIndex < MAX_SCENES_IN_SCENE_CONTROLLER)
             {
               sceneId = sceneControllerProperties.bank[bankIndex].sceneId;

                _mutex_lock(&ScenePropMutex);
                if((sceneId != -1) &&
                   (GetSceneProperties(sceneId, &sceneProperties)))
                {
                   ExecuteScene(sceneId, &sceneProperties, TRUE, state, 0, 1, rampRate);
                   SetSceneProperties(sceneId, &sceneProperties);
                }
                _mutex_unlock(&ScenePropMutex);
             }
          }
       }
   }
   
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleSceneControllerSceneAdjust(byte_t buildingId, byte_t houseId, word_t groupId, byte_t sceneIdx, byte_t value, byte_t change, byte_t rampRate)
{
   byte_t sceneControllerIndex;
   byte_t avgLevel = 0;
   scene_properties sceneProperties;
   scene_controller_properties sceneControllerProperties;
   
   // Determine if this scene controller is already known
   sceneControllerIndex = GetSceneControllerIndex(groupId, buildingId, houseId);
  
   if(systemProperties.addASceneController)
   {
      HandleSceneControllerCommandReceived(sceneControllerIndex, buildingId, houseId, groupId, 0);
   }
   else
   {
      // adjust the currently executing scene
      // Execute the scene request of Scene Controller
      // We are not in Add A Scene Controller Mode
      if(global_houseID == houseId) 
      {
         _mutex_lock(&ScenePropMutex);
         if((sceneIdx != -1) && (GetSceneProperties(sceneIdx, &sceneProperties)) )
         {
            ExecuteScene(sceneIdx, &sceneProperties, 0, TRUE, value, change, rampRate);
            SetSceneProperties(sceneIdx, &sceneProperties);
         }
         _mutex_unlock(&ScenePropMutex);
      } 
   }
   
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleRampReceived(byte_t buildingId, byte_t houseId, word_t groupId, byte_t targetValue, Dev_Type deviceType)
{
   json_t * broadcastObject;
   byte_t zoneIdx;
   zone_properties zoneProperties;

   zoneIdx = GetZoneIndex(groupId, buildingId, houseId);
   if (systemProperties.addALight)
   {
      if ((!AnyZonesInArray()) &&
          (!AnySceneControllersInArray()))
      {
         // Set the system properties configured flag because we added a zone
         systemProperties.configured = true;

         //set house id
         global_houseID = houseId;
         BroadcastSystemInfo();

         // Turn off the house ID not set LED
         SetGPIOOutput(HOUSE_ID_NOT_SET, LED_OFF);
      }

      if (global_houseID == houseId)  // if we got a command from our house
      {
         if (zoneIdx >= MAX_NUMBER_OF_ZONES)  // the group id is not known
         {
            // find the first available slot in our zone array for this new zone
            zoneIdx = GetAvailableZoneIndex();

            if (zoneIdx < MAX_NUMBER_OF_ZONES)
            {
               // fill in zoneArray slot with defaults
               snprintf(zoneProperties.ZoneNameString, sizeof(zoneProperties.ZoneNameString), "zone %d", (zoneIdx + 1));
               zoneProperties.rampRate = 50;
               zoneProperties.AppContextId = 0;
               zoneProperties.roomId = 0;

               // fill in zoneArray slot with rf packet info
               zoneProperties.deviceType = deviceType;
               zoneProperties.buildingId = buildingId;
               zoneProperties.groupId = groupId;
               zoneProperties.houseId = houseId;
               if (targetValue)
               {
                  zoneProperties.powerLevel = targetValue;
                  zoneProperties.targetPowerLevel = targetValue;
                  zoneProperties.state = TRUE;
                  zoneProperties.targetState = TRUE;
               }
               else
               {
                  // When adding a light. And the power level is off default
                  // the value of the power level to 100 so when it gets turned 
                  // on it ramps to 100%
                  zoneProperties.powerLevel = 100;
                  zoneProperties.targetPowerLevel = 100;
                  zoneProperties.state = FALSE;
                  zoneProperties.targetState = FALSE;
               }

               // json broadcast ZoneAdded
               BroadcastZoneAdded(zoneIdx);

               // Set the zone properties in the array
               SetZoneProperties(zoneIdx, &zoneProperties);

               // Add the zone to the Samsung cloud
               Eliot_QueueAddZone(zoneIdx);

               systemProperties.addALight = FALSE;

               // json broadcast SystemPropertiesChanged for addALight property
               broadcastObject = json_object();
               if (broadcastObject)
               {
                  json_object_set_new(broadcastObject, "ID", json_integer(0));
                  json_object_set_new(broadcastObject, "Service", json_string("SystemPropertiesChanged"));
   
                  BuildSystemPropertiesObject(broadcastObject, SYSTEM_PROPERTY_ADD_A_LIGHT_BITMASK);
   
                  json_object_set_new(broadcastObject, "Status", json_string("Success"));
                  SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
                  json_decref(broadcastObject);
               }
            }  // if (zoneIdx < MAX_NUMBER_OF_ZONES)

         }  // if (zoneIdx >= MAX_NUMBER_OF_ZONES)

         else  // else, we have this group in our array, so handle it
         {
            HandleTargetValue(groupId, targetValue, zoneIdx);
         }

      }  // if (global_houseID == houseId)

   }  // if (systemProperties.addALight)

   else  // we are not in Add A Light Mode
   {
      if (zoneIdx < MAX_NUMBER_OF_ZONES)
      {
         if (global_houseID == houseId)  // if we got a command from our house
         {
            HandleTargetValue(groupId, targetValue, zoneIdx);
         }
      }
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleTriggerRampCommand(json_t *root, json_t *responseObject)
{
   json_t * valueObject;
   word_t   groupId;
   word_t   value;
   Dev_Type deviceType;
   byte_t   buildingId;
   byte_t   houseId;
   byte_t   powerLevel;
   byte_t   scaledPowerLevel;
#if DEBUG_SOCKETTASK_ANY
   char                       debugBuf[100];
   MQX_TICK_STRUCT            debugLocalStartTick;
   MQX_TICK_STRUCT            debugLocalEndTick;
   bool                       debugLocalOverflow;
   uint32_t                   debugLocalOverflowMs;
#endif

   // Get the Building ID Object
   valueObject = json_object_get(root, "BuildingID");
   if(!valueObject)
   {
      // No building ID Object. Send an error response
      BuildErrorResponse(responseObject, "No BuildingID field.", SW_NO_BUILDING_ID_FIELD);
      return;
   }
   else if(!json_is_integer(valueObject))
   {
      // Building ID is not an integer. Send an error response
      BuildErrorResponse(responseObject, "BuildingID is not an integer", SW_BUILDING_ID_IS_NOT_AN_INTEGER);
      return;
   }
   else
   {
      // Building ID is valid. Get the byte value
      buildingId = (json_integer_value(valueObject) & 0xFF);
   }

   // Get the House ID Object
   valueObject = json_object_get(root, "HouseID");
   if(!valueObject)
   {
      // No house ID Object. Send an error response
      BuildErrorResponse(responseObject, "No HouseID field.", SW_NO_HOUSE_ID_FIELD);
      return;
   }
   else if(!json_is_integer(valueObject))
   {
      // House ID is not an integer. Send an error response
      BuildErrorResponse(responseObject, "HouseID is not an integer", SW_HOUSEID_IS_NOT_AN_INTEGER);
      return;
   }
   else
   {
      // House ID is valid. Get the byte value
      houseId = (json_integer_value(valueObject) & 0xFF);
   }

   // Get the group ID
   valueObject = json_object_get(root, "GroupID");
   if(!valueObject)
   {
      // No group ID Object. Send an error response
      BuildErrorResponse(responseObject, "No GroupID field", SW_NO_GROUP_ID_FIELD);
      return;
   }
   else if(!json_is_integer(valueObject))
   {
      // Group ID is not an integer. Send an error response
      BuildErrorResponse(responseObject, "GroupID is not an integer", SW_GROUPID_IS_NOT_AN_INTEGER);
      return;
   }
   else
   {
      // Group ID is valid. Get the word value
      groupId = (json_integer_value(valueObject) & 0xFFFF);
   }

   // Get the power level
   valueObject = json_object_get(root, "PowerLevel");
   if(!valueObject)
   {
      // No power level Object. Send an error response
      BuildErrorResponse(responseObject, "No PowerLevel field", SW_NO_POWER_LEVEL_FIELD);
      return;
   }
   else if(!json_is_integer(valueObject))
   {
      // Power Level Object is not an integer. Send an error response
      BuildErrorResponse(responseObject, "PowerLevel is not an integer", SW_POWER_LEVEL_IS_NOT_AN_INTEGER);
      return;
   }

   value = json_integer_value(valueObject);
   if ((value < 0) ||
       (value > MAX_POWER_LEVEL))
   {
      snprintf(jsonErrorString, sizeof(jsonErrorString), 
            "Power Level must be between 0 - %d.", MAX_POWER_LEVEL);
      BuildErrorResponse(responseObject, jsonErrorString, SW_POWER_LEVEL_MUST_BE_BETWEEN);
      return;
   }
   else
   {
      // Power level is valid. Get the byte value
      powerLevel = value & 0xFF;
   }

   // Get the device type
   valueObject = json_object_get(root, "DeviceType");
   if(!valueObject)
   {
      // No Device type object. Send an error response
      BuildErrorResponse(responseObject, "No DeviceType field", SW_NO_DEVICE_TYPE_FIELD);
      return;
   }
   else if(!json_is_integer(valueObject))
   {
      // Device Type Object is not an integer. Send an error response
      BuildErrorResponse(responseObject, "DeviceType is not an integer", SW_DEVICE_TYPE_IS_NOT_AN_INTEGER);
      return;
   }

   value = json_integer_value(valueObject);
   if ((value != DIMMER_TYPE) &&
       (value != SWITCH_TYPE) &&
       (value != SHADE_TYPE) &&
       (value != SHADE_GROUP_TYPE) &&
       (value != FAN_CONTROLLER_TYPE) &&
       (value != NETWORK_REMOTE_TYPE))
   {
      // Device Type Object does not match a valid type. Send an error response
      BuildErrorResponse(responseObject, "DeviceType is not a valid type", SW_DEVICE_TYPE_INVALID);
      return;
   }
   else
   {
      // Device Type is valid. Get the device type
      deviceType = (Dev_Type) value;
   }

   // All parameters are valid. Act like a RAMP command was received
   DEBUG_ZONEARRAY_MUTEX_START;          
   _mutex_lock(&ZoneArrayMutex);
   DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW HandletriggerRampCommand before ZoneArrayMutex",DEBUGBUF);
   DEBUG_ZONEARRAY_MUTEX_START;          

   HandleRampReceived(buildingId, houseId, groupId, powerLevel, deviceType);

   DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW HandletriggerRampCommand after HandleRampReceived",DEBUGBUF);
   _mutex_unlock(&ZoneArrayMutex);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleTriggerSceneControllerCommand(json_t *root, json_t *responseObject)
{
   json_t * valueObject;
   byte_t buildingId;
   byte_t houseId;
   word_t groupId;
   byte_t bankIndex;
   byte_t sceneControllerIndex;
   bool_t state;
   Dev_Type deviceType;

   // Get the Building ID Object
   valueObject = json_object_get(root, "BuildingID");
   if(!valueObject)
   {
      // No building ID Object. Send an error response
      BuildErrorResponse(responseObject, "No BuildingID field.", SW_NO_BUILDING_ID_FIELD);
      return;
   }
   else if(!json_is_integer(valueObject))
   {
      // Building ID is not an integer. Send an error response
      BuildErrorResponse(responseObject, "BuildingID is not an integer", SW_BUILDING_ID_IS_NOT_AN_INTEGER);
      return;
   }
   else
   {
      // Building ID is valid. Get the byte value
      buildingId = (json_integer_value(valueObject) & 0xFF);
   }

   // Get the House ID Object
   valueObject = json_object_get(root, "HouseID");
   if(!valueObject)
   {
      // No house ID Object. Send an error response
      BuildErrorResponse(responseObject, "No HouseID field.", SW_NO_HOUSE_ID_FIELD);
      return;
   }
   else if(!json_is_integer(valueObject))
   {
      // House ID is not an integer. Send an error response
      BuildErrorResponse(responseObject, "HouseID is not an integer", SW_HOUSEID_IS_NOT_AN_INTEGER);
      return;
   }
   else
   {
      // House ID is valid. Get the byte value
      houseId = (json_integer_value(valueObject) & 0xFF);
   }

   // Get the group ID
   valueObject = json_object_get(root, "GroupID");
   if(!valueObject)
   {
      // No group ID Object. Send an error response
      BuildErrorResponse(responseObject, "No GroupID field", SW_NO_GROUP_ID_FIELD);
      return;
   }
   else if(!json_is_integer(valueObject))
   {
      // Group ID is not an integer. Send an error response
      BuildErrorResponse(responseObject, "GroupID is not an integer", SW_GROUPID_IS_NOT_AN_INTEGER);
      return;
   }
   else
   {
      // Group ID is valid. Get the word value
      groupId = (json_integer_value(valueObject) & 0xFFFF);
   }

   // Get the bank index pushed
   valueObject = json_object_get(root, "BankIndex");
   if(!valueObject)
   {
      // No bank index pushed Object. Send an error response
      BuildErrorResponse(responseObject, "No BankIndex field", SW_NO_BANK_INDEX_FIELD);
      return;
   }
   else if(!json_is_integer(valueObject))
   {
      // Bank Index Object is not an integer. Send an error response
      BuildErrorResponse(responseObject, "BankIndex is not an integer", SW_BANKINDEX_IS_NOT_INTEGER);
      return;
   }
   else if((json_integer_value(valueObject) < 0) ||
         (json_integer_value(valueObject) > (MAX_SCENES_IN_SCENE_CONTROLLER-1)))
   {
      snprintf(jsonErrorString, sizeof(jsonErrorString), 
            "BankIndex must be between 0 - %d.", MAX_SCENES_IN_SCENE_CONTROLLER-1);
      BuildErrorResponse(responseObject, jsonErrorString, SW_BANKINDEX_MUST_BE_BETWEEN);
      return;
   }
   else
   {
      // BankIndex is valid. Get the byte value
      bankIndex = (json_integer_value(valueObject) & 0xFF);
   }

   // Get the device type
   valueObject = json_object_get(root, "DeviceType");
   if(!valueObject)
   {
      // No Device type object. Send an error response
      BuildErrorResponse(responseObject, "No DeviceType field", SW_NO_DEVICE_TYPE_FIELD);
      return;
   }
   else if(!json_is_integer(valueObject))
   {
      // Device Type Object is not an integer. Send an error response
      BuildErrorResponse(responseObject, "DeviceType is not an integer", SW_DEVICE_TYPE_IS_NOT_AN_INTEGER);
      return;
   }
   else if(json_integer_value(valueObject) != SCENE_CONTROLLER_4_BUTTON_TYPE)
   {
      // Device Type Object does not match a valid type. Send an error response
      BuildErrorResponse(responseObject, "DeviceType is not a valid type", SW_DEVICE_TYPE_INVALID);
      return;
   }
   else
   {
      // Device Type is valid. Get the device type
      deviceType = (Dev_Type)(json_integer_value(valueObject));
   }
   
   // Get the state of the button. 1 or 0
   valueObject = json_object_get(root, "State");
   if(!valueObject)
   {
      // No State for the scene controller. Send an error response
      BuildErrorResponse(responseObject, "No State field", SW_NO_STATE_FIELD);
      return;
   }
   else if(!json_is_boolean(valueObject))
   {
      // State is not an integer. Send an error response
      BuildErrorResponse(responseObject, "State is not boolean", SW_STATE_IS_NOT_BOOLEAN);
      return;
   }
   else
   {
      // Set the state
      state = json_is_true(valueObject);
   }

   // Determine if this scene controller is already known
   sceneControllerIndex = GetSceneControllerIndex(groupId, buildingId, houseId);

   // All parameters are valid. Act like a scene controller command was received
   HandleSceneControllerSceneRequest(buildingId, houseId, groupId, bankIndex, state, deviceType,0);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleStatusReceived(byte_t buildingId, byte_t houseId, word_t groupId, byte_t targetLevel, byte_t deviceType)
{
   // TODO
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleTriggerRampAllCommand(json_t *root, json_t *responseObject)
{
   json_t *   valueObject;
   byte_t     powerLevel,
              buildingId = 0, //Use wildcards for BuildingID and RoomID
              roomId = 0,
              houseId = global_houseID;
   json_int_t value;
   
#if DEBUG_SOCKETTASK_ANY
   char                       debugBuf[100];
   MQX_TICK_STRUCT            debugLocalStartTick;
   MQX_TICK_STRUCT            debugLocalEndTick;
   bool                       debugLocalOverflow;
   uint32_t                   debugLocalOverflowMs;
#endif

   // Get the power level
   valueObject = json_object_get(root, "PowerLevel");
   if(!valueObject)
   {
      // No power level Object. Send an error response
      BuildErrorResponse(responseObject, "No PowerLevel field", SW_NO_POWER_LEVEL_FIELD);
      return;
   }
   else if(!json_is_integer(valueObject))
   {
      // Power Level Object is not an integer. Send an error response
      BuildErrorResponse(responseObject, "PowerLevel is not an integer", SW_POWER_LEVEL_IS_NOT_AN_INTEGER);
      return;
   }

   value = json_integer_value(valueObject);
   if ((value < 0) || (value > MAX_POWER_LEVEL))
   {
      snprintf(jsonErrorString, sizeof(jsonErrorString), "Power Level must be between 0 - %d.", MAX_POWER_LEVEL);
      BuildErrorResponse(responseObject, jsonErrorString, SW_POWER_LEVEL_MUST_BE_BETWEEN);
      return;
   }
   else
   {
      // Power level is valid. Get the byte value
      powerLevel = value & 0xFF;
   }

   // All parameters are valid. Act like a RAMP command was received
   DEBUG_ZONEARRAY_MUTEX_START;          
   _mutex_lock(&ZoneArrayMutex);
   DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW HandleTriggerRampAllCommand before ZoneArrayMutex ",DEBUGBUF);
   DEBUG_ZONEARRAY_MUTEX_START;          

   //Send Ramp Command
   BuildRampAll(buildingId, roomId, houseId, powerLevel);

   //Update All Zones to New properties
   HandleTargetValueAll(powerLevel);

   DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW HandleTriggerRampAllCommand after HandleRampReceived ",DEBUGBUF);
   _mutex_unlock(&ZoneArrayMutex);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleTargetValueAll(byte_t targetValue)
{
   dword_t announcePropertyBitmask = PWR_BOOL_PROPERTIES_BITMASK | 
                                     PWR_LVL_PROPERTIES_BITMASK;
   zone_properties zoneProperties;

#if DEBUG_SOCKETTASK_ANY
   char                       debugBuf[100];
   MQX_TICK_STRUCT            debugLocalStartTick;
   MQX_TICK_STRUCT            debugLocalEndTick;
   bool                       debugLocalOverflow;
   uint32_t                   debugLocalOverflowMs;
#endif

   for (int zoneIdx = 0; zoneIdx < MAX_NUMBER_OF_ZONES; zoneIdx++)
   {
      if (GetZoneProperties(zoneIdx, &zoneProperties))
      {
         if (zoneProperties.ZoneNameString[0] != 0 && (zoneProperties.deviceType == SWITCH_TYPE || zoneProperties.deviceType == DIMMER_TYPE))
         {
            DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"HandleTargetValueAll powerLevel for zoneIndex %d changed to %d", zoneIdx, targetValue, 0);
            if (zoneProperties.deviceType == SWITCH_TYPE)
            {
               //If Switch, power is either 100% or 0%
               if (targetValue > 49)
               {
                  zoneProperties.state = TRUE;
                  zoneProperties.targetState = TRUE;
                  zoneProperties.powerLevel = 100;
                  zoneProperties.targetPowerLevel = 100;
               }
               else
               {
                  zoneProperties.state = FALSE;
                  zoneProperties.targetState = FALSE;
               }
            }
            else // Dimmers
            {
               zoneProperties.state = (targetValue > 0);
               zoneProperties.targetState = (targetValue > 0);

               if (targetValue == 100)
               {
                  zoneProperties.powerLevel = targetValue;
                  zoneProperties.targetPowerLevel = targetValue;
               }
            }

            // Set the zone properties in the array
            SetZoneProperties(zoneIdx, &zoneProperties);

            //Broadcast that zone Properties Changed
            json_t *broadcastObject = json_object();
            if (broadcastObject)
            {
               json_object_set_new(broadcastObject, "ID", json_integer(0));
               json_object_set_new(broadcastObject, "Service", json_string("ZonePropertiesChanged"));
      
               // build PropertyList based off of announcePropertyBitmask
               BuildZonePropertiesObject(broadcastObject, zoneIdx, announcePropertyBitmask);
      
               //broadcast zonePropertiesChanged with created PropertyList
               json_object_set_new(broadcastObject, "Status", json_string("Success"));
               SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
               json_decref(broadcastObject);
            }

            // Send the zone property changed to the Eliot task
            Eliot_QueueZoneChanged(zoneIdx);
         }
      }
   }
}
