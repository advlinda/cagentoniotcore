#include "common.h"
#include "SAManager.h"
#include "dynamicloader.h"
#include "SAParser.h"
#include <cJSON.h>
#include "SAManagerLog.h"
#include "topic.h"
//#ifdef WIN32
#include "Keepalive.h"
//#endif
#define DEF_CALLBACKREQ_TOPIC			"/cagent/admin/%s/agentcallbackreq"	/*Subscribe*/
#define DEF_ACTIONACK_TOPIC				"/cagent/admin/%s/agentactionack"	/*Subscribe*/
#define DEF_AGENTCONTROL_TOPIC			"/server/admin/+/agentctrl"	/*Subscribe*/
#define DEF_EVENTNOTIFY_TOPIC			"/cagent/admin/%s/eventnotify"	/*publish*/
#define DEF_ACTIONREQ_TOPIC				"/cagent/admin/%s/agentactionreq"	/*publish*/
//#define DEF_CUSTOM_ACTIONREQ_TOPIC	"/cagent/%s/%s/agentactionreq"	/*publish*/
#define DEF_AUTOREPORT_TOPIC			"/cagent/admin/%s/%s"	/*publish*/
#define AUTOREPORT_TOPIC				"deviceinfo"

#define cagent_action_general			2001
#define general_info_spec_rep			2052
#define general_info_upload_rep			2055
#define general_event_notify_rep		2059

/** Action(params) enum define*/
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

SALoader_Interface* g_SALoader = NULL;
SAGeneral_Interface* g_SAGeneral = NULL;
Handler_List_t g_handlerList;
PUBLISHCB g_publishCB = NULL;
SUBSCRIBECB g_subscribeCB = NULL;
CONNECTSERVERCB g_connectserverCB = NULL;
DISCONNECTCB g_disconnectCB = NULL;

LOGHANDLE g_samanagerlogger = NULL;

#ifdef _MSC_VER
BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
	if (reason_for_call == DLL_PROCESS_ATTACH) // Self-explanatory
	{
		printf("DllInitializer\n");
		DisableThreadLibraryCalls(module_handle); // Disable DllMain calls for DLL_THREAD_*
		if (reserved == NULL) // Dynamic load
		{
			// Initialize your stuff or whatever
			// Return FALSE if you don't want your module to be dynamically loaded
		}
		else // Static load
		{
			// Return FALSE if you don't want your module to be statically loaded
		}
	}

	if (reason_for_call == DLL_PROCESS_DETACH) // Self-explanatory
	{
		printf("DllFinalizer\n");
		if (reserved == NULL) // Either loading the DLL has failed or FreeLibrary was called
		{
			// Cleanup
			SAManager_Uninitialize();
		}
		else // Process is terminating
		{
			// Cleanup
			SAManager_Uninitialize();
		}
	}
	return TRUE;
}
#else
__attribute__((constructor))
/**
 * initializer of the shared lib.
 */
static void Initializer(int argc, char** argv, char** envp)
{
    printf("DllInitializer\n");
}

__attribute__((destructor))
/** 
 * It is called when shared lib is being unloaded.
 * 
 */
static void Finalizer()
{
    printf("DllFinalizer\n");
	SAManager_Uninitialize();
}
#endif

/*Message Queue for UpdateConnectState*/
struct statemessage {
	susiaccess_agent_conf_body_t *conf;
	int	status;
};

struct statequeue {
    CAGENT_MUTEX_TYPE   lock;
    CAGENT_COND_TYPE    wait_room;
    CAGENT_COND_TYPE    wait_data;
    unsigned int		size;
    unsigned int		head;
    unsigned int		tail;
    struct statemessage	**queue;
};

struct state_ctx{
   void				 *threadHandler;
   bool				 isThreadRunning;
   struct statequeue *msgqueue;
};

struct state_ctx g_statethreadctx;

bool statemsg_create(struct statemessage *const msg, susiaccess_agent_conf_body_t *conf, int status)
{
	if(!msg)
		return false;
	msg->conf = conf;
	msg->status = status;
}

void statemsg_free(struct statemessage *const msg)
{
	if(!msg)
		return;

	msg->conf = NULL;

	free(msg);
}

bool queue_init(struct statequeue *const q, const unsigned int slots)
{
    if (!q || slots < 1U)
        return false;

    q->queue = malloc(sizeof (struct statemessage *) * (size_t)(slots + 1));
    if (!q->queue)
        return false;

    q->size = slots + 1U; 
    q->head = 0U;
    q->tail = 0U;
	if(app_os_mutex_setup(&q->lock)<0)
	{
		free(q->queue);
		q->queue = NULL;
		return false;
	}
	if(app_os_cond_setup(&q->wait_room)<0)
	{
		free(q->queue);
		q->queue = NULL;
		return false;
	}
	if(app_os_cond_setup(&q->wait_data)<0)
	{
		free(q->queue);
		q->queue = NULL;
		return false;
	}

    return true;
}

