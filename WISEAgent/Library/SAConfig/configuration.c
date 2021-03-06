#include "XMLBase.h"
#include "configuration.h"

#define XML_CONFIG_ROOT "XMLConfigSettings"
#define XML_CONFIG_BASIC "BaseSettings"

#define RUN_MODE_KEY			"RunMode"
#define LAUNCH_CONNECT_KEY		"LaunchConnect"
#define AUTO_START_KEY			"AutoStart"
#define IOT_AUTO_REPORT			"AutoReport"

#define SERVER_IP_KEY			"ServerIP"
#define SERVER_PORT_KEY			"ServerPort"
#define SERVER_AUTH_KEY			"ConnAuth"
//#define USER_NAME_KEY			"UserName"
//#define USER_PASSWORD_KEY		"UserPassword"

#define SERVER_TLS_TYPE			"TLSType"
#define SERVER_TLS_CAFILE		"CAFile"
#define SERVER_TLS_CAPATH		"CAPath"
#define SERVER_TLS_CERTFILE		"CertFile"
#define SERVER_TLS_KEYFILE		"KeyFile"
#define SERVER_TLS_CERTPASS		"CertPW"

#define SERVER_TLS_PSK			"PSK"
#define SERVER_PSK_IDENTIFY		"PSKIdentify"
#define SERVER_PSK_CIPHERS		"PSKCiphers"

