#ifndef _GENERAL_CMD_HANDLER_H
#define _GENERAL_CMD_HANDLER_H\

#include "susiaccess_handler_mgmt.h"

#define cagent_request_general 1001
#define cagent_action_general 2001

typedef enum{
	general_unknown_cmd = 0,

	//--------------------------Global command define(101--130)--------------------------------
	glb_update_cagent_req = 111,
	glb_update_cagent_rep,
	glb_cagent_rename_req = 113,
	glb_cagent_rename_rep,
	glb_update_cagent_stop_req = 119,
	glb_update_cagent_stop_rep = 120,
	glb_update_cagent_retry_req = 121,
	glb_update_cagent_retry_rep = 122,
	glb_get_handler_list_req = 123,
	glb_get_handler_list_rep = 124,
	//glb_server_control_req = 125,

	glb_error_rep = 600,
	//------------------------WSN General command define(2051--2100)------------------------------
	general_info_spec_req = 2051,
	general_info_spec_rep = 2052,
	general_start_auto_upload_req = 2053,
	general_start_auto_upload_rep = 2054,
	general_info_upload_rep = 2055,
	general_stop_auto_upload_req = 2056,
	general_stop_auto_upload_rep = 2057,
}susi_comm_cmd_t;

typedef enum{
	SERVER_UNDEFINED = -1,
	SERVER_LOST_CONNECTION = 0,
	SERVER_AUTH_SUCCESS = 1,
	SERVER_AUTH_FAILED,
	SERVER_CONNECTION_FULL,
	SERVER_RECONNECT,
	SERVER_CONNECT_TO_MASTER,
	SERVER_CONNECT_TO_SEPCIFIC,
	SERVER_PRESERVED_MESSAGE,
	SERVER_SET_SERVER_IP_LIST,
}server_status_code_t;

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#ifndef SAGENERAL_API
#define SAGENERAL_API WINAPI
#endif
#else
#define SAGENERAL_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

int SAGENERAL_API General_Initialize(HANDLER_INFO *pluginfo);
void SAGENERAL_API General_Uninitialize();
void SAGENERAL_API General_HandleRecv( char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2 );
void SAGENERAL_API General_SetPluginHandlers(Handler_List_t *pLoaderList);
void SAGENERAL_API General_OnStatusChange( HANDLER_INFO *pluginfo );
void SAGENERAL_API General_Start();
void SAGENERAL_API General_Stop();

#ifdef __cplusplus
}
#endif

#endif