void queue_uninit(struct statequeue *const q)
{
	struct statemessage *msg;
	app_os_mutex_lock(&q->lock);
    /*if (q->head == q->tail)
	{
		app_os_mutex_unlock(&q->lock);
		return;
	}*/
	while (q->head != q->tail)
	{
		msg = q->queue[q->tail];
		q->queue[q->tail] = NULL;
		q->tail = (q->tail + 1U) % q->size;
		statemsg_free(msg);
		msg = NULL;
	}
	free(q->queue);
	q->queue = NULL;

	app_os_cond_signal(&q->wait_data);

	q->size = 0U;
    q->head = 0U;
    q->tail = 0U;
	
    app_os_mutex_unlock(&q->lock);
    app_os_mutex_cleanup(&q->lock);
    app_os_cond_cleanup(&q->wait_room);
    app_os_cond_cleanup(&q->wait_data);
}

struct statemessage *queue_get(struct statequeue *const q)
{
	struct statemessage *msg = NULL;
	int time = 500;
	int ret = 0;
	if(!q)
		return msg;
    app_os_mutex_lock(&q->lock);
    while (q->head == q->tail)
	{
        ret = app_os_cond_timed_wait(&q->wait_data, &q->lock, &time);
		if(ret != 0)
		{
			app_os_mutex_unlock(&q->lock);
			return msg;
		}
	}

    msg = q->queue[q->tail];
    q->queue[q->tail] = NULL;
    q->tail = (q->tail + 1U) % q->size;

    app_os_cond_signal(&q->wait_room);

    app_os_mutex_unlock(&q->lock);
    return msg;
}

bool queue_put(struct statequeue *const q, struct statemessage *const msg)
{
	int time = 500;
	int ret = 0;
	if(!q)
		return false;
    app_os_mutex_lock(&q->lock);
	while ((q->head + 1U) % q->size == q->tail)
	{
        ret = app_os_cond_timed_wait(&q->wait_room, &q->lock, &time);
		if(ret != 0)
		{
			app_os_mutex_unlock(&q->lock);
			return false;
		}
	}

    q->queue[q->head] = msg;
	
	q->head = (q->head + 1U) % q->size;

    app_os_cond_signal(&q->wait_data);

    app_os_mutex_unlock(&q->lock);
    return true;
}

static CAGENT_PTHREAD_ENTRY(StateQueueThread, args)
{
	struct state_ctx *pstateContex = (struct state_ctx *)args;
	int interval = 100;
	struct statemessage *msg = NULL;
	while(pstateContex->isThreadRunning)
	{
		app_os_sleep(interval);
		msg = queue_get(pstateContex->msgqueue);
		if(!msg)
			continue;
		if(g_SALoader)
		{
			if(g_SALoader->Loader_SetAgentStatus_API)
				g_SALoader->Loader_SetAgentStatus_API(&g_handlerList, msg->conf, msg->status);
		}
		statemsg_free(msg);
		msg = NULL;
	}

	app_os_thread_exit(0);
	return 0;
}

char * ConvertMessageToUTF8(char* wText)
{
	char * utf8RetStr = NULL;
	int tmpLen = 0;
	if(!wText)
		return utf8RetStr;

#ifdef WIN32
	utf8RetStr = ANSIToUTF8(wText);
	tmpLen = !utf8RetStr ? 0 : strlen(utf8RetStr);
	if(tmpLen == 1)
	{
		if(utf8RetStr) free(utf8RetStr);
		utf8RetStr = UnicodeToUTF8(wText);
	}
#else
	utf8RetStr = strdup(wText);
#endif
	return utf8RetStr;
}

susiaccess_packet_body_t*  SAManager_WrapReqPacket(Handler_info const * plugin, int const enum_act, void const * const requestData, unsigned int const requestLen, bool bCust)
{
	susiaccess_packet_body_t* packet = NULL;
	char * data = NULL;
	int length = 0;
	if(plugin == NULL)
		return packet;
	if(plugin->agentInfo == NULL)
		return packet;

	packet = malloc(sizeof(susiaccess_packet_body_t));
	memset(packet, 0, sizeof(susiaccess_packet_body_t));

	if(requestData)
	{
		data = ConvertMessageToUTF8((char*)requestData);
		length = strlen(data);
	}
	packet->content = (char*)malloc(length+1);
	if(packet->content)
	{
		memset(packet->content, 0, length+1);
		memcpy(packet->content, data, length);
	}
	free(data);

	strcpy(packet->devId, plugin->agentInfo->devId);
	strcpy(packet->handlerName, plugin->Name);

	if(bCust)
	{
		packet->requestID = cagent_custom_action;
	}
	else
	{
		packet->requestID = plugin->ActionID;
	}

	packet->cmd = enum_act;
	return packet;
}

AGENT_SEND_STATUS SAManager_SendMessage( HANDLE const handle, int enum_act, 
									   char const * const requestData, unsigned int const requestLen, 
									   void *pRev1, void* pRev2 )
{
	AGENT_SEND_STATUS result = cagent_send_data_error;
	Handler_info* plugin = NULL;
	susiaccess_packet_body_t* packet = NULL;
	char topicStr[128] = {0};

	if(handle == NULL)
		return result;

	plugin = (Handler_info*)handle;

	if(plugin->agentInfo == NULL)
		return result;

	packet = SAManager_WrapReqPacket(plugin, enum_act, requestData, requestLen, false);

	if(packet == NULL)
	{
		SAManagerLog(g_samanagerlogger, Warning, "Request Packet is empty!");
		return result;
	}
	sprintf(topicStr, DEF_ACTIONREQ_TOPIC, plugin->agentInfo->devId);
	if(g_publishCB)
	{
		if(g_publishCB(topicStr, packet) == 0)
			result = cagent_success;
		else
			result = cagent_send_data_error;
	}
	else
		result = cagent_callback_null;

	if(packet->content)
		free(packet->content);
	free(packet);
	return result;
}

