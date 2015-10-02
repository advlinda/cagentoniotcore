#include <libxml/xpath.h>
#include <libxml/parser.h>
#include "moduleconfig.h"
#include "platform.h"
#include "common.h"

#define XML_CONFIG_ROOT "XMLConfigSettings"
#define XML_CONFIG_BASIC "BaseSettings"

xmlXPathObjectPtr module_GetNodeSet(xmlDocPtr doc, const xmlChar *pXpath) 
{
	xmlXPathContextPtr context = NULL;	    
	xmlXPathObjectPtr xpRet = NULL;		
	if(doc == NULL || pXpath == NULL) return xpRet;
	{
		context = xmlXPathNewContext(doc);		
		if (context != NULL) 
		{	
			xpRet = xmlXPathEvalExpression(pXpath, context); 
			xmlXPathFreeContext(context);				
			if (xpRet != NULL) 
			{
				if (xmlXPathNodeSetIsEmpty(xpRet->nodesetval))   
				{
					xmlXPathFreeObject(xpRet);
					xpRet = NULL;
				}
			}
		}
	}

	return xpRet;	
}

xmlDocPtr module_Loadfile(char const * configFile)
{
	xmlDocPtr doc = NULL;

	if(configFile == NULL) 
		return doc;

	if(!app_os_is_file_exist(configFile))
		return doc;

	xmlInitParser();
	doc = xmlReadFile(configFile, "UTF-8", 0);
	if(doc == NULL)
		return doc;

	return doc;
}

int module_GetItemValue(xmlDocPtr doc, char const * const itemName, char * itemValue, int valueLen)
{
	int iRet = false;
	if(NULL == doc || NULL == itemName || NULL == itemValue) return iRet;
	{
		xmlChar * pXPath = NULL;
		xmlXPathObjectPtr xpRet = NULL;
		xmlChar *nodeValue = NULL;
		xmlNodePtr curNode = NULL;
		char xPathStr[128] = {0};

		sprintf_s(xPathStr, sizeof(xPathStr), "/%s/%s/%s",XML_CONFIG_ROOT, XML_CONFIG_BASIC, itemName);
		pXPath = BAD_CAST(xPathStr);

		xpRet = module_GetNodeSet(doc, pXPath);
		if(xpRet) 
		{
			int i = 0;
			xmlNodeSetPtr nodeset = xpRet->nodesetval;
			for (i = 0; i < nodeset->nodeNr; i++) 
			{
				curNode = nodeset->nodeTab[i];    
				if(curNode != NULL) 
				{
					nodeValue = xmlNodeGetContent(curNode);
					if (nodeValue != NULL) 
					{
						if(xmlStrlen(nodeValue) < valueLen-1)
						{
							strcpy(itemValue, (char*)nodeValue);
							iRet = strlen(itemValue);
						}
						xmlFree(nodeValue);
						break;
					}
				}
			}
			xmlXPathFreeObject(xpRet);
		}
	}
	return iRet;
}

void module_FreeDoc(xmlDocPtr doc)
{
	if(doc == NULL) 
		return;

	xmlFreeDoc(doc);
	xmlCleanupParser();
}

int module_get(char const * const configFile, char const * const itemName, char * itemValue, int valueLen)
{
	int iRet = false;
	if(NULL == configFile || NULL == itemName || NULL == itemValue) return iRet;
	{
		xmlDocPtr doc = NULL;
		doc = module_Loadfile(configFile);
		if(doc)
		{
			iRet = module_GetItemValue(doc, itemName, itemValue, valueLen);
			module_FreeDoc(doc);
		}
	}
	return iRet;
}