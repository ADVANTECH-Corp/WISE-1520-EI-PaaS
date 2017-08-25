/*
 * WiseCarrier_MQTT.c
 *
 *  Created on: 2016¦~3¤ë14¤é
 *      Author: Fred.Chang
 */
#include "user.h"
#include "simplelink.h"
#include "network_if.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "common.h"
#include "sl_mqtt_client.h"
#include "WISECarrier_MQTT.h"
#include "wiseutility.h"

/*===============================================================*/
/*Operate Lib in MQTT 3.1 mode.*/
#define MQTT_3_1_1              false /*MQTT 3.1.1 */
#define MQTT_3_1                true /*MQTT 3.1*/

/*Defining QOS levels*/
#define QOS0                    0
#define QOS1                    1
#define QOS2                    2

/*Retain Flag. Used in publish message. */
#define RETAIN                  1

/*Defining Number of topics*/
#define TOPIC_COUNT             3

/*#define WILL_TOPIC              "Client"
#define WILL_MSG                "Client Stopped"*/
#define WILL_TOPIC              "/cagent/admin/devId/willmessage"
#define WILL_MSG                ""
#define WILL_QOS                QOS2
#define WILL_RETAIN             false

//#define SERVER_ADDRESS          "messagesight.demos.ibm.com"
//#define SERVER_ADDRESS          "192.84.45.44"

#define PORT_NUMBER             1883

#define MAX_BROKER_CONN         1

#define SERVER_MODE             MQTT_3_1
/*Specifying Receive time out for the Receive task*/
#define RCV_TIMEOUT             30

/*Background receive task priority*/
#define TASK_PRIORITY           3

/* Keep Alive Timer value*/
#define KEEP_ALIVE_TIMER        60

static void
Mqtt_Recv(void *app_hndl, const char  *topstr, long top_len, const void *payload,
          long pay_len, bool dup,unsigned char qos, bool retain);
static void sl_MqttEvt(void *app_hndl,long evt, const void *buf,
                       unsigned long len);
static void sl_MqttDisconnect(void *app_hndl);

typedef struct connection_config{
    SlMqttClientCtxCfg_t broker_config;
    void *clt_ctx;
    unsigned char *client_id;
    unsigned char *usr_name;
    unsigned char *usr_pwd;
    bool is_clean;
    unsigned int keep_alive_time;
    SlMqttClientCbs_t CallBAcks;
    int num_topics;
    char *topic[TOPIC_COUNT];
    unsigned char qos[TOPIC_COUNT];
    SlMqttWill_t will_params;
    bool is_connected;
} connect_config;

/* connection configuration */
char *secu_file_list[3] = { "/cert/ckey.der", "/cert/client.der" , "/cert/ca.der"};
connect_config usr_connect_config[] =
{
    {
        {
            {
            	SL_MQTT_NETCONN_URL,
				"172.22.12.60",
                1122,
				SL_SO_SEC_METHOD_SSLv3_TLSV1_2,
				0,
                3,
				secu_file_list
            },
            SERVER_MODE,
            true,
        },
        NULL,
		NULL,
        NULL,
        NULL,
        true,
        KEEP_ALIVE_TIMER,
        {Mqtt_Recv, sl_MqttEvt, sl_MqttDisconnect},
        1,
        {""},
        {QOS2},
        {WILL_TOPIC,WILL_MSG,WILL_QOS,WILL_RETAIN},
        false
    }
};

/* library configuration */
SlMqttClientLibCfg_t Mqtt_Client={
    0,
    TASK_PRIORITY,
    30,
    true,
    (long(*)(const char *, ...))wiseprint
};

/* callback array */
#define SUB_TOPIC_LEN 64
typedef struct wisemqtt_sub {
    char topic[SUB_TOPIC_LEN];
    WICAR_MESSAGE_CB cb;
} WiseMqtt_SUB;

#define MAX_SUBS 8
static int gSubCount = 0;
static WiseMqtt_SUB gSubs[MAX_SUBS] = { 0 };

static WICAR_CONNECT_CB g_connect_cb;
static WICAR_DISCONNECT_CB g_disconnect_cb;
static WICAR_LOSTCONNECT_CB g_lostconnect_cb;
static void* g_userdata;

