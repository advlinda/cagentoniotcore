#include "platform.h"
#include "common.h"
#include "SAGeneralHandler.h"
#include <cJSON.h>
#include "generallog.h"
#include <ftphelper.h>
#include "util.h"
//#include "generalconfig.h"
#include <profile.h>

#define AGENTINFO_BODY_STRUCT	"susiCommData"
#define AGENTINFO_CMDTYPE		"commCmd"
#define AGENTINFO_AUTOREPORT	"autoReport"
#define AGENTINFO_REPORTDATALEN	"reportDataLength"
#define AGENTINFO_REPORTDATA	"reportData"

#define GBL_UPDATE_PARMAS                "params"
#define GBL_UPDATE_USERNAME              "userName"
#define GBL_UPDATE_PASSWORD              "pwd"
#define GBL_UPDATE_PORT                  "port"
#define GBL_UPDATE_PATH                  "path"
#define GBL_UPDATE_MD5                   "md5"

#define GBL_RENAME_DEVNAME				 "devName"

#define GBL_SERVER_RESPONSE				 "response"
#define GBL_SERVER_STATUSCODE			 "statuscode"
#define GBL_SERVER_RESPONSEMESSAGE		 "msg"
#define GBL_SERVER_SERVER_NODE			 "server"
#define GBL_SERVER_SERVER_ADDRESS		 "address"
#define GBL_SERVER_SERVER_PORT			 "port"
#define GBL_SERVER_SERVER_AUTH			 "auth"
#define GBL_SERVER_SERVER_IP_LIST		 "serverIPList"
#define GBL_SERVER_N_FLAG				 "n"


#ifdef _WIN32
#define DEF_CAGENT_UPDATER_EXE_NAME                     "CAgentUpdater.exe"
#define DEF_CAGENT_INSTALLER_DOWNLOAD_FILE_NAME         "SA31_CAgent.exe"
#else
#define DEF_CAGENT_UPDATER_EXE_NAME                     "SA31_CAgent.run"
#define DEF_CAGENT_INSTALLER_DOWNLOAD_FILE_NAME         "SA31_CAgent.run"
#endif // _WIN32
#define DEF_ADVANTECH_FOLDER_NAME                       "Advantech"
//#define IP_LIST_BUF_LEN             32
//#define MAX_CONNECT_FAILED_CNT      2

typedef struct update_cagent_params_t{
	char ftpuserName[64];
	char ftpPassword[64];
	int port;
	char installerPath[260];
	char md5[128];

	char filename[MAX_PATH];
	char downloadpath[MAX_PATH];
	char updatepath[MAX_PATH];

	ftp_context_t* ctxdl;
}update_cagent_params_t;

static Handler_info_ex  g_PluginInfo;
static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic
static HandlerConnectServerCbf g_connectservercbf = NULL;	
static HandlerDisconnectCbf g_disconnectcbf = NULL;	
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;	
//static void* g_loghandle = NULL;

static update_cagent_params_t * g_updateParams = NULL;
static CAGENT_THREAD_HANDLE g_DLThreadHandle = NULL;
static CAGENT_THREAD_HANDLE g_DLMonThreadHandle = NULL;
static CAGENT_THREAD_HANDLE g_GetCapabilityThreadHandle = NULL;
static bool g_IsDLMonThreadRunning = false;
//static susiaccess_general_conf_body_t* g_config = NULL;
static char g_ConfigPath[MAX_PATH] = {0};

const char genreral_Topic[MAX_TOPIC_LEN] = {"general"};
const int genreral_RequestID = cagent_request_general;
const int genreral_ActionID = cagent_action_general;

Handler_List_t *g_pPL_List = NULL;
//int g_redundantServerNum = 0;
//int g_connectedFailedCnt = 0;

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
			General_Uninitialize();
		}
		else // Process is terminating
		{
			// Cleanup
			General_Uninitialize();
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
	General_Uninitialize();
}
#endif

int ParseReceivedCMD(void* data, int datalen, int * cmdID)
{
	/*{"susiCommData":{"commCmd":251,"catalogID":4,"requestID":10}}*/

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(!data) return false;
	if(datalen<=0) return false;
	//MonitorLog(g_loghandle, Normal, " %s>Parser_ParseReceivedData [%s]\n", MyTopic, data );
	root = cJSON_Parse(data);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return false;
	}

	target = cJSON_GetObjectItem(body, AGENTINFO_CMDTYPE);
	if(target)
	{
		*cmdID = target->valueint;
	}
	cJSON_Delete(root);
	return true;
}