AGENT_SEND_STATUS SAManager_SendCustMessage( HANDLE const handle, int enum_act, char const * const topic, 
										   char const * const requestData, unsigned int const requestLen, 
										   void *pRev1, void* pRev2 )
{
	AGENT_SEND_STATUS result = cagent_send_data_error;
	Handler_info* plugin = NULL;
	susiaccess_packet_body_t* packet = NULL;
	char topicStr[128] = {0};

	//SAManagerLog(g_samanagerlogger, Normal, "Topic: %s, Data: %s", topic, requestData);
	if(handle == NULL)
		return result;

	plugin = (Handler_info*)handle;

	if(plugin->agentInfo == NULL)
		return result;

	packet = SAManager_WrapReqPacket(plugin, enum_act, requestData, requestLen, true);

	if(packet == NULL)
	{
		SAManagerLog(g_samanagerlogger, Warning, "Request Packet is empty!");
		return result;
	}

	if(g_publishCB)
	{
		if(g_publishCB(topic, packet) == true)
			result = cagent_success;
		else
			result = cagent_send_data_error;
	}
	else
		result = cagent_callback_null;
	if(packet->content)
		free(packet->content);
	free(packet);
	return result;
}

susiaccess_packet_body_t * SAManager_WrapAutoReportPacket(Handler_info const * plugin, void const * const requestData, unsigned int const requestLen, bool bCust)
{
	PJSON ReqInfoJSON = NULL;
	char* pReqInfoPayload = NULL;
	susiaccess_packet_body_t* packet = NULL;

	cJSON* node = NULL;
	cJSON* root = NULL;
	cJSON* pfinfoNode = NULL;
	char* buff = NULL;
	char* data = NULL;
	int length = 0;

	if(plugin == NULL)
		return packet;
	if(plugin->agentInfo == NULL)
		return packet;

	if(requestData)
	{
		data = ConvertMessageToUTF8(requestData);
	}

	root = cJSON_CreateObject();
	pfinfoNode = cJSON_CreateObject();
	//node = cJSON_Parse((const char *)requestData);
	node = cJSON_Parse((const char *)data);
	free(data);
	
	if(pfinfoNode)
	{
		cJSON* chNode = node->child;
		cJSON_AddItemToObject(pfinfoNode, chNode->string, cJSON_Duplicate(chNode, true));	
	}
	cJSON_Delete(node);
	cJSON_AddItemToObject(root, "data", pfinfoNode);
	buff = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	length = strlen(buff);
	
	packet = malloc(sizeof(susiaccess_packet_body_t));
	memset(packet, 0, sizeof(susiaccess_packet_body_t));
	packet->content = (char*)malloc(length+1);
	memset(packet->content, 0, length+1);
	memcpy(packet->content, buff, length);

	free(buff);

	strcpy(packet->devId, plugin->agentInfo->devId);
	strcpy(packet->handlerName, "general");  //Special case for auto report.

	packet->requestID = cagent_action_general;

	packet->cmd = general_info_upload_rep;
	//SAManagerLog(g_samanagerlogger, Normal, "Parser_CreateRequestInfo");

	return packet;
}

AGENT_SEND_STATUS SAManager_SendAutoReport( HANDLE const handle, 
										  char const * const requestData, unsigned int const requestLen, 
										  void *pRev1, void* pRev2 )
{
	AGENT_SEND_STATUS result = cagent_send_data_error;
	Handler_info* plugin = NULL;
	susiaccess_packet_body_t* packet = NULL;
	char topicStr[128] = {0};

	if(handle == NULL)
		return result;

	plugin = (Handler_info*)handle;

	if(plugin->agentInfo == NULL)
		return result;

	packet = SAManager_WrapAutoReportPacket(plugin, requestData, requestLen, false);

	if(packet == NULL)
	{
		SAManagerLog(g_samanagerlogger, Warning, "Request Packet is empty!");
		return result;
	}

	sprintf(topicStr, DEF_AUTOREPORT_TOPIC, plugin->agentInfo->devId, AUTOREPORT_TOPIC);

	if(g_publishCB)
	{
		if(g_publishCB(topicStr, packet) == true)
			result = cagent_success;
		else
			result = cagent_send_data_error;
	}
	else
		result = cagent_callback_null;
	if(packet->content)
		free(packet->content);
	free(packet);
	return result;
}

