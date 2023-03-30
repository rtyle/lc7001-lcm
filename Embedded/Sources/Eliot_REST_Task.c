/*
 * Eliot_REST_Task.c
 *
 *  Created on: Mar 7, 2019
 *      Author: Legrand
 */

#include "Eliot_REST_Task.h"
#include "String_Sanitizer.h"
#include "jansson_private.h"
#include "wolfssl/ssl.h"
#include "wolfssl/error-ssl.h"
#include "includes.h"
#include "Eliot_Common.h"
#include "Eliot_MQTT_Task.h"
#include "jsr.h"

#define ELIOT_LCM_MODULE_NAME        "LCM"
#define ELIOT_LCM_PLANT_NAME         "LC7001"
#define ELIOT_LCM_PLANT_TYPE         "house"
#define ELIOT_COUNTRY                "USA"
#define ELIOT_API_KEY                "KEY"
#define ELIOT_HTTP_BASE              "api.developer.legrand.com"

#define DEVICE_MGMT_MODULES_API      "/devicemanagement/api/v2.0/modules"
#define SERVICE_CATALOG_MODULES_API  "/servicecatalog/api/v3.0/modules"
#define SERVICE_CATALOG_PLANTS_API   "/servicecatalog/api/v3.0/plants"

#define HEADER_SIZE                  1500
#define REQUEST_SIZE                 HEADER_SIZE + 250

#define ELIOT_REST_SERVICE           "Eliot REST"

#define ELIOT_INITIAL_TASK_DELAY_MS  1000
#define ELIOT_WRITE_TIMEOUT_TICKS    5000
#define ELIOT_WANT_READ_RETRY_DELAY  10
#define ELIOT_SSL_SHUTDOWN_RETRIES   100
#define ELIOT_SSL_CONNECT_RETRIES    500
#define ELIOT_POST_CONNECT_DELAY_MS  50
#define ELIOT_POST_WRITE_DELAY_MS    50
#define ELIOT_PRE_RECEIVE_DELAY_MS   350
#define ELIOT_RECEIVE_RETRY_DELAY_MS 100
#define ELIOT_RECEIVE_MAX_RETRIES    200
#define ELIOT_CONNECT_RETRY_DELAY_MS 250
#define ELIOT_CONNECT_ATTEMPTS		 10
#define ELIOT_RENAME_ATTEMPTS        5
#define ELIOT_RANDOM_DEVICE_ENUM	 0	// 1==Random, 0==Sequential
#define ELIOT_LAST_TAGS_DELAY_MS     1000
#define ELIOT_LAST_SYNC_DELAY_MS     1000 * 60 * 60 * 24 // 24 hours

#define DEBUG_ELIOT_HTTP_ERR_LEN     64


#define ELIOT_CLOUD_SWITCH           "RFLC-switch"
#define ELIOT_CLOUD_DIMMER           "RFLC-dimmer"
#define ELIOT_CLOUD_FAN              "RFLC-fan"
#define LC7001_SCENE                 "RFLC-scene"
#define LC7001_HUB                   "RFLC-LCM"
#define ELIOT_CLOUD_SHADE            "RFLC-shade"
#define ELIOT_CLOUD_SHADE_GROUP      "RFLC-shade-group"

#define ELIOT_SERIAL_NUMBER_BUFFER_SIZE 50

static bool_t initialized = false;


char *eliot_primary_key = eliot_empty_string;
char eliot_lcm_plant_id[ELIOT_DEVICE_ID_SIZE];
char eliot_lcm_module_id[ELIOT_DEVICE_ID_SIZE];

uint32_t eliot_loop_counter = 0;
uint32_t eliot_user_token_modified_time = 0;
uint32_t eliot_refresh_token_error_time = 0;
uint32_t eliot_refresh_token_modified_time = 0;
uint32_t eliot_last_quota_exceeded_time = 0;

#define eliot_api_key ELIOT_API_KEY

static boolean ZoneChangedArray[MAX_NUMBER_OF_ZONES] = { 0 };
static boolean SceneChangedArray[MAX_NUMBER_OF_SCENES] = { 0 };

// This function conditionally assigns the address of a pointer to a
// string stored in flash if the strlen() falls within a defined window.
// This allows string pointers at startup to point to an empty string,
// preventing functions from using junk data.
uint32_t Eliot_UseFlashString(char **ptr, uint32_t offset, uint32_t min_length, uint32_t max_length)
{
	char* address = FBK_Address(&eliot_fbk, offset);
	uint32 length = _strnlen(address, max_length+2);

	if( length<=max_length && length>=min_length )
	{
		DBGPRINT_INTSTRSTR(offset, "Eliot_UseFlashString()", "Using data at offset:");
		*ptr = address;
		return ELIOT_RET_OK;
	}
	else
	{
		DBGPRINT_INTSTRSTR(offset, "Eliot_UseFlashString()", "Discarding data at offset:");
		return ELIOT_RET_RESOURCE_NOT_FOUND;
	}
}


// Eliot_LogOut() clears account-specific data in flash and RAM.
// The user may then log into the same or different Eliot account.
void Eliot_LogOut(void)
{
	DBGPRINT_INTSTRSTR(0, "Eliot_LogOut()", "");

	// Strings in RAM:
	eliot_lcm_module_id[0] = 0;
	eliot_user_token[0] = 0;
	eliot_event_topic[0] = 0;

	// Clear the refresh token in flash if not already clear
	if(eliot_refresh_token[0])
	{
		FBK_WriteFlash(&eliot_fbk, (char*)"", LCM_DATA_REFRESH_TOKEN_OFFSET, 1);
	}

	// Reassign the pointers once all flash writes are complete
	eliot_refresh_token = FBK_Address(&eliot_fbk, LCM_DATA_REFRESH_TOKEN_OFFSET);

	// Invalidate last event time from previous login.
	eliot_mqtt_event_last = 0;
}


uint32_t Eliot_TimeNow(void)
{
	TIME_STRUCT mqxTime;
	_time_get(&mqxTime);
	return mqxTime.SECONDS;
}

int wrap_wolfSSL_write(WOLFSSL* sslPtr, const void* packet, int length)
{
	MQX_TICK_STRUCT   startTickCount;
	MQX_TICK_STRUCT   newTickCount;
	int_32            ticksDelta;
	int               bytesSent;
	bool              overflowFlag;
	bool_t            doneFlag = false;

	_time_get_elapsed_ticks(&startTickCount);
	while (!doneFlag)
	{
		bytesSent = wolfSSL_write(sslPtr, packet, length);
		if (bytesSent > 0)
		{
			doneFlag = true;
		}
		else
		{
			int writeError = wolfSSL_get_error(sslPtr, bytesSent);

			if ((writeError != SSL_ERROR_WANT_READ) && (writeError != SSL_ERROR_WANT_WRITE))
			{
				// Socket was closed or there was an error. Shutdown the connections
				doneFlag = true;
				bytesSent = 0;
			}
			else
			{
				_time_get_elapsed_ticks(&newTickCount);
				ticksDelta = _time_diff_milliseconds(&newTickCount, &startTickCount, &overflowFlag);
				if (ticksDelta > 1000)
				{
					doneFlag = true;
					bytesSent = 0;
				}
			}
		}
	}

	return bytesSent;
}


static void cleanupSockets(WOLFSSL** sslPtr, WOLFSSL_CTX** contextPtr, uint32_t* socketPtr)
{
	eliot_error_count.open_sockets--;
	int32_t shutdown_ret = 0;
	int32_t shutdown_retries = -1;

	if(*sslPtr)
	{
		do
		{
			shutdown_ret = wolfSSL_shutdown(*sslPtr);
			shutdown_retries++;
			_time_delay(ELIOT_WANT_READ_RETRY_DELAY);
		} while( (shutdown_ret==SSL_SHUTDOWN_NOT_DONE) && (shutdown_retries<ELIOT_SSL_SHUTDOWN_RETRIES) );

		if(shutdown_ret != SSL_SHUTDOWN_NOT_DONE)
		{
			DBGPRINT_INTSTRSTR(shutdown_retries, "wolfSSL_shutdown() succeeded after retries", "");
		}
		else
		{
			DBGPRINT_ERRORS_INTSTRSTR(shutdown_retries, "wolfSSL_shutdown() failed after retries", "");
		}

		wolfSSL_free(*sslPtr);
		*sslPtr = NULL;
	}

	if(contextPtr)
	{
		wolfSSL_CTX_free(*contextPtr);
		*contextPtr = NULL;
	}

	if(*socketPtr != RTCS_SOCKET_ERROR)
	{
		shutdown(*socketPtr, FLAG_ABORT_CONNECTION);
		*socketPtr = RTCS_SOCKET_ERROR;
	}

}


static void Eliot_Initialize()
{
	if(!initialized)
	{
		wolfSSL_Debugging_ON();
		wolfSSL_Init();
		eliot_lcm_plant_id[0] = 0;    
		eliot_lcm_module_id[0] = 0;
		eliot_device_name[0] = 0;
		eliot_username[0] = 0;
		eliot_msgs_topic_name[0] = 0;
		eliot_event_topic[0] = 0;
		eliot_user_token[0] = 0;

		initialized = true;
	}
}


static bool_t Eliot_Connect(WOLFSSL** sslPtr, WOLFSSL_CTX** contextPtr, uint32_t* socketPtr)
{
	uint32_t result = 0;
	uint8_t retryCount = 0;
	_ip_address hostaddr = 0;
	char ipName[MAX_HOSTNAMESIZE];
	bool_t connected = false;
	*sslPtr = NULL;
	*contextPtr = NULL;
	uint_32 opt_value;
	uint_32 error;
	eliot_error_count.open_sockets++;

	Eliot_Initialize();
	logConnectionSequence(COMSTATE_INIT, "Initializing");

	//Log Connection and set Sequence to 0
	logConnectionSequence(COMSTATE_GET_IP, "Getting IP Address.");

    // Increment this task's watchdog counter
    watchdog_task_counters[ELIOT_REST_TASK]++;

	// Get the IP Address
	while((hostaddr == 0) &&
			(retryCount < MAX_NUMBER_OF_IP_ADDRESS_RESOLUTION_ATTEMPTS))
	{
		switch(*socketPtr)	// Use the socketPtr parameter to specify an alternate host
		{
			case ELIOT_CONNECT_ALT_HOST_TOKEN_REFRESH:
				DBGPRINT_INTSTRSTR(0, "Eliot_Connect() Alternate host:", ELIOT_TOKEN_REFRESH_HOST);
				RTCS_resolve_ip_address(ELIOT_TOKEN_REFRESH_HOST, &hostaddr, ipName, MAX_HOSTNAMESIZE);
				break;

			default:
				RTCS_resolve_ip_address(ELIOT_HTTP_BASE, &hostaddr, ipName, MAX_HOSTNAMESIZE);
				break;
		}
		retryCount++;
	}

	*socketPtr = RTCS_SOCKET_ERROR;


	if(hostaddr != 0)
	{
		// Address was successfully resolved
		logConnectionSequence(COMSTATE_GOT_IP, "Address Successfully Resolved.");
		*socketPtr = socket(AF_INET, SOCK_STREAM, 0);

		if(*socketPtr != RTCS_SOCKET_ERROR)
		{
			// Valid socket handle returned. Set the socket parameters
			struct sockaddr_in eliotServerAddr;
			memset((char *)&eliotServerAddr, 0, sizeof(struct sockaddr_in));
			eliotServerAddr.sin_family = PF_INET;
			eliotServerAddr.sin_addr.s_addr = hostaddr;
			eliotServerAddr.sin_port = 443;

			// Connect the socket
			logConnectionSequence(COMSTATE_GET_SOCKET, "Connecting Socket.");

			result = connect(*socketPtr, (struct sockaddr *)&eliotServerAddr, sizeof(struct sockaddr_in));

			uint32_t optValue = true;
			setsockopt(*socketPtr, SOL_TCP, OPT_SEND_NOWAIT, &optValue, sizeof(optValue));
			optValue = true;
			setsockopt(*socketPtr, SOL_TCP, OPT_RECEIVE_NOWAIT, &optValue, sizeof(optValue)); // SamF fix Artik issue

			if(result == RTCS_OK)
			{
				// Connection successful. Create and connect the SSL socket
				logConnectionSequence(COMSTATE_GET_SSL, "Connecting SSL");
				
				*contextPtr = wolfSSL_CTX_new(wolfTLSv1_2_client_method());

				if(*contextPtr != NULL)
				{
					*sslPtr = wolfSSL_new(*contextPtr);

					if(*sslPtr != NULL)
					{
						// Assign the file descriptor to the SSL session
						if(wolfSSL_set_fd(*sslPtr, *socketPtr) == SSL_SUCCESS)
						{
							wolfSSL_set_verify(*sslPtr, SSL_VERIFY_NONE, 0);

                            // Connect to the session.  Keep retrying as long as we get recoverable errors.
                            // As soon as we get an unrecoverable error, stop trying to connect.
                            bool_t retryConnection = false;
                            int sslConnectRetryCount = 0;
                            
                            do
                            {
                                retryConnection = false;
                                int connectError = SSL_ERROR_NONE;
                                int connectStatus = wolfSSL_connect(*sslPtr);

                                if(connectStatus == SSL_SUCCESS)
                                {
                                    connected = true;
                                }
                                else
                                {
                                    connectError = wolfSSL_get_error(*sslPtr, connectStatus);

                                    if ((connectError == SSL_ERROR_WANT_READ) || (connectError == SSL_ERROR_WANT_WRITE))
                                    {
                                        DBGPRINT_INTSTRSTR(connectError,"Eliot_Connect wolfSSL_connect failed with recoverable error code, retrying","");
                                        retryConnection = true;
                                        sslConnectRetryCount++;
                                        _time_delay(ELIOT_WANT_READ_RETRY_DELAY);
                                    }
                                    else
                                    {
                                        DBGPRINT_ERRORS_INTSTRSTR(connectError,"Eliot_Connect wolfSSL_connect failed with UN-recoverable error code","");
                                    }
                                }
                            } while (retryConnection && (sslConnectRetryCount<ELIOT_SSL_CONNECT_RETRIES) );

                            if(connected)
                            {
                                DBGPRINT_INTSTRSTR(sslConnectRetryCount,"Eliot_Connect wolfSSL_connect succeeded after retries","");
                                logConnectionSequence(COMSTATE_GOT_SSL, "SSL Connected!");
                            }
                            else
                            {
                                DBGPRINT_ERRORS_INTSTRSTR(sslConnectRetryCount,"Eliot_Connect wolfSSL_connect failed after retries","");
                            }
						}
						else
						{
							DBGPRINT_ERRORS_INTSTRSTR(__LINE__,"Eliot_Connect wolfSSL_set_fd failed","");
							eliot_error_count.wolfssl_set_fd++;
						}
					}
					else
					{
						DBGPRINT_ERRORS_INTSTRSTR(__LINE__,"Eliot_Connect wolfSSL_new failed","");
						eliot_error_count.wolfssl_new++;
					}
				}
				else
				{
					DBGPRINT_ERRORS_INTSTRSTR(__LINE__,"Eliot_Connect wolfSSL_CTX_new failed","");
					eliot_error_count.wolfssl_ctx++;
				}
			}
			else
			{
				DBGPRINT_ERRORS_INTSTRSTR(__LINE__,"Eliot_Connect connect failed","");
				eliot_error_count.connect++;
			}
		}
		else
		{
			DBGPRINT_ERRORS_INTSTRSTR(__LINE__,"Eliot_Connect socket creation failed","");
			eliot_error_count.socket_err++;
		}
	}
	else
	{
		DBGPRINT_ERRORS_INTSTRSTR(retryCount,"Eliot_Connect RTCS_resolve_ip_address failed",ipName);
		eliot_error_count.dns++;
	}

	if(!connected)
	{
		DBGPRINT_ERRORS_INTSTRSTR(retryCount,"Eliot_Connect failed",ipName);
		eliot_error_count.eliot_connect++;
		cleanupSockets(sslPtr, contextPtr, socketPtr);
	}
	else if (ELIOT_POST_CONNECT_DELAY_MS)
	{
		_time_delay(ELIOT_POST_CONNECT_DELAY_MS);
	}

	return connected;
}