/******************************************************************************************/
//****************************************************************************
//! Defines Mqtt_Pub_Message_Receive event handler.
//! Client App needs to register this event handler with sl_ExtLib_mqtt_Init
//! API. Background receive task invokes this handler whenever MQTT Client
//! receives a Publish Message from the broker.
//!
//!\param[out]     topstr => pointer to topic of the message
//!\param[out]     top_len => topic length
//!\param[out]     payload => pointer to payload
//!\param[out]     pay_len => payload length
//!\param[out]     retain => Tells whether its a Retained message or not
//!\param[out]     dup => Tells whether its a duplicate message or not
//!\param[out]     qos => Tells the Qos level
//!
//!\return none
//****************************************************************************
static char messagebuf[1024] = { 0 };
static void Mqtt_Recv(void *app_hndl, const char *topstr, long top_len,
		const void *payload, long pay_len, bool dup, unsigned char qos,
		bool retain) {

	strncpy(messagebuf, topstr, 1024);
    //messagebuf[top_len] = 0;
    //wiseprint("########## Topic: %s\r\n", messagebuf);
    WiseMqtt_SUB *sub;
    int iSub;
    for (iSub = 0; iSub < gSubCount; iSub++) {
        sub = &gSubs[iSub];
        if (strncmp(messagebuf, sub->topic,strlen(sub->topic)) == 0) {
			strncpy(messagebuf,payload,pay_len);
			messagebuf[pay_len] = 0;
            sub->cb(sub->topic, messagebuf, pay_len, g_userdata);
            break;
        }
    }

    //TODO
    return;
}

//****************************************************************************
//! Defines sl_MqttEvt event handler.
//! Client App needs to register this event handler with sl_ExtLib_mqtt_Init
//! API. Background receive task invokes this handler whenever MQTT Client
//! receives an ack(whenever user is in non-blocking mode) or encounters an error.
//!
//! param[out]      evt => Event that invokes the handler. Event can be of the
//!                        following types:
//!                        MQTT_ACK - Ack Received
//!                        MQTT_ERROR - unknown error
//!
//!
//! \param[out]     buf => points to buffer
//! \param[out]     len => buffer length
//!
//! \return none
//****************************************************************************
static void sl_MqttEvt(void *app_hndl, long evt, const void *buf,
		unsigned long len) {
    /*int i;
    wiseprint("########## evt: %d\r\n", evt);
    switch (evt) {
      case SL_MQTT_CL_EVT_PUBACK:
        wiseprint("PubAck:\n\r");
        wiseprint("%s\n\r", buf);
        break;

      case SL_MQTT_CL_EVT_SUBACK:
        wiseprint("\n\rGranted QoS Levels are:\n\r");

        for (i = 0; i < len; i++) {
            wiseprint("QoS %d\n\r", ((unsigned char*) buf)[i]);
        }
        break;

      case SL_MQTT_CL_EVT_UNSUBACK:
        wiseprint("UnSub Ack \n\r");
        wiseprint("%s\n\r", buf);
        break;

      default:
        break;

    }*/
}

//****************************************************************************
//
//! callback event in case of MQTT disconnection
//!
//! \param app_hndl is the handle for the disconnected connection
//!
//! return none
//
//****************************************************************************

static void sl_MqttDisconnect(void *app_hndl) {
	g_lostconnect_cb(g_userdata);
	WiCar_MQTT_Reconnect();
   //TODO
}

//static void *app_hndl = (void*)usr_connect_config;
//static connect_config *local_con_conf = (connect_config *)app_hndl;
static connect_config* connconf = &usr_connect_config[0];
static OsiLockObj_t mqttmutex;
/*******************************************************************/
WISE_CARRIER_API const char* WiCar_MQTT_LibraryTag() {
	return "TI_MQTT";
}

WISE_CARRIER_API bool WiCar_MQTT_Init(WICAR_CONNECT_CB on_connect,
		WICAR_DISCONNECT_CB on_disconnect, void *userdata) {
	int ret;
	ret = sl_ExtLib_MqttClientInit(&Mqtt_Client);
	if (ret != 0) {
		// lib initialization failed
		wiseprint("MQTT Client lib initialization failed\n\r");
		wise_reboot(10);
		return false;
	}

	g_connect_cb = on_connect;
	g_disconnect_cb = on_disconnect;
	g_userdata = userdata;
	return true;
}

