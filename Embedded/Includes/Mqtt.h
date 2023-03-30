/*
 * Mqtt.h
 *
 *  Created on: Mar 3, 2019
 *      Author: Legrand
 */

#ifndef MQTT_H_
#define MQTT_H_

#include "includes.h"
#include "wolfmqtt/mqtt_packet.h"
#include "wolfmqtt/mqtt_client.h"

#define MAX_BUFFER_SIZE         1024    /* Maximum size for network read/write callbacks */

#define DEFAULT_MQTT_HOST       "iot.eclipse.org" /* broker.hivemq.com */
#define DEFAULT_CMD_TIMEOUT_MS  30000
#define DEFAULT_CON_TIMEOUT_MS  5000
#define DEFAULT_MQTT_QOS        MQTT_QOS_0
#define DEFAULT_KEEP_ALIVE_SEC  60
#define DEFAULT_CLIENT_ID       "WolfMQTTClient"
#define WOLFMQTT_TOPIC_NAME     "wolfMQTT/example/"
#define DEFAULT_TOPIC_NAME      WOLFMQTT_TOPIC_NAME"testTopic"
#define DEFAULT_AUTH_METHOD     "EXTERNAL"
#define MQTT_MESSAGE_SIZE       80
#define MAX_PACKET_ID           ((1 << 16) - 1)

#define TOKEN_EXPIRY_SEC        (60 * 60 * 23)

#define SIG_FMT           "%s\n%ld"
    /* [device name (URL Encoded)]\n[Expiration sec UTC] */

#define PASSWORD_FMT      "SharedAccessSignature sr=%s&sig=%s&se=%ld"
    /* sr=[device name (URL Encoded)]
       sig=[HMAC-SHA256 of ELIOT_SIG_FMT using ELIOT_KEY (URL Encoded)]
       se=[Expiration sec UTC] */


typedef enum _MQTTCtxState {
    WMQ_BEGIN = 0,
    WMQ_NET_INIT,
    WMQ_INIT,
    WMQ_TCP_CONN,
    WMQ_MQTT_CONN,
    WMQ_SUB,
    WMQ_PUB,
    WMQ_WAIT_MSG,
    WMQ_UNSUB,
    WMQ_DISCONNECT,
    WMQ_NET_DISCONNECT,
    WMQ_DONE
} MQTTCtxState;

/* MQTT Client context */
typedef struct _MQTTCtx {
    MQTTCtxState stat;

    void* app_ctx; /* For storing application specific data */

    /* client and net containers */
    MqttClient client;
    MqttNet net;

    /* temp mqtt containers */
    MqttConnect connect;
    MqttMessage lwt_msg;
    MqttSubscribe subscribe;
    MqttUnsubscribe unsubscribe;
    MqttTopic topics[1], *topic;
    MqttPublish publish;
    MqttDisconnect disconnect;

    /* configuration */
    MqttQoS qos;
    const char* app_name;
    const char* host;
    const char* username;
    const char* password;
    const char* topic_name;
    const char* pub_file;
    const char* client_id;
    byte *tx_buf, *rx_buf;
    int return_code;
    int use_tls;
    int retain;
    int enable_lwt;
    word32 cmd_timeout_ms;
    word32 start_sec;
    word16 keep_alive_sec;
    word16 port;
    byte clean_session;
    byte test_mode;
} MQTTCtx;

static int mStopRead = 0;

/* Local context for Net callbacks */
typedef enum {
    SOCK_BEGIN = 0,
    SOCK_CONN,
} NB_Stat;

typedef struct _SocketContext {
    int fd;
    NB_Stat stat;
    struct sockaddr_in addr;
    MQTTCtx* mqttCtx;
} SocketContext;

void Url_Encoder_Init(void);
char* Url_Encode(char*, unsigned char*, char*);

int SasTokenCreate(char*, int, const char*, const char*);

word16 Mqtt_Get_PacketID(void);
void Mqtt_Init_Ctx(MQTTCtx*);

int MqttClientNet_Init(MqttNet*, MQTTCtx*);
int MqttClientNet_DeInit(MqttNet*);

#endif /* MQTT_H_ */