// Wrapper for Eliot_Connect() - Retry on failed connection
static bool_t Eliot_ConnectWithRetry(WOLFSSL** sslPtr, WOLFSSL_CTX** contextPtr, uint32_t* socketPtr)
{
	bool_t connected = false;
	uint32_t delay = ELIOT_CONNECT_WITH_RETRY_INITIAL_DELAY;
	uint32_t attempts = ELIOT_CONNECT_WITH_RETRY_ATTEMPTS;

	while(!(connected = Eliot_Connect(sslPtr, contextPtr, socketPtr)))
	{
		DBGPRINT_ERRORS_INTSTRSTR(delay, "Eliot_ConnectWithRetry()", "Retry time (ms)");
		_time_delay(delay);

		attempts--;

		if(!attempts)
		{
			DBGPRINT_ERRORS_INTSTRSTR(__LINE__, "Eliot_ConnectWithRetry()", "Giving up");
			break;
		}

		delay <<= 1;	// Double the retry delay with every attempt
	}

	return connected;
}


static bool_t Eliot_SendBytes(WOLFSSL* ssl, char* bytesToSend)
{
	MQX_TICK_STRUCT   startTickCount;
	MQX_TICK_STRUCT   newTickCount;
	int_32            ticksDelta = 0;
	uint32_t       	  totalBytesSent = 0;
	uint32_t       	  totalBytesToSend = strlen(bytesToSend);
	bool              overflowFlag;
	bool_t         	  bytesSent = false;

	_time_get_elapsed_ticks(&startTickCount);
	// If we miss a ping we will recover, not always waiting 5, should be very quick usually
	while ((totalBytesSent < totalBytesToSend) && (ticksDelta < ELIOT_WRITE_TIMEOUT_TICKS))
	{
		int numBytesSent = wrap_wolfSSL_write(ssl, (bytesToSend + totalBytesSent), (totalBytesToSend - totalBytesSent));

		if(numBytesSent == -1)
		{
			bytesSent = false;
			break;
		}

		totalBytesSent += numBytesSent;
		if (totalBytesSent < totalBytesToSend)
		{
			_time_get_elapsed_ticks(&newTickCount);
			ticksDelta = _time_diff_milliseconds(&newTickCount, &startTickCount, &overflowFlag);
			_time_delay(ELIOT_POST_WRITE_DELAY_MS); // prevent tight busy loop
		}
	}

	if(totalBytesSent == totalBytesToSend)
	{
		bytesSent = true;
		DBGPRINT_INTSTRSTR((int)ticksDelta,"Eliot_SendBytes succeeded after num ticks","");
	}
	else if (DEBUG_ELIOT_ANY || DEBUG_JUMPER)
	{
		char debugBuf[100];
		DBGSNPRINTF(DEBUGBUF, SIZEOF_DEBUGBUF,"Eliot_SendBytes failed after sending %d of %d bytes in %d ticks", totalBytesSent, totalBytesToSend, ticksDelta);
		eliot_error_count.send_bytes++;
		DBGPRINT_ERRORS_INTSTRSTR((int)ticksDelta,DEBUGBUF,"");
	}

	return bytesSent;
}


// Eliot_ReceiveStatus returns the HTTP status code as an integer.
// This is useful in cases where the response body is not needed.

static int32_t Eliot_ReceiveStatus(WOLFSSL* ssl)
{
	int32_t bytes_read=0;
	int32_t total_bytes_read=0;
	int32_t status_code=0;
	int32_t retries=ELIOT_RECEIVE_MAX_RETRIES;
	char response_buffer[16];

	do
	{
		bytes_read = wolfSSL_read(ssl, response_buffer+total_bytes_read, sizeof(response_buffer)-total_bytes_read);
		retries--;
		if(bytes_read > 0)
		{
			total_bytes_read += bytes_read;
		}
		else
		{
			_time_delay(ELIOT_RECEIVE_RETRY_DELAY_MS);
		}
	}while(bytes_read && total_bytes_read<sizeof(response_buffer) && retries>0);

	if(!retries)
	{
		eliot_error_count.response_timeouts++;
		return -1;
	}

	if(total_bytes_read < sizeof(response_buffer))
	{
		return -2;
	}

	// Parse the status code.  For HTTP 1.1, the three digit code string spans characters 9-11.
	response_buffer[12]=0;  // Terminate the string after the status code for atoi().
	status_code=atoi(&response_buffer[9]);  // Begin parsing at character 9.

	DBGPRINT_INTSTRSTR(status_code, "Eliot_ReceiveStatus", response_buffer);
	return status_code;
}


static int32_t Eliot_ReceiveBytes(WOLFSSL* ssl, char* buf, uint32_t buffer_size, bool_t header)
{
	char response_buffer[buffer_size];
	int bytes_read = SSL_SUCCESS;
	size_t copy_length = 0;

	while ((bytes_read = wolfSSL_read(ssl, response_buffer, buffer_size)) == -1)
	{
		if (wolfSSL_want_read(ssl))
		{
			continue;
		}

		return bytes_read;
	}

	char* response = strstr(response_buffer, "\r\n\r\n");

	if (header) {
	    size_t header_length = strlen(response);
	    copy_length = header_length > buffer_size ? buffer_size : header_length;
	    strncpy(buf, response_buffer, copy_length);
	} else {
	    response += 4;

	    size_t body_length = bytes_read - (response - response_buffer);
	    copy_length = body_length > buffer_size ? buffer_size : body_length;
	    strncpy(buf, response, copy_length);
	}

	return bytes_read;
}

bool_t Eliot_SendHeader(WOLFSSL* ssl, char* requestType, char* resource, int messageLen)
{
	// Return true if every Eliot_SendBytes() call returns true
	char len_str[8];
	bool_t ret = true;
	
	ret &= Eliot_SendBytes(ssl, requestType);
	ret &= Eliot_SendBytes(ssl, " https://");
	ret &= Eliot_SendBytes(ssl, ELIOT_HTTP_BASE);
	ret &= Eliot_SendBytes(ssl, resource);
	ret &= Eliot_SendBytes(ssl, " HTTP/1.1\r\n");
	
	ret &= Eliot_SendBytes(ssl, "Host: ");
	ret &= Eliot_SendBytes(ssl, ELIOT_HTTP_BASE);
	ret &= Eliot_SendBytes(ssl, "\r\n");
	
	ret &= Eliot_SendBytes(ssl, "Authorization: Bearer ");
	ret &= Eliot_SendBytes(ssl, eliot_user_token);
	ret &= Eliot_SendBytes(ssl, "\r\n");
	
	ret &= Eliot_SendBytes(ssl, "Content-Type: application/json\r\nOcp-Apim-Subscription-Key: ");
	ret &= Eliot_SendBytes(ssl, eliot_api_key);
	ret &= Eliot_SendBytes(ssl, "\r\n");
	
	ret &= Eliot_SendBytes(ssl, "Content-Length: ");
	snprintf(len_str, sizeof(len_str), "%d\r\n\r\n", messageLen);
	ret &= Eliot_SendBytes(ssl, len_str);
	
	return ret;
}


// Eliot_SendBody is used to determine the length of message body fragments
// when mode==0 (summed for Content-Length HTTP header), then to send
// the content through a socket when mode!=0.  Using this function in
// a two iteration loop ensures consistent content lengths without having 
// to assemble large strings in RAM.

int32_t Eliot_SendBody(unsigned char mode, WOLFSSL* ssl, char* data)
{
	if(!mode)
	{
		return strlen(data);
	}
	else
	{
		return (int)Eliot_SendBytes(ssl, data);
	}
}

void Eliot_GenerateUniqueSerialNumber(int format, char* serial, int bufferSize)
{
    switch (format)
    {
        case SERIAL_NUMBER_FORMAT_0:
            // Old format uses MAC address plus current time to make the serial number unique

            snprintf(serial, bufferSize, "%02X%02X%02X%02X%02X%02X-%d",
                myMACAddress[0],
                myMACAddress[1],
                myMACAddress[2],
                myMACAddress[3],
                myMACAddress[4],
                myMACAddress[5],
                Eliot_TimeNow());
            break;

        default:

	        // The current format uses the K64 processor's unique ID
        	snprintf(serial, bufferSize, "%s-%08X%08X%08X%08X", LC7001_HUB, SIM_UIDH, SIM_UIDMH, SIM_UIDML, SIM_UIDL);
        	break;
	}
}


// Eliot_SendAsyncCommand() sends a command to the LCM module.  The command_string
// argument may be set to zero to send an empty command:  { "command": {} }
int32_t Eliot_SendAsyncCommand(char* command_string)
{
	WOLFSSL*             ssl;
	WOLFSSL_CTX*         ctx;
	uint32_t             socket = 0;

	int32_t ret = ELIOT_RET_UNDEFINED;
	uint8_t mode = 0;
	int32_t rx_count = 0;
	int32_t total_read = 0;
	uint16_t message_length = 0;
	int16_t timeout_limit = ELIOT_RECEIVE_MAX_RETRIES;
	int16_t timeout_counter = 0;

	char rx_buf[32];
	char url[128];

	if(!eliot_lcm_module_id[0] || !eliot_user_token[0])
	{
		DBGPRINT_INTSTRSTR(__LINE__, "Eliot_SendAsyncCommand() missing LCM module ID or user token", "");
		return ELIOT_RET_INPUT_ERROR;
	}

	snprintf(url, sizeof(url), "%s/%s/commands", DEVICE_MGMT_MODULES_API, eliot_lcm_module_id);

	if(Eliot_ConnectWithRetry(&ssl, &ctx, &socket) )
	{
		for(mode=0; mode<=1; mode++)
		{
			// mode==0 sums the string lengths in message_length
			// mode==1 sends the strings
			message_length += Eliot_SendBody(mode, ssl, "{ \"command\": {");

			if(command_string)
			{
				message_length += Eliot_SendBody(mode, ssl, command_string);
			}

			message_length += Eliot_SendBody(mode, ssl, "}}");

			if(!mode)
			{
				Eliot_SendHeader(ssl, "POST", url, message_length);
			}
		}

		do
		{
			rx_count = wolfSSL_read(ssl, rx_buf, sizeof(rx_buf));

			if(rx_count > 0)
			{
				total_read += rx_count;
				timeout_counter = 0;
			}
			else
			{
				_time_delay(ELIOT_RECEIVE_RETRY_DELAY_MS);
				timeout_counter++;
			}
		} while( (rx_count > 0 || !total_read)  && (timeout_counter<timeout_limit) );

		// Cleanup sockets
		cleanupSockets(&ssl, &ctx, &socket);

		if(command_string)
		{
			DBGPRINT_INTSTRSTR(total_read, "Eliot_SendAsyncCommand() total_read:", command_string);
		}
		else
		{
			DBGPRINT_INTSTRSTR(total_read, "Eliot_SendAsyncCommand() total_read:", "");
		}
	}
	else
	{
		return ELIOT_RET_CONNECT_FAILURE;
	}
	return ELIOT_RET_OK;
}