int ParseUpdateCMD(void* data, int datalen, update_cagent_params_t *pUpdateParams)
{
	/*{"susiCommData":{"commCmd":111,"catalogID":4,"requestID":16,"params":{"userName":"sa30Read","pwd":"sa30Read","port":2121,"path":"/upgrade/SA30Agent_V3.0.15.exe","md5":"758C9D0A8654A93D09F375D33E262507"}}}*/
	cJSON * root = NULL, *body = NULL, *pSubItem = NULL; 
	cJSON *pUpdateParamsItem = NULL;
	bool bRet = false;
	if(!data) return false;
	if(datalen<=0) return false;
	if(!pUpdateParams) return false;

	root = cJSON_Parse(data);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return false;
	}

	pUpdateParamsItem = cJSON_GetObjectItem(body, GBL_UPDATE_PARMAS);
	if(pUpdateParams && pUpdateParamsItem)
	{
		pSubItem = cJSON_GetObjectItem(pUpdateParamsItem, GBL_UPDATE_USERNAME);
		if(pSubItem)
		{
			strcpy(pUpdateParams->ftpuserName, pSubItem->valuestring);
			pSubItem = cJSON_GetObjectItem(pUpdateParamsItem, GBL_UPDATE_PASSWORD);
			if(pSubItem)
			{
				strcpy(pUpdateParams->ftpPassword, pSubItem->valuestring);
				pSubItem = cJSON_GetObjectItem(pUpdateParamsItem, GBL_UPDATE_PORT);
				if(pSubItem)
				{
					pUpdateParams->port = pSubItem->valueint;
					pSubItem = cJSON_GetObjectItem(pUpdateParamsItem, GBL_UPDATE_PATH);
					if(pSubItem)
					{
						strcpy(pUpdateParams->installerPath, pSubItem->valuestring);
						pSubItem = cJSON_GetObjectItem(pUpdateParamsItem, GBL_UPDATE_MD5);
						if(pSubItem)
						{
							strcpy(pUpdateParams->md5, pSubItem->valuestring);
							bRet = true;
						}
					}
				}
			}
		}
	}
	cJSON_Delete(root);
	return bRet;
}

int ParseRenameCMD(void* data, int datalen, char* pNewName)
{
	/*{"susiCommData":{"devName":"pc-test1","commCmd":113,"requestID":1001,"agentID":"","handlerName":"","sendTS":1434447015}}*/
	cJSON * root = NULL, *body = NULL, *pSubItem = NULL; 
	bool bRet = false;
	if(!data) return false;
	if(datalen<=0) return false;
	if(!pNewName) return false;

	root = cJSON_Parse(data);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return false;
	}

	pSubItem = cJSON_GetObjectItem(body, GBL_RENAME_DEVNAME);
	if(pSubItem)
	{
		strcpy(pNewName, pSubItem->valuestring);
		bRet = true;
	}
	cJSON_Delete(root);
	return bRet;
}

int ParseServerCtrl(void* data, int datalen, RESPONSE_MESSAGE *pMessage)
{
	/*{"susiCommData":{"commCmd":125,"handlerName":"general","catalogID":4,"response":{"statuscode":0,"msg":"Server losyconnection"}}}*/
	cJSON *root = NULL, *body = NULL, *pSubItem = NULL, *pTarget = NULL, *pServer = NULL,*pServerIPList = NULL; 
	bool bRet = false;
	if(!data) return false;
	if(datalen<=0) return false;
	if(!pMessage) return false;

	root = cJSON_Parse(data);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return false;
	}

	pSubItem = cJSON_GetObjectItem(body, GBL_SERVER_RESPONSE);
	if(pSubItem)
	{
		pTarget = cJSON_GetObjectItem(pSubItem, GBL_SERVER_STATUSCODE);
		if(pTarget)
		{
			pMessage->statuscode = pTarget->valueint;
		}

		pTarget = cJSON_GetObjectItem(pSubItem, GBL_SERVER_RESPONSEMESSAGE);
		if(pTarget)
		{
			if(pTarget->valuestring)
			{
				if(strlen(pTarget->valuestring)<=0)
					pMessage->msg = NULL;
				else
				{
					pMessage->msg = malloc(strlen(pTarget->valuestring)+1);
					memset(pMessage->msg, 0, strlen(pTarget->valuestring)+1);
					strcpy(pMessage->msg, pTarget->valuestring);
				}
			}
		}

		pServer = cJSON_GetObjectItem(pSubItem, GBL_SERVER_SERVER_NODE);
		if(pServer)
		{
			pTarget = cJSON_GetObjectItem(pServer, GBL_SERVER_SERVER_ADDRESS);
			if(pTarget)
			{
				strcpy(pMessage->serverIP, pTarget->valuestring);
			}

			pTarget = cJSON_GetObjectItem(pServer, GBL_SERVER_SERVER_PORT);
			if(pTarget)
			{
				pMessage->serverPort = pTarget->valueint;
			}

			pTarget = cJSON_GetObjectItem(pServer, GBL_SERVER_SERVER_AUTH);
			if(pTarget)
			{
				strcpy(pMessage->serverAuth, pTarget->valuestring);
			}
		}
		pServerIPList = cJSON_GetObjectItem(pSubItem, GBL_SERVER_SERVER_IP_LIST);
		if(pServerIPList)
		{
			cJSON * subItem = NULL;
			cJSON * valItem = NULL;
			int i = 0;
			FILE *fp = NULL;
			int nCount = cJSON_GetArraySize(pServerIPList);
			char filepath[MAX_PATH] = {0};
			path_combine(filepath, g_PluginInfo.WorkDir, DEF_SERVER_IP_LIST_FILE);
			if(fp=fopen(filepath,"wt+"))
			{
				for(i = 0; i<nCount; i++)
				{
					subItem = cJSON_GetArrayItem(pServerIPList, i);
					if(subItem)
					{
						valItem = cJSON_GetObjectItem(subItem, GBL_SERVER_N_FLAG);
						if(valItem)
						{
							fputs(valItem->valuestring,fp);
							fputc('\n',fp);
						}
					}
				}
			}
			fclose(fp);
		}
		bRet = true;
	}
	cJSON_Delete(root);
	return bRet;
}

