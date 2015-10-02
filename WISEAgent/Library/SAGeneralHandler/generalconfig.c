#include <XMLBase.h>
#include "generalconfig.h"

#define XML_CONFIG_ROOT "XMLConfigSettings"
#define XML_CONFIG_BASIC "General"


#define AGENTINFO_AUTOREPORT	"autoReport"
#define AGENTINFO_REPORTDATALEN	"reportDataLength"
#define AGENTINFO_REPORTDATA	"reportData"

int general_load(char const * configFile, susiaccess_general_conf_body_t * conf)
{
	int iRet = false;
	xml_doc_info * doc = NULL;
	bool bModify = false;
	char length[64] = {0};

	if(configFile == NULL) 
		return iRet;
	if(conf == NULL)
		return iRet;
	
	doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	if(doc == NULL)
		return iRet;

	memset(conf, 0, sizeof(susiaccess_general_conf_body_t));

	if(!xml_GetItemValue(doc, AGENTINFO_AUTOREPORT, conf->autoReportEn, sizeof(conf->autoReportEn)))
		memset(conf->autoReportEn, 0, sizeof(conf->autoReportEn));
	if(strlen(conf->autoReportEn)<=0)
	{
		strncpy(conf->autoReportEn, "False", strlen("False")+1);
		xml_SetItemValue(doc, AGENTINFO_AUTOREPORT, conf->autoReportEn);
		bModify = true;
	}

	if(!xml_GetItemValue(doc, AGENTINFO_REPORTDATALEN, length, sizeof(length)))
		memset(length, 0, sizeof(length));
	if(strlen(length)<=0)
	{
		conf->reportDataLength = 0;
		sprintf(length, "%d", conf->reportDataLength);
		xml_SetItemValue(doc, AGENTINFO_REPORTDATALEN, length);
		bModify = true;
	}
	else
	{
		conf->reportDataLength = atoi(length);
	}

	if(conf->reportDataLength>0)
	{
		if(conf->reportData)
			free(conf->reportData);
		conf->reportData = malloc(conf->reportDataLength+1);
		memset(conf->reportData, 0, conf->reportDataLength+1);

		if(!xml_GetItemValue(doc, AGENTINFO_REPORTDATA, conf->reportData, conf->reportDataLength))
			memset(conf->reportData, 0, conf->reportDataLength);
		if(strlen(conf->reportData)<=0)
		{
			memset(conf->reportData, 0, conf->reportDataLength);
			xml_SetItemValue(doc, AGENTINFO_REPORTDATA, "");
			bModify = true;
		}
	}
	

	if(bModify)
	{
		xml_SaveFile(configFile, doc);
	}
	iRet = true;
	xml_FreeDoc(doc);
	return iRet;
}

int general_save(char const * configFile, susiaccess_general_conf_body_t const * const conf)
{
	int iRet = false;
	xml_doc_info * doc = NULL;
	char length[64] = {0};
	
	if(configFile == NULL) 
		return iRet;
	if(conf == NULL)
		return iRet;
	
	doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	if(doc == NULL)
	{
		doc = xml_CreateDoc( XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	}

	if(xml_SetItemValue(doc,AGENTINFO_AUTOREPORT, conf->autoReportEn)<0)
		goto SAVE_EXIT;

	sprintf(length, "%d", conf->reportDataLength);
	if(xml_SetItemValue(doc,AGENTINFO_REPORTDATALEN, length)<0)
		goto SAVE_EXIT;

	if(conf->reportData)
	{
		if(xml_SetItemValue(doc,AGENTINFO_REPORTDATA, conf->reportData)<0)
			goto SAVE_EXIT;
	}
	
	xml_SaveFile(configFile, doc);
	iRet = true;
SAVE_EXIT:
	xml_FreeDoc(doc);
	return iRet;
}

int general_create(char const * configFile, susiaccess_general_conf_body_t * conf)
{
	int iRet = false;
	xml_doc_info * doc = NULL;

	if(configFile == NULL) 
		return iRet;
	if(conf == NULL)
		return iRet;

	memset(conf, 0, sizeof(susiaccess_general_conf_body_t));

	doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	if(doc == NULL)
	{
		doc = xml_CreateDoc( XML_CONFIG_ROOT, XML_CONFIG_BASIC);

		strncpy(conf->autoReportEn, "False", strlen("False")+1);
		if(xml_SetItemValue(doc,AGENTINFO_AUTOREPORT, conf->autoReportEn)<0)
			goto CREATE_EXIT;

		if(xml_SetItemValue(doc,AGENTINFO_REPORTDATALEN, "0")<0)
			goto CREATE_EXIT;

		if(xml_SetItemValue(doc,AGENTINFO_REPORTDATA, "")<0)
			goto CREATE_EXIT;

		xml_SaveFile(configFile, doc);
		iRet = true;
CREATE_EXIT:
		xml_FreeDoc(doc);
		return iRet;
	}
	else
	{
		xml_FreeDoc(doc);
		return general_load(configFile, conf);
	}
}

int general_get(char const * const configFile, char const * const itemName, char * itemValue, int valueLen)
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

int general_set(char const * const configFile, char const * const itemName, char const * const itemValue)
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