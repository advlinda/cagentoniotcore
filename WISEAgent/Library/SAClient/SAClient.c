#include "SAClientLog.h"
#include "network.h"
#include "platform.h"
#include "SAClient.h"
#include "common.h"
#include "mqtthelper.h"
#include "topic.h"
#include "SAClientParser.h"
#include "samanagerloader.h"
#include "DES.h"
#include "Base64.h"

#define AGNET_INFO_CMD				1 
#define cagent_catalog_susi_func	4
#define cagent_agent_info			21
#define cagent_private_request_max	99
#define cagent_global				109
#define glb_get_init_info_rep		116
#define cagent_action_general		2001
#define general_info_upload_rep		2055
//#define IP_LIST_BUF_LEN             32

#define DEF_DES_KEY					"29B4B9C5"
#define DEF_DES_IV					"42b19631"

#define AUTOREPORT_TOPIC "deviceinfo"

/** Action(params) enum define preserved for SA3.0*/
typedef enum cagent_action_request{
	cagent_start_file_download = 0,
	cagent_stop_file_download,
	cagent_connect_server,
	cagent_disconnect_server,
	cagent_connect_status,
	cagent_current_version,
	
	//susi agent add (10--100)
	cagent_request_device_monitoring = 10,
	cagent_request_power_onoff,
	cagent_request_remote_kvm,
	cagent_request_protection,
	cagent_request_recovery,
	cagent_request_software_monitoring,
	cagent_request_global,
	cagent_request_terminal,
	cagent_request_screenshot,

	//QA Test and Customized  30001-31000
	cagent_qa_test = 30001,
	cagent_custom_action,
	cagent_request_custom_max
}cagent_action_request_t;

void* g_coreloghandle = NULL;

susiaccess_agent_conf_body_t * g_config = NULL; 
susiaccess_agent_profile_body_t * g_profile = NULL;
struct mosquitto *g_mosq = NULL;
void* g_reconnThreadHandler = NULL;
bool g_bRetry = false;
bool g_bConnected = false;

SACLIENT_CONNECTED_CALLBACK g_conn_cb = NULL;
SACLIENT_LOSTCONNECT_CALLBACK g_lost_conn_cb = NULL;
SACLIENT_DISCONNECT_CALLBACK g_disconn_cb = NULL;

SAManager_Interface* g_SAManager = NULL;

bool DES_BASE64Decode(char * srcBuf,char **destBuf)
{
	bool bRet = false;
	char plaintext[512] = {0};
	int iRet = 0;
	if(srcBuf == NULL || destBuf == NULL) return bRet;
	{
		char *base64Dec = NULL;
		int decLen = 0;
		int len = strlen(srcBuf);
		iRet = Base64Decode(srcBuf, len, &base64Dec, &decLen);
		if(iRet == 0)
		{
			iRet = DESDecodeEx(DEF_DES_KEY, DEF_DES_IV,  base64Dec, decLen, plaintext);
			if(iRet == 0)
			{
				*destBuf = (char *)malloc(len + 1);
				memset(*destBuf, 0, len + 1);
				strcpy(*destBuf, plaintext);
				bRet = TRUE;
			}
		}
		if(base64Dec) free(base64Dec);
	}

	return bRet;
}

void saclient_on_connect_cb()
{
	g_bConnected = true;
	if(g_conn_cb != NULL)
		g_conn_cb();
}

void saclient_on_lost_connect_cb()
{
	g_bConnected = false;
	if(g_SAManager)
	{
		if(g_SAManager->SAManager_UpdateConnectState_API)
			g_SAManager->SAManager_UpdateConnectState_API(g_config, AGENT_STATUS_OFFLINE);
	}
	if(g_lost_conn_cb != NULL)
		g_lost_conn_cb();
	else
	{
		saclient_reconnect();
	}
}

void saclient_on_disconnect_cb()
{
	g_bConnected = false;
	if(g_SAManager)
	{
		if(g_SAManager->SAManager_UpdateConnectState_API)
			g_SAManager->SAManager_UpdateConnectState_API(g_config, AGENT_STATUS_OFFLINE);
	}
	if(g_disconn_cb != NULL)
		g_disconn_cb();
}

