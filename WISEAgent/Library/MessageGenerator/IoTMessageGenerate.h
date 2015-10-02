#ifndef _IOT_MESSAGE_GENERATE_H_
#define _IOT_MESSAGE_GENERATE_H_
#include "MsgGenerator.h"

#ifdef __cplusplus
extern "C" {
#endif

	MSG_CLASSIFY_T* IoT_CreateRoot(char* handlerName);
	MSG_CLASSIFY_T* IoT_AddGroup(MSG_CLASSIFY_T* pNode, char* groupName);
	MSG_ATTRIBUTE_T* IoT_AddGroupAttribute(MSG_CLASSIFY_T* pNode, char* attrName);
	MSG_ATTRIBUTE_T* IoT_AddSensorNode(MSG_CLASSIFY_T* pNode, char* senName);
	
	bool IoT_SetFloatValue(MSG_ATTRIBUTE_T* attr, float value, char* readwritemode, char *unit);
	bool IoT_SetFloatValueWithMaxMin(MSG_ATTRIBUTE_T* attr, float value, char* readwritemode, float max, float min, char *unit);
	bool IoT_SetBoolValue(MSG_ATTRIBUTE_T* attr, bool bvalue, char* readwritemode);
	bool IoT_SetStringValue(MSG_ATTRIBUTE_T* attr, char *svalue, char* readwritemode);

	MSG_CLASSIFY_T* IoT_FindGroup(MSG_CLASSIFY_T* pParentNode, char* groupName);
	MSG_ATTRIBUTE_T* IoT_FindGroupAttribute(MSG_CLASSIFY_T* pNode, char* attrName);
	MSG_ATTRIBUTE_T* IoT_FindSensorNode(MSG_CLASSIFY_T* pNode, char* senName);

	bool IoT_DelGroup(MSG_CLASSIFY_T* pParentNode, char* groupName);
	bool IoT_DelGroupAttribute(MSG_CLASSIFY_T* pNode, char* attrName);
	bool IoT_DelSensorNode(MSG_CLASSIFY_T* pNode, char* senName);
	void IoT_ReleaseAll(MSG_CLASSIFY_T* pRoot);

	char  *IoT_PrintCapability(MSG_CLASSIFY_T* pRoot);
	char *IoT_PrintData(MSG_CLASSIFY_T* pRoot);

#ifdef __cplusplus
}
#endif
#endif