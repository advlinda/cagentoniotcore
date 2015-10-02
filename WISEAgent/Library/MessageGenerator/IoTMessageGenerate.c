#include "IoTMessageGenerate.h"

#pragma region Add_Resource
MSG_CLASSIFY_T* IoT_CreateRoot(char* handlerName)
{
	MSG_CLASSIFY_T *pRoot = MSG_CreateRoot();
	MSG_AddJSONClassify(pRoot, handlerName, NULL, false);
	return pRoot;
}

MSG_CLASSIFY_T* IoT_AddGroup(MSG_CLASSIFY_T* pNode, char* groupName)
{
	MSG_CLASSIFY_T *pCurNode = NULL;
	if(pNode)
	{
		if(pNode->type == class_type_root)
			pNode = pNode->sub_list;

		pCurNode = MSG_FindClassify(pNode, groupName);
		if(!pCurNode)
			pCurNode = MSG_AddIoTClassify(pNode, groupName, NULL, false);
	}
	return pCurNode;
}

MSG_ATTRIBUTE_T* IoT_AddGroupAttribute(MSG_CLASSIFY_T* pNode, char* attrName)
{
	MSG_ATTRIBUTE_T *attr = NULL;
	if(pNode)
	{
		if(pNode->type == class_type_root)
			pNode = pNode->sub_list;

		attr = MSG_FindJSONAttribute(pNode, attrName);
		if(!attr)
			attr = MSG_AddJSONAttribute(pNode, attrName);
	}
	return attr;
}

MSG_ATTRIBUTE_T* IoT_AddSensorNode(MSG_CLASSIFY_T* pNode, char* senName)
{
	MSG_ATTRIBUTE_T *attr = NULL;
	if(pNode)
	{
		if(pNode->type == class_type_root)
			pNode = pNode->sub_list;

		attr = MSG_FindIoTSensor(pNode, senName);
		if(!attr)
			attr = MSG_AddIoTSensor(pNode, senName);
	}
	return attr;
}
	
bool IoT_SetFloatValue(MSG_ATTRIBUTE_T* attr, float value, char* readwritemode, char *unit)
{
	return MSG_SetFloatValue(attr, value, readwritemode, unit);
}

bool IoT_SetFloatValueWithMaxMin(MSG_ATTRIBUTE_T* attr, float value, char* readwritemode, float max, float min, char *unit)
{
	return MSG_SetFloatValueWithMaxMin(attr, value, readwritemode, max, min, unit);
}

bool IoT_SetBoolValue(MSG_ATTRIBUTE_T* attr, bool bvalue, char* readwritemode)
{
	return MSG_SetBoolValue(attr, bvalue,readwritemode);
}

bool IoT_SetStringValue(MSG_ATTRIBUTE_T* attr, char *svalue, char* readwritemode)
{
	return MSG_SetStringValue(attr, svalue, readwritemode);
}
#pragma endregion Add_Resource

#pragma region Release_Resource
bool IoT_DelGroup(MSG_CLASSIFY_T* pParentNode, char* groupName)
{
	return MSG_DelClassify(pParentNode, groupName);
}

bool IoT_DelGroupAttribute(MSG_CLASSIFY_T* pNode, char* attrName)
{
	return MSG_DelJSONAttribute(pNode, attrName);
}

bool IoT_DelSensorNode(MSG_CLASSIFY_T* pNode, char* senName)
{
	return MSG_DelIoTSensor(pNode, senName);
}

void IoT_ReleaseAll(MSG_CLASSIFY_T* pNode)
{
	MSG_ReleaseRoot(pNode);
}
#pragma endregion Release_Resource

#pragma region Find_Resource
MSG_CLASSIFY_T* IoT_FindGroup(MSG_CLASSIFY_T* pParentNode, char* groupName)
{
	MSG_CLASSIFY_T* pTargetNode = NULL;
	if(pParentNode)
	{
		if(pParentNode->type == class_type_root)
			pParentNode = pParentNode->sub_list;
		pTargetNode = MSG_FindClassify(pParentNode, groupName);
	}
	return pTargetNode;
}

MSG_ATTRIBUTE_T* IoT_FindGroupAttribute(MSG_CLASSIFY_T* pNode, char* attrName)
{
	MSG_ATTRIBUTE_T* pTargetAttr = NULL;
	if(pNode)
	{
		if(pNode->type == class_type_root)
			pNode = pNode->sub_list;
		pTargetAttr = MSG_FindJSONAttribute(pNode, attrName);
	}
	return pTargetAttr;
}

MSG_ATTRIBUTE_T* IoT_FindSensorNode(MSG_CLASSIFY_T* pNode, char* senName)
{
	MSG_ATTRIBUTE_T* pTargetAttr = NULL;
	if(pNode)
	{
		if(pNode->type == class_type_root)
			pNode = pNode->sub_list;
		pTargetAttr = MSG_FindIoTSensor(pNode, senName);
	}
	return pTargetAttr;
}

#pragma endregion Find_Resource

#pragma region Generate_JSON 
char *IoT_PrintCapability(MSG_CLASSIFY_T* pRoot)
{
	return MSG_PrintUnformatted(pRoot);
}

char *IoT_PrintData(MSG_CLASSIFY_T* pRoot)
{
	int size = 6;
	char* filter[] ={"n", "bn", "v","sv","bv","id"};
	return MSG_PrintWithFiltered(pRoot, filter, size);
}
#pragma endregion Generate_JSON 