int SACONFIG_API cfg_load(char const * configFile, susiaccess_agent_conf_body_t * conf)
{
	int iRet = false;
	xml_doc_info * doc = NULL;
	bool bModify = false;
	char temp[256] = {0};

	if(configFile == NULL) 
		return iRet;
	if(conf == NULL)
		return iRet;
	
	doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	if(doc == NULL)
		return iRet;

	memset(conf, 0, sizeof(susiaccess_agent_conf_body_t));

	if(!xml_GetItemValue(doc, RUN_MODE_KEY, conf->runMode, sizeof(conf->runMode)))
	{
		memset(conf->runMode, 0, sizeof(conf->runMode));
	}
	if(strlen(conf->runMode)<=0)
	{
		strncpy(conf->runMode, DEF_STANDALONE_RUN_MODE, strlen(DEF_STANDALONE_RUN_MODE)+1);
		xml_SetItemValue(doc, RUN_MODE_KEY, conf->runMode);
		bModify = true;
	}

	/*if(!xml_GetItemValue(doc, LAUNCH_CONNECT_KEY, conf->lunchConnect, sizeof(conf->lunchConnect)))
	{
		memset(conf->lunchConnect, 0, sizeof(conf->lunchConnect));
	}
	if(strlen(conf->lunchConnect)<=0)
	{
		strncpy(conf->lunchConnect, "False", strlen("False")+1);
		xml_SetItemValue(doc, LAUNCH_CONNECT_KEY, conf->lunchConnect);
		bModify = true;
	}*/

	if(!xml_GetItemValue(doc, AUTO_START_KEY, conf->autoStart, sizeof(conf->autoStart)))
	{
		memset(conf->autoStart, 0, sizeof(conf->autoStart));
	}
	if(strlen(conf->autoStart)<=0)
	{
		strncpy(conf->autoStart, "True", strlen("True")+1);
		xml_SetItemValue(doc, AUTO_START_KEY, conf->autoStart);
		bModify = true;
	}

	/*if(!xml_GetItemValue(doc, IOT_AUTO_REPORT, conf->autoReport, sizeof(conf->autoReport)))
	{
		memset(conf->autoReport, 0, sizeof(conf->autoReport));
	}
	if(strlen(conf->autoReport)<=0)
	{
		strncpy(conf->autoReport, "True", strlen("True")+1);
		xml_SetItemValue(doc, IOT_AUTO_REPORT, conf->autoReport);
		bModify = true;
	}*/

	if(!xml_GetItemValue(doc, SERVER_IP_KEY, conf->serverIP, sizeof(conf->serverIP)))
		memset(conf->serverIP, 0, sizeof(conf->serverIP));

	if(!xml_GetItemValue(doc, SERVER_PORT_KEY, conf->serverPort, sizeof(conf->serverPort)))
	{
		memset(conf->serverPort, 0, sizeof(conf->serverPort));
	}
	if(strlen(conf->serverPort)<=0)
	{
		strncpy(conf->serverPort, "10001", strlen("10001")+1);
		xml_SetItemValue(doc, SERVER_PORT_KEY, conf->serverPort);
		bModify = true;
	}

	if(!xml_GetItemValue(doc, SERVER_AUTH_KEY, conf->serverAuth, sizeof(conf->serverAuth)))
	{
		memset(conf->serverAuth, 0, sizeof(conf->serverAuth));
	}
	if(strlen(conf->serverAuth)<=0)
	{
		strncpy(conf->serverAuth, "F0PE1/aaU8o=", strlen("F0PE1/aaU8o=")+1);
		xml_SetItemValue(doc, SERVER_AUTH_KEY, conf->serverAuth);
		bModify = true;
	}

	if(!xml_GetItemValue(doc, SERVER_TLS_TYPE, temp, sizeof(temp)))
	{
		conf->tlstype = 0;
	}
	else
	{
		conf->tlstype = atoi(temp);
	}
	if(strlen(temp)<=0)
	{
		sprintf(temp, "%d", conf->tlstype);
		xml_SetItemValue(doc,SERVER_TLS_TYPE, temp);
		bModify = true;
	}

	if(!xml_GetItemValue(doc, SERVER_TLS_CAFILE, conf->cafile, sizeof(conf->cafile)))
	{
		memset(conf->cafile, 0, sizeof(conf->cafile));
	}
	
	if(!xml_GetItemValue(doc, SERVER_TLS_CAPATH, conf->capath, sizeof(conf->capath)))
	{
		memset(conf->capath, 0, sizeof(conf->capath));
	}

	if(!xml_GetItemValue(doc, SERVER_TLS_CERTFILE, conf->certfile, sizeof(conf->certfile)))
	{
		memset(conf->certfile, 0, sizeof(conf->certfile));
	}

	if(!xml_GetItemValue(doc, SERVER_TLS_KEYFILE, conf->keyfile, sizeof(conf->keyfile)))
	{
		memset(conf->keyfile, 0, sizeof(conf->keyfile));
	}

	if(!xml_GetItemValue(doc, SERVER_TLS_CERTPASS, conf->cerpasswd, sizeof(conf->cerpasswd)))
	{
		memset(conf->cerpasswd, 0, sizeof(conf->cerpasswd));
	}

	if(!xml_GetItemValue(doc, SERVER_TLS_PSK, conf->psk, sizeof(conf->psk)))
	{
		memset(conf->psk, 0, sizeof(conf->psk));
	}

	if(!xml_GetItemValue(doc, SERVER_PSK_IDENTIFY, conf->identity, sizeof(conf->identity)))
	{
		memset(conf->identity, 0, sizeof(conf->identity));
	}

	if(!xml_GetItemValue(doc, SERVER_PSK_CIPHERS, conf->ciphers, sizeof(conf->ciphers)))
	{
		memset(conf->ciphers, 0, sizeof(conf->ciphers));
	}

	/*if(!xml_GetItemValue(doc, USER_NAME_KEY, conf->loginID, sizeof(conf->loginID)))
	{
		memset(conf->loginID, 0, sizeof(conf->loginID));
	}
	if(strlen(conf->loginID)<=0)
	{
		strncpy(conf->loginID, "admin", strlen("admin")+1);
		xml_SetItemValue(doc,USER_NAME_KEY, conf->loginID);
		bModify = true;
	}

	if(!xml_GetItemValue(doc, USER_PASSWORD_KEY, conf->loginPwd, sizeof(conf->loginPwd)))
	{
		memset(conf->loginPwd, 0, sizeof(conf->loginPwd));
	}
	if(strlen(conf->loginPwd)<=0)
	{
		strncpy(conf->loginPwd, "GP25qY7TjJA=", strlen("GP25qY7TjJA=")+1);
		xml_SetItemValue(doc,USER_PASSWORD_KEY, conf->loginPwd);
		bModify = true;
	}*/

	if(bModify)
	{
		xml_SaveFile(configFile, doc);
	}
	iRet = true;
	xml_FreeDoc(doc);
	return iRet;
}

