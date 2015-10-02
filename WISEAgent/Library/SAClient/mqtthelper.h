#ifndef _CAGENT_MQTTHELPER_H_
#define _CAGENT_MQTTHELPER_H_
#include <mosquitto.h>

#define DEF_WILLMSG_TOPIC        "/cagent/admin/%s/willmessage"	/*publish*/
#define DEF_INFOACK_TOPIC		"/cagent/admin/%s/agentinfoack"	/*publish*/
#define DEF_AUTOREPORT_TOPIC	"/cagent/admin/%s/%s"	/*publish*/
#define DEF_AGENTCONTROL_TOPIC	"/server/admin/+/agentctrl"	/*Subscribe*/

#define DEF_CALLBACKACK_TOPIC		"/cagent/admin/%s/agentcallbackack"	/*publish*/
#define DEF_CALLBACKREQ_TOPIC		"/cagent/admin/%s/agentcallbackreq"	/*Subscribe*/

#define DEF_ACTIONACK_TOPIC		"/cagent/admin/%s/agentactionack"	/*Subscribe*/
#define DEF_ACTIONREQ_TOPIC		"/cagent/admin/%s/agentactionreq"	/*publish*/

#define DEF_CUSTOM_CALLBACKACK_TOPIC		"/cagent/%s/%s/agentcallbackack"	/*publish*/
#define DEF_CUSTOM_CALLBACKREQ_TOPIC		"/cagent/%s/%s/agentcallbackreq"	/*Subscribe*/

#define DEF_CUSTOM_ACTIONACK_TOPIC		"/cagent/%s/%s/agentactionack"	/*Subscribe*/
#define DEF_CUSTOM_ACTIONREQ_TOPIC		"/cagent/%s/%s/agentactionreq"	/*publish*/

typedef enum mqtt_connection_result{
	mqtt_success = 0,			/** * 0 - success*/
	mqtt_unaccept_protocol,		/** * 1 - connection refused (unacceptable protocol version)*/
	mqtt_reject_identifier,		/** * 2 - connection refused (identifier rejected)*/
	mqtt_unavailable_broker,	/** * 3 - connection refused (broker unavailable)*/
}conn_connection_result_enum;

typedef void (*MQTT_CONNECT_CALLBACK)();
typedef void (*MQTT_LOSTCONNECT_CALLBACK)();
typedef void (*MQTT_DISCONNECT_CALLBACK)();
typedef void (*MQTT_MESSAGE_CALLBACK)(struct mosquitto *, void *, const struct mosquitto_message *);

typedef struct{
  struct mosquitto *mosq;
  struct topic_entry *subscribe_topics;
  MQTT_CONNECT_CALLBACK on_connect;
  MQTT_DISCONNECT_CALLBACK on_disconnect;
}cagent_callback_body_t;

struct mosquitto * MQTT_Initialize(char const * devid);
void MQTT_Uninitialize(struct mosquitto *mosq);
void MQTT_Callback_Set(struct mosquitto *mosq, void* connect_cb, void* lostconnect_cb, void* disconnectcb, void* message_cb);
int MQTT_SetTLS(struct mosquitto *mosq, const char *cafile, const char *capath, const char *certfile, const char *keyfile, int (*pw_callback)(char *buf, int size, int rwflag, void *userdata));
int MQTT_SetTLSPSK(struct mosquitto *mosq, const char *psk, const char *identity, const char *ciphers);
int MQTT_Connect(struct mosquitto *mosq, char const * ip, int port, char const * username, char const * password, int keepalive, char* willtopic, const void *willmsg, int msglen );
void MQTT_Disconnect(struct mosquitto *mosq);
int MQTT_Publish(struct mosquitto *mosq,  char* topic, const void *msg, int msglen, int qos, bool retain);
int MQTT_Subscribe(struct mosquitto *mosq,  char* topic, int qos);
void MQTT_Unsubscribe(struct mosquitto *mosq,  char* topic);
int MQTT_GetSocket(struct mosquitto *mosq);
#endif