// Eliot_AddLcmModule() creates a module for the LCM under the
// LCM plant.  The LCM plant ID must be set before this function
// will succeed.

int32_t Eliot_AddLcmModule(bool includeMAC)
{
	WOLFSSL*             ssl;
	WOLFSSL_CTX*         ctx;
	uint32_t             socket = 0;
	
	int32_t ret = ELIOT_RET_UNDEFINED;
	uint16_t messageLen = 0;
	uint8_t mode = 0;
	int32_t rx_count = 0;
	int32_t total_read = 0;
	int16_t timeout_limit = ELIOT_RECEIVE_MAX_RETRIES;
	int16_t timeout_counter = 0;
	char rx_buf[ELIOT_JSR_BLOCK_SIZE];
	char temp_key[16];
	char temp_value[64];
	char mac_string[20];
	char version_string[ELIOT_MAX_VERSION_SIZE + 1];
	char default_mac_string[20];
	char str_message[DEBUG_ELIOT_HTTP_ERR_LEN];
	char primary_key[ELIOT_PRIMARY_KEY_SIZE];
	JSR_PAIR pair[2];
	JSR_ENV jsr;

	str_message[0] = 0; // Clear the error message string
	primary_key[0] = 0; // Clear the primary key string

	// Eliot requires colon delimiters
	snprintf(mac_string, sizeof(mac_string), "%02X:%02X:%02X:%02X:%02X:%02X",
	    myMACAddress[0],
	    myMACAddress[1],
	    myMACAddress[2],
	    myMACAddress[3],
	    myMACAddress[4],
	    myMACAddress[5]);

    uint8_t default_mac[] = DEFAULT_MAC_ADDRESS;
	snprintf(default_mac_string, sizeof(default_mac_string), "%02X:%02X:%02X:%02X:%02X:%02X",
	    default_mac[0],
	    default_mac[1],
	    default_mac[2],
	    default_mac[3],
	    default_mac[4],
	    default_mac[5]);

	// The LCM plant ID must be set
	if(!eliot_lcm_plant_id[0])
	{
		DBGPRINT_ERRORS_INTSTRSTR(__LINE__, "Eliot_AddLcmModule()", "eliot_lcm_plant_id is not set");
		return ELIOT_RET_INPUT_ERROR;	
	}
	
	// "primaryKey": ???
	pair[0].key_str = "primaryKey";
	pair[0].value_str = primary_key;
	pair[0].value_max_length = ELIOT_PRIMARY_KEY_SIZE;
	pair[0].option = JSR_PAIR_KEY;

	// "message": ???	This pair detects HTTP errors
	pair[1].key_str = "message";
	pair[1].value_str = str_message;
	pair[1].value_max_length = sizeof(str_message);
	pair[1].option = JSR_PAIR_KEY;
	
	jsr_env_init(&jsr);
	jsr.key_str = temp_key;
	jsr.key_max_length = sizeof(temp_key);
	jsr.value_str = temp_value;
	jsr.value_max_length = sizeof(temp_value);
	
	jsr.option = JSR_OPTION_SKIP_HTTP_HEADER; // Skip the HTTP header
	jsr.pair_count = 2;	// Define two JSON key/value pairs
	jsr.pairs = pair;	// Link the JSR_ENV to the JSR_PAIR array
	jsr.match_threshold = 1;	
		
	DBGPRINT_INTSTRSTR(__LINE__, "Eliot_AddLcmModule(): Adding LCM module under plant", eliot_lcm_plant_id);
		
	if(!eliot_lcm_plant_id[0])
	{
		DBGPRINT_ERRORS_INTSTRSTR(__LINE__, "Eliot_AddLcmModule()", "LCM plant ID is undefined");
		return ELIOT_RET_INPUT_ERROR;
	}

	char serial[ELIOT_SERIAL_NUMBER_BUFFER_SIZE];
	Eliot_GenerateUniqueSerialNumber(SERIAL_NUMBER_FORMAT_CURRENT, serial, sizeof(serial));

	if(Eliot_ConnectWithRetry(&ssl, &ctx, &socket) )
	{
		for(mode=0; mode<=1; mode++)
		{
			// mode==0 sums the string lengths in messageLen
			// mode==1 sends the strings
			messageLen += Eliot_SendBody(mode, ssl, "{ \"plantId\": \"");
			messageLen += Eliot_SendBody(mode, ssl, eliot_lcm_plant_id);
			
			messageLen += Eliot_SendBody(mode, ssl, "\", \"serialNumber\": \"");
			messageLen += Eliot_SendBody(mode, ssl, serial);

			messageLen += Eliot_SendBody(mode, ssl, "\", \"name\": \"");
			messageLen += Eliot_SendBody(mode, ssl, ELIOT_LCM_MODULE_NAME);

            // Special case: If the MAC address is the default one, then
            // there could be other devices in the world with the same MAC.  Submitting the same value
            // to Eliot in the macAddress field for multiple devices causes cloud connection issues.  Work around
            // that issue by not sending macAddress for those devices that have the
            // default MAC address.
            if (strcmp(mac_string, default_mac_string) != 0 && includeMAC)
            {
                messageLen += Eliot_SendBody(mode, ssl, "\", \"macAddress\": \"");
                messageLen += Eliot_SendBody(mode, ssl, mac_string);
            }

            // Always send internalId as the MAC address
			messageLen += Eliot_SendBody(mode, ssl, "\", \"internalId\": \"");
			messageLen += Eliot_SendBody(mode, ssl, mac_string);

            // Use serial number for hardwareId
			messageLen += Eliot_SendBody(mode, ssl, "\",\"hardwareId\": \"");
			messageLen += Eliot_SendBody(mode, ssl, serial);
						
			messageLen += Eliot_SendBody(mode, ssl, "\",\"deviceType\": \"");
			messageLen += Eliot_SendBody(mode, ssl, LC7001_HUB);

			messageLen += Eliot_SendBody(mode, ssl, "\",\"device\": \"light\",");

			// Set initial firmware version using an "_init" suffix.  If Eliot_PublishFirmwareVersion()
			// is functioning correctly, the suffix will be dropped when MQTT connects.  If "_init"
			// persists in Eliot, MQTT problems are likely.
			// Also make sure version_string does not exceed the length allowed by Eliot.
            messageLen += Eliot_SendBody(mode, ssl, "\"firmwareVersion\": \"");
			snprintf(version_string, sizeof(version_string), "%s_init", SYSTEM_INFO_FIRMWARE_VERSION);
			messageLen += Eliot_SendBody(mode, ssl, version_string);

			messageLen += Eliot_SendBody(mode, ssl, "\",\"hardwareVersion\": \"1.0\",");

			messageLen += Eliot_SendBody(mode, ssl, "\"metrics\": [	");
			messageLen += Eliot_SendBody(mode, ssl, "{\"label\": \"mStatus\", \"metricType\": \"Text\"},");
			messageLen += Eliot_SendBody(mode, ssl, "{\"label\": \"mVersion\", \"metricType\": \"Text\"},");
			messageLen += Eliot_SendBody(mode, ssl, "{\"label\": \"mReleaseNotes\", \"metricType\": \"Text\"},");
			messageLen += Eliot_SendBody(mode, ssl, "{\"label\": \"mProgress\", \"metricType\": \"Number\"},");
			messageLen += Eliot_SendBody(mode, ssl, "{\"label\": \"mUpdateHash\", \"metricType\": \"Text\"},");
			messageLen += Eliot_SendBody(mode, ssl, "{\"label\": \"mUpdateHost\", \"metricType\": \"Text\"},");
			messageLen += Eliot_SendBody(mode, ssl, "{\"label\": \"mUpdatePath\", \"metricType\": \"Text\"},");
			messageLen += Eliot_SendBody(mode, ssl, "{\"label\": \"mDiagnostics\", \"metricType\": \"Text\"} ],	");

			messageLen += Eliot_SendBody(mode, ssl, "\"gatewayInstallation\": true  }");
		
			if(!mode)
			{
				Eliot_SendHeader(ssl, "POST", DEVICE_MGMT_MODULES_API, messageLen);
			}
		}
		
		do
		{
			rx_count = wolfSSL_read(ssl, rx_buf, sizeof(rx_buf));

            if (DEBUG_ELIOT_ANY || DEBUG_JUMPER)
            {
                if (rx_count < 0)
                {
                    DBGPRINT_INTSTRSTR(rx_count, "Eliot_AddLcmModule() rx_buf", "<EMPTY>");
                }
                else if (rx_count > sizeof(rx_buf))
                {
                    DBGPRINT_ERRORS_INTSTRSTR(rx_count, "Eliot_AddLcmModule()", "wolfSSL_read exceeded buffer");
                }
                else
                {
                    char rx_buf_dbg[ELIOT_JSR_BLOCK_SIZE + 1];
                    memcpy(rx_buf_dbg, rx_buf, rx_count);
                    rx_buf_dbg[rx_count] = 0;
                    DBGPRINT_INTSTRSTR(rx_count, "Eliot_AddLcmModule() rx_buf", rx_buf_dbg);
                }
            }
					
			if(rx_count > 0)
			{
                int32_t httpMajorVer = 0;
                int32_t httpMinorVer = 0;
                int32_t httpResultCode = 0;
			    int32_t result = sscanf(rx_buf, "HTTP/%d.%d %d", &httpMajorVer, &httpMinorVer, &httpResultCode);

			    if (result == 3)
			    {
			        switch (httpResultCode)
			        {
			            case ELIOT_HTTP_QUOTA_EXCEEDED:
			            case ELIOT_HTTP_QUOTA_EXCEEDED2:
			                ret = ELIOT_RET_QUOTA_EXCEEDED;
			                break;

			            case ELIOT_HTTP_CONFLICT:
			                ret = ELIOT_RET_CONFLICT;
			                break;
			        }
			    }

				while( jsr_read(&jsr, rx_buf, rx_count) ==  JSR_RET_END_OBJECT )
				{
					// End of a matching object - JSR pair struct
					// points to primary_key, no need to strcpy.
					DBGPRINT_INTSTRSTR(__LINE__, "Eliot_AddLcmModule() primary key: ", primary_key);
				}
				
				total_read += rx_count;
				timeout_counter = 0;
			}
			else
			{
				_time_delay(ELIOT_RECEIVE_RETRY_DELAY_MS);
				timeout_counter++;
			}
		} while( (rx_count > 0 || !total_read || jsr.array_depth) && (timeout_counter<timeout_limit) );
		
		if(timeout_counter >= timeout_limit)
		{
			ret = ELIOT_RET_RESPONSE_TIMEOUT;
			DBGPRINT_INTSTRSTR(__LINE__, "Eliot_AddLcmModule()", "Response timed out");
			eliot_error_count.response_timeouts++;
		}
		else
		{
			if(primary_key[0])
			{
			    DBGPRINT_INTSTRSTR(__LINE__, "Eliot_AddLcmModule(): primary_key is filled", primary_key);
				// Write primary_key to flash.
				FBK_WriteFlash(&eliot_fbk, primary_key, LCM_DATA_PRIMARY_KEY_OFFSET, ELIOT_PRIMARY_KEY_SIZE );
				// Point eliot_primary_key to the latest copy of the primary key in flash.
				eliot_primary_key = FBK_Address(&eliot_fbk, LCM_DATA_PRIMARY_KEY_OFFSET);

				DBGPRINT_INTSTRSTR(__LINE__, "Eliot_AddLcmModule(): eliot_primary_key is filled", eliot_primary_key);

				// Module was created and primaryKey was received
				ret = ELIOT_RET_OK;
			}
			else if (ret == ELIOT_RET_UNDEFINED)
			{
			    DBGPRINT_INTSTRSTR(__LINE__, "Eliot_AddLcmModule(): primary_key is empty", "");
				if(str_message[0])
				{
					// HTTP error
					ret = ELIOT_RET_CLOUD_ERROR;
					eliot_error_count.rest++;
				}
				else
				{
					// Other error
					ret = ELIOT_RET_RESOURCE_NOT_FOUND;
				}
			}
		}

		// Cleanup sockets
		cleanupSockets(&ssl, &ctx, &socket);
	}
	else
	{	// Eliot_Connect failure
		DBGPRINT_ERRORS_INTSTRSTR(__LINE__, "Eliot_AddLcmModule()", "Eliot_Connect() failure");
		ret = ELIOT_RET_CONNECT_FAILURE;
	}
	
	if (ret == ELIOT_RET_OK)
   {  // if we were successful adding the LCM module, set the extendedSystemProperties flag indicating
      // that the new signed firmware metrics were instantiated in the cloud object.
      extendedSystemProperties.signedFWMetricsInstantiatedToEliot = SIGNED_FW_METRICS_INSTANTIATED_TO_ELIOT;
		SaveSystemPropertiesToFlash();
   }
   
   DBGPRINT_INTSTRSTR(ret, "Eliot_AddLcmModule()", "Returning");
	return ret;
}



