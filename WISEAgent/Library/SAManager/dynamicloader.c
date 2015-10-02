#include "dynamicloader.h"
#include "common.h"

#ifdef WIN32
#define DEF_SALOADER_LIB_NAME	"SAHandlerLoader.dll"
#define DEF_SAGENERAL_LIB_NAME	"SAGeneralHandler.dll"
#else
#define DEF_SALOADER_LIB_NAME	"libSAHandlerLoader.so"
#define DEF_SAGENERAL_LIB_NAME	"libSAGeneralHandler.so"
#endif

void dl_GetSALoaderFunction(SALoader_Interface * SALoader)
{
	if(SALoader != NULL && SALoader->Handler != NULL)
	{
		SALoader->Loader_Initialize_API = (LOADER_INITIALIZE)app_get_proc_address(SALoader->Handler, "Loader_Initialize");
		SALoader->Loader_Uninitialize_API = (LOADER_UNINITIALIZE)app_get_proc_address(SALoader->Handler, "Loader_Uninitialize");
		SALoader->Loader_GetBasicHandlerLoaderInterface_API = (LOADER_GETBASICHANDLERLLOADERIINTERFACE)app_get_proc_address(SALoader->Handler, "Loader_GetBasicHandlerLoaderInterface");
		SALoader->Loader_SetAgentStatus_API = (LOADER_SETAGENTSTATUS)app_get_proc_address(SALoader->Handler, "Loader_SetAgentStatus");
		SALoader->Loader_SetFuncCB_API = (LOADER_SETFUNCCB)app_get_proc_address(SALoader->Handler, "Loader_SetFuncCB");
		SALoader->Loader_GetLastHandler_API = (LOADER_GETLASTHANDLER)app_get_proc_address(SALoader->Handler, "Loader_GetLastHandler");
		SALoader->Loader_FindHandler_API = (LOADER_FINDHANDLER)app_get_proc_address(SALoader->Handler, "Loader_FindHandler");
		SALoader->Loader_FindHandlerByReqID_API = (LOADER_FINDHANDLERBYREQID)app_get_proc_address(SALoader->Handler, "Loader_FindHandlerByReqID");
		SALoader->Loader_LoadHandler_API = (LOADER_LOADHANDLER)app_get_proc_address(SALoader->Handler, "Loader_LoadHandler");
		SALoader->Loader_AddHandler_API = (LOADER_ADDHANDLER)app_get_proc_address(SALoader->Handler, "Loader_AddHandler");
		SALoader->Loader_ReleaseHandler_API = (LOADER_RELEASEHANDLER)app_get_proc_address(SALoader->Handler, "Loader_ReleaseHandler");
		SALoader->Loader_LoadAllHandler_API = (LOADER_LOADALLHANDLER)app_get_proc_address(SALoader->Handler, "Loader_LoadAllHandler");
		SALoader->Loader_StartAllHandler_API = (LOADER_STARTALLHANDLER)app_get_proc_address(SALoader->Handler, "Loader_StartAllHandler");
		SALoader->Loader_StopAllHandler_API = (LOADER_STOPALLHANDLER)app_get_proc_address(SALoader->Handler, "Loader_StopAllHandler");
		SALoader->Loader_ReleaseAllHandler_API = (LOADER_RELEASEALLHANDLER)app_get_proc_address(SALoader->Handler, "Loader_ReleaseAllHandler");
		SALoader->Loader_ConcurrentReleaseAllHandler_API = (LOADER_RELEASEALLHANDLER)app_get_proc_address(SALoader->Handler, "Loader_ConcurrentReleaseAllHandler");
	}
}

bool dl_IsExistSALoaderLib(char* path)
{
	bool bRet = false;
	void * hSALOADERDLL = NULL;
	char file[MAX_PATH] = {0};
	path_combine(file, path, DEF_SALOADER_LIB_NAME);
	hSALOADERDLL = app_load_library(file);
	if(hSALOADERDLL != NULL)
	{
		bRet = true;
		app_free_library(hSALOADERDLL);
		hSALOADERDLL = NULL;
	}
	return bRet;
}