bool  GeneralSend(int cmd, char const * msg, int len, void *pRev1, void* pRev2)
{
	bool bRet = false;
	char* payload = NULL; 
	cJSON* node = cJSON_Parse(msg);
	int payloadlenth = 0;
	if(node == NULL)
	{
		payloadlenth = len+strlen("{\"result\":\"\"}"); //"msg":"XXX"
		payload = malloc(payloadlenth+1);
		memset(payload, 0, payloadlenth+1);
		sprintf(payload, "{\"result\":\"%s\"}", msg);
	}
	else
	{
		cJSON_Delete(node);
		payloadlenth = len;
		payload = malloc(payloadlenth+1);
		memset(payload, 0, payloadlenth+1);
		strcpy(payload, msg);
	}
	if(g_sendcbf)
		g_sendcbf(&g_PluginInfo, cmd, payload, payloadlenth, pRev1, pRev2);
	free(payload);
	payload = NULL;
	return bRet;
}

static CAGENT_PTHREAD_ENTRY( CAgentGetCapabilityThreadStart, args)
{
	Handler_Loader_Interface *pInterfaceTmp = NULL;
	int length = 0;
	Handler_List_t *pLoaderList = (Handler_List_t *)args;
	if(pLoaderList == NULL)
	{
		app_os_thread_exit(-1);
		return -1;
	}

	pInterfaceTmp = pLoaderList->items;
	while(pInterfaceTmp)
	{
		char* tmpinfo = NULL;
		if(pInterfaceTmp->Workable == false)
		{
			pInterfaceTmp = pInterfaceTmp->next;
			continue;
		}
		if(pInterfaceTmp->type != user_handler)
		{
			pInterfaceTmp = pInterfaceTmp->next;
			continue;
		}

		if(pInterfaceTmp->Handler_Get_Capability_API)
			length = pInterfaceTmp->Handler_Get_Capability_API(&tmpinfo);
	
		if(length>0)
		{
			if(g_sendcapabilitycbf)
				g_sendcapabilitycbf(&g_PluginInfo, tmpinfo, strlen(tmpinfo), NULL,NULL);
		}
		if(tmpinfo)
		{
			if(pInterfaceTmp->Handler_MemoryFree_API)
				pInterfaceTmp->Handler_MemoryFree_API(tmpinfo);
			else
				free(tmpinfo);
		}
			
		pInterfaceTmp = pInterfaceTmp->next;
	}
	app_os_thread_exit(0);
	return 0;
}

bool StartAutoReport(Handler_List_t *pLoaderList, char* data)
{
	Handler_Loader_Interface *pInterfaceTmp = NULL;
	if(!pLoaderList) return false;

	pInterfaceTmp = pLoaderList->items;
	while(pInterfaceTmp)
	{
		if(pInterfaceTmp->Workable == false)
		{
			pInterfaceTmp = pInterfaceTmp->next;
			continue;
		}
		if(pInterfaceTmp->type != user_handler)
		{
			pInterfaceTmp = pInterfaceTmp->next;
			continue;
		}
		if(pInterfaceTmp->Handler_AutoReportStart_API)
			pInterfaceTmp->Handler_AutoReportStart_API(data);
		pInterfaceTmp = pInterfaceTmp->next;
	}
	return true;
}

bool StopAutoReport(Handler_List_t *pLoaderList, char* data)
{
	Handler_Loader_Interface *pInterfaceTmp = NULL;

	if(!pLoaderList) return false;

	if(pLoaderList->total <= 0) return false;

	pInterfaceTmp = pLoaderList->items;
	while(pInterfaceTmp)
	{
		if(pInterfaceTmp->Workable == false)
		{
			pInterfaceTmp = pInterfaceTmp->next;
			continue;
		}
		if(pInterfaceTmp->type != user_handler)
		{
			pInterfaceTmp = pInterfaceTmp->next;
			continue;
		}
		if(pInterfaceTmp->Handler_AutoReportStop_API)
			pInterfaceTmp->Handler_AutoReportStop_API(data);

		pInterfaceTmp = pInterfaceTmp->next;
	}
	return true;
}

bool CAgentInstallerRun(char * installPath)
{
	bool bRet = false;
	if(installPath == NULL || !IsFileExist(installPath)) return bRet;
	{
		char updaterPath[MAX_PATH] = {0}; 
#ifdef _WIN32
		char modulePath[MAX_PATH] = {0};     
		app_os_get_module_path(modulePath);
		path_combine(updaterPath, modulePath, DEF_CAGENT_UPDATER_EXE_NAME);
#else
		sprintf(updaterPath, "%s", installPath);
		app_os_set_executable(updaterPath);
#endif // _WIN32
		if(IsFileExist(updaterPath))
		{
			app_os_kill_process_name(DEF_CAGENT_UPDATER_EXE_NAME);
			bRet = ExecuteInstaller(updaterPath);
			if(bRet)
			{
				SAGeneralLog(Normal, " %s> %s", genreral_Topic, "Create updater process ok!");
			}
			else
			{
				SAGeneralLog(Error, " %s> %s", genreral_Topic, "Create updater process error!");
			}
		}
		else
		{
			SAGeneralLog(Error, " %s> updaterPath not exist:%s", genreral_Topic, updaterPath);
		}
	}
	return bRet;
}