// Eliot_SearchForModuleId() will search for modules matching the
// provided device type or serial number string.  The return value indicates whether the
// module was found.  If the module ID is not needed, module_id and
// id_len may be set to zero.  If both are nonzero, the module's ID
// string will be copied to module_id.  The search is limited to
// modules within the LCM plant and the LCM plant ID must be set
// before this function is called.

int32_t Eliot_SearchForModuleId(char* device_type, char* module_serial, char* module_id, int id_len)
{
	WOLFSSL*             ssl;
	WOLFSSL_CTX*         ctx;
	uint32_t             socket = 0;

	int32_t rx_count = 0;
	int32_t ret = ELIOT_RET_RESOURCE_NOT_FOUND;
	int32_t total_read = 0;
	int16_t timeout_limit = ELIOT_RECEIVE_MAX_RETRIES;
	int16_t timeout_counter = 0;
	int16_t connect_retry = ELIOT_CONNECT_ATTEMPTS;
	
	char rx_buf[ELIOT_JSR_BLOCK_SIZE];
	char str_output[ELIOT_DEVICE_ID_SIZE];
	char temp_key[16];
	char temp_value[64];
	char str_message[DEBUG_ELIOT_HTTP_ERR_LEN];
	char str_last_seen[ELIOT_LAST_SEEN_SIZE];
	char newest_last_seen[ELIOT_LAST_SEEN_SIZE];
	JSR_PAIR pair[5];
	JSR_ENV jsr;

	str_message[0] = 0; 
	module_id[0] = 0; 
	str_output[0] = 0;
	str_last_seen[0] = 0;
	newest_last_seen[0] = 0;

	if (!device_type && !module_serial)
	{
        DBGPRINT_ERRORS_INTSTRSTR(__LINE__, "Eliot_SearchForModuleId()", "device_type and module_serial are NULL");
		return ELIOT_RET_INPUT_ERROR;
	}

	if (device_type)
	{
        // "deviceType": device_type
        pair[0].key_str = "deviceType";
        pair[0].value_str = device_type;
	}
	else
	{
        // "serialNumber": module_serial
        pair[0].key_str = "serialNumber";
        pair[0].value_str = module_serial;
	}

	pair[0].option = JSR_PAIR_BOTH;

	// "id": ???
	pair[1].key_str = "id";
	pair[1].value_str = str_output;
	pair[1].value_max_length = sizeof(str_output);
	pair[1].option = JSR_PAIR_KEY;

	// "plantId:" : eliot_lcm_plant_id
	pair[2].key_str = "plantId";
	pair[2].value_str = eliot_lcm_plant_id;
	pair[2].option = JSR_PAIR_BOTH;

	// "message": ???	This pair detects HTTP errors
	pair[3].key_str = "message";
	pair[3].value_str = str_message;
	pair[3].value_max_length = sizeof(str_message);
	pair[3].option = JSR_PAIR_KEY;

	// "lastSeen": Used to determine most recently seen module of the desired type
	pair[4].key_str = "lastSeen";
	pair[4].value_str = str_last_seen;
	pair[4].value_max_length = sizeof(str_last_seen);
	pair[4].option = JSR_PAIR_KEY | JSR_PAIR_TRANSIENT;
	
	// The LCM plant ID must be set
	if(!eliot_lcm_plant_id[0])
	{
		DBGPRINT_ERRORS_INTSTRSTR(__LINE__, "Eliot_SearchForModuleId()", "eliot_lcm_plant_id is not set");
		return ELIOT_RET_INPUT_ERROR;	
	}
		
	jsr_env_init(&jsr);
	jsr.key_str = temp_key;
	jsr.key_max_length = sizeof(temp_key);

	jsr.value_str = temp_value;
	jsr.value_max_length = sizeof(temp_value);
	
	jsr.option = JSR_OPTION_SKIP_HTTP_HEADER; // Skip the HTTP header
	jsr.pair_count = 5;	// Define five JSON key/value pairs
	jsr.pairs = pair;	// Link the JSR_ENV to the JSR_PAIR array
	// jsr_read() matches four pairs before returning JSR_RET_END_OBJECT
	jsr.match_threshold = 4;	// Match on 'plantId', 'serialNumber', 'id', and 'lastSeen'
	
	DBGPRINT_INTSTRSTR(__LINE__, "Eliot_SearchForModuleId(): Searching for deviceType or serialNumber:", pair[0].value_str);
	DBGPRINT_INTSTRSTR(__LINE__, "Eliot_SearchForModuleId(): Searching within plant:", pair[2].value_str);
	
	while(connect_retry > 0)
	{
		timeout_counter = 0;
		if(Eliot_ConnectWithRetry(&ssl, &ctx, &socket) )
		{
			Eliot_SendHeader(ssl, "GET", SERVICE_CATALOG_MODULES_API, 0);

			do
			{
				rx_count = wolfSSL_read(ssl, rx_buf, sizeof(rx_buf));
						
				if(rx_count > 0)
				{
					while( jsr_read(&jsr, rx_buf, rx_count) ==  JSR_RET_END_OBJECT )
					{
					    if (str_output[0] && module_id && id_len)
					    {
                            if (str_last_seen[0])
                            {
                                if ((newest_last_seen[0] == 0) || (strncmp(str_last_seen, newest_last_seen, ELIOT_LAST_SEEN_SIZE) > 0))
                                {
                                    // Store the newest lastSeen value we have seen so far, for
                                    // use in later iterations of this loop.
                                    strncpy(newest_last_seen, str_last_seen, ELIOT_LAST_SEEN_SIZE);

                                    // Found first matching object OR a matching object with a newer (i.e. greater) lastSeen time than
                                    // others already seen in this loop.
                                    // So...Replace the module ID with this more recently seen one.
                                    strncpy(module_id, str_output, id_len);
                                }
                            }
                            else
                            {
                                // Did not find lastSeen value.  This should not happen, but if it does make sure we return a
                                // module_id.  If a subsequent module with lastSeen is found later in this loop, it will be overwritten
                                // at that time.
                                strncpy(module_id, str_output, id_len);
                            }
						}
						ret = ELIOT_RET_OK;
					}
					total_read += rx_count;
					timeout_counter = 0;
				}
				else
				{
					_time_delay(ELIOT_RECEIVE_RETRY_DELAY_MS);
					timeout_counter++;
				}
				// If device was found, don't parse any more objects.
				if(ret == ELIOT_RET_OK)
				{
					DBGPRINT_INTSTRSTR(__LINE__, "Eliot_SearchForModuleId() found module ID", str_output);
					break;
				}
			} while( (rx_count > 0 || !total_read || jsr.array_depth) && (timeout_counter<timeout_limit) );

			DBGPRINT_INTSTRSTR(rx_count, "Eliot_SearchForModuleId() AFTER LOOP rx_count", "");
			DBGPRINT_INTSTRSTR(total_read, "Eliot_SearchForModuleId() AFTER LOOP total_read", "");
			DBGPRINT_INTSTRSTR(jsr.array_depth, "Eliot_SearchForModuleId() AFTER LOOP jsr.array_depth", "");
			DBGPRINT_INTSTRSTR(timeout_counter, "Eliot_SearchForModuleId() AFTER LOOP timeout_counter", "");
			
			if(timeout_counter >= timeout_limit)
			{
				DBGPRINT_ERRORS_INTSTRSTR(__LINE__, "Eliot_SearchForModuleId() response timed out", "");
				eliot_error_count.response_timeouts++;
			}
			else
			{
				// Timeout was not exceeded, response was received, don't retry
				connect_retry = 0;
			}
			
		}
		else
		{	// Eliot_Connect failure
			connect_retry--;
			DBGPRINT_ERRORS_INTSTRSTR(connect_retry, "Eliot_SearchForModuleId()", "Eliot_Connect() failure.  Attempts remaining:");

			if(!connect_retry)
			{
				_time_delay(ELIOT_CONNECT_RETRY_DELAY_MS);
			}
			else
			{
				ret = ELIOT_RET_CONNECT_FAILURE;
			}			
		}
		
		cleanupSockets(&ssl, &ctx, &socket);
	}
	
	// Indicate an error if a "message" key was found in the JSON
	if(pair[3].value_str[0]) 
	{
		DBGPRINT_ERRORS_INTSTRSTR(__LINE__, "Eliot_SearchForModuleId() cloud error:", pair[3].value_str );
		eliot_error_count.rest++;
		ret = ELIOT_RET_CLOUD_ERROR;
	}
	
	if(ret == ELIOT_RET_RESOURCE_NOT_FOUND)
	{
		DBGPRINT_ERRORS_INTSTRSTR(__LINE__, "Eliot_SearchForModuleId()", "Module not found");
	}
	
	DBGPRINT_INTSTRSTR(total_read, "Eliot_SearchForModuleId()", "Bytes received:");
	DBGPRINT_INTSTRSTR(ret, "Eliot_SearchForModuleId()", "Returning");
	return ret;
}



// Eliot_GetLcmPlantId() will fetch the ID string of the LCM plant
// and place it into eliot_lcm_plant_id, if it exists.

int32_t Eliot_GetLcmPlantId()
{
	WOLFSSL*             ssl;
	WOLFSSL_CTX*         ctx;
	uint32_t             socket = 0;
	int32_t rx_count = 0;
	int32_t ret = ELIOT_RET_RESOURCE_NOT_FOUND;
	int32_t total_read = 0;
	int16_t timeout_limit = ELIOT_RECEIVE_MAX_RETRIES;
	int16_t timeout_counter = 0;
	int16_t connect_retry = 10;
	
	char rx_buf[ELIOT_JSR_BLOCK_SIZE];
	char str_output[ELIOT_DEVICE_ID_SIZE + 2];
	char temp_key[16];
	char temp_value[64];
	char str_message[DEBUG_ELIOT_HTTP_ERR_LEN];
	JSR_PAIR pair[3];
	JSR_ENV jsr;

	str_output[0] = 0;
	str_message[0] = 0;
	
	// "name": "LCM"
	pair[0].key_str = "name";
	pair[0].value_str = ELIOT_LCM_PLANT_NAME;	
	pair[0].option = JSR_PAIR_BOTH;

	// "id": ???
	pair[1].key_str = "id";
	pair[1].value_str = str_output;
	pair[1].value_max_length = sizeof(str_output);
	pair[1].option = JSR_PAIR_KEY;

	// "message": ???	This pair detects HTTP errors
	pair[2].key_str = "message";
	pair[2].value_str = str_message;
	pair[2].value_max_length = sizeof(str_message);
	pair[2].option = JSR_PAIR_KEY;
	
	jsr_env_init(&jsr);
	jsr.key_str = temp_key;
	jsr.key_max_length = sizeof(temp_key);

	jsr.value_str = temp_value;
	jsr.value_max_length = sizeof(temp_value);
	
	jsr.option = JSR_OPTION_SKIP_HTTP_HEADER; // Skip the HTTP header
	jsr.pair_count = 3;	// Define three JSON key/value pairs
	jsr.pairs = pair;	// Link the JSR_ENV to the JSR_PAIR array
	// jsr_read() matches 'name' and 'id' pairs before returning JSR_RET_END_OBJECT
	jsr.match_threshold = 2;	
	
	DBGPRINT_INTSTRSTR(__LINE__, "GetLcmPlantId(): Searching for plant name:", pair[0].value_str);
	
	while(connect_retry > 0)
	{
		timeout_counter = 0;
		if(Eliot_ConnectWithRetry(&ssl, &ctx, &socket) )
		{
			Eliot_SendHeader(ssl, "GET", SERVICE_CATALOG_PLANTS_API, 0);

			do
			{
				rx_count = wolfSSL_read(ssl, rx_buf, sizeof(rx_buf));
						
				if(rx_count > 0)
				{
					while( jsr_read(&jsr, rx_buf, rx_count) ==  JSR_RET_END_OBJECT )
					{
						// End of a matching object - copy the ID string
						strncpy(eliot_lcm_plant_id, str_output, ELIOT_DEVICE_ID_SIZE);
						ret = ELIOT_RET_OK;
						DBGPRINT_INTSTRSTR(__LINE__, "GetLcmPlantId() found LCM plant ID:", eliot_lcm_plant_id);
					}
					total_read += rx_count;
					timeout_counter = 0;
				}
				else
				{	
					_time_delay(ELIOT_RECEIVE_RETRY_DELAY_MS);
					timeout_counter++;
				}
				
			} while( (rx_count > 0 || !total_read || jsr.array_depth) && (timeout_counter<timeout_limit) );
			
			if(timeout_counter >= timeout_limit)
			{
				DBGPRINT_ERRORS_INTSTRSTR(__LINE__, "GetLcmPlantId() response timed out", "");
				eliot_error_count.response_timeouts++;
			}
			else
			{
				// Timeout was not exceeded, response was received, don't retry
				connect_retry = 0;
			}
		}
		else
		{	// Eliot_Connect failure
			connect_retry--;
			DBGPRINT_ERRORS_INTSTRSTR(connect_retry, "GetLcmPlantId()", "Eliot_Connect() failure.  Attempts remaining:");

			if(!connect_retry)
			{
				_time_delay(ELIOT_CONNECT_RETRY_DELAY_MS);
			}
			else
			{
				ret = ELIOT_RET_CONNECT_FAILURE;
			}	
		}
		
		cleanupSockets(&ssl, &ctx, &socket);
	}

	// Indicate an error if a "message" key was found in the JSON
	if(pair[2].value_str[0]) 
	{
		DBGPRINT_ERRORS_INTSTRSTR(__LINE__, "GetLcmPlantId() cloud error:", pair[2].value_str );
		eliot_error_count.rest++;
		ret = ELIOT_RET_CLOUD_ERROR;
	}
	if(ret == ELIOT_RET_RESOURCE_NOT_FOUND)
	{
		DBGPRINT_INTSTRSTR(__LINE__, "GetLcmPlantId()", "LCM plant not found");
	}

	DBGPRINT_INTSTRSTR(total_read, "GetLcmPlantId()", "Bytes received:");
	DBGPRINT_INTSTRSTR(ret, "GetLcmPlantId()", "Returning");
	return ret;
}



