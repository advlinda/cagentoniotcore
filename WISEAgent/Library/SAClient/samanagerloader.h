#ifndef _SA_SAMGRLOADER_H_
#define _SA_SAMGRLOADER_H_
#include "platform.h"
#include "susiaccess_def.h"
#include "susiaccess_handler_mgmt.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once

#ifndef SAMGRLOADER_API
#define SAMGRLOADER_API WINAPI
#endif
#else
#define SAMGRLOADER_API
#endif

typedef void (*MESSAGERECVCBFN)(char* topic, susiaccess_packet_body_t *pkt, void *pRev1, void* pRev2);
typedef int (SAMGRLOADER_API *PUBLISHCBFN) (char const * topic, susiaccess_packet_body_t const * pkt);
typedef int (SAMGRLOADER_API *SUBSCRIBECBFN)(char const * topic, MESSAGERECVCBFN msg_recv_callback);
typedef int (SAMGRLOADER_API *CONNECTSERVERCBFN)(char const * ip, int port, char const * mqttauth);
typedef void (SAMGRLOADER_API *DISCONNECTCBFN)();

typedef void (SAMGRLOADER_API *SAMANAGER_INITIALIZE)(susiaccess_agent_conf_body_t * config, susiaccess_agent_profile_body_t * profile, void * loghandle);
typedef void (SAMGRLOADER_API *SAMANAGER_UNINITIALIZE)();
typedef void (SAMGRLOADER_API *SAMANAGER_SETPBUBLISHCB)(PUBLISHCBFN func);
typedef void (SAMGRLOADER_API *SAMANAGER_SETSUBSCRIBECB)(SUBSCRIBECBFN func);
typedef void (SAMGRLOADER_API *SAMANAGER_SETCONNECTSERVERCB)(CONNECTSERVERCBFN func);
typedef void (SAMGRLOADER_API *SAMANAGER_SETDISCONNECTCB)(DISCONNECTCBFN func);
typedef void (SAMGRLOADER_API *SAMANAGER_ADDINTERNALCCALLBACK)(susiaccess_agent_profile_body_t * profile);
typedef void (SAMGRLOADER_API *SAMANAGER_UPDATECONNECTSTATE)(susiaccess_agent_conf_body_t const * conf, int status);

typedef struct SAMANAGER_INTERFACE
{
	void*								Handler;               // handle of to load so library
	SAMANAGER_INITIALIZE				SAManager_Initialize_API;
	SAMANAGER_UNINITIALIZE				SAManager_Uninitialize_API;
	SAMANAGER_SETPBUBLISHCB				SAManager_SetPublishCB_API;
	SAMANAGER_SETSUBSCRIBECB			SAManager_SetSubscribeCB_API;
	SAMANAGER_SETCONNECTSERVERCB		SAManager_SetConnectServerCB_API;
	SAMANAGER_SETDISCONNECTCB			SAManager_SetDisconnectCB_API;
	SAMANAGER_ADDINTERNALCCALLBACK		SAManager_AddInternalCallback_API;
	SAMANAGER_UPDATECONNECTSTATE		SAManager_UpdateConnectState_API;
}SAManager_Interface;

#ifdef __cplusplus
extern "C" {
#endif

bool dl_IsExistSAManagerLib(char* path);
bool dl_LoadSAManagerLib(char* path, SAManager_Interface * SAManager);
bool dl_CleanupSAManagerLib(SAManager_Interface * SAManager);

#ifdef __cplusplus
}
#endif

#endif
