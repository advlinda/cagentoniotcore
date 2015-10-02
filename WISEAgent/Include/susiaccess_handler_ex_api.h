#ifndef _CAGENT_HANDLER_EX_H_
#define _CAGENT_HANDLER_EX_H_

#include "susiaccess_handler_api.h"

typedef int (*HandlerConnectServerCbf)(char const * ip, int port, char const * mqttauth);
typedef void (*HandlerDisconnectCbf)();

typedef struct HANDLER_INFO_EX
{	
#ifdef _MSC_VER  
	struct HANDLER_INFO;
#else
	char Name[MAX_TOPIC_LEN];						// The handler name
	char ServerIP[MAX_PATH];
	int ServerPort;
	char WorkDir[MAX_PATH];							
	int RequestID;
	int ActionID;
	//key_t shmKey;									// shared memory key	
	void* loghandle;								// log file handler

	//GetLibFNP  GetLibAPI;							// Get the Function Point of comman library api
	cagent_agent_info_body_t * agentInfo;			// Info of the Agent
	HandlerSendCbf  sendcbf;						// Client Send information (in JSON format) to Cloud Server	
	HandlerSendCustCbf  sendcustcbf;			    // Client Send information (in JSON format) to Cloud Server with custom topic
	HandlerSubscribeCustCbf subscribecustcbf;		// Client subscribe the custom topic to receive message from Cloud Server 
	HandlerSendCapabilityCbf sendcapabilitycbf;			// Client Send Spec.Info (in JSON format) to Cloud Server with SpecInfo topic	
	HandlerAutoReportCbf sendreportcbf;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic
	HandlerSendEventCbf sendeventcbf;				// Client Send Event Notify (in JSON format) to Cloud Server with EventNotify topic	
#endif

	HandlerConnectServerCbf connectservercbf;				// connect to specific server callback function
	HandlerDisconnectCbf disconnectcbf;						// disconnect callback function
	char serverAuth[DEF_USER_PASS_LENGTH];
}HANDLER_INFO_EX, Handler_info_ex;

typedef struct RESPONSE_MESSAGE
{
	int statuscode;
	char* msg;
	char serverIP[DEF_MAX_STRING_LENGTH];
	int serverPort;
	char serverAuth[DEF_USER_PASS_LENGTH];
}RESPONSE_MESSAGE, Response_Msg;

#endif /* _CAGENT_HANDLER_EX_H_ */