// Creates an LCM plant by name, without checking for duplicates.

int32_t Eliot_AddLcmPlant()
{
	WOLFSSL*             ssl;
	WOLFSSL_CTX*         ctx;
	uint32_t             socket = 0;
	int32_t ret = ELIOT_RET_UNDEFINED;
	uint16_t messageLen = 0;
	uint8_t mode;
	
	if(Eliot_ConnectWithRetry(&ssl, &ctx, &socket) )
	{
		for(mode=0; mode<=1; mode++)
		{
			// mode==0 sums the string lengths in messageLen
			// mode==1 sends the strings
			messageLen += Eliot_SendBody(mode, ssl, "{ \"name\": \"");
			messageLen += Eliot_SendBody(mode, ssl, ELIOT_LCM_PLANT_NAME);

			messageLen += Eliot_SendBody(mode, ssl, "\", \"type\": \"");
			messageLen += Eliot_SendBody(mode, ssl, ELIOT_LCM_PLANT_TYPE);

			messageLen += Eliot_SendBody(mode, ssl, "\", \"internalId\": \"");
			messageLen += Eliot_SendBody(mode, ssl, "LCM-PLANT-1");

			messageLen += Eliot_SendBody(mode, ssl, "\", \"country\": \"");
			messageLen += Eliot_SendBody(mode, ssl, ELIOT_COUNTRY);
			messageLen += Eliot_SendBody(mode, ssl, "\"}");
		
			if(!mode)
			{
				Eliot_SendHeader(ssl, "POST", SERVICE_CATALOG_PLANTS_API, messageLen);
			}
			else
			{
                char buf[REQUEST_SIZE];
                memset(buf, 0, sizeof(buf));
                DBGPRINT_INTSTRSTR(__LINE__, "Eliot_AddLcmPlant calls Eliot_ReceiveBytes", "");
                int32_t bytesReceived = Eliot_ReceiveBytes(ssl, buf, sizeof(buf), false);
                DBGPRINT_INTSTRSTR(0, ELIOT_REST_SERVICE, buf);

                if (bytesReceived > 0)
                {
                    json_error_t jsonError;
                    json_t* root = json_loads(buf, 0, &jsonError);

                    if (root)
                    {
                        json_t * idObject = json_object_get(root, "id");

                        if(idObject && json_is_string(idObject))
                        {
                            char deviceId[ELIOT_DEVICE_ID_SIZE];
                            strncpy(deviceId, json_string_value(idObject), ELIOT_DEVICE_ID_SIZE);
                            
                            // Write deviceID to flash.
                            FBK_WriteFlash(&eliot_fbk, deviceId, LCM_DATA_PLANT_ID_OFFSET, ELIOT_DEVICE_ID_SIZE );
                            deviceId[ELIOT_DEVICE_ID_SIZE - 1] = 0;
                            json_delete(idObject);

                            DBGPRINT_INTSTRSTR(__LINE__, "Eliot_AddLcmPlant plant created", deviceId);
                        }

                        json_delete(root);
                    } else {
                        DBGPRINT_INTSTRSTR(__LINE__, "Eliot_AddLcmPlant bad root object received!", "");
                        eliot_error_count.rest++;
                    }
                } else {
                    DBGPRINT_INTSTRSTR(__LINE__, "Eliot_AddLcmPlant bad receive!", "");
                    eliot_error_count.rest++;
                }
			}
		}
		ret = ELIOT_RET_OK;

		// Cleanup sockets
        cleanupSockets(&ssl, &ctx, &socket);
	}
	else
	{	// Eliot_Connect failure
		DBGPRINT_ERRORS_INTSTRSTR(__LINE__, "Eliot_AddLcmPlant()", "Eliot_Connect() failure");
		ret = ELIOT_RET_CONNECT_FAILURE;
	}
	
	return ret;
}

void Eliot_AddDevice(const Dev_Type deviceType, char* deviceId, const char* name)
{
	WOLFSSL*             ssl;
	WOLFSSL_CTX*         ctx;
	uint32_t             socket = 0;
	Rest_API_Error_Type  returnValue = REST_API_UNEXPECTED_ERROR;
	bool_t               registrationOK = false;
	JSR_ENV              jsr;

	char temp_key[16];
	char temp_value[64];
	char id_string[ELIOT_DEVICE_ID_SIZE];
	char type_string[16];

	int32_t rx_count = 0;
	int32_t total_read = 0;
	char rx_buf[ELIOT_JSR_BLOCK_SIZE];
	JSR_PAIR pairs[1];

    DBGPRINT_INTSTRSTR(0, "Eliot_AddDevice()", name);

	pairs[0].key_str = "id";
	pairs[0].value_str = id_string;
	pairs[0].value_max_length = ELIOT_DEVICE_ID_SIZE;
	pairs[0].option = JSR_PAIR_KEY;

	jsr_env_init(&jsr);

	jsr.key_str = temp_key;
	jsr.key_max_length = sizeof(temp_key);

	jsr.value_str = temp_value;
	jsr.value_max_length = sizeof(temp_value);

	jsr.pairs = pairs;
	jsr.pair_count = 1;
	jsr.option = JSR_OPTION_SKIP_HTTP_HEADER;
	jsr.match_threshold = 1;

	eliot_add_count++;

	char serial[ELIOT_SERIAL_NUMBER_BUFFER_SIZE];

	// For now, keep using the old serial number format for non-LCM devices
	Eliot_GenerateUniqueSerialNumber(SERIAL_NUMBER_FORMAT_0, serial, sizeof(serial));

	if(Eliot_ConnectWithRetry(&ssl, &ctx, &socket))
	{
		// Build the query
		json_t* deviceObject = json_object();
		json_t* metrics_definition = NULL;
		if (deviceObject)
		{
			json_object_set_new(deviceObject, "plantId", json_string(eliot_lcm_plant_id));
			json_object_set_new(deviceObject, "serialNumber", json_string(serial));
			json_object_set_new(deviceObject, "hardwareId", json_string(serial));

			if(deviceType == SWITCH_TYPE)
			{
				json_object_set_new(deviceObject, "deviceType", json_string(ELIOT_CLOUD_SWITCH));
				json_object_set_new(deviceObject, "device", json_string("light"));
				metrics_definition = json_pack("[{ss,ss}]",
                    "label", "powerState",
                    "metricType", "number"
                );

			}
			else if(deviceType == DIMMER_TYPE)
			{
				json_object_set_new(deviceObject, "deviceType", json_string(ELIOT_CLOUD_DIMMER));
				json_object_set_new(deviceObject, "device", json_string("light"));
                metrics_definition = json_pack("[{ss,ss},{ss,ss}]",
                    "label", "powerState",
                    "metricType", "number",
                    "label", "level",
                    "metricType", "number"
                );
			}
            else if(deviceType == FAN_CONTROLLER_TYPE)
            {
                json_object_set_new(deviceObject, "deviceType", json_string(ELIOT_CLOUD_FAN));
                json_object_set_new(deviceObject, "deviceModel", json_string("avd-rflc-fan"));
                json_object_set_new(deviceObject, "device", json_string("Automation"));
                metrics_definition = json_pack("[{ss,ss},{ss,ss}]",
                    "label", "powerState",
                    "metricType", "number",
                    "label", "level",
                    "metricType", "number"
                );
            }
#ifdef SHADE_CONTROL_ADDED
			else if(deviceType == SHADE_TYPE || deviceType == SHADE_GROUP_TYPE)
			{
			    if(deviceType == SHADE_TYPE)
			    {
				    json_object_set_new(deviceObject, "deviceType", json_string(ELIOT_CLOUD_SHADE));
				}
				else
				{
                    json_object_set_new(deviceObject, "deviceType", json_string(ELIOT_CLOUD_SHADE_GROUP));
                }

				json_object_set_new(deviceObject, "device", json_string("shutter"));
				metrics_definition = json_pack("[{ss,ss},{ss,ss}]",
                    "label", "powerState",
                    "metricType", "number",
                    "label", "level",
					"metricType", "number"
				);
			}
#endif
			else if(deviceType == SCENE_TYPE)
			{
				json_object_set_new(deviceObject, "deviceType", json_string(LC7001_SCENE));
				json_object_set_new(deviceObject, "device", json_string("scene"));
				metrics_definition = json_pack("[{ss,ss}]",
				    "label", "running",
				    "metricType", "number"
				);
			}
			else if(deviceType == NO_TYPE)
			{
			    // We have a remote device, and those don't need to be in Eliot
			    return;
			}

			json_object_set_new(deviceObject, "firmwareVersion", json_string("4.0"));
			json_object_set_new(deviceObject, "hardwareVersion", json_string("1.0"));
			json_object_set_new(deviceObject, "gatewayInstallation", json_true());
			json_object_set_new(deviceObject, "name", json_string(TaskSanitizeString(name)));
			json_object_set_new(deviceObject, "metrics", metrics_definition);

			char* json_string = json_dumps(deviceObject, (JSON_COMPACT | JSON_PRESERVE_ORDER));
			if (json_string)
			{
				int messageLen = strlen((const char*) json_string);

				Eliot_SendHeader(ssl, "POST", DEVICE_MGMT_MODULES_API, messageLen);

				// Send the json payload
				if (Eliot_SendBytes(ssl, json_string))
				{
    			    bool have_id = false;

					do
					{
						rx_count = wolfSSL_read(ssl, rx_buf, sizeof(rx_buf));

						if(rx_count > 0)
						{
							while(jsr_read(&jsr, rx_buf, rx_count) ==  JSR_RET_END_OBJECT)
							{
								if(id_string[0])
								{
									strncpy(deviceId, id_string, ELIOT_DEVICE_ID_SIZE);
									deviceId[ELIOT_DEVICE_ID_SIZE - 1] = 0;
									have_id = true;
								}
							}

							total_read += rx_count;
						}
					} while(rx_count > 0 || !total_read);

                    if (!have_id)
                    {
                        rx_buf[ELIOT_JSR_BLOCK_SIZE - 1] = 0;
                        DBGPRINT_INTSTRSTR(0, "Eliot_AddDevice() ERROR no device ID: ", rx_buf);
                    }

				}

				jsonp_free(json_string);
			}
		}

		// Cleanup sockets
		cleanupSockets(&ssl, &ctx, &socket);

		json_delete(deviceObject);
		json_delete(metrics_definition);
	}
	else // Eliot_ConnectWithRetry() failure
	{
		eliot_error_count.add_conn++;
		return;
	}
}

// Eliot_LcmPlantAndModuleInit() attempts to establish:
// 1.  An LCM plant in Eliot
// 2.  An LCM module in Eliot
// 3.  An LCM plant ID in RAM
// 4.  An LCM module ID in RAM
// 5.  A primary key in flash

// Call this function once per boot.  No transactions with
// Eliot will occur if items 3-5 are all present, so subsequent
// calls to this function are harmless.

