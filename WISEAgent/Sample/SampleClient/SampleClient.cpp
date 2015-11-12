// SampleClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <SAClient.h>

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

void on_msgrecv(char* topic, susiaccess_packet_body_t *pkt, void *pRev1, void* pRev2)
{
	printf("Packet received, %s\r\n", pkt->content);
}

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// This is not a service.
extern __declspec(dllexport)
int _OPENSSL_isservice() { return 0; }

#ifdef __cplusplus
}
#endif  /* __cplusplus */

int _tmain(int argc, _TCHAR* argv[])
{
	int iRet = 0;
	susiaccess_agent_conf_body_t config;
	susiaccess_agent_profile_body_t profile;

	memset(&config, 0 , sizeof(susiaccess_agent_conf_body_t));
	strcpy(config.runMode,"remote");
	strcpy(config.autoStart,"True");
	strcpy(config.serverIP,"adv-wisecloud.cloudapp.net");
	strcpy(config.serverPort,"1883");
	strcpy(config.serverAuth,"fENl4B7tnuwpIbs61I5xJQ==");
	config.tlstype = tls_type_none;
	switch(config.tlstype)
	{
	case tls_type_none:
		break;
	case tls_type_tls:
		{
			strcpy(config.cafile, "ca.crt");
			strcpy(config.capath, "");
			strcpy(config.certfile, "server.crt");
			strcpy(config.keyfile, "server.key");
			strcpy(config.cerpasswd, "123456");
		}
		break;
	case tls_type_psk:
		{
			strcpy(config.psk, "05155853");
			strcpy(config.identity, "SAClientSample");
			strcpy(config.ciphers, "");
		}
		break;
	}

	memset(&profile, 0 , sizeof(susiaccess_agent_profile_body_t));
	sprintf_s(profile.version, DEF_VERSION_LENGTH, "%d.%d.%d.%d", 3, 1, 0, 0);
	strcpy(profile.hostname,"SAClientSample");
	strcpy(profile.devId,"000014DAE996BE03");
	strcpy(profile.sn,"14DAE996BE03");
	strcpy(profile.mac,"14DAE996BE03");
	strcpy(profile.type,"IPC");
	strcpy(profile.product,"Sample Agent");
	strcpy(profile.manufacture,"test");
	strcpy(profile.osversion,"NA");
	strcpy(profile.biosversion,"NA");
	strcpy(profile.platformname,"NA");
	strcpy(profile.processorname,"NA");
	strcpy(profile.osarchitect,"NA");
	profile.totalmemsize = 40832;
	strcpy(profile.maclist,"14DAE996BE03");
	strcpy(profile.localip,"172.22.12.21");
	strcpy(profile.account,"anonymous");
	strcpy(profile.passwd,"");

	iRet = saclient_initialize(&config, &profile, NULL);

	if(iRet != saclient_success)
	{
		printf("Unable to initialize AgentCore.\r\n");
		goto EXIT;
	}
	printf("Agent Initialized\r\n");

	saclient_connection_callback_set(on_connect_cb, on_lost_connect_cb, on_disconnect_cb);

	printf("Agent Set Callback");
	
	iRet = saclient_connect();

	if(iRet != saclient_success){
		printf("Unable to connect to broker.\r\n");
		goto EXIT;
	} else {
		printf("Connect to broker: %s\r\n", config.serverIP);
	}

	{
		
		char topicStr[128] = {0};
		susiaccess_packet_body_t pkt;

		/* Add  subscribe topic Callback*/
		sprintf(topicStr, "/cagent/admin/%s/testreq", profile.devId);
		saclient_subscribe(topicStr, on_msgrecv);
		
		/*Send packet to specific topic*/
		strcpy(pkt.devId, profile.devId);
		strcpy(pkt.handlerName, "Test");
		//packet.catalogID = cagent_catalog_susi_func;
		pkt.requestID = 0;
		pkt.cmd = 0;
		pkt.content = (char*)malloc(strlen("{\"Test\":100}")+1);
		memset(pkt.content, 0, strlen("{\"Test\":100}")+1);
		strcpy(pkt.content, "{\"Test\":100}");
		saclient_publish(topicStr, &pkt);
		free(pkt.content);
	}

EXIT:
	printf("Click enter to exit");
	fgetc(stdin);

	saclient_disconnect();
	printf("Send Client Info: disconnect\r\n");
	saclient_uninitialize();

	return iRet;
}