WISE_CARRIER_API void WiCar_MQTT_Uninit() {
	wise_reboot(10);
}

WISE_CARRIER_API bool WiCar_MQTT_SetWillMsg(const char* topic, const void *msg,
		int msglen) {
	connconf->will_params.will_topic = topic;
	connconf->will_params.will_msg = msg;
	return true;
}

WISE_CARRIER_API bool WiCar_MQTT_SetAuth(const char* username,
		const char* password) {
	if(strlen(username) != 0)
		connconf->usr_name = (unsigned char *)username;
	else
		connconf->usr_name = NULL;

	if(strlen(password) != 0)
		connconf->usr_pwd = (unsigned char *)password;
	else
		connconf->usr_pwd = NULL;

	return true;
}

WISE_CARRIER_API bool WiCar_MQTT_SetKeepLive(int keepalive) {
	connconf->keep_alive_time = keepalive;
	return true;
}

#define DATE                4    /* Current Date */
#define MONTH               8     /* Month 1-12 */
#define YEAR                2016  /* Current year */
#define HOUR                12    /* Time - hours */
#define MINUTE              32    /* Time - minutes */
#define SECOND              0     /* Time - seconds */

static int set_time() {
    long retVal;
    SlDateTime_t time;
    time.sl_tm_day = DATE;
    time.sl_tm_mon = MONTH;
    time.sl_tm_year = YEAR;
    time.sl_tm_sec = SECOND;
    time.sl_tm_hour = HOUR;
    time.sl_tm_min = MINUTE;

    retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
	SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME, sizeof(SlDateTime_t),
            (_u8 *)(&time));

    ASSERT_ON_ERROR(retVal);
    return 1;
}

static bool CheckFileIsExist(char *filename) {
	int iRetVal = 0;
	if (filename == NULL || strlen(filename) == 0)
		return true;

	SlFsFileInfo_t fsFileInfo;
	iRetVal = sl_FsGetInfo(filename, 0, &fsFileInfo);
	if(iRetVal == 0) {
		return true;
	}

	return false;
}

WISE_CARRIER_API bool WiCar_MQTT_SetTls(const char *cafile, const char *capath,
		const char *certfile, const char *keyfile, const char* password) {
	SlMqttServer_t  *server_info = &connconf->broker_config.server_info;
	int i;
	if (cafile == NULL && capath == NULL)
		server_info->n_files = 2;
	for(i = 0 ; i < server_info->n_files ; i++) {
		if(!CheckFileIsExist(server_info->secure_files[i])) {
			wiseprint("%s not found.\r\n",server_info->secure_files[i]);
			return false;
		}
	}

	set_time();
    connconf->broker_config.server_info.netconn_flags |= SL_MQTT_NETCONN_SEC;
	return true;
}

WISE_CARRIER_API bool WiCar_MQTT_SetTlsPsk(const char *psk,
		const char *identity, const char *ciphers) {
    /* not support*/
	return false;
}

WISE_CARRIER_API bool WiCar_MQTT_Reconnect() {
	//RebootMCU(60);
	if (connconf->is_connected == true)
		sl_ExtLib_MqttClientDisconnect((void*) connconf->clt_ctx);

	int ret;
	ret = sl_ExtLib_MqttClientConnect((void*) connconf->clt_ctx,
			connconf->is_clean, connconf->keep_alive_time);
	if (ret & 0xff != 0) {
		wiseprint("\n\rBroker connect fail for conn %d.\n\r", ret);
		connconf->is_connected = false;
		return false;
	} else {
		wiseprint("\n\rSuccess: conn to Broker\n\r ");
		connconf->is_connected = true;
		return true;
	}
	return false;
}