int SACONFIG_API cfg_save(char const * configFile, susiaccess_agent_conf_body_t const * const conf)
{
	int iRet = false;
	xml_doc_info * doc = NULL;
	char temp[256] = {0};
	if(configFile == NULL) 
		return iRet;
	if(conf == NULL)
		return iRet;
	
	doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	if(doc == NULL)
	{
		doc = xml_CreateDoc( XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	}

	if(xml_SetItemValue(doc, RUN_MODE_KEY, conf->runMode)<0)
		goto SAVE_EXIT;

	//if(xml_SetItemValue(doc, LAUNCH_CONNECT_KEY, conf->lunchConnect)<0)
	//	goto SAVE_EXIT;

	if(xml_SetItemValue(doc, AUTO_START_KEY, conf->autoStart)<0)
		goto SAVE_EXIT;

	
	/*if(xml_SetItemValue(doc, IOT_AUTO_REPORT, conf->autoReport)<0)
		goto SAVE_EXIT;*/

	if(xml_SetItemValue(doc, SERVER_IP_KEY, conf->serverIP)<0)
		goto SAVE_EXIT;

	if(xml_SetItemValue(doc, SERVER_PORT_KEY, conf->serverPort)<0)
		goto SAVE_EXIT;

	if(xml_SetItemValue(doc,SERVER_AUTH_KEY, conf->serverAuth)<0)
		goto SAVE_EXIT;

	sprintf(temp, "%d", conf->tlstype);
	if(xml_SetItemValue(doc,SERVER_TLS_TYPE, temp)<0)
		goto SAVE_EXIT;

	if(xml_SetItemValue(doc, SERVER_TLS_CAFILE, conf->cafile)<0)
		goto SAVE_EXIT;
	
	if(xml_SetItemValue(doc, SERVER_TLS_CAPATH, conf->capath)<0)
		goto SAVE_EXIT;

	if(xml_SetItemValue(doc, SERVER_TLS_CERTFILE, conf->certfile)<0)
		goto SAVE_EXIT;

	if(xml_SetItemValue(doc, SERVER_TLS_KEYFILE, conf->keyfile)<0)
		goto SAVE_EXIT;

	if(xml_SetItemValue(doc, SERVER_TLS_CERTPASS, conf->cerpasswd)<0)
		goto SAVE_EXIT;

	if(xml_SetItemValue(doc, SERVER_TLS_PSK, conf->psk)<0)
		goto SAVE_EXIT;

	if(xml_SetItemValue(doc, SERVER_PSK_IDENTIFY, conf->identity)<0)
		goto SAVE_EXIT;

	if(xml_SetItemValue(doc, SERVER_PSK_CIPHERS, conf->ciphers)<0)
		goto SAVE_EXIT;

	/*if(xml_SetItemValue(doc,USER_NAME_KEY, conf->loginID)<0)
		goto SAVE_EXIT;

	if(xml_SetItemValue(doc,USER_PASSWORD_KEY, conf->loginPwd)<0)
		goto SAVE_EXIT;*/

	xml_SaveFile(configFile, doc);
	iRet = true;
SAVE_EXIT:
	xml_FreeDoc(doc);
	return iRet;
}

int SACONFIG_API cfg_create(char const * configFile, susiaccess_agent_conf_body_t * conf)
{
	int iRet = false;
	xml_doc_info * doc = NULL;
	char temp[256] = {0};

	if(configFile == NULL) 
		return iRet;
	if(conf == NULL)
		return iRet;

	memset(conf, 0, sizeof(susiaccess_agent_conf_body_t));

	doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	if(doc == NULL)
	{
		doc = xml_CreateDoc( XML_CONFIG_ROOT, XML_CONFIG_BASIC);

		strncpy(conf->runMode, DEF_STANDALONE_RUN_MODE, strlen(DEF_STANDALONE_RUN_MODE)+1);
		if(xml_SetItemValue(doc, RUN_MODE_KEY, conf->runMode)<0)
			goto CREATE_EXIT;

		/*strncpy(conf->lunchConnect, "False", strlen("False")+1);
		if(xml_SetItemValue(doc, LAUNCH_CONNECT_KEY, conf->lunchConnect)<0)
			goto CREATE_EXIT;*/

		strncpy(conf->autoStart, "True", strlen("True")+1);
		if(xml_SetItemValue(doc, AUTO_START_KEY, conf->autoStart)<0)
			goto CREATE_EXIT;

		/*strncpy(conf->autoReport, "True", strlen("True")+1);
		if(xml_SetItemValue(doc, IOT_AUTO_REPORT, conf->autoReport)<0)
			goto CREATE_EXIT;*/

		strncpy(conf->serverIP, "127.0.0.1", strlen("127.0.0.1")+1);
		if(xml_SetItemValue(doc, SERVER_IP_KEY, conf->serverIP)<0)
			goto CREATE_EXIT;

		strncpy(conf->serverPort, "10001", strlen("10001")+1);
		if(xml_SetItemValue(doc, SERVER_PORT_KEY, conf->serverPort)<0)
			goto CREATE_EXIT;

		strncpy(conf->serverAuth, "F0PE1/aaU8o=", strlen("F0PE1/aaU8o=")+1);
		if(xml_SetItemValue(doc,SERVER_AUTH_KEY, conf->serverAuth)<0)
			goto CREATE_EXIT;

		conf->tlstype = 0;
		sprintf(temp, "%d", conf->tlstype);
		if(xml_SetItemValue(doc,SERVER_TLS_TYPE, temp)<0)
			goto CREATE_EXIT;

		/*strncpy(conf->loginID, "ral", strlen("ral")+1);
		if(xml_SetItemValue(doc,USER_NAME_KEY, conf->loginID)<0)
			goto CREATE_EXIT;

		strncpy(conf->loginPwd, "123", strlen("123")+1);
		if(xml_SetItemValue(doc,USER_PASSWORD_KEY, conf->loginPwd)<0)
			goto CREATE_EXIT;*/

		xml_SaveFile(configFile, doc);
		iRet = true;
CREATE_EXIT:
		xml_FreeDoc(doc);
		return iRet;
	}
	else
	{
		xml_FreeDoc(doc);
		return cfg_load(configFile, conf);
	}
}

int SACONFIG_API cfg_get(char const * const configFile, char const * const itemName, char * itemValue, int valueLen)
{
	int iRet = false;
	if(NULL == configFile || NULL == itemName || NULL == itemValue) return iRet;
	{
		xml_doc_info * doc = NULL;
		doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
		if(doc)
		{
			iRet = xml_GetItemValue(doc, itemName, itemValue, valueLen);
			xml_FreeDoc(doc);
		}
	}
	return iRet;
}

int SACONFIG_API cfg_set(char const * const configFile, char const * const itemName, char const * const itemValue)
{
	int iRet = false;
   if(NULL == configFile || NULL == itemName || NULL == itemValue) return iRet;
	{
		xml_doc_info * doc = NULL;
		doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
		if(doc)
		{
			if(xml_SetItemValue(doc, itemName, itemValue)<0)
				return iRet;

			xml_SaveFile(configFile, doc);
			xml_FreeDoc(doc);
			iRet = true;
		}
	}
	return iRet;
}