char* saclient_GetAgentInfo_string(int status)
{
	PJSON AgentInfoJSON = NULL;
	PJSON BaseInfoJSON = NULL;
	char* pAgentInfoPayload;
	susiaccess_packet_body_t packet;

	saclient_getsocketaddress(g_profile->localip, sizeof(g_profile->localip));
	
	BaseInfoJSON = SAClientParser_AgentBaseInfoToJSON(g_profile, status);

	strcpy(packet.devId, g_profile->devId);
	strcpy(packet.handlerName, "general");
	//packet.catalogID = cagent_catalog_susi_func;
	packet.requestID = cagent_agent_info;
	packet.cmd = AGNET_INFO_CMD;
	packet.content = SAClientParser_PrintUnformatted(BaseInfoJSON);
	SAClientParser_Free(BaseInfoJSON);
	
	AgentInfoJSON = SAClientParser_CreateAgentPacketToJSON(&packet);
	pAgentInfoPayload = SAClientParser_PrintUnformatted(AgentInfoJSON);
	SAClientParser_Free(AgentInfoJSON);
	free(packet.content);
	return pAgentInfoPayload;
}

bool saclient_SendAgentInfo(int status)
{
	int result = MOSQ_ERR_SUCCESS;
	char topicStr[128] = {0};
	char* pAgentInfoPayload;
	bool bRet = false;
	if(!g_mosq)
		return bRet;

	if(!g_profile)
		return bRet;

	pAgentInfoPayload = saclient_GetAgentInfo_string(status);
	//SAClientLog(g_coreloghandle, Normal, "AgentInfo Send: %s!", pAgentInfoPayload);
	
	sprintf(topicStr, DEF_INFOACK_TOPIC, g_profile->devId);
	result = MQTT_Publish(g_mosq, topicStr, pAgentInfoPayload, strlen(pAgentInfoPayload), 0, true);
	if(result == MOSQ_ERR_SUCCESS)
	{
		SAClientLog(g_coreloghandle, Debug, "Send Topic: %s, Data: %s", topicStr, pAgentInfoPayload);
		bRet = true;
	}
	else 
		SAClientLog(g_coreloghandle, Error, "Send AgentInfo Failed, error code: %d, Packet: %s", result, pAgentInfoPayload);

	free(pAgentInfoPayload);
	return bRet;
}

char* saclient_GetOSInfo_string()
{
	PJSON AgentInfoJSON = NULL;
	PJSON OSInfoJSON = NULL;
	char* pAgentInfoPayload;
	susiaccess_packet_body_t packet;

	OSInfoJSON = SAClientParser_CreateOSInfo(g_profile);

	strcpy(packet.devId, g_profile->devId);
	strcpy(packet.handlerName, "general");
	//packet.catalogID = cagent_catalog_susi_func;
	packet.requestID = cagent_global;
	packet.cmd = glb_get_init_info_rep;
	packet.content = SAClientParser_PrintUnformatted(OSInfoJSON);
	SAClientParser_Free(OSInfoJSON);

	AgentInfoJSON = SAClientParser_CreateAgentPacketToJSON(&packet);
	pAgentInfoPayload = SAClientParser_PrintUnformatted(AgentInfoJSON);
	SAClientParser_Free(AgentInfoJSON);
	free(packet.content);
	return pAgentInfoPayload;
}

bool saclient_SendOSInfo()
{
	int result = MOSQ_ERR_SUCCESS;
	char topicStr[128] = {0};
	char* pOSInfoPayload;
	bool bRet = false;
	if(!g_mosq)
		return bRet;

	if(!g_profile)
		return bRet;

	pOSInfoPayload = saclient_GetOSInfo_string();
	//SAClientLog(g_coreloghandle, Normal, "OSInfo Send: %s!", pOSInfoPayload);

	sprintf(topicStr, DEF_ACTIONREQ_TOPIC,  g_profile->devId);
	result = MQTT_Publish(g_mosq, topicStr, pOSInfoPayload, strlen(pOSInfoPayload), 0, false);
	if(result == MOSQ_ERR_SUCCESS)
	{
		SAClientLog(g_coreloghandle, Debug, "Send Topic: %s, Data: %s", topicStr, pOSInfoPayload);
		 bRet = true;
	}
	else 
		SAClientLog(g_coreloghandle, Error, "Send OSInfo Failed, error code: %d, Packet: %s", result, pOSInfoPayload);

	free(pOSInfoPayload);
	return bRet;
}

