#include "XMLBase.h"
#include "common.h"
xmlXPathObjectPtr static xml_GetNodeSet(xmlDocPtr doc, const xmlChar *pXpath) 
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

xml_doc_info * SACONFIG_API xml_Loadfile(char const * configFile, char const * const xml_root, char const * const xml_base)
{
	xml_doc_info * docinfo = NULL;
	xmlDocPtr doc = NULL;
	int count = 10;
	if(configFile == NULL) 
		return docinfo;

	if(!app_os_is_file_exist(configFile))
		return docinfo;

	xmlInitParser();

	while(!app_os_file_access(configFile, R_OK))
	{
		if(count <= 0)
			return docinfo;
		count--;
		app_os_sleep(500);
	}

	doc = xmlReadFile(configFile, "UTF-8", 0);
	if(doc == NULL)
		return docinfo;


	docinfo = (xml_doc_info *)malloc(sizeof(xml_doc_info));
	memset(docinfo, 0, sizeof(xml_doc_info));
	docinfo->isInitParser = true;
	docinfo->doc = doc;
	strcpy(docinfo->xml_root, xml_root);
	strcpy(docinfo->xml_base, xml_base);
	return docinfo;
}

xml_doc_info * SACONFIG_API xml_CreateDoc(char const * const xml_root, char const * const xml_base)
{
	xml_doc_info * docinfo = NULL;
	xmlDocPtr doc = NULL;
	xmlNodePtr curNode = NULL;
	xmlInitParser();
	doc = xmlNewDoc(BAD_CAST("1.0"));
	curNode = xmlNewNode(NULL, BAD_CAST(xml_root));
	xmlDocSetRootElement(doc, curNode);
	curNode = xmlNewChild(curNode, NULL, BAD_CAST(xml_base), NULL);

	docinfo = (xml_doc_info *)malloc(sizeof(xml_doc_info));
	memset(docinfo, 0, sizeof(xml_doc_info));
	docinfo->isInitParser = true;
	docinfo->doc = doc;
	strcpy(docinfo->xml_root, xml_root);
	strcpy(docinfo->xml_base, xml_base);
	return docinfo;
}

int SACONFIG_API xml_SaveFile(char const * configFile, xml_doc_info * doc)
{
	int iRet = false;
	int count = 10;
	if(configFile == NULL) 
		return iRet;

	if(doc == NULL)
		return iRet;

	while(!app_os_file_access(configFile, W_OK))
	{
		if(count <= 0)
			return iRet;
		count--;
		app_os_sleep(500);
	}

	iRet = xmlSaveFile(configFile, doc->doc);

	return iRet;
}

void SACONFIG_API xml_FreeDoc(xml_doc_info * doc)
{
	if(doc == NULL) 
		return;

	xmlFreeDoc(doc->doc);
	if(doc->isInitParser)
		xmlCleanupParser();
	doc->doc = NULL;
	free(doc);
	doc = NULL;
}

int SACONFIG_API xml_GetItemValue(xml_doc_info const * doc, char const * const itemName, char * itemValue, int valueLen)
{
	int iRet = false;
	if(NULL == doc || NULL == itemName || NULL == itemValue) return iRet;
	{
		xmlChar * pXPath = NULL;
		xmlXPathObjectPtr xpRet = NULL;
		xmlChar *nodeValue = NULL;
		xmlNodePtr curNode = NULL;
		char xPathStr[128] = {0};

		sprintf_s(xPathStr, sizeof(xPathStr), "/%s/%s/%s",doc->xml_root, doc->xml_base, itemName);
		pXPath = BAD_CAST(xPathStr);

		xpRet = xml_GetNodeSet(doc->doc, pXPath);
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
						if(xmlStrlen(nodeValue) <= valueLen)
						{
							strcpy(itemValue, (char*)nodeValue);
							iRet = strlen(itemValue);
						}
						else
						{
							strncpy(itemValue, (char*)nodeValue, valueLen);
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

int SACONFIG_API xml_SetItemValue(xml_doc_info const * doc, char const * const itemName, char const * const itemValue)
{
	int iRet = false;
	
	if(NULL == doc || NULL == itemName || NULL == itemValue) return iRet;
	{
		xmlChar * pXPath = NULL;
		xmlXPathObjectPtr xpRet = NULL;
		xmlNodePtr curNode = NULL;
		xmlNodePtr root = NULL;
		char xPathStr[128] = {0};

		//if(strlen(itemValue))
		{
			sprintf_s(xPathStr, sizeof(xPathStr), "/%s/%s/%s",doc->xml_root, doc->xml_base, itemName);
			pXPath = BAD_CAST(xPathStr);
			xpRet = xml_GetNodeSet(doc->doc, pXPath);
			if(xpRet) 
			{
				int i = 0;
				xmlNodeSetPtr nodeset = xpRet->nodesetval;
				for (i = 0; i < nodeset->nodeNr; i++) 
				{
					curNode = nodeset->nodeTab[i];    
					if(curNode != NULL) 
					{
						xmlNodeSetContent(curNode, BAD_CAST(itemValue));
						iRet = true;
						break;
					}
				}
				xmlXPathFreeObject (xpRet);
			}
			else
			{
				memset(xPathStr, 0, sizeof(xPathStr));
				sprintf_s(xPathStr, sizeof(xPathStr), "/%s/%s",doc->xml_root, doc->xml_base);
				pXPath = BAD_CAST(xPathStr);
				xpRet = xml_GetNodeSet(doc->doc, pXPath);
				if(xpRet)
				{
					int i = 0;
					xmlNodeSetPtr nodeset = xpRet->nodesetval;
					for (i = 0; i < nodeset->nodeNr; i++) 
					{
						curNode = nodeset->nodeTab[i];    
						if(curNode != NULL) 
						{
							xmlNewTextChild(curNode, NULL, BAD_CAST(itemName), BAD_CAST(itemValue));
							iRet = true;
							break;
						}
					}
					xmlXPathFreeObject (xpRet);
				}
				else
				{
					memset(xPathStr, 0, sizeof(xPathStr));
					sprintf_s(xPathStr, sizeof(xPathStr), "/%s",doc->xml_root);
					pXPath = BAD_CAST(xPathStr);
					xpRet = xml_GetNodeSet(doc->doc, pXPath);
					if(xpRet)
					{
						int i = 0;
						xmlNodeSetPtr nodeset = xpRet->nodesetval;
						for (i = 0; i < nodeset->nodeNr; i++) 
						{
							curNode = nodeset->nodeTab[i];    
							if(curNode != NULL) 
							{
								xmlNodePtr tNode = xmlNewChild(curNode, NULL, BAD_CAST(doc->xml_base), NULL);
								xmlNewTextChild(tNode, NULL, BAD_CAST(itemName), BAD_CAST(itemValue));
								iRet = true;
								break;
							}
						}
						xmlXPathFreeObject (xpRet);
					}
				}
			}
		}
	}
	return iRet;
}