int32_t Eliot_LcmPlantAndModuleInit()
{
	uint32_t ret = ELIOT_RET_UNDEFINED;
	uint8_t attempts = 3;

    // If there is no LCM ID, uninitialize MQTT
    if(!eliot_lcm_module_id[0])
    {
        DBGPRINT_INTSTRSTR(__LINE__, "Eliot_LcmPlantAndModuleInit", "NO LCM ID");
        Eliot_BuildMqttStrings();
    }

	if(!eliot_user_token[0])
	{
		DBGPRINT_ERRORS_INTSTRSTR(__LINE__, "Eliot_LcmPlantAndModuleInit()", "No user token");
		eliot_error_count.user_token++;
		return ELIOT_RET_INPUT_ERROR;
	}

	while(!eliot_lcm_plant_id[0] && (attempts > 0))
	{
		// Don't create a plant if retrieving the plant list failed
		if(Eliot_GetLcmPlantId() == ELIOT_RET_RESOURCE_NOT_FOUND)
		{
			Eliot_AddLcmPlant();
		}
		attempts--;
	}
	
	if(!eliot_lcm_plant_id[0])
	{
		// Failed to get plant list and/or create LCM plant
		DBGPRINT_ERRORS_INTSTRSTR(ret, "Eliot_LcmPlantAndModuleInit()", "failed to get plant ID, returning");
		eliot_error_count.lcm_plant_id++;
		return ret;
	}
	else
	{
		char *plant_id_flash = FBK_Address(&eliot_fbk, LCM_DATA_PLANT_ID_OFFSET);

		//Check if LCM Plant ID is same as one stored in Flash. If not have it create a new LCM Module
		if((strncmp(eliot_lcm_plant_id, plant_id_flash, ELIOT_DEVICE_ID_SIZE)!=0))
		{
			DBGPRINT_INTSTRSTR(__LINE__, "New Eliot_AddLcmPlant plant not matching Old ID", "");
			DBGPRINT_INTSTRSTR(__LINE__, "Overwriting Old ID with New ID in Flash", "");
			
			//Check if a previous ID was written to flash, if so create new LCM Module
			if (_strnlen(plant_id_flash, (FBK_BANK_SIZE-LCM_DATA_PLANT_ID_OFFSET))==_strnlen(eliot_lcm_plant_id, ELIOT_DEVICE_ID_SIZE))
			{
				DBGPRINT_INTSTRSTR(__LINE__, "Old Key Seems Legit, Wiping Old Eliot Keys", "");
				eliot_primary_key = eliot_empty_string;
			}
			//Write new ID to flash
			FBK_WriteFlash(&eliot_fbk, eliot_lcm_plant_id, LCM_DATA_PLANT_ID_OFFSET, ELIOT_DEVICE_ID_SIZE );
		}
	}

	attempts = 3;
	bool includeMAC = true;

	// Create/recreate the LCM module until we have a primary key
   while (((!eliot_primary_key[0]) ||
           (extendedSystemProperties.signedFWMetricsInstantiatedToEliot != SIGNED_FW_METRICS_INSTANTIATED_TO_ELIOT)) &&
          (attempts > 0) && (ret != ELIOT_RET_QUOTA_EXCEEDED))
	{
		// Delete existing LCM modules from Eliot before creating one
		char existing_lcm_module_id[ELIOT_DEVICE_ID_SIZE];

		do
		{
			existing_lcm_module_id[0] = 0;
			Eliot_SearchForModuleId(LC7001_HUB, NULL, existing_lcm_module_id, sizeof(existing_lcm_module_id));

			if (existing_lcm_module_id[0])
			{
				DBGPRINT_INTSTRSTR(__LINE__, "Eliot_LcmPlantAndModuleInit(): deleting existing LCM", existing_lcm_module_id);
				Eliot_DeleteDevice(existing_lcm_module_id);
			}
		}
		while (existing_lcm_module_id[0]);

		// This sets eliot_primary_key
		ret = Eliot_AddLcmModule(includeMAC);

		if (ret == ELIOT_RET_QUOTA_EXCEEDED)
		{
		    eliot_last_quota_exceeded_time = Eliot_TimeNow();
		}
		else if (ret == ELIOT_RET_CONFLICT)
		{
		    includeMAC = false;
		}
		
		attempts--;
	}

	if(!eliot_lcm_module_id[0])
	{
		// The while loop above doesn't run on boots where the primary key is available
		// in flash, so Eliot_SearchForModuleId() is moved here to ensure we can always
		// get the LCM module ID if we need it.

		// This sets eliot_lcm_module_id.
		Eliot_SearchForModuleId(LC7001_HUB, NULL, eliot_lcm_module_id, sizeof(eliot_lcm_module_id));
	}

	if(!eliot_primary_key[0])
	{
		// Failed to create LCM module or get its primaryKey
		DBGPRINT_ERRORS_INTSTRSTR(ret, "Eliot_LcmPlantAndModuleInit()", "failed to get primaryKey, returning");
		eliot_error_count.primary_key++;
		return ret;
	}

	if(!eliot_lcm_module_id[0])
	{
		DBGPRINT_ERRORS_INTSTRSTR(ret, "Eliot_LcmPlantAndModuleInit()", "failed to get LCM module ID, returning");
		eliot_error_count.lcm_module_id++;

		// At this point, we have a primary key but no LCM module.  This can happen if the
		// user switches accounts or if the LCM module is missing.  Pointing eliot_primary_key
		// to an empty string allows this function to create an LCM module the next time
		// it is called.  eliot_primary_key will then point to the new primary key in flash.
		eliot_primary_key = (char*)eliot_empty_string;
		return ret;
	}

	// Build the MQTT strings if they haven't been built
	if(!eliot_username[0])
	{
		// This requires both eliot_primary_key and eliot_lcm_module_id.
		Eliot_BuildMqttStrings();
	}

	return ELIOT_RET_OK;
}



void Eliot_RenameDevice(char* deviceId, const char* name)
{
	WOLFSSL*             ssl;
	WOLFSSL_CTX*         ctx;
	uint32_t             socket=0;
	int32_t              http_status=0;
	int32_t              attempts=ELIOT_RENAME_ATTEMPTS;

	if (strlen(deviceId) != ELIOT_DEVICE_ID_SIZE - 1)
	{
		return;
	}

	do
	{
		if (Eliot_ConnectWithRetry(&ssl, &ctx, &socket))
		{
			json_t* json = json_object();
			json_object_set_new(json, "name", json_string(TaskSanitizeString(name)));

			char* json_string = json_dumps(json, (JSON_COMPACT | JSON_PRESERVE_ORDER));

			if (json_string)
			{
				int messageLen = strlen((const char*) json_string);

				char endpoint[80];
				snprintf(endpoint, sizeof(endpoint), "%s/%s", SERVICE_CATALOG_MODULES_API, deviceId);

				// Send header
				if (Eliot_SendHeader(ssl, "PATCH", endpoint, messageLen))
				{
					// Send the json payload
					DBGPRINT_INTSTRSTR(http_status, "Eliot_RenameDevice() Renaming device ID", deviceId);
					Eliot_SendBytes(ssl, json_string);
					http_status = Eliot_ReceiveStatus(ssl);
				}

				jsonp_free(json_string);
			}
			else
			{
				DBGPRINT_ERRORS_INTSTRSTR(http_status, "Eliot_RenameDevice() json_dumps failure for deviceId", deviceId);
			}

			json_delete(json);

			// Cleanup sockets
			cleanupSockets(&ssl, &ctx, &socket);

			if(http_status == ELIOT_HTTP_OK)
			{
				break;
			}
			else
			{
				DBGPRINT_ERRORS_INTSTRSTR(http_status, "Eliot_RenameDevice() failed with HTTP status.  Retrying...", name);
				_time_delay(500);
				attempts--;
			}
		}
		else
		{
			// If Eliot_ConnectWithRetry() can't connect, give up.
			eliot_error_count.rename_conn++;
			break;
		}
	} while(attempts > 0);

	return;
}

void Eliot_DeleteDevice(const char* deviceId)
{
    WOLFSSL*             ssl;
    WOLFSSL_CTX*         ctx;
    uint32_t             socket = 0;

    if (Eliot_ConnectWithRetry(&ssl, &ctx, &socket))
    {
        char endpoint[80];
        snprintf(endpoint, sizeof(endpoint), "%s/%s", DEVICE_MGMT_MODULES_API, deviceId);

        // Send header
        Eliot_SendHeader(ssl, "DELETE", endpoint, 0);

        char buf[16];
        Eliot_ReceiveBytes(ssl, buf, sizeof(buf), true);

        DBGPRINT_INTSTRSTR(sizeof(buf), "Eliot_DeleteDevice():", buf);

        // Cleanup sockets
        cleanupSockets(&ssl, &ctx, &socket);
    }
    else // Eliot_ConnectWithRetry() failure
    {
        eliot_error_count.delete_conn++;
        return;
    }
}

void Eliot_QueueDeleteDevice(const char* deviceId)
{
   json_t * messageObj;

   if (deviceId)
   {
      messageObj = json_object();
      if (messageObj)
      {
         json_object_set_new(messageObj, "id", json_string(deviceId));
         Eliot_SendTaskMessage(ELIOT_COMMAND_DELETE_ZONE, 0, messageObj);
      }
   }
}

static void Eliot_HandleTaskMessage(ELIOT_TASK_MESSAGE_PTR EliotTaskMessagePtr)
{
	bool_t packetSent = false;

	if (EliotTaskMessagePtr)
	{
		switch (EliotTaskMessagePtr->command)
		{
			case ELIOT_COMMAND_ADD_ZONE:
			{
				zone_properties      zoneProperties;
				byte_t               zoneIndex;
				Rest_API_Error_Type  errorCode;


				zoneIndex = EliotTaskMessagePtr->index;

				if (GetZoneProperties(zoneIndex, &zoneProperties))
				{
					if (zoneProperties.ZoneNameString[0])
					{
						// Get LCM plant and module IDs - Create them if they don't exist
						Eliot_LcmPlantAndModuleInit();
						
						if(eliot_lcm_plant_id[0])
						{
							Eliot_AddDevice(zoneProperties.deviceType, zoneProperties.EliotDeviceId, zoneProperties.ZoneNameString);

                            // Set the zone properties in the array
                            _mutex_lock(&ZoneArrayMutex);
                            SetZoneProperties(zoneIndex, &zoneProperties);
                            _mutex_unlock(&ZoneArrayMutex);

                            // Report initial metrics
                            if (last_metrics_zone_index == EliotTaskMessagePtr->index)
                            {
                                // Force this zone's merics to be sent
                                last_metrics_zone_index = -1;
                            }

                            Mqtt_QueueZoneChanged(EliotTaskMessagePtr->index);
						}
						else
						{
						    DBGPRINT_INTSTRSTR(__LINE__, "Eliot_AddDevice not called because we don't have a plant ID", "");
						}
					}
				}

				break;
			}
			case ELIOT_COMMAND_ZONE_CHANGED:
			{
			    // Inform the MQTT task that a zone changed
			    Mqtt_QueueZoneChanged(EliotTaskMessagePtr->index);
			    break;
			}
			case ELIOT_COMMAND_SCENE_STARTED:
			{
			    // Inform the MQTT task that a scene changed
			    Mqtt_QueueSceneStarted(EliotTaskMessagePtr->index);
			    break;
			}
			case ELIOT_COMMAND_DELETE_ZONE:
			{
			    if (EliotTaskMessagePtr->jsonObj)
			    {
			        json_t* id_object = json_object_get(EliotTaskMessagePtr->jsonObj, "id");
			        if (json_is_string(id_object))
			        {
			            Eliot_DeleteDevice(json_string_value(id_object));
			            eliot_add_count--;
			        }

			        json_decref(EliotTaskMessagePtr->jsonObj);
			    }

			    break;
			}
			case ELIOT_COMMAND_ADD_SCENE:
            {
                scene_properties sceneProperties;
                byte_t sceneIndex = EliotTaskMessagePtr->index;

                if (GetSceneProperties(sceneIndex, &sceneProperties))
                {
                    if (sceneProperties.SceneNameString[0])
                    {
                        Eliot_AddDevice(SCENE_TYPE, sceneProperties.EliotDeviceId, TaskSanitizeString(sceneProperties.SceneNameString));

                        _mutex_lock(&ZoneArrayMutex);
                        SetSceneProperties(sceneIndex, &sceneProperties);
                        _mutex_unlock(&ZoneArrayMutex);

                        // Report initial metrics
                        if (last_metrics_scene_index == EliotTaskMessagePtr->index)
                        {
                            // Force this scene's merics to be sent
                            last_metrics_scene_index = -1;
                        }

                        Mqtt_QueueSceneFinished(EliotTaskMessagePtr->index);
                    }
                }
            }
            case ELIOT_COMMAND_RENAME_ZONE:
            {
                ZoneChangedArray[EliotTaskMessagePtr->index] = true;
                break;
            }
            case ELIOT_COMMAND_RENAME_SCENE:
            {
                SceneChangedArray[EliotTaskMessagePtr->index] = true;
                break;
            }
		}

		_msg_free(EliotTaskMessagePtr);
	}
}

// Callback used by oneshot timer in SocketTask that is
// called when a scene finishes execution. This function
// notifies the MQTT task that it needs to push metrics for
// a specified scene
void Eliot_QueueSceneFinished(_timer_id id, void* data, uint32_t mode, uint32_t milliseconds)
{
    // The actual scene index is passed through data as an int.
    Mqtt_QueueSceneFinished((uint32_t)data);
}

void Eliot_SendAllChangedDeviceStatus(void)
{
    scene_properties  sceneProperties;
    zone_properties   zoneProperties;
    uint8_t           index;

    for (index = 0; index < MAX_NUMBER_OF_ZONES; index++)
    {
        if (GetZoneProperties(index, &zoneProperties))
        {
            if (ZoneChangedArray[index])
            {
                Eliot_RenameDevice(zoneProperties.EliotDeviceId, zoneProperties.ZoneNameString);
                ZoneChangedArray[index] = false;
            }
        }
    }

    for (index = 0; index < MAX_NUMBER_OF_SCENES; index++)
    {
        if (GetSceneProperties(index, &sceneProperties))
        {
            if (SceneChangedArray[index])
            {
                Eliot_RenameDevice(sceneProperties.EliotDeviceId, sceneProperties.SceneNameString);
                SceneChangedArray[index] = false;
            }
        }
    }
}