void  saclient_HandlRecv(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	struct topic_entry * topic = Topic_FindTopic(msg->topic);
	SAClientLog(g_coreloghandle, Debug, "Received Topic: %s, Data: %s", msg->topic, msg->payload);
	if(topic != NULL)
	{
		susiaccess_packet_body_t* pkt = malloc(sizeof(susiaccess_packet_body_t));
		//printf("Topic fond\n");

		SAClientParser_ParseRecvMessage(msg->payload, msg->payloadlen, pkt);

		topic->callback_func(msg->topic, pkt, NULL, NULL);

		free(pkt->content);
		free(pkt);
		pkt = NULL;
	}
}

int pw_callback(char *buf, int size, int rwflag, void *userdata)
{
	char* data = NULL;
	int length = 0;
	
	if(!buf)
		return 0;

	if(!g_config)
		return 0;

	data = g_config->cerpasswd;
	length = strlen(data);

	memset(buf, 0, size);
	if(length+1 >= size)
	{
		strncpy(buf, g_config->cerpasswd, size);
		return size;
	}
	else
	{
		strncpy(buf, g_config->cerpasswd, length+1);
		return length;
	}
}

int SACLIENT_API saclient_initialize(susiaccess_agent_conf_body_t * config, susiaccess_agent_profile_body_t * profile, void * loghandle)
{
	int iRet = saclient_false;
	int major = 0;
	int minor = 0;
	int revision = 0;

	struct mosquitto *mosq = NULL;

	if(config == NULL)
	{
		iRet = saclient_config_error;
		return iRet;
	}

	if(profile == NULL)
	{
		iRet = saclient_profile_error;
		return iRet;
	}

	if(config->tlstype == tls_type_tls)
	{
		FILE *fptr = NULL;
		if((strlen(config->cafile)==0 && strlen(config->capath)==0) || (strlen(config->certfile)>0 && strlen(config->keyfile)==0) || (strlen(config->certfile)==0 && strlen(config->keyfile)>0)) return saclient_config_error;
		if(strlen(config->certfile) > 0)
		{
			fptr = fopen(config->certfile, "r");
			if(fptr){
				fclose(fptr);
			}else{
				return saclient_config_error;
			}
		}

		if(strlen(config->cafile) > 0)
		{
			fptr = fopen(config->cafile, "r");
			if(fptr){
				fclose(fptr);
			}else{
				return saclient_config_error;
			}
		}

		if(strlen(config->keyfile) > 0)
		{
			fptr = fopen(config->keyfile, "r");
			if(fptr){
				fclose(fptr);
			}else{
				return saclient_config_error;
			}
		}
	}

	if(loghandle != NULL)
	{
		g_coreloghandle = loghandle;
		SAClientLog(g_coreloghandle, Debug, "Start logging: %s", __FUNCTION__);
	}
	else
	{
		g_coreloghandle = NULL;
	}

	if(mosquitto_lib_version(&major, &minor, &revision) != 0)
	{
		SAClientLog(g_coreloghandle, Normal, "Mosquitto version: %d.%d.%d", major, minor, revision);
	}

	mosq = MQTT_Initialize(profile->devId);

	if(mosq)
	{
		if(config->tlstype == tls_type_tls)
		{
			int result = MQTT_SetTLS(mosq, strlen(config->cafile)>0?config->cafile:NULL, strlen(config->capath)>0?config->capath:NULL, strlen(config->certfile)>0?config->certfile:NULL, strlen(config->keyfile)>0?config->keyfile:NULL, pw_callback);
			if(result == MOSQ_ERR_SUCCESS)
				iRet = saclient_success;
			else
			{
				SAClientLog(g_coreloghandle, Error, "Mosquitto SetTLS Failed, error code: %d", result);
				iRet = saclient_config_error;
				return iRet;
			}
		}
		else if(config->tlstype == tls_type_psk)
		{
			int result = MQTT_SetTLSPSK(mosq, config->psk, config->identity, strlen(config->ciphers)>0?config->ciphers:NULL);
			if(result == MOSQ_ERR_SUCCESS)
				iRet = saclient_success;
			else
			{
				SAClientLog(g_coreloghandle, Error, "Mosquitto SetTLSPSK Failed, error code: %d", result);
				iRet = saclient_config_error;
				return iRet;
			}
		}
		
	}

	iRet = mosq!=NULL ? saclient_success : saclient_false;

	if(iRet == saclient_success)
	{
		g_mosq = mosq;
		MQTT_Callback_Set(g_mosq, saclient_on_connect_cb, saclient_on_lost_connect_cb, saclient_on_disconnect_cb, saclient_HandlRecv);

		g_config = malloc(sizeof(susiaccess_agent_conf_body_t));
		memset(g_config, 0, sizeof(susiaccess_agent_conf_body_t));
		memcpy(g_config, config, sizeof(susiaccess_agent_conf_body_t));
		g_profile = malloc(sizeof(susiaccess_agent_profile_body_t));
		memset(g_profile, 0, sizeof(susiaccess_agent_profile_body_t));
		memcpy(g_profile, profile, sizeof(susiaccess_agent_profile_body_t));
	}
	/*Load SAManager*/
	if(dl_IsExistSAManagerLib(profile->workdir))
	{
		g_SAManager = malloc(sizeof(SAManager_Interface));
		if(dl_LoadSAManagerLib(profile->workdir, g_SAManager))
		{
			if(g_SAManager->SAManager_SetPublishCB_API)
				g_SAManager->SAManager_SetPublishCB_API(saclient_publish);
			if(g_SAManager->SAManager_SetSubscribeCB_API)
				g_SAManager->SAManager_SetSubscribeCB_API(saclient_subscribe);

			if(g_SAManager->SAManager_SetConnectServerCB_API)
				g_SAManager->SAManager_SetConnectServerCB_API(saclient_connectserver);
			if(g_SAManager->SAManager_SetDisconnectCB_API)
				g_SAManager->SAManager_SetDisconnectCB_API(saclient_disconnect);

			if(g_SAManager->SAManager_Initialize_API)
				g_SAManager->SAManager_Initialize_API(g_config, g_profile, loghandle);
		}
		else
		{
			SAClientLog(g_coreloghandle, Warning, "Load SAManager failed!!");
		}
	}
	else
	{
		SAClientLog(g_coreloghandle, Warning, "Cannot find SAManager!!");
	}
	return iRet;
}

