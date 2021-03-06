#include "platform.h"
#include "SAClientParser.h"
#include <cJSON.h>

#define BASICINFO_BODY_STRUCT	"susiCommData"
#define BASICINFO_REQID			"requestID"
#define BASICINFO_HANDLERNAME	"handlerName"
#define BASICINFO_CMDTYPE		"commCmd"
#define BASICINFO_CATALOG		"catalogID"
#define BASICINFO_AGENTID		"agentID"
#define BASICINFO_TIMESTAMP		"sendTS"

#define AGENTINFO_DEVID			"devID"
#define AGENTINFO_HOSTNAME		"hostname"
#define AGENTINFO_SN			"sn"
#define AGENTINFO_MAC			"mac"
#define AGENTINFO_VERSION		"version"
#define AGENTINFO_TYPE			"type"
#define AGENTINFO_PRODUCT		"product"
#define AGENTINFO_MANUFACTURE	"manufacture"
#define AGENTINFO_STATUS		"status"
#define AGENTINFO_USERNAME		"account"
#define AGENTINFO_PASSWORD		"passwd"

#define GLOBAL_OS_INFO                   "osInfo"
#define GLOBAL_OS_VERSION                "osVersion"
#define GLOBAL_AGENT_VERSION             "cagentVersion"
#define GLOBAL_AGENT_TYPE             "cagentType"
#define GLOBAL_BIOS_VERSION              "biosVersion"
#define GLOBAL_PLATFORM_NAME             "platformName"
#define GLOBAL_PROCESSOR_NAME            "processorName"
#define GLOBAL_OS_ARCH                   "osArch"
#define GLOBAL_SYS_TOTAL_PHYS_MEM        "totalPhysMemKB"
#define GLOBAL_SYS_MACS					 "macs"
#define GLOBAL_SYS_IP					 "IP"

char *SAClientParser_Print(PJSON item)
{
	if(item == NULL)
		return NULL;
	return cJSON_Print(item);
}

char *SAClientParser_PrintUnformatted(PJSON item)
{
	if(item == NULL)
		return NULL;
	return cJSON_PrintUnformatted(item);
}

void SAClientParser_Free(PJSON ptr)
{
	cJSON *pAgentInfo = ptr;
	cJSON_Delete(pAgentInfo);
}

PJSON SAClientParser_AgentBaseInfoToJSON(susiaccess_agent_profile_body_t const * pProfile, int status)
{
	/*
{"susiCommData":
  {
    "commCmd":1,
	"requestID":21,
	"devID":"00004437E646AC6D",
	"hostname":"WIN-A0F7V3LOTLL",
	"sn":"00004437E646AC6D",
	"version":"1.0.0",
	"account":"admin",
	"passwd":"admin",
	"status":1
  }
}
	*/
   cJSON *pAgentInfo = NULL;

   if(!pProfile) return NULL;
   pAgentInfo = cJSON_CreateObject();
   cJSON_AddStringToObject(pAgentInfo, AGENTINFO_DEVID, pProfile->devId);
   cJSON_AddStringToObject(pAgentInfo, AGENTINFO_HOSTNAME, pProfile->hostname);
   cJSON_AddStringToObject(pAgentInfo, AGENTINFO_SN, pProfile->sn);
   cJSON_AddStringToObject(pAgentInfo, AGENTINFO_MAC, pProfile->mac);
   cJSON_AddStringToObject(pAgentInfo, AGENTINFO_VERSION, pProfile->version);
   cJSON_AddStringToObject(pAgentInfo, AGENTINFO_TYPE, pProfile->type);
   cJSON_AddStringToObject(pAgentInfo, AGENTINFO_PRODUCT, pProfile->product);
   cJSON_AddStringToObject(pAgentInfo, AGENTINFO_MANUFACTURE, pProfile->manufacture);
   cJSON_AddStringToObject(pAgentInfo, AGENTINFO_USERNAME, pProfile->account);
   cJSON_AddStringToObject(pAgentInfo, AGENTINFO_PASSWORD, pProfile->passwd);
   cJSON_AddNumberToObject(pAgentInfo, AGENTINFO_STATUS, status);

   return pAgentInfo;
}

