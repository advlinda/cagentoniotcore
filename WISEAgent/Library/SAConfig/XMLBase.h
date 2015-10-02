#ifndef _CAGENT_XMLBASE_H_
#define _CAGENT_XMLBASE_H_

#include <libxml/xpath.h>
#include <libxml/parser.h>
#include "platform.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once

#ifndef SACONFIG_API
#define SACONFIG_API WINAPI
#endif
#else
#define SACONFIG_API
#endif

typedef struct XML_DOC_INFO
{	
	bool isInitParser;
	xmlDocPtr doc;
	char xml_root[MAX_PATH];
	char xml_base[MAX_PATH];
}xml_doc_info;

#ifdef __cplusplus
extern "C" {
#endif

xml_doc_info * SACONFIG_API xml_Loadfile(char const * configFile, char const * const xml_root, char const * const xml_base);
xml_doc_info * SACONFIG_API xml_CreateDoc(char const * const xml_root, char const * const xml_base);
	 int SACONFIG_API xml_SaveFile(char const * configFile, xml_doc_info * doc);
	void SACONFIG_API xml_FreeDoc(xml_doc_info * doc);
	 int SACONFIG_API xml_GetItemValue(xml_doc_info const * doc, char const * const itemName, char * itemValue, int valueLen);
	 int SACONFIG_API xml_SetItemValue(xml_doc_info const * doc, char const * const itemName, char const * const itemValue);
 
#ifdef __cplusplus
}
#endif

#endif