susiaccess_packet_body_t * SAManager_WrapCapabilityPacket(Handler_info const * plugin, void const * const requestData, unsigned int const requestLen, bool bCust)
{
	PJSON ReqInfoJSON = NULL;
	char* pReqInfoPayload = NULL;
	susiaccess_packet_body_t* packet = NULL;

	cJSON* node = NULL;
	cJSON* root = NULL;
	cJSON* pfinfoNode = NULL;
	char* buff = NULL;
	char* data = NULL;
	int length = 0;

	if(plugin == NULL)
		return packet;
	if(plugin->agentInfo == NULL)
		return packet;

	if(requestData)
	{
		data = ConvertMessageToUTF8(requestData);
	}

	root = cJSON_CreateObject();
	pfinfoNode = cJSON_CreateObject();
	//node = cJSON_Parse((const char *)requestData);
	node = cJSON_Parse((const char *)data);
	free(data);
	if(pfinfoNode)
	{
		cJSON* chNode = node->child;
		cJSON_AddItemToObject(pfinfoNode, chNode->string, cJSON_Duplicate(chNode, true));	
	}
	cJSON_Delete(node);
	cJSON_AddItemToObject(root, "infoSpec", pfinfoNode);
	buff = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	length = strlen(buff);

	packet = malloc(sizeof(susiaccess_packet_body_t));
	memset(packet, 0, sizeof(susiaccess_packet_body_t));
	packet->content = (char*)malloc(length+1);
	memset(packet->content, 0, length+1);
	memcpy(packet->content, buff, length);

	free(buff);

	strcpy(packet->devId, plugin->agentInfo->devId);
	strcpy(packet->handlerName, "general");  //Special case for auto report.

	packet->requestID = cagent_action_general;

	packet->cmd = general_info_spec_rep;
	//SAManagerLog(g_samanagerlogger, Normal, "Parser_CreateRequestInfo");

	return packet;
}

AGENT_SEND_STATUS SAManager_SendCapability( HANDLE const handle, 
										 char const * const requestData, unsigned int const requestLen, 
										 void *pRev1, void* pRev2 )
{
	AGENT_SEND_STATUS result = cagent_send_data_error;
	Handler_info* plugin = NULL;
	susiaccess_packet_body_t* packet = NULL;
	char topicStr[128] = {0};

	if(handle == NULL)
		return result;

	plugin = (Handler_info*)handle;

	if(plugin->agentInfo == NULL)
		return result;
	
	packet = SAManager_WrapCapabilityPacket(handle, requestData, requestLen, false);

	if(packet == NULL)
	{
		SAManagerLog(g_samanagerlogger, Warning, "Request Packet is empty!");
		return result;
	}
	sprintf(topicStr, DEF_ACTIONREQ_TOPIC, plugin->agentInfo->devId);
	if(g_publishCB)
	{
		if(g_publishCB(topicStr, packet) == true)
			result = cagent_success;
		else
			result = cagent_send_data_error;
	}
	else
		result = cagent_callback_null;

	if(packet->content)
		free(packet->content);
	free(packet);
	return result;
}

susiaccess_packet_body_t * SAManager_WrapEventNotifyPacket(Handler_info const * plugin, HANDLER_NOTIFY_SEVERITY severity, void const * const requestData, unsigned int const requestLen, bool bCust)
{
	PJSON ReqInfoJSON = NULL;
	char* pReqInfoPayload = NULL;
	char* data = NULL; 
	susiaccess_packet_body_t* packet = NULL;

	cJSON* node = NULL;
	cJSON* root = NULL;
	cJSON* pfinfoNode = NULL;
	char* buff = NULL;
	int length = 0;

	if(plugin == NULL)
		return packet;
	if(plugin->agentInfo == NULL)
		return packet;

	if(requestData)
	{
		data = ConvertMessageToUTF8(requestData);
	}

	root = cJSON_CreateObject();
	pfinfoNode = cJSON_CreateObject();
	//node = cJSON_Parse((const char *)requestData);
	node = cJSON_Parse((const char *)data);
	free(data);
	if(pfinfoNode)
	{
		cJSON* chNode = node->child;
		while(chNode)
		{
			cJSON_AddItemToObject(pfinfoNode, chNode->string, cJSON_Duplicate(chNode, true));	
			chNode = chNode->next;
		}
		cJSON_AddNumberToObject(pfinfoNode, "severity", severity);
		cJSON_AddStringToObject(pfinfoNode, "handler", plugin->Name);
	}
	cJSON_Delete(node);
	cJSON_AddItemToObject(root, "eventnotify", pfinfoNode);
	buff = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	length = strlen(buff);

	packet = malloc(sizeof(susiaccess_packet_body_t));
	memset(packet, 0, sizeof(susiaccess_packet_body_t));
	packet->content = (char*)malloc(length+1);
	memset(packet->content, 0, length+1);
	memcpy(packet->content, buff, length);

	free(buff);

	strcpy(packet->devId, plugin->agentInfo->devId);
	strcpy(packet->handlerName, "general");  //Special case for event notify.

	packet->requestID = cagent_action_general;

	packet->cmd = general_event_notify_rep;
	//SAManagerLog(g_samanagerlogger, Normal, "Parser_CreateRequestInfo");

	return packet;
}