void SACLIENT_API  saclient_uninitialize()
{
	if(g_reconnThreadHandler)
	{
		g_bRetry = false;
		app_os_thread_join(g_reconnThreadHandler);
		g_reconnThreadHandler = NULL;
	}

	saclient_disconnect();

	/*Release Handler Loader*/	
	if(g_SAManager)
	{
		if(g_SAManager->SAManager_Uninitialize_API)
			g_SAManager->SAManager_Uninitialize_API();
		if(g_SAManager->SAManager_SetPublishCB_API)
			g_SAManager->SAManager_SetPublishCB_API(NULL);
		if(g_SAManager->SAManager_SetSubscribeCB_API)
			g_SAManager->SAManager_SetSubscribeCB_API(NULL);

		if(g_SAManager->SAManager_SetConnectServerCB_API)
			g_SAManager->SAManager_SetConnectServerCB_API(NULL);
		if(g_SAManager->SAManager_SetDisconnectCB_API)
			g_SAManager->SAManager_SetDisconnectCB_API(NULL);

		dl_CleanupSAManagerLib(g_SAManager);
		free(g_SAManager);
		g_SAManager = NULL;
	}
	
	g_conn_cb = NULL;
	g_lost_conn_cb = NULL;
	g_disconn_cb = NULL;

	saclient_disconnect();

	if(g_config != NULL)
	{
		free(g_config);
		g_config = NULL;
	}
	if(g_profile != NULL)
	{
		free(g_profile);
		g_profile = NULL;
	}
	
	if(g_mosq != NULL)
	{
		MQTT_Callback_Set(g_mosq, NULL, NULL, NULL, NULL);
		MQTT_Uninitialize(g_mosq);
		g_mosq = NULL;
	}
	g_coreloghandle = NULL;
}