static CAGENT_PTHREAD_ENTRY( CAgentDLMonThreadStart, args)
{
	if(args != NULL)
	{
		update_cagent_params_t* pUpdateParam = NULL;
		FTPSTATUS ftpDlStatus = FTP_UNKNOWN;
		bool isBreak = false;
		int iRet = 0, i = 0;
		pUpdateParam = (update_cagent_params_t*)args;
		app_os_sleep(10);
		while(g_IsDLMonThreadRunning)
		{
			iRet = 0;
			if(pUpdateParam->ctxdl)
				iRet = ftphelper_FTPGetStatus(pUpdateParam->ctxdl, &ftpDlStatus);
			if(iRet == 0)
			{
				switch(ftpDlStatus)
				{
				case FTP_START:
					{
						for(i = 0; i<5 && g_IsDLMonThreadRunning; i++) app_os_sleep(100);
						break;
					}
				case FTP_TRANSFERRING:
					{
						char downloadDetial[128] = {0};
						DWORD dPercent = 0;
						DWORD dCurSizeKB = 0;
						float dSpeed = 0;
						ftphelper_FTPGetPersent(pUpdateParam->ctxdl, &dPercent);
						ftphelper_FtpGetSpeedKBS(pUpdateParam->ctxdl, &dSpeed);
						ftphelper_FTPGetCurSizeKB(pUpdateParam->ctxdl, &dCurSizeKB);
						sprintf(downloadDetial,"Downloading,Download Cursize:%dKB, Persent:%d%%, Speed: %4.2fKB/S\n", dCurSizeKB, dPercent, dSpeed);
						GeneralSend(glb_update_cagent_rep, downloadDetial, strlen(downloadDetial)+1, NULL, NULL);
						for(i = 0; i<10 && g_IsDLMonThreadRunning; i++) app_os_sleep(100);
						break;
					}
				case FTP_FINISHED:
					{
						char downloadDetial[128] = {0};
						DWORD dPercent = 0;
						DWORD dCurSizeKB = 0;
						float dSpeed = 0;
						ftphelper_FTPGetPersent(pUpdateParam->ctxdl, &dPercent);
						ftphelper_FtpGetSpeedKBS(pUpdateParam->ctxdl, &dSpeed);
						ftphelper_FTPGetCurSizeKB(pUpdateParam->ctxdl, &dCurSizeKB);
						sprintf(downloadDetial,"Download Finished,Download Cursize:%dKB, Persent:%d%%, Speed: %4.2fKB/S\n", dCurSizeKB, dPercent, dSpeed);
						GeneralSend(glb_update_cagent_rep, downloadDetial, strlen(downloadDetial)+1, NULL, NULL);
						isBreak = TRUE;
						break;
					}
				case FTP_ERROR:
					{
						char lastMsgTmp[1024] = {0};
						char lastDownloaderErrorMsg[512] = {0};
						ftphelper_FTPGetLastError(pUpdateParam->ctxdl, lastDownloaderErrorMsg, sizeof(lastDownloaderErrorMsg));
						sprintf(lastMsgTmp,"File downloader status error!Error msg:%s\n", lastDownloaderErrorMsg);
						GeneralSend(glb_update_cagent_rep, lastMsgTmp, strlen(lastMsgTmp)+1, NULL, NULL);
						isBreak = TRUE;
						break;
					}
				default:
				case FTP_UNKNOWN:
					break;
				}
			}
			if(isBreak) break;
			app_os_sleep(10);
		}
	}
	g_IsDLMonThreadRunning = false;
	app_os_thread_exit(0);
	return 0;
}

bool CAgentDownload(update_cagent_params_t *pUpdateParams)
{
	char repMsg[1024] = {0};
	char md5Str[64] = {0};
	ftp_context_t* ctxdl = NULL;

	if(!pUpdateParams)
		return false;
	
	ftphelper_EnableLog(SAGeneralLogHandle);

	ctxdl = ftphelper_FtpDownload(g_PluginInfo.ServerIP, pUpdateParams->port, pUpdateParams->ftpuserName, pUpdateParams->ftpPassword, pUpdateParams->installerPath, pUpdateParams->downloadpath);
	pUpdateParams->ctxdl = ctxdl;
	ftphelper_WaitTransferComplete(ctxdl);
	g_IsDLMonThreadRunning = false;
	ftphelper_FTPGetLastError(ctxdl, repMsg, sizeof(repMsg));
	ftphelper_FtpCleanup(ctxdl);

	if(strlen(repMsg))
	{
		SAGeneralLog(Normal, " %s> FTP ERR MSG: %s", genreral_Topic, repMsg);
		GeneralSend(glb_update_cagent_rep, repMsg, strlen(repMsg), NULL, NULL);
		return false;
	}

	if(GetFileMD5(pUpdateParams->downloadpath, md5Str))
	{
		if(_stricmp(md5Str, pUpdateParams->md5) != 0)
		{
			memset(repMsg, 0, sizeof(repMsg));
			sprintf(repMsg, "Check md5 error!");
			SAGeneralLog(Error, " %s> %s DWMd5:%s, CalcMD5:%s", genreral_Topic, repMsg, pUpdateParams->md5, md5Str);
			GeneralSend(glb_update_cagent_rep, repMsg, strlen(repMsg), NULL, NULL);
			return false;
		}
	}
	else
	{
		SAGeneralLog(Error, " %s> Calculate MD5 failed on: %s", genreral_Topic, pUpdateParams->filename);
		sprintf(repMsg, "Calculate MD5 failed!");
		GeneralSend(glb_update_cagent_rep, repMsg, strlen(repMsg), NULL, NULL);
		return false;
	}

	SAGeneralLog(Normal, " %s> Check md5 OK! DWMd5:%s, CalcMD5:%s", genreral_Topic,pUpdateParams->md5, md5Str);

	if(IsFileExist(pUpdateParams->updatepath))
	{
		if(remove(pUpdateParams->updatepath)<0)
		{
			sprintf(repMsg, "Cannot remove file: %s", pUpdateParams->updatepath);
			SAGeneralLog(Error, " %s> %s", genreral_Topic, repMsg);
		}
	}

	if(rename(pUpdateParams->downloadpath, pUpdateParams->updatepath)<0)
	{
		sprintf(repMsg, "Change file name failed on: %s", pUpdateParams->downloadpath);
		SAGeneralLog(Error, " %s> %s", genreral_Topic, repMsg);
		GeneralSend(glb_update_cagent_rep, repMsg, strlen(repMsg), NULL, NULL);
		return false;
	}
	memset(repMsg, 0, sizeof(repMsg));
	sprintf(repMsg, "%s", "CAgentUpdater run...");
	SAGeneralLog(Normal, " %s> %s", genreral_Topic, repMsg);
	GeneralSend(glb_update_cagent_rep, repMsg, strlen(repMsg), NULL, NULL);
	//run installer
	if(!CAgentInstallerRun(pUpdateParams->updatepath))
	{
		memset(repMsg, 0, sizeof(repMsg));
		sprintf(repMsg, "%s", "CAgentUpdater run error!");
		SAGeneralLog(Error, " %s> %sInstallerDownloadPath:%s", genreral_Topic, repMsg, pUpdateParams->downloadpath);
		GeneralSend(glb_update_cagent_rep, repMsg, strlen(repMsg), NULL, NULL);
		return false;
	}
	return true;
}