WISE_CARRIER_API bool WiCar_MQTT_Connect(const char* address, int port,
		const char* clientdId, WICAR_LOSTCONNECT_CB on_lostconnect) {

	if(connconf->is_connected == false) {
		int ret;
		connconf->client_id = (unsigned char *)clientdId;
		connconf->broker_config.server_info.server_addr = address;
		connconf->broker_config.server_info.port_number = port;

		//create client context
		connconf->clt_ctx = sl_ExtLib_MqttClientCtxCreate(
				&connconf->broker_config, &connconf->CallBAcks, connconf);

		//
		// Set Client ID
		//
		sl_ExtLib_MqttClientSet((void*) connconf->clt_ctx,
				SL_MQTT_PARAM_CLIENT_ID, connconf->client_id,
				strlen((char*) (connconf->client_id)));

		//
		// Set will Params
		//
		if (connconf->will_params.will_topic != NULL) {
			sl_ExtLib_MqttClientSet((void*)connconf->clt_ctx,
			SL_MQTT_PARAM_WILL_PARAM, &(connconf->will_params),
									sizeof(SlMqttWill_t));
		}

		//
		// setting username and password
		//
		if (connconf->usr_name != NULL) {
			sl_ExtLib_MqttClientSet((void*)connconf->clt_ctx,
			SL_MQTT_PARAM_USER_NAME, connconf->usr_name,
								strlen((char*)connconf->usr_name));

			if (connconf->usr_pwd != NULL) {
				sl_ExtLib_MqttClientSet((void*)connconf->clt_ctx,
				SL_MQTT_PARAM_PASS_WORD, connconf->usr_pwd,
								strlen((char*)connconf->usr_pwd));
			}
		}

		//
		// connectin to the broker
		//
		wiseprint("connconf->broker_config.server_info.netconn_flags = %08X\n",
				connconf->broker_config.server_info.netconn_flags);
		ret = sl_ExtLib_MqttClientConnect((void*) connconf->clt_ctx,
				connconf->is_clean, connconf->keep_alive_time);
		if (ret & 0xff != 0) {
			wiseprint("\n\rBroker connect fail for conn %d.\n\r", ret);
			return false;
		} else {
			wiseprint("\n\rSuccess: conn to Broker\n\r ");
			connconf->is_connected = true;
			g_connect_cb(g_userdata);
			g_lostconnect_cb = on_lostconnect;
			return true;
		}
	}

	return false;
}

WISE_CARRIER_API bool WiCar_MQTT_Disconnect(int force) {
	if(connconf->is_connected == true) {
		sl_ExtLib_MqttClientDisconnect(connconf->clt_ctx);
		connconf->is_connected = false;
		g_disconnect_cb(g_userdata);
		return true;
	}
	return false;
}

WISE_CARRIER_API bool WiCar_MQTT_Publish(const char* topic, const void *msg,
		int msglen, int retain, int qos) {
	if(connconf->is_connected == true) {
		int ret = 0;
		wiseprint("## topic = [%s]\r\n",topic);
		wiseprint("## msg = [%s]\r\n",msg);
		ret = sl_ExtLib_MqttClientSend((void*)connconf->clt_ctx, topic,msg,msglen,qos,retain);
		if(ret < 0) {
			//re-send
			g_lostconnect_cb(g_userdata);
			if(WiCar_MQTT_Reconnect()) {
				ret = sl_ExtLib_MqttClientSend((void*) connconf->clt_ctx, topic, msg, msglen, qos, retain);
				return true;
			}
			return false;
		}
		return true;
	}
	return false;
}

WISE_CARRIER_API bool WiCar_MQTT_Subscribe(const char* topic, int qos, WICAR_MESSAGE_CB on_recieve) {
	if (gSubCount == MAX_SUBS)
		return false;
	//
	// Subscribe to topics
	//
	connconf->topic[0] = (char *)topic;
	strncpy(gSubs[gSubCount].topic, topic, SUB_TOPIC_LEN);

	_u8 qq = qos;
	if (sl_ExtLib_MqttClientSub((void*) connconf->clt_ctx, connconf->topic,
			&qq, 1) < 0) {
		wiseprint("\n\r Subscription Error for topic [%s]\n\r",
				connconf->topic[0]);
		return false;
	} else {
		gSubs[gSubCount].cb = on_recieve;
		gSubCount++;

		int iSub;
		wiseprint("Client subscribed on following topics:\n\r");
		for (iSub = 0; iSub < gSubCount; iSub++) {
			wiseprint("%s\n\r", gSubs[iSub].topic);
		}
	}

	return true;
}

WISE_CARRIER_API bool WiCar_MQTT_UnSubscribe(const char* topic) {
	return false;
}

WISE_CARRIER_API bool WiCar_MQTT_GetLocalIP(const char *address) {
	strcpy(address, (char *) GetIp());
	return true;
}

WISE_CARRIER_API const char *WiCar_MQTT_GetCurrentErrorString() {
	return "none";
}