bool_t Eliot_SendTaskMessage(byte_t command, byte_t index, json_t * messageObj)
{
	ELIOT_TASK_MESSAGE_PTR EliotTaskMessagePtr;
	bool_t errorFlag = true;

	if (EliotTaskMessagePoolID)
	{
		EliotTaskMessagePtr = (ELIOT_TASK_MESSAGE_PTR) _msg_alloc(EliotTaskMessagePoolID);
		if (EliotTaskMessagePtr)
		{
			EliotTaskMessagePtr->HEADER.SOURCE_QID = 0;  //not relevant
			EliotTaskMessagePtr->HEADER.TARGET_QID = _msgq_get_id(0, ELIOT_TASK_MESSAGE_QUEUE);
			EliotTaskMessagePtr->HEADER.SIZE = sizeof(ELIOT_TASK_MESSAGE);

			EliotTaskMessagePtr->command = command;
			EliotTaskMessagePtr->index = index;
			EliotTaskMessagePtr->jsonObj = messageObj;

			errorFlag = !_msgq_send(EliotTaskMessagePtr);
		}
	}

	return errorFlag;
}


// Eliot_UpdateTokens will update the user token if possible.  It will also
// update the refresh token if it's older than ELIOT_REFRESH_TOKEN_LIFESPAN.
// The connection is not retried since Eliot_UserTokenMonitor() will retry
// this function as needed.

uint32_t Eliot_UpdateTokens()
{
	WOLFSSL*             ssl;
	WOLFSSL_CTX*         ctx;
	// Tell Eliot_Connect() to use a different server:
	uint32_t             socket = ELIOT_CONNECT_ALT_HOST_TOKEN_REFRESH;
	int32_t rx_count = 0;
	int32_t ret = ELIOT_RET_RESOURCE_NOT_FOUND;
	int32_t total_read = 0;
	int16_t timeout_limit = ELIOT_RECEIVE_MAX_RETRIES;
	int16_t timeout_counter = 0;
	int16_t connect_retry = 10;

	uint8_t mode = 0;
	uint16_t messageLen = 0;
	uint16_t refresh_token_length = 0;
	char rx_buf[ELIOT_JSR_BLOCK_SIZE];
	char temp_key[16];
	char len_str[8];
	char temp_value[ELIOT_TOKEN_SIZE];
	char str_message[DEBUG_ELIOT_HTTP_ERR_LEN];

	JSR_PAIR pair[3];
	JSR_ENV jsr;

	// The refresh token is updated in flash at longer intervals than the user token
	// in order to limit flash wear.  If it's time to write a new refresh token in
	// flash, malloc a buffer for the new token rather than growing the stack.  If the
	// malloc fails, the new refresh token will not be used, but the user token will be
	// updated.

	bool_t update_refresh_token = Eliot_TimeNow() > (eliot_refresh_token_modified_time + ELIOT_REFRESH_TOKEN_LIFESPAN);
	char** new_refresh_token = &pair[2].value_str;

	if(!eliot_refresh_token[0])
	{
		eliot_error_count.refresh_token++;
		return ELIOT_RET_INPUT_ERROR;
	}

	str_message[0] = 0;

	// "access_token": ???
	pair[0].key_str = "access_token";
	pair[0].value_str = eliot_user_token;
	pair[0].value_max_length = ELIOT_TOKEN_SIZE;
	pair[0].option = JSR_PAIR_KEY;

	// "error": ???
	pair[1].key_str = "error";
	pair[1].value_str = str_message;
	pair[1].value_max_length = sizeof(str_message);
	pair[1].option = JSR_PAIR_KEY;

	// "refresh_token": ???  -  Used if(update_refresh_token)
	pair[2].key_str = "refresh_token";
	pair[2].value_str = 0; // new_refresh_token points to this
	pair[2].value_max_length = ELIOT_TOKEN_SIZE;
	pair[2].option = JSR_PAIR_KEY;

	jsr_env_init(&jsr);
	jsr.key_str = temp_key;
	jsr.key_max_length = sizeof(temp_key);

	jsr.value_str = temp_value;
	jsr.value_max_length = sizeof(temp_value);

	jsr.option = JSR_OPTION_SKIP_HTTP_HEADER; // Skip the HTTP header

	jsr.pairs = pair;	// Link the JSR_ENV to the JSR_PAIR array
	jsr.match_threshold = 1;

	if(Eliot_Connect(&ssl, &ctx, &socket) )
	{
		DBGPRINT_INTSTRSTR(rx_count, "Eliot_UpdateTokens()", "Connected");

		for(mode=0; mode<=1; mode++)
		{
			// mode==0 sums the string lengths in messageLen
			// mode==1 sends the strings
			messageLen += Eliot_SendBody(mode, ssl, "client_id=");
			messageLen += Eliot_SendBody(mode, ssl, ELIOT_CLIENT_ID);
			messageLen += Eliot_SendBody(mode, ssl, "&grant_type=refresh_token&refresh_token=");
			messageLen += Eliot_SendBody(mode, ssl, eliot_refresh_token);

			if(!mode)
			{
				Eliot_SendBytes(ssl, "POST "ELIOT_TOKEN_REFRESH_URL);
				Eliot_SendBytes(ssl, " HTTP/1.1\r\nContent-Length: ");
				snprintf(len_str, sizeof(len_str), "%d", messageLen);
				Eliot_SendBytes(ssl, len_str);
				Eliot_SendBytes(ssl, "\r\nHost: "ELIOT_TOKEN_REFRESH_HOST);
				Eliot_SendBytes(ssl, "\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n");
			}
		}

		if(update_refresh_token)
		{
			if(! (*new_refresh_token=malloc(ELIOT_TOKEN_SIZE)) )
			{
				DBGPRINT_ERRORS_INTSTRSTR(ELIOT_TOKEN_SIZE, "Eliot_UpdateTokens()", "malloc failed - not enough memory to update refresh token!");
			}
		}

		if(*new_refresh_token)  // The buffer was allocated successfully
		{
			**new_refresh_token = 0;  // Clear the token string
			jsr.pair_count = 3;  // Capture access_token, error and refresh_token
		}
		else
		{
			jsr.pair_count = 2;  // Capture access_token and error
		}

		do
		{
			rx_count = wolfSSL_read(ssl, rx_buf, sizeof(rx_buf));

            if (DEBUG_ELIOT_ANY || DEBUG_JUMPER)
            {
                if (rx_count > sizeof(rx_buf))
                {
                    DBGPRINT_ERRORS_INTSTRSTR(rx_count, "Eliot_UpdateTokens()", "wolfSSL_read exceeded buffer");
                }
                else if (rx_count > 0)
                {
                    char rx_buf_dbg[ELIOT_JSR_BLOCK_SIZE + 1];
                    memcpy(rx_buf_dbg, rx_buf, rx_count);
                    rx_buf_dbg[rx_count] = 0;
                    DBGPRINT_INTSTRSTR(rx_count, "Eliot_UpdateTokens() rx_buf", rx_buf_dbg);
                }
                else
                {
                    DBGPRINT_INTSTRSTR(rx_count, "Eliot_UpdateTokens() rx_buf", "<EMPTY>");
                }
            }

			if(rx_count > 0)
			{
				while( jsr_read(&jsr, rx_buf, rx_count) ==  JSR_RET_END_OBJECT )
				{
					if(str_message[0])  // The match is an error message
					{
						DBGPRINT_ERRORS_INTSTRSTR(total_read, "Eliot_UpdateTokens() Error message:", str_message);
					}
					else
					{
						DBGPRINT_INTSTRSTR(_strnlen(eliot_user_token, ELIOT_TOKEN_SIZE), "Eliot_UpdateTokens() User token:", eliot_user_token);
						eliot_user_token_modified_time = Eliot_TimeNow();
					}
				}

				total_read += rx_count;
				timeout_counter = 0;
			}
			else
			{
				_time_delay(ELIOT_RECEIVE_RETRY_DELAY_MS);
				timeout_counter++;
			}
			// Use jsr.depth here rather than jsr.array_depth as in most Eliot calls.  jsr.depth will continue reading
			// until the object closes '}' while jsr.array_depth will continue reading until the outer array ']' closes.
			// jsr.array_depth is ineffective here since the tokens are contained within a single object.
		} while( (rx_count > 0 || !total_read || jsr.depth) && (timeout_counter<timeout_limit) );

		if(*new_refresh_token)  // Memory was allocated
		{
			if(**new_refresh_token)  // Token is present
			{
				// Check the length of the refresh token
				refresh_token_length = _strnlen(*new_refresh_token, ELIOT_TOKEN_SIZE);
				if( (refresh_token_length>(ELIOT_TOKEN_SIZE>>2)) && (refresh_token_length<(ELIOT_TOKEN_SIZE-2)) )
				{
					// Refresh token length is OK
					FBK_WriteFlash(&eliot_fbk, *new_refresh_token, LCM_DATA_REFRESH_TOKEN_OFFSET, ELIOT_TOKEN_SIZE);
					eliot_refresh_token_modified_time = Eliot_TimeNow();
					eliot_refresh_token = FBK_Address(&eliot_fbk, LCM_DATA_REFRESH_TOKEN_OFFSET);
					DBGPRINT_INTSTRSTR(refresh_token_length, "Eliot_UpdateTokens() Refresh token received:", *new_refresh_token);
					DBGPRINT_INTSTRSTR(eliot_fbk.active_bank, "Eliot_UpdateTokens() Refresh token written to bank:", eliot_refresh_token);
				}
				else
				{
					DBGPRINT_ERRORS_INTSTRSTR(eliot_refresh_token, "Eliot_UpdateTokens()", "Rejected refresh token of length:");
				}
			}
			free(*new_refresh_token);
		}

		if(timeout_counter >= timeout_limit)
		{
			DBGPRINT_INTSTRSTR(__LINE__, "Eliot_UpdateTokens()", "Response timed out");
			eliot_error_count.response_timeouts++;
			return ELIOT_RET_RESPONSE_TIMEOUT;
		}

		DBGPRINT_INTSTRSTR(total_read, "Eliot_UpdateTokens()", "Bytes received:");

		// Cleanup sockets
		cleanupSockets(&ssl, &ctx, &socket);
	}
	else
	{
		DBGPRINT_ERRORS_INTSTRSTR(rx_count, "Eliot_UpdateTokens()", "Connection failure");
		return ELIOT_RET_CONNECT_FAILURE;
	}

	if(str_message[0])
	{
		DBGPRINT_ERRORS_INTSTRSTR(rx_count, "Eliot_UpdateTokens() Auth error:", str_message);
		eliot_error_count.rest++;
		// This prevents a bad refresh token from being re-sent.
		eliot_refresh_token_error_time = Eliot_TimeNow();

		return ELIOT_RET_CLOUD_ERROR;
	}
	return ELIOT_RET_OK;
}