AGENT_SEND_STATUS SAManager_SendEventNotify( HANDLE const handle, HANDLER_NOTIFY_SEVERITY severity,
										 char const * const requestData, unsigned int const requestLen, 
										 void *pRev1, void* pRev2 )
{
	AGENT_SEND_STATUS result = cagent_send_data_error;
	Handler_info* plugin = NULL;
	susiaccess_packet_body_t* packet = NULL;
	char topicStr[128] = {0};

	if(handle == NULL)
		return result;

	plugin = (Handler_info*)handle;

	if(plugin->agentInfo == NULL)
		return result;

	packet = SAManager_WrapEventNotifyPacket(handle, severity, requestData, requestLen, false);

	if(packet == NULL)
	{
		SAManagerLog(g_samanagerlogger, Warning, "Request Packet is empty!");
		return result;
	}
	sprintf(topicStr, DEF_EVENTNOTIFY_TOPIC, plugin->agentInfo->devId);
	if(g_publishCB)
	{
		if(g_publishCB(topicStr, packet) == true)
			result = cagent_success;
		else
			result = cagent_send_data_error;
	}
	else
		result = cagent_callback_null;

	if(packet->content)
		free(packet->content);
	free(packet);
	return result;
}

AGENT_SEND_STATUS SAManager_ConnectServer(char const * ip, int port, char const * mqttauth)
{
	AGENT_SEND_STATUS result = cagent_send_data_error;
	if(g_connectserverCB)
		if(g_connectserverCB(ip, port, mqttauth) == true)
			result = cagent_success;
		else
			result = cagent_connect_error;
	else
		result = cagent_callback_null;
	return result;
}

AGENT_SEND_STATUS SAManager_Disconnect()
{
	AGENT_SEND_STATUS result = cagent_callback_null;
	if(g_disconnectCB)
		g_disconnectCB();
	else
		result = cagent_callback_null;
	return result;
}

void SAManager_RecvInternalCommandReq(char* topic, susiaccess_packet_body_t *pkt, void *pRev1, void* pRev2)
{
	Handler_Loader_Interface* handler = NULL;
	PJSON ReqInfoJSON = NULL;
	char* pReqInfoPayload = NULL;
	ReqInfoJSON = SAParser_CreateAgentPacketToJSON(pkt);
	pReqInfoPayload = SAParser_PrintUnformatted(ReqInfoJSON);
	SAParser_Free(ReqInfoJSON);

	if(g_SALoader)
	{
		/*Support V3.1 Version */
		handler = g_SALoader->Loader_FindHandler_API(&g_handlerList, pkt->handlerName);
	}
	if(handler != NULL)
	{
		//SAManagerLog(g_samanagerlogger, Normal, "Handler find by name: %s", pkt->handlerName );
		if( handler->Handler_Recv_API )
			handler->Handler_Recv_API(topic, pReqInfoPayload, strlen(pReqInfoPayload), pRev1, pRev2);
	}
	/*Support V3.0 or Older Version */
	else{
		int reqID = pkt->requestID;
		if(g_SALoader)
		{
			/*Support V3.0 Version */
			handler = g_SALoader->Loader_FindHandlerByReqID_API(&g_handlerList, reqID);
		}
		if(handler != NULL)
		{
			//SAManagerLog(g_samanagerlogger, Normal, "Handler find by ID: %d", reqID );
			if( handler->Handler_Recv_API )
				handler->Handler_Recv_API(topic, pReqInfoPayload, strlen(pReqInfoPayload), pRev1, pRev2);
		}
		/*Support Older Version */
		else if(reqID == cagent_request_device_monitoring)
		{
			if(g_SALoader)
			{
				handler = g_SALoader->Loader_FindHandler_API(&g_handlerList, "device_monitoring");
			}
			if(handler != NULL)
			{
				if( handler->Handler_Recv_API )
					handler->Handler_Recv_API(topic, pReqInfoPayload, strlen(pReqInfoPayload), pRev1, pRev2);
			}
			else
			{
				SAManagerLog(g_samanagerlogger, Warning, "Cannot find handler: %s", "device_monitoring" );
			}
		}
		else if(reqID == cagent_request_power_onoff)
		{
			if(g_SALoader)
			{
				handler = g_SALoader->Loader_FindHandler_API(&g_handlerList, "power_onoff");
			}
			if(handler != NULL)
			{
				if( handler->Handler_Recv_API )
					handler->Handler_Recv_API(topic, pReqInfoPayload, strlen(pReqInfoPayload), pRev1, pRev2);
			}
			else
			{
				SAManagerLog(g_samanagerlogger, Warning, "Cannot find handler: %s", "power_onoff" );
			}
		}
		else if(reqID == cagent_request_remote_kvm)
		{
			if(g_SALoader)
			{
				handler = g_SALoader->Loader_FindHandler_API(&g_handlerList, "remote_kvm");
			}
			if(handler != NULL)
			{
				if( handler->Handler_Recv_API )
					handler->Handler_Recv_API(topic, pReqInfoPayload, strlen(pReqInfoPayload), pRev1, pRev2);
			}
			else
			{
				SAManagerLog(g_samanagerlogger, Warning, "Cannot find handler: %s", "remote_kvm" );
			}
		}
		else if(reqID == cagent_request_protection)
		{
			if(g_SALoader)
			{
				handler = g_SALoader->Loader_FindHandler_API(&g_handlerList, "protection");
			}
			if(handler != NULL)
			{
				if( handler->Handler_Recv_API )
					handler->Handler_Recv_API(topic, pReqInfoPayload, strlen(pReqInfoPayload), pRev1, pRev2);
			}
			else
			{
				SAManagerLog(g_samanagerlogger, Warning, "Cannot find handler: %s", "protection" );
			}
		}
		else if(reqID == cagent_request_recovery)
		{
			if(g_SALoader)
			{
				handler = g_SALoader->Loader_FindHandler_API(&g_handlerList, "recovery");
			}
			if(handler != NULL)
			{
				if( handler->Handler_Recv_API )
					handler->Handler_Recv_API(topic, pReqInfoPayload, strlen(pReqInfoPayload), pRev1, pRev2);
			}
			else
			{
				SAManagerLog(g_samanagerlogger, Warning, "Cannot find handler: %s", "recovery" );
			}
		}
		else if(reqID == cagent_request_software_monitoring)
		{
			if(g_SALoader)
			{
				handler = g_SALoader->Loader_FindHandler_API(&g_handlerList, "software_monitoring");
			}
			if(handler != NULL)
			{
				if( handler->Handler_Recv_API )
					handler->Handler_Recv_API(topic, pReqInfoPayload, strlen(pReqInfoPayload), pRev1, pRev2);
			}
			else
			{
				SAManagerLog(g_samanagerlogger, Warning, "Cannot find handler: %s", "software_monitoring" );
			}
		}
		else if(reqID == cagent_request_global)
		{
			if(g_SALoader)
			{
				handler = g_SALoader->Loader_FindHandler_API(&g_handlerList, "general");
			}
			if(handler != NULL)
			{
				if( handler->Handler_Recv_API )
					handler->Handler_Recv_API(topic, pReqInfoPayload, strlen(pReqInfoPayload), pRev1, pRev2);
			}
			else
			{
				SAManagerLog(g_samanagerlogger, Warning, "Cannot find handler: %s", "global" );
			}
		}
		else if(reqID == cagent_request_terminal)
		{
			if(g_SALoader)
			{
				handler = g_SALoader->Loader_FindHandler_API(&g_handlerList, "terminal");
			}
			if(handler != NULL)
			{
				if( handler->Handler_Recv_API )
					handler->Handler_Recv_API(topic, pReqInfoPayload, strlen(pReqInfoPayload), pRev1, pRev2);
			}
			else
			{
				SAManagerLog(g_samanagerlogger, Warning, "Cannot find handler: %s", "terminal" );
			}
		}
		else if(reqID == cagent_request_screenshot)
		{
			if(g_SALoader)
			{
				handler = g_SALoader->Loader_FindHandler_API(&g_handlerList, "screenshot");
			}
			if(handler != NULL)
			{
				if( handler->Handler_Recv_API )
					handler->Handler_Recv_API(topic, pReqInfoPayload, strlen(pReqInfoPayload), pRev1, pRev2);
			}
			else
			{
				SAManagerLog(g_samanagerlogger, Warning, "Cannot find handler: %s", "screenshot" );
			}
		}
	}
	free(pReqInfoPayload);
}