int _saclient_connect()
{
	int iRet = saclient_false;
	int serverport = 0;
	char* pAgentInfoPayload;
	char* loginID = NULL;
	char* loginPwd = NULL;
	char* desSrc = NULL;
	char topicStr[128] = {0};
	int result = MOSQ_ERR_SUCCESS;

	if(!g_mosq)
		return iRet;

	if(!g_config)
		return iRet;

	if(DES_BASE64Decode(g_config->serverAuth, &desSrc))
	{
		loginID = strtok(desSrc, ":");
		loginPwd =  strtok(NULL, ":");
	}

	SAClientLog(g_coreloghandle, Normal, "Connect to broker: %s", g_config->serverIP);
	serverport = atoi(g_config->serverPort);
	pAgentInfoPayload = saclient_GetAgentInfo_string(AGENT_STATUS_OFFLINE);
	sprintf(topicStr, DEF_WILLMSG_TOPIC, g_profile->devId);
	result = MQTT_Connect(g_mosq, g_config->serverIP, serverport, loginID, loginPwd, 10, topicStr, pAgentInfoPayload, strlen(pAgentInfoPayload));
	/*if(!bRet)
	{
		FILE *fp = NULL;
		char filepath[MAX_PATH] = {0};
		char serverNetInfo[IP_LIST_BUF_LEN] = {0};
		path_combine(filepath, g_profile->workdir, DEF_SERVER_IP_LIST_FILE);
		if(fp=fopen(filepath,"rt"))
		{
			char tmpInfoStr[IP_LIST_BUF_LEN] = {0};
			char* tmp_loginID = NULL;
			char* tmp_loginPwd = NULL;
			char* tmp_desSrc = NULL;
			const int IP_ADDRE_NUM = 0;
			const int IP_PORT_NUM = 1;
			const int IP_AUTH_NUM = 2;
			while(!bRet && fgets(serverNetInfo, IP_LIST_BUF_LEN, fp) != NULL)
			{
				int redServerPort = 0;
				char * sliceStr[16] = {NULL};
				char * buf = NULL;
				int i = 0;
				strcpy(tmpInfoStr, serverNetInfo);
				buf = tmpInfoStr;
				while(sliceStr[i] = strtok(buf, ":"))
				{
					i++;
					buf = NULL;
				}
				if(i>IP_PORT_NUM)
				{
					//int ipAddrInt = ntohl( inet_addr( ipAddr ));
					//strcpy( ipAddr, (char*)inet_ntoa(htonl((unsigned long)ipAddrInt)));
					int tmpLen = strlen(sliceStr[IP_AUTH_NUM]); //Delete '/n'
					sliceStr[IP_AUTH_NUM][tmpLen-1] = 0;
					if(DES_BASE64Decode(sliceStr[IP_AUTH_NUM], &tmp_desSrc))
					{
						tmp_loginID = strtok(tmp_desSrc, ":");
						tmp_loginPwd =  strtok(NULL, ":");
					}
					redServerPort = atoi(sliceStr[IP_PORT_NUM]);
					SAClientLog(g_coreloghandle, Normal, "Connect to redundant broker: %s", sliceStr[0]);
					bRet = MQTT_Connect(g_mosq, sliceStr[IP_ADDRE_NUM], redServerPort, tmp_loginID, tmp_loginPwd, 10, topicStr, pAgentInfoPayload, strlen(pAgentInfoPayload));
					if(!bRet)
						SAClientLog(g_coreloghandle, Normal, "Can't connect to redundant broker: %s", sliceStr[IP_ADDRE_NUM]);
					if(tmp_desSrc)
					{
						free(tmp_desSrc);
						tmp_desSrc = NULL;
						tmp_loginID = NULL;
						tmp_loginPwd = NULL;
					}
				}
				memset(serverNetInfo,0, sizeof(char)*IP_LIST_BUF_LEN);
				memset(tmpInfoStr, 0, sizeof(char)*IP_LIST_BUF_LEN);
			}
			fclose(fp);
		}
	}
	*/
	if(desSrc)
	{
		free(desSrc);
		desSrc = NULL;
		loginID = NULL;
		loginPwd = NULL;
	}
	free(pAgentInfoPayload);
	if(result != MOSQ_ERR_SUCCESS)
	{
		if(g_SAManager)
		{
			if(g_SAManager->SAManager_UpdateConnectState_API)
				g_SAManager->SAManager_UpdateConnectState_API(g_config, AGENT_STATUS_CONNECTION_FAILED);
		}
		SAClientLog(g_coreloghandle, Error, "Unable to connect to broker: %s, error code: %d.", g_config->serverIP, result);
		return iRet;
	}
	else
	{
		if(g_SAManager)
		{
			if(g_SAManager->SAManager_AddInternalCallback_API)
				g_SAManager->SAManager_AddInternalCallback_API(g_profile);
		}
		
		if(saclient_SendAgentInfo(AGENT_STATUS_ONLINE) == true)
		{
			saclient_SendOSInfo();
			//SAClientLog(g_coreloghandle, Normal, "Send Client Info: AGENT_STATUS_ONLINE");
			g_bConnected = true;
			if(g_SAManager)
			{
				if(g_SAManager->SAManager_UpdateConnectState_API)
					g_SAManager->SAManager_UpdateConnectState_API(g_config, AGENT_STATUS_ONLINE);
			}
			//saclient_on_connect_cb();
			iRet = saclient_success;
		} else {
			//saclient_disconnect();
			MQTT_Disconnect(g_mosq);
			SAClientLog(g_coreloghandle, Error, "Unable to send Client Info: AGENT_STATUS_ONLINE");
			iRet = saclient_false;
		}	
	}
	return iRet;
}