static CAGENT_PTHREAD_ENTRY(UpdateCagentThreadStart, args)
{
	update_cagent_params_t * pUpdateParams = (update_cagent_params_t *)args;

	if(pUpdateParams)
	{
		char buf [MAX_PATH] = {0};
		char filename[MAX_PATH] = {0};
		char downloadpath[MAX_PATH] = {0};
		char updatepath[MAX_PATH] = {0};
		char repMsg[1024] = {0};

		CAGENT_THREAD_HANDLE DLMonThreadHandle = NULL;

		split_path_file(pUpdateParams->installerPath, buf, filename);

		memset(buf, 0, MAX_PATH);
		
		if(app_os_get_temppath(buf, MAX_PATH) > 0)
		{
			char advantchPath[MAX_PATH] = {0};
			path_combine(advantchPath, buf, DEF_ADVANTECH_FOLDER_NAME);
			app_os_create_directory(advantchPath);
			path_combine(downloadpath, advantchPath, filename);
			path_combine(updatepath, advantchPath, DEF_CAGENT_INSTALLER_DOWNLOAD_FILE_NAME);
		}
		else
		{
			char modulePath[MAX_PATH] = {0};     
			app_os_get_module_path(modulePath);
			path_combine(downloadpath, modulePath, filename);
			path_combine(updatepath, modulePath, DEF_CAGENT_INSTALLER_DOWNLOAD_FILE_NAME);
		}

		strcpy(pUpdateParams->downloadpath, downloadpath);
		strcpy(pUpdateParams->updatepath, updatepath);
		strcpy(pUpdateParams->filename, filename);
		g_IsDLMonThreadRunning = true;
		if(app_os_thread_create(&DLMonThreadHandle, CAgentDLMonThreadStart, pUpdateParams) != 0)
		{
			g_IsDLMonThreadRunning = false;
			DLMonThreadHandle = NULL;
			memset(repMsg, 0, sizeof(repMsg));
			sprintf(repMsg, "%s", "CAgent installer download monitor start failed!");
			SAGeneralLog(Normal, "%s", repMsg);
			GeneralSend(glb_update_cagent_rep, repMsg, strlen(repMsg), NULL, NULL);
		}
		else
		{
			g_DLMonThreadHandle = DLMonThreadHandle;
			CAgentDownload(pUpdateParams);
			g_IsDLMonThreadRunning = false;
			app_os_thread_join(DLMonThreadHandle);
		}
	}

	app_os_thread_exit(0);
	return 0;
}

bool UpdateCagent(void* data, int datalen, update_cagent_params_t *pUpdateParams, char* reqMSG)
{
	char repMsg[1024] = {0};

	if(!data) return false;
	if(datalen<=0) return false;
	if(!pUpdateParams) return false;
	
	{
		//char repMsg[2*1024] = {0};
		if(ParseUpdateCMD(data, datalen, pUpdateParams)==true)
		{
			if (app_os_thread_create(&g_DLThreadHandle, UpdateCagentThreadStart, pUpdateParams) != 0)
			{
				sprintf(repMsg, "%s", "Update cagent thread start error!");
			}
		}
		else
		{
			sprintf(repMsg, "%s", "Update command parse error!");
			GeneralSend(glb_update_cagent_rep, repMsg, strlen(repMsg), NULL, NULL);
			return false;
		}
	}
	return true;
}