void SAManager_CustMessageRecv(char* topic, susiaccess_packet_body_t *pkt, void *pRev1, void* pRev2)
{
	PJSON ReqInfoJSON = NULL;
	char* pReqInfoPayload = NULL;
	struct samanager_topic_entry * topicentry = samanager_Topic_FindTopic(topic);
	
	ReqInfoJSON = SAParser_CreateAgentPacketToJSON(pkt);
	pReqInfoPayload = SAParser_PrintUnformatted(ReqInfoJSON);
	SAParser_Free(ReqInfoJSON);
	//SAManagerLog(g_samanagerlogger, Normal, "Received Topic: %s, Data: %s", topic, pReqInfoPayload);
	if(topicentry != NULL)
	{
		int length = strlen(pReqInfoPayload);
		topicentry->callback_func(topic, pReqInfoPayload, length, NULL, NULL);
	}
	free(pReqInfoPayload);
}

AGENT_SEND_STATUS SAManager_SubscribeCustTopic(void const * const topic, HandlerCustMessageRecvCbf recvCbf)
{
	struct samanager_topic_entry *topicentry = NULL;
	if(!g_subscribeCB)
		return cagent_callback_null;

	topicentry = samanager_Topic_FindTopic(topic);
	if(topicentry == NULL)
	{
		if(g_subscribeCB(topic, SAManager_CustMessageRecv) == 0/*saclient_success*/)
		{
			samanager_Topic_AddTopic(topic, (samanager_topic_msg_cb_func_t)recvCbf);
		}
	}	
	else
		topicentry->callback_func = (samanager_topic_msg_cb_func_t)recvCbf;

	//SAManagerLog(g_samanagerlogger, Normal, "subscribe Customized Topic: %s", topic);
	return cagent_success;
}