// Synchronize devices with Eliot cloud, add devices that aren't there
// and delete the ones that do not exist locally
void Eliot_SyncDevices()
{
    if (!eliot_lcm_plant_id[0] || !eliot_user_token[0]) return;

    DBGPRINT_INTSTRSTR(0, "Eliot_SyncDevices()", "Syncing devices with Eliot cloud!");

    WOLFSSL*      ssl;
    WOLFSSL_CTX*  ctx;
    uint32_t      socket = 0;
    JSR_ENV       jsr;
    int32_t       http_status=0;
    bool          sync = false;

    bool_t ZoneNamesFound[MAX_NUMBER_OF_ZONES] = { 0 };
    bool_t SceneNamesFound[MAX_NUMBER_OF_SCENES] = { 0 };

    char temp_key[16];
    char temp_value[64];
    char id_string[ELIOT_DEVICE_ID_SIZE];
    char type_string[16];
    char name_string[32];
    char logMessage[80];

    int32_t rx_count = 0;
    int32_t total_read = 0;
    char rx_buf[ELIOT_JSR_BLOCK_SIZE];
    JSR_PAIR pairs[3];

    pairs[0].key_str = "id";
    pairs[0].value_str = id_string;
    pairs[0].value_max_length = ELIOT_DEVICE_ID_SIZE;
    pairs[0].option = JSR_PAIR_KEY;

    pairs[1].key_str = "deviceType";
    pairs[1].value_str = type_string;
    pairs[1].value_max_length = 16;
    pairs[1].option = JSR_PAIR_KEY;

    pairs[2].key_str = "name";
    pairs[2].value_str = name_string;
    pairs[2].value_max_length = 32;
    pairs[2].option = JSR_PAIR_KEY;

    jsr_env_init(&jsr);

    jsr.key_str = temp_key;
    jsr.key_max_length = sizeof(temp_key);

    jsr.value_str = temp_value;
    jsr.value_max_length = sizeof(temp_value);

    jsr.pairs = pairs;
    jsr.pair_count = 3;
    jsr.option = JSR_OPTION_SKIP_HTTP_HEADER;
    jsr.match_threshold = 3;

    if(Eliot_ConnectWithRetry(&ssl, &ctx, &socket))
    {
        char endpoint[80];
        snprintf(endpoint, sizeof(endpoint), "%s/%s/modules", SERVICE_CATALOG_PLANTS_API, eliot_lcm_plant_id);
        Eliot_SendHeader(ssl, "GET", endpoint, 0);

        //Check if eliot cloud is up before syncing
        http_status = Eliot_ReceiveStatus(ssl);
        sync = ((http_status == ELIOT_HTTP_OK) || (http_status == ELIOT_HTTP_NO_CONTENT));
        //If Eliot wasn't properly responding, don't sync
        if(sync)
        {
            do
            {
                rx_count = wolfSSL_read(ssl, rx_buf, sizeof(rx_buf));

                if(rx_count > 0)
                {
                    while(jsr_read(&jsr, rx_buf, rx_count) ==  JSR_RET_END_OBJECT)
                    {
                        bool_t delete_device = true;
                        if (id_string[0] && type_string[0] && name_string[0])
                        {
                            if (strcmp(type_string, LC7001_HUB) == 0)
                            {
                                // If LCM ID matches the ID we have stored internally, do not delete it from Eliot.
                                // Otherwise, it is an orphan and needs to be deleted.
                                if (strcmp(id_string, eliot_lcm_module_id) == 0)
                                {
                                    delete_device = false;
                                }
                            }
                            else if (strcmp(type_string, LC7001_SCENE) == 0)
                            {
                                int index = FindSceneIndexById(id_string);
                                if (index != -1)
                                {
                                    scene_properties properties;
                                    GetSceneProperties(index, &properties);

                                    // If name does not match
                                    if (strcmp(TaskSanitizeString(properties.SceneNameString), name_string) != 0)
                                    {
                                        Eliot_RenameDevice(id_string, properties.SceneNameString);
                                    }

                                    SceneNamesFound[index] = true;
                                    delete_device = false;
                                }
                            }
                            else
                            {
                                int index = FindZoneIndexById(id_string);
                                if (index != -1)
                                {
                                    zone_properties properties;
                                    GetZoneProperties(index, &properties);

                                    // If name does not match
                                    if (strcmp(properties.ZoneNameString, name_string) != 0)
                                    {
                                        snprintf(logMessage, sizeof(logMessage), "Eliot module %s does not match LCM module %s - renaming", name_string, properties.ZoneNameString);
                                        DBGPRINT_INTSTRSTR(0, "Eliot_SyncDevices()", logMessage);
                                        Eliot_RenameDevice(id_string, properties.ZoneNameString);
                                    }
                                    else
                                    {
                                        snprintf(logMessage, sizeof(logMessage), "Eliot module %s matches LCM module %s", name_string, properties.ZoneNameString);
                                        DBGPRINT_INTSTRSTR(0, "Eliot_SyncDevices()", logMessage);
                                    }

                                    ZoneNamesFound[index] = true;
                                    delete_device = false;
                                }
                            }

                            if (delete_device)
                            {
                                snprintf(logMessage, sizeof(logMessage), "Eliot module %s with ID %s not found - deleting", name_string, id_string);
                                DBGPRINT_INTSTRSTR(0, "Eliot_SyncDevices()", logMessage);
                                Eliot_DeleteDevice(id_string);
                            }
                        }
                    }
                    total_read += rx_count;
                }
            } while(rx_count > 0 || !total_read || jsr.array_depth);
        }
        cleanupSockets(&ssl, &ctx, &socket);
    }
    else // Eliot_ConnectWithRetry() failure
    {
        eliot_error_count.sync_conn++;
        DBGPRINT_ERRORS_INTSTRSTR(eliot_error_count.sync_conn, "Eliot_SyncDevices() - Failed to connect", "sync_conn");
        return;
    }

    if(!total_read && sync)
    {
        eliot_error_count.sync_read++;
        DBGPRINT_ERRORS_INTSTRSTR(eliot_error_count.sync_read, "Eliot_SyncDevices() - No response", "sync_read");
        return;
    }

    // Iterate over our list of found indices, and push any devices or scenes
    // whose names were not found in Eliot cloud
    //If Eliot wasn't properly responding, don't sync
    if(sync)
    {
        for (uint8_t index = 0; index < MAX_NUMBER_OF_ZONES; index++)
        {
            zone_properties properties;

            if (GetZoneProperties(index, &properties))
            {
                // If our zone name wasn't found in Eliot push it
                if (!ZoneNamesFound[index])
                {
                    snprintf(logMessage, sizeof(logMessage), "LCM module %s not found in Eliot - adding", properties.ZoneNameString);
                    DBGPRINT_INTSTRSTR(0, "Eliot_SyncDevices()", logMessage);

                    Eliot_AddDevice(properties.deviceType, properties.EliotDeviceId, properties.ZoneNameString);

                    _mutex_lock(&ZoneArrayMutex);
                    SetZoneProperties(index, &properties);
                    _mutex_unlock(&ZoneArrayMutex);

                    // Report initial metrics
                    if (last_metrics_zone_index == index)
                    {
                        // Force this zone's merics to be sent
                        last_metrics_zone_index = -1;
                    }

                    Mqtt_QueueZoneChanged(index);
                }
                else
                {
                    snprintf(logMessage, sizeof(logMessage), "LCM module %s found in Eliot", properties.ZoneNameString);
                    DBGPRINT_INTSTRSTR(0, "Eliot_SyncDevices()", logMessage);
                }
            }
        }

        for (uint8_t index = 0; index < GetAvailableSceneIndex(); index++)
        {
            // If our scene name wasn't found in Eliot push it
            if (!SceneNamesFound[index])
            {
                scene_properties properties;
                GetSceneProperties(index, &properties);

                Eliot_AddDevice(SCENE_TYPE, properties.EliotDeviceId, properties.SceneNameString);

                _mutex_lock(&ZoneArrayMutex);
                SetSceneProperties(index, &properties);
                _mutex_unlock(&ZoneArrayMutex);

                // Report initial metrics
                if (last_metrics_scene_index == index)
                {
                    // Force this scene's merics to be sent
                    last_metrics_scene_index = -1;
                }

                Mqtt_QueueSceneFinished(index);
            }
        }

        DBGPRINT_INTSTRSTR(0, "Eliot_SyncDevices()", "Devices synced with Eliot cloud!");
    }
    else
    {
        DBGPRINT_ERRORS_INTSTRSTR(http_status, "Eliot_SyncDevices() - No proper response", "abort_sync");
    }
}

// Eliot_UserTokenMonitor() looks for local refresh/user tokens and their timestamps,
// refreshing the user token by calling Eliot_UpdateTokens() as necessary.  This
// function is called at intervals from Eliot_REST_Task().

uint32_t Eliot_UserTokenMonitor()
{
	uint32_t now = Eliot_TimeNow();
	uint16_t rt_len = strlen(eliot_refresh_token);

	// Perform a sanity check on the refresh token length.
	if(rt_len<(ELIOT_TOKEN_SIZE>>2) || rt_len>ELIOT_TOKEN_SIZE)
	{
		DBGPRINT_INTSTRSTR(rt_len,  "Eliot_UserTokenMonitor()", "Invalid refresh token length");
		eliot_error_count.refresh_token++;
		return ELIOT_RET_INPUT_ERROR;
	}

	// If this refresh token has caused an invalid grant error, don't
	// try again until we receive another refresh token from the app.
	if(eliot_refresh_token_error_time > eliot_refresh_token_modified_time)
	{
		DBGPRINT_ERRORS_INTSTRSTR(rt_len,  "Eliot_UserTokenMonitor()", "Invalid grant - new refresh token required");
		eliot_error_count.refresh_token++;
		return ELIOT_RET_INPUT_ERROR;
	}

	// At this point, the refresh token is valid.

	// If the user token is missing, refresh.
	if(!eliot_user_token[0])
	{
		logConnectionSequence(COMSTATE_NO_TOKEN, "No User Token, Refreshing");
		DBGPRINT_INTSTRSTR(rt_len,  "Eliot_UserTokenMonitor()", "Refresh token OK, no user token - Refreshing...");
		eliot_error_count.user_token++;
		Eliot_UpdateTokens();
		return ELIOT_RET_OK;
	}

	// If the user token is about to expire, refresh it.
	if( (eliot_user_token_modified_time + ELIOT_USER_TOKEN_LIFESPAN) < now)
	{
		logConnectionSequence(COMSTATE_TOKEN_EXPIRED, "User Token About To Expire, Refreshing");
		DBGPRINT_INTSTRSTR(0 , "Eliot_UserTokenMonitor()", "Preemptive refresh...");
		Eliot_UpdateTokens();
	}
	else
	{
		// Print the user token's time-to-live and last few characters.
		DBGPRINT_INTSTRSTR(eliot_user_token_modified_time + ELIOT_USER_TOKEN_LIFESPAN - now , "Eliot_UserTokenMonitor() Token TTL", eliot_user_token+strlen(eliot_user_token)-8);
	}

	return ELIOT_RET_OK;
}


void Eliot_REST_Task()
{
	ELIOT_TASK_MESSAGE_PTR EliotTaskMessagePtr;
	MQX_TICK_STRUCT   lastTagsTickCount;
	MQX_TICK_STRUCT   lastSyncTickCount;
	MQX_TICK_STRUCT   currentTickCount;
	int_32            ticksDelta;
	bool              overflowFlag;
	bool              firstSync = true;
	uint32_t          now = 0;

	logConnectionSequence(COMSTATE_START_REST, "Starting REST_Task");

	// Open a message queue
	_queue_id EliotTaskMessageQID = _msgq_open(ELIOT_TASK_MESSAGE_QUEUE, 0);

	// Create a message pool
	EliotTaskMessagePoolID = _msgpool_create(sizeof(ELIOT_TASK_MESSAGE), NUMBER_OF_ELIOT_MESSAGES_IN_POOL, 0, 0);

	_time_delay(ELIOT_INITIAL_TASK_DELAY_MS);

	_time_get_elapsed_ticks(&lastTagsTickCount);
	_time_get_elapsed_ticks(&lastSyncTickCount);

	eliot_fbk.bank_info[0] = (FBK_Bank_Info*)LCM_DATA_ELIOT_BANK_0_START_ADDRESS;
	eliot_fbk.bank_info[1] = (FBK_Bank_Info*)LCM_DATA_ELIOT_BANK_1_START_ADDRESS;
	FBK_Init(&eliot_fbk);

	// If the primary key or refresh tokens in flash are valid, use them.
	Eliot_UseFlashString(&eliot_primary_key, LCM_DATA_PRIMARY_KEY_OFFSET, 1, ELIOT_PRIMARY_KEY_SIZE);
	Eliot_UseFlashString(&eliot_refresh_token, LCM_DATA_REFRESH_TOKEN_OFFSET, 1, ELIOT_TOKEN_SIZE);

	Eliot_ResetErrorCounters();

	while (1)
	{
        // Increment this task's watchdog counter
        watchdog_task_counters[ELIOT_REST_TASK]++;
	    now = Eliot_TimeNow();

		// If cloud operations disabled or we are in a quota exceeded state, continue
		if (!cloudState || (now >= eliot_last_quota_exceeded_time) && (now < (eliot_last_quota_exceeded_time + ELIOT_QUOTA_EXCEEDED_PERIOD)))
		{
			_time_delay(50);
			continue;
		}

		eliot_loop_counter++;
		EliotTaskMessagePtr = _msgq_poll(EliotTaskMessageQID);
		if (EliotTaskMessagePtr)
		{
			Eliot_HandleTaskMessage(EliotTaskMessagePtr);
		}

		_time_get_elapsed_ticks(&currentTickCount);
		ticksDelta = _time_diff_milliseconds(&currentTickCount, &lastTagsTickCount, &overflowFlag);
		if (ticksDelta > ELIOT_LAST_TAGS_DELAY_MS)
		{
		    Eliot_SendAllChangedDeviceStatus();
		    _time_get_elapsed_ticks(&lastTagsTickCount);
		}

		// Run every ~30 seconds
		if(!(eliot_loop_counter & ELIOT_LOOP_INTERVAL_MASK))
		{
			Eliot_UserTokenMonitor();

			// Because the user token is no longer a #define, we can't rely on
			// it being valid on boot.  Calling this function here allows the
			// plant and module init tasks to be deferred until a user token
			// is obtained.
			Eliot_LcmPlantAndModuleInit();

			DBGPRINT_INTSTRSTR(strlen(eliot_primary_key) , "eliot_primary_key", eliot_primary_key);
			DBGPRINT_INTSTRSTR(strlen(eliot_user_token), "eliot_user_token", eliot_user_token);
			DBGPRINT_INTSTRSTR(strlen(eliot_refresh_token), "eliot_refresh_token", eliot_refresh_token);
			Eliot_PrintErrorCounters();
		}

		// Only setup synchronization timer if we have a valid token and can send MQTT messages
		if(eliot_lcm_plant_id[0] && eliot_user_token[0] && Eliot_MqttIsConnected())
		{
		    _time_get_elapsed_ticks(&currentTickCount);
		    ticksDelta = _time_diff_milliseconds(&currentTickCount, &lastSyncTickCount, &overflowFlag);
		    if (ticksDelta > ELIOT_LAST_SYNC_DELAY_MS || firstSync)
		    {
		        Eliot_SyncDevices();
		        _time_get_elapsed_ticks(&lastSyncTickCount);
		        firstSync = false;
		    }
		}

		_time_delay(50);
	}
}
