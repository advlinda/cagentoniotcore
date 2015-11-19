// SampleServiceMain.cpp : Defines the entry point for the console application.
//

#include "platform.h"
#include "common.h"
#include "service.h"
#include "SAClient.h"

typedef struct {
	void* threadHandler;
	bool isThreadRunning;
	susiaccess_agent_conf_body_t config;
	susiaccess_agent_profile_body_t profile;
}connect_context_t;

bool g_bConnectRetry = false;
connect_context_t g_ConnContex;
char g_CAgentStatusPath[MAX_PATH] = { 0 };

void on_connect_cb();
void on_lost_connect_cb();
void on_disconnect_cb();

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// This is a service.
extern __declspec(dllexport)
	int _OPENSSL_isservice() { return 1; }

#ifdef __cplusplus
}
#endif  /* __cplusplus */

static CAGENT_PTHREAD_ENTRY(ConnectThreadStart, args)
{
	bool bRet = false;
	connect_context_t *pConnContex = (connect_context_t *)args;

	if (pConnContex == NULL)
	{
		app_os_thread_exit(-1);
		return -1;
	}

CONN_RETRY:

	bRet = saclient_connect() == saclient_success;

	app_os_thread_exit(0);
	return 0;
}

bool ConnectToServer(connect_context_t *pConnContex)
{
	bool bRet = false;
	if (pConnContex->isThreadRunning)
		return bRet;

	g_bConnectRetry = true;
	if (app_os_thread_create(&pConnContex->threadHandler, ConnectThreadStart, pConnContex) != 0)
	{
		pConnContex->isThreadRunning = false;
		bRet = false;
	}
	else
	{
		pConnContex->isThreadRunning = true;
		bRet = true;
	}
	return bRet;
}

bool Disconnect()
{
	bool bRet = false;
	g_bConnectRetry = false;

	if (g_ConnContex.isThreadRunning == true)
	{
		g_ConnContex.isThreadRunning = false;
		app_os_thread_join(g_ConnContex.threadHandler);
		g_ConnContex.threadHandler = NULL;
	}
	saclient_disconnect();

	return bRet;
}

void on_connect_cb()
{
	printf("CB_Connected \r\n");
}

void on_lost_connect_cb()
{
	printf("CB_Lostconnect \r\n");
}

void on_disconnect_cb()
{
	printf("CB_Disconnect \r\n");
}

int CAgentStart()
{
	int iRet = 0;

	memset(&g_ConnContex.config, 0, sizeof(susiaccess_agent_conf_body_t));
	strcpy(g_ConnContex.config.runMode, "remote");
	strcpy(g_ConnContex.config.autoStart, "True");
	strcpy(g_ConnContex.config.serverIP, "adv-wisecloud.cloudapp.net");
	strcpy(g_ConnContex.config.serverPort, "1883");
	strcpy(g_ConnContex.config.serverAuth, "fENl4B7tnuwpIbs61I5xJQ==");
	g_ConnContex.config.tlstype = tls_type_none;
	switch (g_ConnContex.config.tlstype)
	{
	case tls_type_none:
		break;
	case tls_type_tls:
	{
		strcpy(g_ConnContex.config.cafile, "ca.crt");
		strcpy(g_ConnContex.config.capath, "");
		strcpy(g_ConnContex.config.certfile, "server.crt");
		strcpy(g_ConnContex.config.keyfile, "server.key");
		strcpy(g_ConnContex.config.cerpasswd, "123456");
	}
	break;
	case tls_type_psk:
	{
		strcpy(g_ConnContex.config.psk, "05155853");
		strcpy(g_ConnContex.config.identity, "SAClientSample");
		strcpy(g_ConnContex.config.ciphers, "");
	}
	break;
	}

	memset(&g_ConnContex.profile, 0, sizeof(susiaccess_agent_profile_body_t));
	sprintf_s(g_ConnContex.profile.version, DEF_VERSION_LENGTH, "%d.%d.%d.%d", 3, 1, 0, 0);
	strcpy(g_ConnContex.profile.hostname, "SAClientSample_adv");
	strcpy(g_ConnContex.profile.devId, "000014DAE996BE04");
	strcpy(g_ConnContex.profile.sn, "14DAE996BE04");
	strcpy(g_ConnContex.profile.mac, "14DAE996BE04");
	strcpy(g_ConnContex.profile.type, "IPC");
	strcpy(g_ConnContex.profile.product, "Sample Agent");
	strcpy(g_ConnContex.profile.manufacture, "test");
	strcpy(g_ConnContex.profile.osversion, "NA");
	strcpy(g_ConnContex.profile.biosversion, "NA");
	strcpy(g_ConnContex.profile.platformname, "NA");
	strcpy(g_ConnContex.profile.processorname, "NA");
	strcpy(g_ConnContex.profile.osarchitect, "NA");
	g_ConnContex.profile.totalmemsize = 40832;
	strcpy(g_ConnContex.profile.maclist, "14DAE996BE04");
	strcpy(g_ConnContex.profile.localip, "172.22.12.21");
	strcpy(g_ConnContex.profile.account, "anonymous");
	strcpy(g_ConnContex.profile.passwd, "");


	if (saclient_initialize(&g_ConnContex.config, &g_ConnContex.profile, NULL) != saclient_success)
	{
		saclient_uninitialize();
		goto EXIT_START;
	}

	saclient_connection_callback_set(on_connect_cb, on_lost_connect_cb, on_disconnect_cb);

	iRet = ConnectToServer(&g_ConnContex) ? 0 : -1;

EXIT_START:
	return iRet;
}

int CAgentStop()
{
	int iRet = 0;

	Disconnect();
	saclient_uninitialize();

	return iRet;
}

int main(int argc, char *argv[])
{
	bool isSrvcInit = false;
	char moudlePath[MAX_PATH] = { 0 };
	char CAgentLogPath[MAX_PATH] = { 0 };
	char CAgentConfigPath[MAX_PATH] = { 0 };
	char serverName[DEF_OSVERSION_LEN] = { 0 };
	char version[DEF_VERSION_LENGTH] = { 0 };
#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	_CrtMemCheckpoint(&memStateStart);
#endif  

	sprintf(version, "%d.%d.%d.%d", 3, 1, 0, 0);

	memset(moudlePath, 0, sizeof(moudlePath));

	app_os_get_module_path(moudlePath);
	
	if (true)
	{
		isSrvcInit = ServiceInit("AgentService_31", version, CAgentStart, CAgentStop, NULL) == 0 ? true : false;
	}
	else
	{
		isSrvcInit = ServiceInit(NULL, version, CAgentStart, CAgentStop, NULL) == 0 ? true : false;
	}


	if (!isSrvcInit) return -1;

	if (argv[1] != NULL)
	{
		bool bRet = false;
		char cmdChr[MAX_CMD_LEN] = { '\0' };
		memcpy(cmdChr, argv[1], strlen(argv[1]));
		do
		{
			bRet = ExecuteCmd(strtok(cmdChr, "\n")) == 0 ? true : false;
			if (!bRet) break;
			memset(cmdChr, 0, sizeof(cmdChr));
			gets_s(cmdChr, sizeof(cmdChr));
		} while (true);
	}
	else
	{
		if (isSrvcInit)
		{
			LaunchService();
		}
	}
	if (isSrvcInit) ServiceUninit();

#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint(&memStateEnd);
	if (_CrtMemDifference(&memStateDiff, &memStateStart, &memStateEnd))
		_CrtMemDumpStatistics(&memStateDiff);
#endif
	return 0;
}