void UpdateCagentStop()
{
	if(g_updateParams)
	{
		if(g_IsDLMonThreadRunning && g_DLMonThreadHandle)
		{
			g_IsDLMonThreadRunning = false;
			app_os_thread_join(g_DLMonThreadHandle);
			g_DLMonThreadHandle = NULL;
		}

		ftphelper_FtpCleanup(g_updateParams->ctxdl);
		free(g_updateParams);

		if(g_DLThreadHandle)
		{
			app_os_thread_join(g_DLThreadHandle);
			g_DLThreadHandle = NULL;
		}
	}
	g_updateParams = NULL;	
}

static void UpdateCagentRetry(char * data, int datalen)
{
	char reqMSG[256];
	UpdateCagentStop();
	
	g_updateParams = malloc(sizeof(update_cagent_params_t));
	memset(g_updateParams, 0, sizeof(update_cagent_params_t));

	if(!UpdateCagent(data, datalen, g_updateParams, reqMSG))
	{
		free(g_updateParams);
		g_updateParams = NULL;
	}
}

bool SendHandlerList(Handler_List_t *pLoaderList)
{
	/*{"susiCommData":{"commCmd":124,"catalogID":4,"requestID":1001,"handlerName":"general","handlerlist":["handler1","handler2"]}}*/
	bool bRet = false;
	cJSON * root = NULL, *body = NULL; 
	char* cPayload = NULL;
	Handler_Loader_Interface *pInterfaceTmp = NULL;

	if(!pLoaderList) return bRet;

	if(pLoaderList->total <= 0) return bRet;

	body = cJSON_CreateArray();
	pInterfaceTmp = pLoaderList->items;
	while(pInterfaceTmp)
	{
		cJSON* pTempNode = NULL;
		if(pInterfaceTmp->Workable == false)
		{
			pInterfaceTmp = pInterfaceTmp->next;
			continue;
		}
		if(pInterfaceTmp->type != user_handler)
		{
			pInterfaceTmp = pInterfaceTmp->next;
			continue;
		}
		pTempNode = cJSON_CreateString(pInterfaceTmp->pHandlerInfo->Name);
		cJSON_AddItemToArray(body, pTempNode);
		pInterfaceTmp = pInterfaceTmp->next;
	}

	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "handlerlist", body);
	cPayload = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	
	bRet = GeneralSend(glb_get_handler_list_rep, cPayload, strlen(cPayload), NULL, NULL);
	free(cPayload);
	return bRet;
}
/*
void ServercontrolAction(Response_Msg *pRespMsg)
{
	SAGeneralLog(Normal, " %s> Server Response: %s", genreral_Topic, pRespMsg->msg);

	switch (pRespMsg->statuscode)
	{
	case SERVER_LOST_CONNECTION:
	case SERVER_AUTH_SUCCESS:
	case SERVER_PRESERVED_MESSAGE:
		break;
	case SERVER_AUTH_FAILED:
	case SERVER_CONNECTION_FULL:
		{
			if(g_disconnectcbf)
				g_disconnectcbf();
		}
		break;
	case SERVER_RECONNECT:
		{
			if(g_connectservercbf)
				g_connectservercbf(g_PluginInfo.ServerIP, g_PluginInfo.ServerPort, g_PluginInfo.serverAuth);
		}
		break;
	case SERVER_CONNECT_TO_MASTER:
		{
			//connect to master
			//if(g_connectservercbf)
			//	g_connectservercbf(master_ip, master_port, master_auth);
		}
		break;
	case SERVER_CONNECT_TO_SEPCIFIC:
		{
			//connect to server
			if(g_connectservercbf)
				g_connectservercbf(pRespMsg->serverIP, pRespMsg->serverPort, pRespMsg->serverAuth);
		}
		break;
	default:
		break;
	}
}*/
/*
int GetRedundantServerCnt()
{
	FILE *fp = NULL;
	int i = 0;
	//bool bRet = false;
	char filepath[MAX_PATH] = {0};
	char serverNetInfo[IP_LIST_BUF_LEN] = {0};
	path_combine(filepath, g_PluginInfo.WorkDir, DEF_SERVER_IP_LIST_FILE);
	if(fp=fopen(filepath,"rt"))
	{
		while(fgets(serverNetInfo, IP_LIST_BUF_LEN, fp) != NULL)
		{
			i++;
		}
		fclose(fp);
	}
	return i;
}

void ConnectRedundantServer(int redundantServerNum)
{
	FILE *fp = NULL;
	//bool bRet = false;
	char filepath[MAX_PATH] = {0};
	char serverNetInfo[IP_LIST_BUF_LEN] = {0};
	path_combine(filepath, g_PluginInfo.WorkDir, DEF_SERVER_IP_LIST_FILE);
	if(fp=fopen(filepath,"rt"))
	{
		char tmpInfoStr[IP_LIST_BUF_LEN] = {0};
		char* tmp_loginID = NULL;
		char* tmp_loginPwd = NULL;
		char* tmp_desSrc = NULL;
		const int IP_ADDRE_NUM = 0;
		const int IP_PORT_NUM = 1;
		const int IP_AUTH_NUM = 2;
		int j = 0;
		while(fgets(serverNetInfo, IP_LIST_BUF_LEN, fp) != NULL)
		{
			int redServerPort = 0;
			char * sliceStr[16] = {NULL};
			char * buf = NULL;
			int i = 0;
			if(j < redundantServerNum)
			{
				j++;
				continue;
			}
			strcpy(tmpInfoStr, serverNetInfo);
			buf = tmpInfoStr;
			while(sliceStr[i] = strtok(buf, ":"))
			{
				i++;
				buf = NULL;
			}
			if(i>IP_PORT_NUM)
			{

				int tmpLen = strlen(sliceStr[IP_AUTH_NUM]);
				sliceStr[IP_AUTH_NUM][tmpLen-1] = 0;
				redServerPort = atoi(sliceStr[IP_PORT_NUM]);
				SAGeneralLog(Normal, "Connect to redundant broker: %s", sliceStr[0]);
				if(g_connectservercbf)
					g_connectservercbf(sliceStr[IP_ADDRE_NUM], redServerPort, sliceStr[IP_AUTH_NUM]);
			}
			memset(serverNetInfo,0, sizeof(char)*IP_LIST_BUF_LEN);
			memset(tmpInfoStr, 0, sizeof(char)*IP_LIST_BUF_LEN);
			break;
		}
		fclose(fp);
	}
}
*/
int SAGENERAL_API General_Initialize(HANDLER_INFO *pluginfo)
{
	HANDLER_INFO_EX* tmpinfo = NULL;
	if( pluginfo == NULL )
		return handler_fail;

	tmpinfo = (HANDLER_INFO_EX*)pluginfo;
	SAGeneralLogHandle = tmpinfo->loghandle;

	// 1. Topic of this handler
	snprintf( tmpinfo->Name, sizeof(tmpinfo->Name), "%s", genreral_Topic );
	SAGeneralLog(Normal, " %s> Initialize", genreral_Topic);
	tmpinfo->RequestID = genreral_RequestID;
	tmpinfo->ActionID = genreral_ActionID;
	
	// 2. Copy agent info 
	memcpy(&g_PluginInfo, tmpinfo, sizeof(HANDLER_INFO_EX));

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf = g_PluginInfo.sendcbf = tmpinfo->sendcbf;
	g_sendcustcbf = g_PluginInfo.sendcustcbf = tmpinfo->sendcustcbf;
	g_subscribecustcbf = g_PluginInfo.subscribecustcbf = tmpinfo->subscribecustcbf;
	g_sendreportcbf = g_PluginInfo.sendreportcbf = tmpinfo->sendreportcbf;
	g_sendcapabilitycbf = g_PluginInfo.sendcapabilitycbf = tmpinfo->sendcapabilitycbf;
	g_connectservercbf = g_PluginInfo.connectservercbf = tmpinfo->connectservercbf;	
	g_disconnectcbf = g_PluginInfo.disconnectcbf = tmpinfo->disconnectcbf;
	
	path_combine(g_ConfigPath, tmpinfo->WorkDir, DEF_CONFIG_FILE_NAME);
	/*
	g_config = malloc(sizeof(susiaccess_general_conf_body_t));
	memset(g_config, 0, sizeof(susiaccess_general_conf_body_t));
	SAGeneralLog(Normal, " %s> general_load", genreral_Topic);
	if(!general_load(g_ConfigPath, g_config))
	{
		SAGeneralLog(Normal, " %s> general_create", genreral_Topic);
		general_create(g_ConfigPath, g_config);
	}
	*/
	g_sendcbf = tmpinfo->sendcbf;
	return handler_success;
}