PJSON SAClientParser_CreateOSInfo(susiaccess_agent_profile_body_t const * pProfile)
{
/*
{
    "osInfo":{"cagentVersion":"1.0.0","osVersion":"Windows 7 Service Pack 1","biosVersion":"","platformName":"","processorName":"","osArch":"X64","totalPhysMemKB":8244060,"macs":"84:8F:69:CB:12:96;C4:17:FE:DB:F4:F5;00:50:56:C0:00:01;00:50:56:C0:00:08","IP":"172.22.12.24"},
}
*/
	cJSON *OSInfoHead = NULL;
  	cJSON * pOSInfoItem = NULL;

	if(!pProfile) return OSInfoHead;

	pOSInfoItem = cJSON_CreateObject();

	cJSON_AddStringToObject(pOSInfoItem, GLOBAL_AGENT_VERSION, pProfile->version);
	cJSON_AddStringToObject(pOSInfoItem, GLOBAL_AGENT_TYPE, pProfile->type);
	cJSON_AddStringToObject(pOSInfoItem, GLOBAL_OS_VERSION, pProfile->osversion);
	cJSON_AddStringToObject(pOSInfoItem, GLOBAL_BIOS_VERSION, pProfile->biosversion);
	cJSON_AddStringToObject(pOSInfoItem, GLOBAL_PLATFORM_NAME, pProfile->platformname);
	cJSON_AddStringToObject(pOSInfoItem, GLOBAL_PROCESSOR_NAME, pProfile->processorname);
	cJSON_AddStringToObject(pOSInfoItem, GLOBAL_OS_ARCH, pProfile->osarchitect);
	cJSON_AddNumberToObject(pOSInfoItem, GLOBAL_SYS_TOTAL_PHYS_MEM, pProfile->totalmemsize);
	cJSON_AddStringToObject(pOSInfoItem, GLOBAL_SYS_MACS, pProfile->maclist);
	cJSON_AddStringToObject(pOSInfoItem, GLOBAL_SYS_IP, pProfile->localip);

	OSInfoHead = cJSON_CreateObject();
	cJSON_AddItemToObject(OSInfoHead, GLOBAL_OS_INFO, pOSInfoItem);

	return OSInfoHead;
}

PJSON SAClientParser_CreateAgentPacketToJSON(susiaccess_packet_body_t const * pPacket)
{
	/*
{"susiCommData":{"commCmd":271,"requestID":103, XXX}}
	*/
   cJSON *pReqInfoHead = NULL;
   cJSON* root = NULL;
   long long tick = 0;

   if(!pPacket) return NULL;
   if(pPacket->content)
   {
	   root = cJSON_Parse(pPacket->content);
   }
   if(!root)
	   root = cJSON_CreateObject();

   
   pReqInfoHead = cJSON_CreateObject();

   cJSON_AddItemToObject(pReqInfoHead, BASICINFO_BODY_STRUCT, root);
   cJSON_AddNumberToObject(root, BASICINFO_CMDTYPE, pPacket->cmd);
   cJSON_AddNumberToObject(root, BASICINFO_REQID, pPacket->requestID);
   cJSON_AddStringToObject(root, BASICINFO_AGENTID, pPacket->devId);
   cJSON_AddStringToObject(root, BASICINFO_HANDLERNAME, pPacket->handlerName);
   //cJSON_AddNumberToObject(root, BASICINFO_CATALOG, pPacket->catalogID);

   tick = (long long) time((time_t *) NULL);
   cJSON_AddNumberToObject(root, BASICINFO_TIMESTAMP, tick);
   return pReqInfoHead;
}

int SAClientParser_ParseRecvMessage(void* data, int datalen, susiaccess_packet_body_t * pkt)
{
	/*{"susiCommData":{"commCmd":251,"catalogID":4,"requestID":10}}*/

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* content = NULL;

	if(!data) return false;

	if(!pkt) return false;

	memset(pkt, 0 , sizeof(susiaccess_packet_body_t));

	root = cJSON_Parse(data);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, BASICINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return false;
	}

	target = body->child;
	while (target)
	{
		if(!strcmp(target->string, BASICINFO_CMDTYPE))
			pkt->cmd = target->valueint;
		else if(!strcmp(target->string, BASICINFO_REQID))
			pkt->requestID = target->valueint;
		else if(!strcmp(target->string, BASICINFO_AGENTID))
			strcpy(pkt->devId, target->valuestring);
		else if(!strcmp(target->string, BASICINFO_HANDLERNAME))
			strcpy(pkt->handlerName, target->valuestring);
		else if(!strcmp(target->string, BASICINFO_CATALOG))
		{
			//pkt->catalogID = target->valueint;
		}
		else
		{
			if(!content)
				content = cJSON_CreateObject();

			cJSON_AddItemToObject(content, target->string, cJSON_Duplicate(target,true));
		}
		target = target->next;
	}

	if(content)
	{
		char* strcontent = cJSON_PrintUnformatted(content);
		cJSON_Delete(content);
		pkt->content = _strdup(strcontent);
		free(strcontent);
	}

	cJSON_Delete(root);
	return true;
}