#include "samanagerloader.h"
#include "common.h"

#ifdef WIN32
#define DEF_SAMANAGER_LIB_NAME	"SAManager.dll"
#else
#define DEF_SAMANAGER_LIB_NAME	"libSAManager.so"
#endif

void dl_GetSAManagerFunction(SAManager_Interface * SAManager)
{
	if(SAManager != NULL && SAManager->Handler != NULL)
	{
		SAManager->SAManager_Initialize_API = (SAMANAGER_INITIALIZE)app_get_proc_address(SAManager->Handler, "SAManager_Initialize");
		SAManager->SAManager_Uninitialize_API = (SAMANAGER_UNINITIALIZE)app_get_proc_address(SAManager->Handler, "SAManager_Uninitialize");
		SAManager->SAManager_SetPublishCB_API = (SAMANAGER_SETPBUBLISHCB)app_get_proc_address(SAManager->Handler, "SAManager_SetPublishCB");
		SAManager->SAManager_SetSubscribeCB_API = (SAMANAGER_SETSUBSCRIBECB)app_get_proc_address(SAManager->Handler, "SAManager_SetSubscribeCB");
		SAManager->SAManager_SetConnectServerCB_API = (SAMANAGER_SETCONNECTSERVERCB)app_get_proc_address(SAManager->Handler, "SAManager_SetConnectServerCB");
		SAManager->SAManager_SetDisconnectCB_API = (SAMANAGER_SETDISCONNECTCB)app_get_proc_address(SAManager->Handler, "SAManager_SetDisconnectCB");
		SAManager->SAManager_AddInternalCallback_API = (SAMANAGER_ADDINTERNALCCALLBACK)app_get_proc_address(SAManager->Handler, "SAManager_AddInternalCallback");
		SAManager->SAManager_UpdateConnectState_API = (SAMANAGER_UPDATECONNECTSTATE)app_get_proc_address(SAManager->Handler, "SAManager_UpdateConnectState");
	}
}

bool dl_IsExistSAManagerLib(char* path)
{
	bool bRet = false;
	void * hSAMANAGERDLL = NULL;
	char file[MAX_PATH] = {0};
	path_combine(file, path, DEF_SAMANAGER_LIB_NAME);
	hSAMANAGERDLL = app_load_library(file);
	if(hSAMANAGERDLL != NULL)
	{
		bRet = true;
		app_free_library(hSAMANAGERDLL);
		hSAMANAGERDLL = NULL;
	}
	return bRet;
}

bool dl_LoadSAManagerLib(char* path, SAManager_Interface * SAManager)
{
	bool bRet = false;
	void * hSAMANAGERDLL = NULL;
	char file[MAX_PATH] = {0};
	if(!SAManager)
		return bRet;
	path_combine(file, path, DEF_SAMANAGER_LIB_NAME);
	hSAMANAGERDLL = app_load_library(file);
	if(hSAMANAGERDLL != NULL)
	{
		memset(SAManager, 0, sizeof(SAManager_Interface));
		SAManager->Handler = hSAMANAGERDLL;
		dl_GetSAManagerFunction(SAManager);
	}
	bRet = true;
	return bRet;
}

bool dl_CleanupSAManagerLib(SAManager_Interface * SAManager)
{
	bool bRet = true;
	if(SAManager != NULL)
	{
		if(SAManager->Handler)
			app_free_library(SAManager->Handler);
		SAManager->Handler = NULL;
	}
	return bRet;
}