static CAGENT_PTHREAD_ENTRY(ReconnThreadStart, args)
{
	int count;
CONN_RETRY:
	count = 50;
	while (count>0)
	{
		count--;
		app_os_sleep(100);
		if(!g_bRetry)
		{
			app_os_thread_exit(0);
			return 0;
		}
	}
	
	if(_saclient_connect() != saclient_success)
	{
		if(g_bRetry)
			goto CONN_RETRY;
	}
	else
	{
		if(g_SAManager)
		{
			if(g_SAManager->SAManager_UpdateConnectState_API)
				g_SAManager->SAManager_UpdateConnectState_API(g_config, AGENT_STATUS_OFFLINE);
		}
	}
	app_os_thread_exit(0);
	return 0;
}

int SACLIENT_API saclient_connect()
{
	int iRet = saclient_false;
	
	if(g_reconnThreadHandler)
	{
		g_bRetry = false;
		app_os_thread_join(g_reconnThreadHandler);
		g_reconnThreadHandler = NULL;
	}

	iRet = _saclient_connect();

	if(iRet == saclient_false)
	{
		if(_stricmp(g_config->autoStart, "True")==0)
		{
			saclient_reconnect();
		}
	}
	return iRet;
}

int SACLIENT_API  saclient_connectserver(char const * ip, int port, char const * mqttauth)
{

	if(g_reconnThreadHandler)
	{
		g_bRetry = false;
		app_os_thread_join(g_reconnThreadHandler);
		g_reconnThreadHandler = NULL;
	}

	saclient_disconnect();

	if(ip)
	{
		strncpy(g_config->serverIP, ip, strlen(ip)+1);
	}

	if(port>0)
	{
		sprintf(g_config->serverPort, "%d", port);
	}

	if(mqttauth)
	{
		strncpy(g_config->serverAuth, mqttauth, strlen(mqttauth)+1);
	}
	app_os_sleep(1000);
	return  saclient_connect();
}

