/*
 * Mqtt.c
 *
 *  Created on: Mar 3, 2019
 *      Author: Legrand
 */

#include "Mqtt.h"

#include "wolfssl/ssl.h"
#include "wolfssl/error-ssl.h"
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/coding.h>
#include <wolfssl/wolfcrypt/hmac.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>

#include <stdlib.h>

static int mPacketIdLast;
static char mRfc3986[256] = {0};

void Url_Encoder_Init(void)
{
    int i;
    for (i = 0; i < 256; i++){
        mRfc3986[i] = XISALNUM( i) || i == '~' || i == '-' || i == '.' || i == '_' ? i : 0;
    }
}

int SasTokenCreate(char* sasToken, int sasTokenLen, const char* encodedKey, const char* deviceId)
{
    int rc;
    byte decodedKey[32+1];
    word32 decodedKeyLen = (word32)sizeof(decodedKey);
    char deviceName[150]; /* uri */
    char sigData[200]; /* max uri + expiration */
    byte sig[32];
    byte base64Sig[32*2];
    word32 base64SigLen = (word32)sizeof(base64Sig);
    byte encodedSig[32*4];
    long lTime;
    Hmac hmac;

    if (sasToken == NULL) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    /* Decode Key */
    rc = Base64_Decode((const byte*)encodedKey, (word32)strlen(encodedKey), decodedKey, &decodedKeyLen);
    if (rc != 0) {
        return rc;
    }

    /* Get time */
    TIME_STRUCT mqxTime;
    _time_get(&mqxTime);
    lTime = mqxTime.SECONDS;

    if (rc != 0) {
        return rc;
    }
    lTime += TOKEN_EXPIRY_SEC;

    /* URL encode uri (device name) */
    Url_Encode(mRfc3986, (byte*)deviceId, deviceName);

    /* Build signature sting "uri \n expiration" */
    snprintf(sigData, sizeof(sigData), SIG_FMT, deviceName, lTime);

    /* HMAC-SHA256 Hash sigData using decoded key */
    rc = wc_HmacSetKey(&hmac, 2, decodedKey, decodedKeyLen);
    if (rc < 0) {
        return rc;
    }
    rc = wc_HmacUpdate(&hmac, (byte*)sigData, (word32)strlen(sigData));
    if (rc < 0) {
        return rc;
    }
    rc = wc_HmacFinal(&hmac, sig);
    if (rc < 0) {
        return rc;
    }

    /* Base64 encode signature */
    memset(base64Sig, 0, base64SigLen);
    rc = Base64_Encode_NoNl(sig, sizeof(sig), base64Sig, &base64SigLen);
    if (rc < 0) {
        return rc;
    }

    /* URL encode signature */
    Url_Encode(mRfc3986, base64Sig, (char*)encodedSig);

    /* Build sasToken */
    snprintf(sasToken, sasTokenLen, PASSWORD_FMT, deviceName, encodedSig, lTime);

    return 0;
}

void Mqtt_Init_Ctx(MQTTCtx* mqttCtx)
{
    memset(mqttCtx, 0, sizeof(MQTTCtx));
    mqttCtx->host = DEFAULT_MQTT_HOST;
    mqttCtx->qos = DEFAULT_MQTT_QOS;
    mqttCtx->clean_session = 1;
    mqttCtx->keep_alive_sec = DEFAULT_KEEP_ALIVE_SEC;
    mqttCtx->client_id = DEFAULT_CLIENT_ID;
    mqttCtx->topic_name = DEFAULT_TOPIC_NAME;
    mqttCtx->cmd_timeout_ms = DEFAULT_CMD_TIMEOUT_MS;
}

word16 Mqtt_Get_PacketID(void)
{
    mPacketIdLast = (mPacketIdLast >= MAX_PACKET_ID) ?
        1 : mPacketIdLast + 1;
    return (word16)mPacketIdLast;
}

int MqttClientNet_Init(MqttNet* net, MQTTCtx* mqttCtx)
{
    if (net) {
        SocketContext* sockCtx;

        memset(net, 0, sizeof(MqttNet));

        sockCtx = (SocketContext*)WOLFMQTT_MALLOC(sizeof(SocketContext));
        if (sockCtx == NULL) {
            return MQTT_CODE_ERROR_MEMORY;
        }
        net->context = sockCtx;
        memset(sockCtx, 0, sizeof(SocketContext));
        sockCtx->stat = SOCK_BEGIN;
        sockCtx->mqttCtx = mqttCtx;
    }

    return MQTT_CODE_SUCCESS;
}

int MqttClientNet_DeInit(MqttNet* net)
{
    if (net) {
        if (net->context) {
            WOLFMQTT_FREE(net->context);
        }
        memset(net, 0, sizeof(MqttNet));
    }
    return 0;
}

char* Url_Encode(char* table, unsigned char *s, char *enc)
{
    for (; *s; s++){
        if (table[*s]) {
            snprintf(enc, 2, "%c", table[*s]);
        }
        else {
            snprintf(enc, 4, "%%%02x", *s);
        }
        while (*++enc); /* locate end */
    }
    return enc;
}