void SAMANAGER_API SAManager_Initialize(susiaccess_agent_conf_body_t * config, susiaccess_agent_profile_body_t * profile, void * loghandle)
{
	char workdir[MAX_PATH] = {0};
	Handler_Loader_Interface GlobalPlugin;
	g_samanagerlogger = loghandle;
	
	memset(&g_statethreadctx, 0, sizeof(struct state_ctx));
	g_statethreadctx.msgqueue = malloc(sizeof( struct statequeue));
	if(g_statethreadctx.msgqueue)
	{	
		if(!queue_init(g_statethreadctx.msgqueue, 1000))
		{
			free(g_statethreadctx.msgqueue);
			g_statethreadctx.msgqueue = NULL;
		}
		g_statethreadctx.isThreadRunning = true;
		if (app_os_thread_create(&g_statethreadctx.threadHandler, StateQueueThread, &g_statethreadctx) != 0)
		{
			g_statethreadctx.isThreadRunning = false;
			queue_uninit(g_statethreadctx.msgqueue);
			free(g_statethreadctx.msgqueue);
			g_statethreadctx.msgqueue = NULL;
		}
	}

	if(strlen(profile->workdir)>0)
		strcpy(workdir, profile->workdir);
	else
		app_os_get_module_path(workdir);

	memset(&g_handlerList, 0, sizeof(Handler_List_t));
	/*Load Handler Loader*/	
	if(dl_IsExistSALoaderLib(profile->workdir))
	{
		g_SALoader = malloc(sizeof(SALoader_Interface));
		memset(g_SALoader, 0, sizeof(SALoader_Interface));
		if(dl_LoadSALoaderLib(profile->workdir, g_SALoader))
		{
			printf("HandlerLoader loaded\r\n");
			if(g_SALoader->Loader_Initialize_API)
				g_SALoader->Loader_Initialize_API(workdir, config, profile, loghandle);
			if(g_SALoader->Loader_SetFuncCB_API)
			{
				Callback_Functions_t functions;
				memset(&functions, 0, sizeof(Callback_Functions_t));
				functions.sendcbf = SAManager_SendMessage;
				functions.sendcustcbf = SAManager_SendCustMessage;
				functions.subscribecustcbf = SAManager_SubscribeCustTopic;
				functions.sendreportcbf = SAManager_SendAutoReport;
				functions.sendcapabilitycbf = SAManager_SendCapability;
				functions.sendevnetcbf = SAManager_SendEventNotify;
				functions.connectservercbf = SAManager_ConnectServer;
				functions.disconnectcbf = SAManager_Disconnect;
				g_SALoader->Loader_SetFuncCB_API(&functions);
			}
			//if(g_SALoader->Loader_SetSendCB_API)
			//	g_SALoader->Loader_SetSendCB_API(SAManager_SendMessage);
			//if(g_SALoader->Loader_SetCustSendCB_API)
			//	g_SALoader->Loader_SetCustSendCB_API(SAManager_SendCustMessage);
			//if(g_SALoader->Loader_SetAutoReportCB_API)
			//	g_SALoader->Loader_SetAutoReportCB_API(SAManager_SendAutoReport);
			//if(g_SALoader->Loader_SetCustSubscribeCB_API)
			//	g_SALoader->Loader_SetCustSubscribeCB_API(SAManager_SubscribeCustTopic);
		}
		else
		{
			char *err = dl_GetLoadError();
			SAManagerLog(g_samanagerlogger, Warning, "Load HandlerLoader failed!\r\n  %s!!", err);
			free(err);
		}
	}
	else
	{
		char *err = dl_GetLoadError();
		SAManagerLog(g_samanagerlogger, Warning, "Cannot find HandlerLoader!\r\n  %s!!", err);
		free(err);
	}

	memset(&GlobalPlugin, 0, sizeof(Handler_Loader_Interface));
	if(dl_IsExistSAGeneralLib(profile->workdir))
	{
		g_SAGeneral = malloc(sizeof(SAGeneral_Interface));
		memset(g_SAGeneral, 0, sizeof(SAGeneral_Interface));
		if(dl_LoadSAGeneralLib(profile->workdir, g_SAGeneral))
		{
			
			printf("GeneralHandler loaded\r\n");
			if(g_SALoader)
			{
				if(g_SALoader->Loader_GetBasicHandlerLoaderInterface_API)
					g_SALoader->Loader_GetBasicHandlerLoaderInterface_API(&GlobalPlugin);
			}
			strcpy(GlobalPlugin.Name, "general");
			strncpy(GlobalPlugin.pHandlerInfo->ServerIP,  config->serverIP, strlen(config->serverIP)+1);
			GlobalPlugin.pHandlerInfo->ServerPort = atol(config->serverPort);
			GlobalPlugin.type = core_handler;
			if(g_SAGeneral->General_Initialize_API)
			{
				GlobalPlugin.Handler_Init_API = (HANDLER_INITLIZE)g_SAGeneral->General_Initialize_API;
				g_SAGeneral->General_Initialize_API(GlobalPlugin.pHandlerInfo);
			}

			if(g_SAGeneral->General_HandleRecv_API)
				GlobalPlugin.Handler_Recv_API = (HANDLER_RECV)g_SAGeneral->General_HandleRecv_API;

			if(g_SAGeneral->General_Start_API)
				GlobalPlugin.Handler_Start_API = (HANDLER_START)g_SAGeneral->General_Start_API;

			if(g_SAGeneral->General_Stop_API)
				GlobalPlugin.Handler_Stop_API = (HANDLER_STOP)g_SAGeneral->General_Stop_API;

			if(g_SAGeneral->General_OnStatusChanges_API)
				GlobalPlugin.Handler_OnStatusChange_API = (HANDLER_ONSTATUSCAHNGE)g_SAGeneral->General_OnStatusChanges_API;


			GlobalPlugin.Workable = true;
		}
		else
		{
			char *err = dl_GetLoadError();
			SAManagerLog(g_samanagerlogger, Warning, "Load GeneralHandler failed!\r\n  %s!!", err);
			free(err);
		}
	}
	else
	{
		char *err = dl_GetLoadError();
		SAManagerLog(g_samanagerlogger, Warning, "Cannot find GeneralHandler!\r\n  %s!!", err);
		free(err);
	}

	if(g_SALoader)
	{
		if(g_SAGeneral)
		{
			if(g_SAGeneral->General_SetPluginHandlers_API)
				g_SAGeneral->General_SetPluginHandlers_API(&g_handlerList);
			if(g_SALoader->Loader_AddHandler_API)
				g_SALoader->Loader_AddHandler_API(&g_handlerList, &GlobalPlugin);
		}
		g_SALoader->Loader_LoadAllHandler_API(&g_handlerList, workdir);
		g_SALoader->Loader_StartAllHandler_API(&g_handlerList);
//#ifdef WIN32
		keepalive_initialize(&g_handlerList, g_samanagerlogger);
//#endif
	}
}

