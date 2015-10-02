#ifndef _SA_DYNLOADER_H_
#define _SA_DYNLOADER_H_
#include "platform.h"
#include "susiaccess_def.h"
#include "susiaccess_handler_mgmt.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once

#ifndef DYLOADER_API
#define DYLOADER_API WINAPI
#endif
#else
#define DYLOADER_API
#endif

typedef void (DYLOADER_API *LOADER_INITIALIZE) (char const * workdir, susiaccess_agent_conf_body_t const * conf, susiaccess_agent_profile_body_t const * profile, void* loghandler);
typedef void (DYLOADER_API *LOADER_UNINITIALIZE) (void);
typedef void (DYLOADER_API *LOADER_GETBASICHANDLERLLOADERIINTERFACE)(Handler_Loader_Interface * handler);
typedef void (DYLOADER_API *LOADER_SETAGENTSTATUS)(Handler_List_t *pLoaderList,susiaccess_agent_conf_body_t const * conf, int status);
typedef void (DYLOADER_API *LOADER_SETFUNCCB)(Callback_Functions_t* funcs);

typedef Handler_Loader_Interface * (DYLOADER_API *LOADER_GETLASTHANDLER)(Handler_List_t *pLoaderList);
typedef Handler_Loader_Interface * (DYLOADER_API *LOADER_FINDHANDLER)(Handler_List_t *pLoaderList, char const *name);
typedef Handler_Loader_Interface * (DYLOADER_API *LOADER_FINDHANDLERBYREQID)(Handler_List_t *pLoaderList, int reqID);

typedef int (DYLOADER_API *LOADER_LOADHANDLER)(Handler_List_t *pLoaderList, char const *handlerpath, char const *name);
typedef int (DYLOADER_API *LOADER_ADDHANDLER)(Handler_List_t *pLoaderList, Handler_Loader_Interface * pluginInfo);
typedef int (DYLOADER_API *LOADER_RELEASEHANDLER)(Handler_List_t *pLoaderList, Handler_Loader_Interface *pLoader);
typedef int (DYLOADER_API *LOADER_LOADALLHANDLER)(Handler_List_t *pLoaderList, char const * workdir);
typedef void (DYLOADER_API *LOADER_STARTALLHANDLER)(Handler_List_t *pLoaderList);
typedef void (DYLOADER_API *LOADER_STOPALLHANDLER)(Handler_List_t *pLoaderList);
typedef void (DYLOADER_API *LOADER_RELEASEALLHANDLER)(Handler_List_t *pLoaderList);
typedef void (DYLOADER_API *LOADER_CONCURRENTRELEASEALLHANDLER)(Handler_List_t *pLoaderList);

typedef struct SALOADER_INTERFACE
{
	void*									Handler;               // handle of to load so library
	LOADER_INITIALIZE						Loader_Initialize_API;
	LOADER_UNINITIALIZE						Loader_Uninitialize_API;
	LOADER_GETBASICHANDLERLLOADERIINTERFACE	Loader_GetBasicHandlerLoaderInterface_API;
	LOADER_SETAGENTSTATUS					Loader_SetAgentStatus_API;
	LOADER_SETFUNCCB						Loader_SetFuncCB_API;
	LOADER_GETLASTHANDLER					Loader_GetLastHandler_API;
	LOADER_FINDHANDLER						Loader_FindHandler_API;
	LOADER_FINDHANDLERBYREQID				Loader_FindHandlerByReqID_API;
	LOADER_LOADHANDLER						Loader_LoadHandler_API;
	LOADER_ADDHANDLER						Loader_AddHandler_API;
	LOADER_RELEASEHANDLER					Loader_ReleaseHandler_API;
	LOADER_LOADALLHANDLER					Loader_LoadAllHandler_API;
	LOADER_STARTALLHANDLER					Loader_StartAllHandler_API;
	LOADER_STOPALLHANDLER					Loader_StopAllHandler_API;
	LOADER_RELEASEALLHANDLER				Loader_ReleaseAllHandler_API;
	LOADER_CONCURRENTRELEASEALLHANDLER		Loader_ConcurrentReleaseAllHandler_API;
}SALoader_Interface;

typedef int (DYLOADER_API *GENERAL_INITIALIZE)(HANDLER_INFO *pluginfo);
typedef void (DYLOADER_API *GENERAL_UNINITIALIZE)(void);
typedef void (DYLOADER_API *GENERAL_HANDLERECV)( char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2 );
typedef void (DYLOADER_API *GENERAL_SETSENDCB)(HandlerSendCbf  sendcbf);
typedef void (DYLOADER_API *GENERAL_SETPLUGINHANDLERS)(Handler_List_t *pLoaderList);
typedef void (DYLOADER_API *GENERAL_ONSTATUSCHANGE)(HANDLER_INFO *pluginfo);
typedef void (DYLOADER_API *GENERAL_START)(void);
typedef void (DYLOADER_API *GENERAL_STOP)(void);

typedef struct SAGENERAL_INTERFACE
{
	void*						Handler;               // handle of to load so library
	GENERAL_INITIALIZE			General_Initialize_API;
	GENERAL_UNINITIALIZE		General_Uninitialize_API;
	GENERAL_HANDLERECV			General_HandleRecv_API;
	GENERAL_SETSENDCB			General_SetSendCB_API;
	GENERAL_SETPLUGINHANDLERS	General_SetPluginHandlers_API;
	GENERAL_ONSTATUSCHANGE		General_OnStatusChanges_API;
	GENERAL_START				General_Start_API;
	GENERAL_STOP				General_Stop_API;
}SAGeneral_Interface;
#ifdef __cplusplus
extern "C" {
#endif

bool dl_IsExistSALoaderLib(char* path);
bool dl_LoadSALoaderLib(char* path, SALoader_Interface * SALoader);
bool dl_CleanupSALoaderLib(SALoader_Interface * SALoader);

bool dl_IsExistSAGeneralLib(char* path);
bool dl_LoadSAGeneralLib(char* path, SAGeneral_Interface * SAGeneral);
bool dl_CleanupSAGeneralLib(SAGeneral_Interface * SAGeneral);

char* dl_GetLoadError();

#ifdef __cplusplus
}
#endif

#endif
