#ifndef _SA_CLIENT_H_
#define _SA_CLIENT_H_
#include "susiaccess_def.h"

typedef enum {
   saclient_false = -1,               // Has error. 
   saclient_success = 0,               // No error.
   saclient_config_error = 1,
   saclient_profile_error = 2,
   saclient_no_init, 
   saclient_callback_null,
   saclient_callback_error,
   saclient_no_connnect,
   saclient_connect_error,
   saclient_init_error,
   saclient_network_sock_timeout = 0x10,
   saclient_network_sock_error,
   saclient_send_data_error,
   saclient_report_agentinfo_error,
   saclient_send_willmsg_error,
   /*TBD*/
} saclient_result;

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once

#ifndef SACLIENT_API
	#define SACLIENT_API WINAPI
#endif
#else
	#define SACLIENT_API
#endif

typedef void (*SACLIENT_CONNECTED_CALLBACK)();
typedef void (*SACLIENT_LOSTCONNECT_CALLBACK)();
typedef void (*SACLIENT_DISCONNECT_CALLBACK)();
typedef void (*SACLIENT_MESSAGE_RECV_CALLBACK)(char* topic, susiaccess_packet_body_t *pkt, void *pRev1, void* pRev2);

#ifdef __cplusplus
extern "C" {
#endif

int SACLIENT_API saclient_initialize(susiaccess_agent_conf_body_t * config, susiaccess_agent_profile_body_t * profile, void * loghandle);
void SACLIENT_API saclient_uninitialize();
int SACLIENT_API saclient_connect();
int SACLIENT_API  saclient_connectserver(char const * ip, int port, char const * mqttauth);
void SACLIENT_API saclient_disconnect();
void SACLIENT_API saclient_reconnect();
void SACLIENT_API saclient_connection_callback_set(SACLIENT_CONNECTED_CALLBACK on_connect, SACLIENT_LOSTCONNECT_CALLBACK on_lost_connect, SACLIENT_DISCONNECT_CALLBACK on_disconnect);
int SACLIENT_API saclient_publish(char const * topic, susiaccess_packet_body_t const * pkt);
int SACLIENT_API saclient_subscribe(char const * topic, SACLIENT_MESSAGE_RECV_CALLBACK msg_recv_callback);
int SACLIENT_API  saclient_getsocketaddress(char* clientip, int size);

#ifdef __cplusplus
}
#endif

#endif