void SAMANAGER_API SAManager_Uninitialize()
{
	struct samanager_topic_entry *iter_topic = NULL;
	struct samanager_topic_entry *tmp_topic = NULL;

	if(g_statethreadctx.isThreadRunning == true)
	{
		g_statethreadctx.isThreadRunning = false;
		app_os_thread_join(g_statethreadctx.threadHandler);
		g_statethreadctx.threadHandler = NULL;
	}

	if(g_statethreadctx.msgqueue)
	{
		queue_uninit(g_statethreadctx.msgqueue);
		free(g_statethreadctx.msgqueue);
		g_statethreadctx.msgqueue = NULL;
	}

	/*Release Handler Loader*/	
	if(g_SALoader)
	{
//#ifdef WIN32
		keepalive_uninitialize();
//#endif
		if(g_SALoader->Loader_ConcurrentReleaseAllHandler_API)
			g_SALoader->Loader_ConcurrentReleaseAllHandler_API(&g_handlerList);
		if(g_SALoader->Loader_Uninitialize_API)
			g_SALoader->Loader_Uninitialize_API();
		dl_CleanupSALoaderLib(g_SALoader);
		free(g_SALoader);
		g_SALoader = NULL;
	}
	
	if(g_SAGeneral)
	{
		dl_CleanupSAGeneralLib(g_SAGeneral);
		free(g_SAGeneral);
		g_SAGeneral = NULL;

		app_os_sleep(500);
	}
	
	iter_topic = samanager_Topic_FirstTopic();
	while(iter_topic != NULL)
	{
		tmp_topic = iter_topic->next;
		samanager_Topic_RemoveTopic(iter_topic->name);
		iter_topic = tmp_topic;
	}
}

void SAMANAGER_API SAManager_SetPublishCB(PUBLISHCB func)
{
	g_publishCB = func;
}

void SAMANAGER_API SAManager_SetSubscribeCB(SUBSCRIBECB func)
{
	g_subscribeCB = func;
}

void SAMANAGER_API SAManager_SetConnectServerCB(CONNECTSERVERCB func)
{
	g_connectserverCB = func;
}

void SAMANAGER_API SAManager_SetDisconnectCB(DISCONNECTCB func)
{
	g_disconnectCB = func;
}

void SAMANAGER_API SAManager_AddInternalCallback(susiaccess_agent_profile_body_t * profile)
{
	/* Add Topic Callback*/
	char topicStr[128] = {0};
	sprintf(topicStr, DEF_CALLBACKREQ_TOPIC, profile->devId);
	if(g_subscribeCB)
		g_subscribeCB(topicStr, SAManager_RecvInternalCommandReq);

	memset(topicStr, 0, sizeof(topicStr));
	sprintf(topicStr, DEF_ACTIONACK_TOPIC, profile->devId);
	if(g_subscribeCB)
		g_subscribeCB(topicStr, SAManager_RecvInternalCommandReq);

	/* Add Server Broadcast Topic Callback*/
	if(g_subscribeCB)
		g_subscribeCB(DEF_AGENTCONTROL_TOPIC, SAManager_RecvInternalCommandReq);
}

void SAMANAGER_API SAManager_UpdateConnectState(susiaccess_agent_conf_body_t const * conf, int status)
{
	/* Add Topic Callback*/
	struct statemessage *newmsg = NULL;
	if(g_statethreadctx.msgqueue)
	{
		newmsg = malloc(sizeof(struct statemessage));
		statemsg_create(newmsg, conf, status);
		if(!queue_put(g_statethreadctx.msgqueue, newmsg))
		{
			statemsg_free(newmsg);
			newmsg = NULL;
		}
	}
	else
	{
		if(g_SALoader)
		{
			if(g_SALoader->Loader_SetAgentStatus_API)
				g_SALoader->Loader_SetAgentStatus_API(&g_handlerList, conf, status);
		}
	}
}