void SAGENERAL_API General_Uninitialize()
{
	if(g_GetCapabilityThreadHandle)
	{
		app_os_thread_join(g_GetCapabilityThreadHandle);
		g_GetCapabilityThreadHandle = NULL;
	}
	StopAutoReport(g_pPL_List, NULL);
	g_sendcbf = NULL;
	SAGeneralLogHandle = NULL;

	g_pPL_List = NULL;

	/*
	if(g_config)
	{
		if(g_config->reportData)
			free(g_config->reportData);
		g_config->reportData = NULL;
		free(g_config);
	}
	g_config = NULL;
	*/
}

void SAGENERAL_API General_HandleRecv( char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2 )
{
	int cmdID = 0;
	SAGeneralLog(Normal, " %s>Recv Topic [%s] Data %s", genreral_Topic, topic, (char*) data );

	if(!ParseReceivedCMD(data, datalen, &cmdID))
		return;
	switch (cmdID)
	{
	case general_info_spec_req:
		{
			if(g_pPL_List)
			{
				if(g_GetCapabilityThreadHandle)
				{
					app_os_thread_join(g_GetCapabilityThreadHandle);
					g_GetCapabilityThreadHandle = NULL;
				}
				app_os_thread_create(&g_GetCapabilityThreadHandle, CAgentGetCapabilityThreadStart, g_pPL_List);
			}
		}
		break;
	case general_start_auto_upload_req:
		{
			StartAutoReport(g_pPL_List, data);
			GeneralSend(general_start_auto_upload_rep, "SUCCESS", strlen("SUCCESS"), NULL, NULL);
			/*
			if(!g_config)
			{
				g_config = malloc(sizeof(susiaccess_general_conf_body_t));
				memset(g_config, 0, sizeof(susiaccess_general_conf_body_t));
			}
			else
			{
				g_config->reportDataLength = 0;
				if(g_config->reportData)
					free(g_config->reportData);
				g_config->reportData = NULL;
			}
			strcpy(g_config->autoReportEn, "True");
			g_config->reportDataLength = datalen;
			g_config->reportData = malloc(datalen+1);
			memset(g_config->reportData, 0, datalen+1);
			strcpy(g_config->reportData, (char*) data);
			general_save(g_ConfigPath, g_config);
			*/

		}
		break;
	case general_stop_auto_upload_req:
		{
			StopAutoReport(g_pPL_List, data);
			GeneralSend(general_stop_auto_upload_rep, "SUCCESS", strlen("SUCCESS"), NULL, NULL);
			/*
			if(!g_config)
			{
				g_config = malloc(sizeof(susiaccess_general_conf_body_t));
				memset(g_config, 0, sizeof(susiaccess_general_conf_body_t));
			}
			else
			{
				g_config->reportDataLength = 0;
				free(g_config->reportData);
				g_config->reportData = NULL;
			}
			strcpy(g_config->autoReportEn, "False");
			general_save(g_ConfigPath, g_config);
			*/
		}
		break;
	case glb_update_cagent_req:
		{
			char reqMSG[256];
			g_updateParams = malloc(sizeof(update_cagent_params_t));
			memset(g_updateParams, 0, sizeof(update_cagent_params_t));
			
			if(!UpdateCagent(data, datalen, g_updateParams, reqMSG))
			{
				free(g_updateParams);
				g_updateParams = NULL;
			}
		}
		break;
	case glb_update_cagent_stop_req:
		{
			char repMsg[2*1024] = {0};
			UpdateCagentStop();
			sprintf(repMsg, "%s", "Update cagent stop success!");
			GeneralSend(glb_update_cagent_stop_rep, repMsg, strlen(repMsg), NULL, NULL);
			break;
		}
	case glb_update_cagent_retry_req:
		{
			UpdateCagentRetry((char *)data, datalen);
			break;
		}
	case glb_get_handler_list_req:
		{
			SendHandlerList(g_pPL_List);
			break;
		}
	case glb_cagent_rename_req:
		{
			char name[DEF_HOSTNAME_LENGTH] = {0};
			//SAGeneralLog(Normal, " %s> Rename: %s", genreral_Topic, data);
			ParseRenameCMD(data, datalen, name);
			profile_set(g_ConfigPath, "DeviceName", name);
			break;
		}
	/*case glb_server_control_req:
		{
			int statuscode = SERVER_UNDEFINED;
			char *message = NULL;
			Response_Msg pRespMsg;
			memset(&pRespMsg,0,sizeof(Response_Msg));
			ParseServerCtrl(data, datalen, &pRespMsg);
			ServercontrolAction(&pRespMsg);
			if(pRespMsg.msg)
				free(pRespMsg.msg);
			break;
		}*/
	default:
		{
			char * errorRepJsonStr = NULL;
			char errorStr[128];
			int jsonStrlen = 0;
			sprintf(errorStr, "%s", "Unknown cmd!");
			if(jsonStrlen > 0 && errorRepJsonStr != NULL)
			{
				GeneralSend(glb_error_rep, errorStr, strlen(errorStr), NULL, NULL);
			}
			if(errorRepJsonStr)free(errorRepJsonStr);
			break;
		}
	}
}