bool dl_LoadSALoaderLib(char* path, SALoader_Interface * SALoader)
{
	bool bRet = false;
	void * hSALOADERDLL = NULL;
	char file[MAX_PATH] = {0};
	if(!SALoader)
		return bRet;
	path_combine(file, path, DEF_SALOADER_LIB_NAME);
	hSALOADERDLL = app_load_library(file);
	if(hSALOADERDLL != NULL)
	{
		memset(SALoader, 0, sizeof(SALoader_Interface));
		SALoader->Handler = hSALOADERDLL;
		dl_GetSALoaderFunction(SALoader);
	}
	bRet = true;
	return bRet;
}

bool dl_CleanupSALoaderLib(SALoader_Interface * SALoader)
{
	bool bRet = true;
	if(SALoader != NULL)
	{
		if(SALoader->Handler)
			app_free_library(SALoader->Handler);
		SALoader->Handler = NULL;
	}
	return bRet;
}

void dl_GetSAGeneralFunction(SAGeneral_Interface * SAGeneral)
{
	if(SAGeneral != NULL && SAGeneral->Handler != NULL)
	{
		SAGeneral->General_Initialize_API = (GENERAL_INITIALIZE)app_get_proc_address(SAGeneral->Handler, "General_Initialize");
		SAGeneral->General_Uninitialize_API = (GENERAL_UNINITIALIZE)app_get_proc_address(SAGeneral->Handler, "General_Uninitialize");
		SAGeneral->General_HandleRecv_API = (GENERAL_HANDLERECV)app_get_proc_address(SAGeneral->Handler, "General_HandleRecv");
		SAGeneral->General_SetSendCB_API = (GENERAL_SETSENDCB)app_get_proc_address(SAGeneral->Handler, "General_SetSendCB");
		SAGeneral->General_SetPluginHandlers_API = (GENERAL_SETPLUGINHANDLERS)app_get_proc_address(SAGeneral->Handler, "General_SetPluginHandlers");
		SAGeneral->General_OnStatusChanges_API = (GENERAL_ONSTATUSCHANGE)app_get_proc_address(SAGeneral->Handler, "General_OnStatusChange");
		SAGeneral->General_Start_API = (GENERAL_START)app_get_proc_address(SAGeneral->Handler, "General_Start");
		SAGeneral->General_Stop_API = (GENERAL_STOP)app_get_proc_address(SAGeneral->Handler, "General_Stop");
	}
}

bool dl_IsExistSAGeneralLib(char* path)
{
	bool bRet = false;
	void * hSAGENRERALDLL = NULL;
	char file[MAX_PATH] = {0};
	path_combine(file, path, DEF_SAGENERAL_LIB_NAME);
	hSAGENRERALDLL = app_load_library(file);
	if(hSAGENRERALDLL != NULL)
	{
		bRet = true;
		app_free_library(hSAGENRERALDLL);
		hSAGENRERALDLL = NULL;
	}
	return bRet;
}

bool dl_LoadSAGeneralLib(char* path, SAGeneral_Interface * SAGeneral)
{
	bool bRet = false;
	void * hSAGENRERALDLL = NULL;
	char file[MAX_PATH] = {0};
	if(!SAGeneral)
		return bRet;
	path_combine(file, path, DEF_SAGENERAL_LIB_NAME);
	hSAGENRERALDLL = app_load_library(file);
	if(hSAGENRERALDLL != NULL)
	{
		memset(SAGeneral, 0, sizeof(SAGeneral_Interface));
		SAGeneral->Handler = hSAGENRERALDLL;
		dl_GetSAGeneralFunction(SAGeneral);
	}
	bRet = true;
	return bRet;
}

bool dl_CleanupSAGeneralLib(SAGeneral_Interface * SAGeneral)
{
	bool bRet = true;
	if(SAGeneral != NULL)
	{
		if(SAGeneral->Handler)
			app_free_library(SAGeneral->Handler);
		SAGeneral->Handler = NULL;
	}
	return bRet;
}

char* dl_GetLoadError()
{
	return app_load_error();
}