void SACLIENT_API saclient_disconnect()
{
	struct topic_entry *iter_topic = NULL;
	struct topic_entry *tmp_topic = NULL;

	if(g_reconnThreadHandler)
	{
		g_bRetry = false;
		app_os_thread_join(g_reconnThreadHandler);
		g_reconnThreadHandler = NULL;
	}

	if(g_bConnected)
	{
		saclient_SendAgentInfo(AGENT_STATUS_OFFLINE);
	}
	
	iter_topic = Topic_FirstTopic();
	while(iter_topic != NULL)
	{
		tmp_topic = iter_topic->next;
		if(g_mosq)
		{
			MQTT_Unsubscribe(g_mosq, iter_topic->name);
		}
		Topic_RemoveTopic(iter_topic->name);
		iter_topic = tmp_topic;
	}
	g_bConnected = false;
	if(g_mosq)
	{
		MQTT_Disconnect(g_mosq);
	}
}

void SACLIENT_API saclient_reconnect()
{
	if(g_reconnThreadHandler)
	{
		g_bRetry = false;
		app_os_thread_join(g_reconnThreadHandler);
		g_reconnThreadHandler = NULL;
	}

	saclient_disconnect();
	g_bRetry = true;
	app_os_thread_create(&g_reconnThreadHandler, ReconnThreadStart, NULL);
}


void SACLIENT_API  saclient_connection_callback_set(SACLIENT_CONNECTED_CALLBACK on_connect, SACLIENT_LOSTCONNECT_CALLBACK on_lost_connect, SACLIENT_DISCONNECT_CALLBACK on_disconnect)
{
	g_conn_cb = on_connect;
	g_lost_conn_cb = on_lost_connect;
	g_disconn_cb = on_disconnect;
}

int SACLIENT_API  saclient_publish(char const * topic, susiaccess_packet_body_t const * pkt)
{
	int iRet = saclient_false;
	int result = MOSQ_ERR_SUCCESS;
	PJSON AgentInfoJSON = NULL;
	char* pPayload = NULL;
	
	if(!g_mosq)
		return saclient_no_init;

	if(!pkt)
		return saclient_send_data_error;

	if(!g_bConnected)
		return saclient_no_connnect;

	AgentInfoJSON = SAClientParser_CreateAgentPacketToJSON(pkt);
	pPayload = SAClientParser_PrintUnformatted(AgentInfoJSON);
	SAClientParser_Free(AgentInfoJSON);
	if(!pPayload)
	{
		SAClientLog(g_coreloghandle, Error, "Incorrect Packet Format: <name: %s, command: %d, content: %s>", pkt->handlerName, pkt->cmd, pkt->content);
		return saclient_send_data_error;
	}
	result = MQTT_Publish(g_mosq, (char *)topic, pPayload, strlen(pPayload), 0, false);
	if(result == MOSQ_ERR_SUCCESS)
	{
		iRet = saclient_success;
		SAClientLog(g_coreloghandle, Debug, "Send Topic: %s, Data: %s", topic, pPayload);
	}
	else {
		SAClientLog(g_coreloghandle, Error, "Send Failed, error code: %d, Packet: <name: %s, command: %d, content: %s>", result, pkt->handlerName, pkt->cmd, pkt->content);
	}
	free(pPayload);
	return iRet;
}

int SACLIENT_API  saclient_subscribe(char const * topic, SACLIENT_MESSAGE_RECV_CALLBACK msg_recv_callback)
{
	int iRet = saclient_false;
	
	if(!g_mosq)
		return saclient_no_init;

	if(!msg_recv_callback)
		return saclient_callback_null;
	
	if(Topic_FindTopic(topic) == NULL)
	{
		int result = MQTT_Subscribe(g_mosq, (char *)topic, 1);
		if(result == MOSQ_ERR_SUCCESS)
		{
			Topic_AddTopic(topic, (topic_msg_cb_func_t)msg_recv_callback);
			iRet = saclient_success;
		}
		else
		{
			SAClientLog(g_coreloghandle, Error, "Mosquitto Subscribe Failed, error code: %d", result);
		}
	}	
	return iRet;
}

int SACLIENT_API  saclient_getsocketaddress(char* clientip, int size)
{
	int iRet = 0;
	int newfd = MQTT_GetSocket(g_mosq);
	if(newfd<0)
		return saclient_false;
	iRet = app_get_socket_ip(newfd, clientip, size);
	return iRet==0?saclient_success:saclient_false;
}