void SAGENERAL_API General_SetPluginHandlers(Handler_List_t *pLoaderList)
{
	g_pPL_List = pLoaderList;
}

void SAGENERAL_API General_OnStatusChange( HANDLER_INFO *pluginfo )
{
	printf(" %s> Update Status", genreral_Topic);
	if(pluginfo)
		memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	else
	{
		memset(&g_PluginInfo, 0, sizeof(HANDLER_INFO));
		snprintf( g_PluginInfo.Name, sizeof( g_PluginInfo.Name), "%s", genreral_Topic );
		g_PluginInfo.RequestID = genreral_RequestID;
		g_PluginInfo.ActionID = genreral_ActionID;
	}
	if(pluginfo->agentInfo->status == AGENT_STATUS_ONLINE)
	{
		SendHandlerList(g_pPL_List);
		///g_redundantServerNum = 0;
		//g_connectedFailedCnt = 0;
	}
	/*else if(pluginfo->agentInfo->status == AGENT_STATUS_CONNECTION_FAILED && g_connectedFailedCnt > MAX_CONNECT_FAILED_CNT)
	{
		int serverCnt = GetRedundantServerCnt();
		if(g_redundantServerNum < serverCnt) 
		{
			ConnectRedundantServer(g_redundantServerNum);
			g_redundantServerNum++;
		}
		else
		{
			printf(" %s> Update Status, connect redundant server failed!\n", genreral_Topic);
		}
	}
	else if(pluginfo->agentInfo->status == AGENT_STATUS_CONNECTION_FAILED)
	{
		g_connectedFailedCnt++;
	}*/
}

void SAGENERAL_API General_Start()
{
	/*
	if(!g_config)
		return;

	if(!g_config->reportData)
		return;

	if(!_stricmp(g_config->autoReportEn, "True"))
	{
		if(!g_GetCapabilityThreadHandle)
			app_os_thread_create(&g_GetCapabilityThreadHandle, CAgentGetCapabilityThreadStart, g_pPL_List);
		app_os_thread_join(g_GetCapabilityThreadHandle);
		g_GetCapabilityThreadHandle = NULL;

		StartAutoReport(g_pPL_List, g_config->reportData);
	}
	*/
}

void SAGENERAL_API General_Stop()
{
	/*
	if(!g_config)
		return;

	if(!g_config->reportData)
		return;


	if(!_stricmp(g_config->autoReportEn, "True"))
	{
		StopAutoReport(g_pPL_List, NULL);
	}
	*/
}
