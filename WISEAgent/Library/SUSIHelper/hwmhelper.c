#include "platform.h"
#include "common.h"
#include "hwmhelper.h"
#include "HWM3PartyHelper.h"
#include <Susi4.h>
#include "OsDeclarations.h"

//----------------------------------Susi lib data define---------------------------------------
typedef enum SUSILIBTYPE
{
	SA_LIB_UNKNOWN,
	SA_LIB_3,
	SA_LIB_4,
	SA_LIB_3PTY,
}SUSILIBTYPE;

SUSILIBTYPE   SusiLibType = SA_LIB_UNKNOWN;
#ifdef WIN32
#define DEF_SUSI4_LIB_NAME    "Susi4.dll"
#else
#define DEF_SUSI4_LIB_NAME    "libSUSI-4.00.so"
#endif
typedef SusiStatus_t (SUSI_API *PSusiLibInitialize)();
typedef SusiStatus_t (SUSI_API *PSusiLibUninitialize)();
typedef SusiStatus_t (SUSI_API *PSusiBoardGetValue)(SusiId_t Id, uint32_t *pValue);
typedef SusiStatus_t (SUSI_API *PSusiBoardGetStringA)(SusiId_t Id, char *pBuffer, uint32_t *pBufLen);
void * hSUSI4Dll = NULL;
PSusiLibInitialize pSusiLibInitialize = NULL;
PSusiLibUninitialize pSusiLibUninitialize = NULL;
PSusiBoardGetValue pSusiBoardGetValue = NULL;
PSusiBoardGetStringA pSusiBoardGetStringA = NULL;

#ifdef WIN32
#define DEF_SUSI3_LIB_NAME    "Susi.dll"
#else
#define DEF_SUSI3_LIB_NAME    "libSUSI-3.02.so"
#endif
typedef int (*PSusiDllInit)();
typedef int (*PSusiDllUnInit)();
typedef void (*PSusiDllGetVersion)(unsigned long *major, unsigned long *minor);
typedef int (*PSusiHWMAvailable)();
typedef int (*PSusiHWMGetFanSpeed)(unsigned short fanType, unsigned short *retval, unsigned short *typeSupport);
typedef int (*PSusiHWMGetTemperature)(unsigned short tempType, float *retval, unsigned short *typeSupport);
typedef int (*PSusiHWMGetVoltage)(unsigned long voltType, float *retval, unsigned long *typeSupport);
typedef int  (*PSusiCoreGetBIOSVersion)(char* BIOSVersion, unsigned long* size);
typedef int  (*PSusiCoreGetPlatformName)(char* PlatformName, unsigned long* size);

void * hSUSI3Dll = NULL;
PSusiDllInit pSusiDllInit = NULL;
PSusiDllUnInit pSusiDllUnInit = NULL;
PSusiDllGetVersion pSusiDllGetVersion = NULL;
PSusiHWMAvailable pSusiHWMAvailable = NULL;
PSusiHWMGetTemperature pSusiHWMGetTemperature = NULL;
PSusiHWMGetFanSpeed  pSusiHWMGetFanSpeed = NULL;
PSusiHWMGetVoltage pSusiHWMGetVoltage = NULL;
PSusiCoreGetBIOSVersion pSusiCoreGetBIOSVersion = NULL;
PSusiCoreGetPlatformName pSusiCoreGetPlatformName = NULL;
//----------------------------------------------------------------------------------------------
static char SUSI3TempDefTagArray[16][DEF_HWMNAME_LENGTH] = {
	"TCPU",
	"TSYS",
	"TAUX",
	"TCPU2",
	"OEM0",
	"OEM1",
	"OEM2",
	"OEM3",
	"OEM4",
	"OEM5",
	"OEM6",
	"OEM7",
	"OEM8",
	"OEM9",
	"OEM10",
	"OEM11"
};

static char SUSI3VoltDefTagArray[32][DEF_HWMNAME_LENGTH] = {
	"VCORE",
	"TSYS",
	"TAUX",
	"TCPU2",
	"V25",
	"V33",
	"V50",
	"V120",
	"V5SB",
	"V3SB",
	"OEM6",
	"OEM7",
	"OEM8",
	"OEM9",
	"OEM10",
	"OEM11",
	"TCPU",
	"TSYS",
	"TAUX",
	"TCPU2",
	"VBAT",
	"VN50",
	"VN120",
	"VTT",
	"VCORE2",
	"V105",
	"V15",
	"V18",
	"V240",
	"OEM0",
	"OEM1",
	"OEM2"
};

static char SUSI3FanDefTagArray[16][DEF_HWMNAME_LENGTH] = {
	"FCPU",
	"FSYS",
	"F2ND",
	"FCPU2",
	"FAUX2",
	"OEM0",
	"OEM1",
	"OEM2",
	"OEM3",
	"OEM4",
	"OEM5",
	"OEM6",
	"OEM7",
	"OEM8",
	"OEM9",
	"OEM10"
};

static void hwm_GetSUSI3Function(void * hSUSI3DLL)
{
	if(hSUSI3Dll!=NULL)
	{
		pSusiDllInit = (PSusiDllInit)app_get_proc_address(hSUSI3Dll, "SusiDllInit");
		pSusiDllUnInit = (PSusiDllUnInit)app_get_proc_address(hSUSI3Dll, "SusiDllUnInit");
		pSusiHWMAvailable = (PSusiHWMAvailable)app_get_proc_address(hSUSI3Dll, "SusiHWMAvailable");
		pSusiHWMGetTemperature = (PSusiHWMGetTemperature)app_get_proc_address(hSUSI3Dll, "SusiHWMGetTemperature");
		pSusiHWMGetFanSpeed = (PSusiHWMGetFanSpeed)app_get_proc_address(hSUSI3Dll, "SusiHWMGetFanSpeed");
		pSusiHWMGetVoltage = (PSusiHWMGetVoltage)app_get_proc_address(hSUSI3Dll, "SusiHWMGetVoltage");
		pSusiCoreGetBIOSVersion = (PSusiCoreGetBIOSVersion)app_get_proc_address(hSUSI3Dll, "SusiCoreGetBIOSVersion");
		pSusiCoreGetPlatformName = (PSusiCoreGetPlatformName)app_get_proc_address(hSUSI3Dll, "SusiCoreGetPlatformName");
	}
}

static bool hwm_IsExistSUSI3Lib()
{
	bool bRet = false;
	void * hSUSI3 = NULL;
	hSUSI3 = app_load_library(DEF_SUSI3_LIB_NAME);
	if(hSUSI3 != NULL)
	{
		bRet = true;
		app_free_library(hSUSI3);
		hSUSI3 = NULL;
	}
	return bRet;
}

static bool hwm_StartupSUSI3Lib()
{
	bool bRet = false;
	hSUSI3Dll = app_load_library(DEF_SUSI3_LIB_NAME);
	if(hSUSI3Dll != NULL)
	{
		hwm_GetSUSI3Function(hSUSI3Dll);
		if(pSusiDllInit)
		{
			if(pSusiDllInit())
			{
				bRet = true;
			}
		}
	}
	return bRet;
}

static bool hwm_CleanupSUSI3Lib()
{
	bool bRet = false;
	if(pSusiDllUnInit)
	{
		if(pSusiDllUnInit())
		{
			bRet = true;
		}
	}
	if(hSUSI3Dll != NULL)
	{
		app_free_library(hSUSI3Dll);
		hSUSI3Dll = NULL;
		pSusiDllInit = NULL;
		pSusiDllUnInit = NULL;
		pSusiHWMGetTemperature = NULL;
		pSusiHWMGetFanSpeed = NULL;
		pSusiHWMGetVoltage = NULL;
	}
	return bRet;
}

static void hwm_GetSUSI4Function(void * hSUSI4DLL)
{
	if(hSUSI4Dll!=NULL)
	{
		pSusiLibInitialize = (PSusiLibInitialize)app_get_proc_address(hSUSI4Dll, "SusiLibInitialize");
		pSusiLibUninitialize = (PSusiLibUninitialize)app_get_proc_address(hSUSI4Dll, "SusiLibUninitialize");
		pSusiBoardGetValue = (PSusiBoardGetValue)app_get_proc_address(hSUSI4Dll, "SusiBoardGetValue");
		pSusiBoardGetStringA = (PSusiBoardGetStringA)app_get_proc_address(hSUSI4Dll, "SusiBoardGetStringA");
	}
}

static bool hwm_IsExistSUSI4Lib()
{
	bool bRet = false;
	void * hSUSI4 = NULL;
	hSUSI4 = app_load_library(DEF_SUSI4_LIB_NAME);
	if(hSUSI4 != NULL)
	{
		bRet = true;
		app_free_library(hSUSI4);
		hSUSI4 = NULL;
	}
	return bRet;
}

static bool hwm_StartupSUSI4Lib()
{
	bool bRet = false;
	hSUSI4Dll = app_load_library(DEF_SUSI4_LIB_NAME);
	if(hSUSI4Dll != NULL)
	{
		hwm_GetSUSI4Function(hSUSI4Dll);
		if(pSusiLibInitialize)
		{
			uint32_t iRet = pSusiLibInitialize();
			if(iRet != SUSI_STATUS_NOT_INITIALIZED)
			{
				bRet = true;
			}
		}
	}
	return bRet;
}

static bool hwm_CleanupSUSI4Lib()
{
	bool bRet = false;
	if(pSusiLibUninitialize)
	{
		uint32_t iRet = pSusiLibUninitialize();
		if(iRet == SUSI_STATUS_SUCCESS)
		{
			bRet = true;
		}
	}
	if(hSUSI4Dll != NULL)
	{
		app_free_library(hSUSI4Dll);
		hSUSI4Dll = NULL;
		pSusiLibInitialize = NULL;
		pSusiLibUninitialize = NULL;
		pSusiBoardGetValue = NULL;
		pSusiBoardGetStringA = NULL;
	}
	return bRet;
}

bool hwm_IsExistSUSILib()
{
	bool bRet = false;
	bRet = IsExistHWM3PartyLib();
	if(!bRet)
	{
		bRet = hwm_IsExistSUSI4Lib();
		if(!bRet)
		{
			bRet = hwm_IsExistSUSI3Lib();
		}
	}
	return bRet;
}

bool hwm_StartupSUSILib()
{
	bool bRet = false;
	bRet = StartupHWM3PartyLib();
	if(bRet)
	{
		bRet = HWM3PartyHWMAvailable() <= 0 ? false : true;
	}
	if(!bRet)
	{
		bRet = hwm_StartupSUSI4Lib();
		if(!bRet)
		{
			bRet = hwm_StartupSUSI3Lib();
			if(bRet)
			{
				SusiLibType = SA_LIB_3;
			}
		}
		else
		{
			SusiLibType = SA_LIB_4;
		}
	}
	else
	{
		
		SusiLibType = SA_LIB_3PTY;
	}
	return bRet;
}

bool hwm_CleanupSUSILib()
{
	CleanupHWM3PartyLib();
	hwm_CleanupSUSI4Lib();
	hwm_CleanupSUSI3Lib();
	return true;
}

bool hwm_GetPlatformName(char* name, int length)
{
	bool bRet = false;
	if(SusiLibType == SA_LIB_3PTY)
	{
		uint32_t pBufLen = length;
		bRet = HWM3PartyGetPlatformName(name, &pBufLen);
	}
	else if(SusiLibType == SA_LIB_4)
	{
		if(pSusiBoardGetStringA)
		{
			uint32_t pBufLen = length;
			int iRet = pSusiBoardGetStringA(SUSI_ID_BOARD_NAME_STR, name, &pBufLen);
			if(iRet == SUSI_STATUS_SUCCESS)
			{
				bRet = true;
			}
		}
	}
	else if(SusiLibType == SA_LIB_3)
	{
		if(pSusiCoreGetPlatformName)
		{
			unsigned long pBufLen = 0;
			if(pSusiCoreGetPlatformName(name, &pBufLen))
			{
				bRet = true;
			}
		}
	}
	
	return bRet;
}

bool hwm_GetBIOSVersion(char* version, int length)
{
	bool bRet = false;
	if(SusiLibType == SA_LIB_4)
	{
		if(pSusiBoardGetStringA)
		{
			uint32_t pBufLen = length;
			int iRet = pSusiBoardGetStringA(SUSI_ID_BOARD_BIOS_REVISION_STR, version, &pBufLen);
			if(iRet == SUSI_STATUS_SUCCESS)
			{
				bRet = true;
			}
		}
	}
	else if(SusiLibType == SA_LIB_3)
	{
		if(pSusiCoreGetBIOSVersion)
		{
			unsigned long pBufLen = 0;
			if(pSusiCoreGetBIOSVersion(version, &pBufLen))
			{
				bRet = true;
			}
		}
	}

	return bRet;
}

bool hwm_GetHWMTempInfo(hwm_info_t * pHWMInfo)
{
	bool bRet = false;
	char type[DEF_HWMTYPE_LENGTH] = DEF_SENSORTYPE_TEMPERATURE;
	char unit[DEF_HWMUNIT_LENGTH] = DEF_UNIT_TEMPERATURE_CELSIUS;
	if(!pHWMInfo) return bRet;
	if(SusiLibType == SA_LIB_3PTY)
	{
		if(pHWMInfo->total<=0)
			HWM3PartyGetHWMPlatformInfo(pHWMInfo);
		bRet = HWM3PartyGetHWMTempInfo(pHWMInfo);
	}
	else if(SusiLibType == SA_LIB_4)
	{
		if(pSusiBoardGetValue)
		{
			int tempUnitMax = SUSI_ID_HWM_TEMP_MAX;
			uint32_t iRet = SUSI_STATUS_SUCCESS;
			uint32_t tempValue = 0;
			int i = 0;
			int id = 0;
			for(i=0; i<tempUnitMax; i++)
			{
				tempValue = DEF_INVALID_VALUE;
				id = SUSI_ID_HWM_TEMP_BASE + i;
				iRet = pSusiBoardGetValue(id, &tempValue);
				if(iRet == SUSI_STATUS_SUCCESS)
				{
					hwm_item_t* item = NULL;
					char tag[DEF_HWMTAG_LENGTH] = {0};
					//float value =  (float)tempValue/10;										/*Kelvin*/
					float value =  (float)(tempValue - DEF_TEMP_KELVINS_OFFSET)/10;				/*Celsius*/
					//float value =  (float)(tempValue - DEF_TEMP_KELVINS_OFFSET)/10*1.8+32;	/*Fahrenheit*/
					sprintf(tag, "V%d", SUSI_ID_MAPPING_GET_NAME_HWM(id));
					item = hwm_FindItem(pHWMInfo, tag);
					if(item == NULL)
					{
						char name[DEF_HWMNAME_LENGTH] = {0};
						if(pSusiBoardGetStringA)
						{
							uint32_t length = sizeof(name);
							if(pSusiBoardGetStringA(SUSI_ID_MAPPING_GET_NAME_HWM(id), name, &length) == SUSI_STATUS_SUCCESS)
							{
								hwm_AddItem(pHWMInfo, type, name, tag, unit, value);
								//printf(">Add Item: (%s, %s, %s, %s, %f)\n", type, name, tag, unit, value);
							}
						}
					}
					else
					{
						item->value = value;
						//printf(">Update Item: (%s, %s, %s, %s, %f)\n", type, item->name, tag, unit, value);
					}
				}
			}
			bRet = true;
		}
	}	
	else if(SusiLibType == SA_LIB_3)
	{
		int offset = 32;
		if(pSusiHWMAvailable)
		{
			if(pSusiHWMAvailable()<=0)
			{
				bRet = false; 
				return bRet;
			}
		}
		if(pSusiHWMGetTemperature)
		{
			unsigned short u16TempSupport=0;
			if(pSusiHWMGetTemperature(0,0,&u16TempSupport))
			{
				short i=0;
				for(i=0; i<16; i++)
				{
					unsigned short id = (unsigned short)(1<<i);
					if ((id & u16TempSupport) != 0)
					{
						float value;
						if (!pSusiHWMGetTemperature(id, &value, NULL))
						{
							hwm_item_t* item = NULL;
							char tag[DEF_HWMTAG_LENGTH];
							sprintf(tag, "V%d", offset+i);
							item = hwm_FindItem(pHWMInfo, tag);
							if(item == NULL)
							{
								char name[DEF_HWMNAME_LENGTH];
								strcpy(name, SUSI3TempDefTagArray[i]);
								hwm_AddItem(pHWMInfo, type, name, tag, unit, value);
								//printf(">Add Item: (%s, %s, %s, %s, %f)\n", type, name, tag, unit, value);
							}
							else
							{
								item->value = value;
								//printf(">Update Item: (%s, %s, %s, %s, %f)\n", type, item->name, tag, unit, value);
							}

						}
					}
				}
			}
			bRet = true;
		}
	}
	return bRet;
}

bool hwm_GetHWMVoltInfo(hwm_info_t * pHWMInfo)
{
	bool bRet = false;
	char type[DEF_HWMTYPE_LENGTH] = DEF_SENSORTYPE_VOLTAGE;
	char unit[DEF_HWMUNIT_LENGTH] = DEF_UNIT_VOLTAGE;
	if(!pHWMInfo) return bRet;
	if(SusiLibType == SA_LIB_3PTY)
	{
		if(pHWMInfo->total<=0)
			HWM3PartyGetHWMPlatformInfo(pHWMInfo);
		bRet = HWM3PartyGetHWMVoltInfo(pHWMInfo);
	}
	else if(SusiLibType == SA_LIB_4)
	{
		if(pSusiBoardGetValue)
		{
			int tempUnitMax = SUSI_ID_HWM_VOLTAGE_MAX;
			uint32_t iRet = SUSI_STATUS_SUCCESS;
			uint32_t tempValue = 0;
			int i = 0;
			int id = 0;
			for(i=0; i<tempUnitMax; i++)
			{
				tempValue = DEF_INVALID_VALUE;
				id = SUSI_ID_HWM_VOLTAGE_BASE + i;
				iRet = pSusiBoardGetValue(id, &tempValue);
				if(iRet == SUSI_STATUS_SUCCESS)
				{
					hwm_item_t* item = NULL;
					char tag[DEF_HWMTAG_LENGTH];
					float value =  (float)tempValue/1000;
					sprintf(tag, "V%d", SUSI_ID_MAPPING_GET_NAME_HWM(id));
					item = hwm_FindItem(pHWMInfo, tag);
					if(item == NULL)
					{
						
						char name[DEF_HWMNAME_LENGTH];
						if(pSusiBoardGetStringA)
						{
							uint32_t length = sizeof(name);
							if(pSusiBoardGetStringA(SUSI_ID_MAPPING_GET_NAME_HWM(id), name, &length) == SUSI_STATUS_SUCCESS)
							{
								hwm_AddItem(pHWMInfo, type, name, tag, unit, value);
								//printf(">Add Item: (%s, %s, %s, %s, %f)\n", type, name, tag, unit, value);
							}
						}
					}
					else
					{
						item->value = value;
						//printf(">Update Item: (%s, %s, %s, %s, %f)\n", type, item->name, tag, unit, value);
					}
				}
			}
			bRet = true;
		}
	}
	else if(SusiLibType == SA_LIB_3)
	{
		int offset = 0;
		if(pSusiHWMAvailable)
		{
			if(pSusiHWMAvailable()<=0)
			{
				bRet = false; 
				return bRet;
			}
		}
		if(pSusiHWMGetVoltage)
		{
			unsigned long u32VoltSupport=0;
			if(pSusiHWMGetVoltage(0,0,&u32VoltSupport))
			{
				short i=0;
				for(i=0; i<32; i++)
				{
					unsigned short id = (unsigned short)(1<<i);
					if ((id & u32VoltSupport) != 0)
					{
						float value;
						if (!pSusiHWMGetVoltage(id, &value, NULL))
						{
							hwm_item_t* item = NULL;
							char tag[DEF_HWMTAG_LENGTH];
							sprintf(tag, "V%d", offset+i);
							item = hwm_FindItem(pHWMInfo, tag);
							if(item == NULL)
							{
								char name[DEF_HWMNAME_LENGTH];
								strcpy(name, SUSI3VoltDefTagArray[i]);
								hwm_AddItem(pHWMInfo, type, name, tag, unit, value);
								//printf(">Add Item: (%s, %s, %s, %s, %f)\n", type, name, tag, unit, value);
							}
							else
							{
								item->value = value;
								//printf(">Update Item: (%s, %s, %s, %s, %f)\n", type, item->name, tag, unit, value);
							}

						}
					}
				}
			}
			bRet = true;
		}
	}
	return bRet;
}

bool hwm_GetHWMFanInfo(hwm_info_t * pHWMInfo)
{
	bool bRet = false;
	char type[DEF_HWMTYPE_LENGTH] = DEF_SENSORTYPE_FANSPEED;
	char unit[DEF_HWMUNIT_LENGTH] = DEF_UNIT_FANSPEED;
	if(!pHWMInfo) return bRet;
	if(SusiLibType == SA_LIB_3PTY)
	{
		if(pHWMInfo->total<=0)
			HWM3PartyGetHWMPlatformInfo(pHWMInfo);
		bRet = HWM3PartyGetHWMFanInfo(pHWMInfo);
	}
	else if(SusiLibType == SA_LIB_4)
	{
		if(pSusiBoardGetValue)
		{
			int tempUnitMax = SUSI_ID_HWM_FAN_MAX;
			uint32_t iRet = SUSI_STATUS_SUCCESS;
			uint32_t tempValue = 0;
			int i = 0;
			int id = 0;
			for(i=0; i<tempUnitMax; i++)
			{
				tempValue = DEF_INVALID_VALUE;
				id = SUSI_ID_HWM_FAN_BASE + i;
				iRet = pSusiBoardGetValue(id, &tempValue);
				if(iRet == SUSI_STATUS_SUCCESS)
				{
					hwm_item_t* item = NULL;
					char tag[DEF_HWMTAG_LENGTH];
					float value =  (float)tempValue;
					sprintf(tag, "V%d", SUSI_ID_MAPPING_GET_NAME_HWM(id));
					item = hwm_FindItem(pHWMInfo, tag);
					if(item == NULL)
					{

						char name[DEF_HWMNAME_LENGTH];
						if(pSusiBoardGetStringA)
						{
							uint32_t length = sizeof(name);
							if(pSusiBoardGetStringA(SUSI_ID_MAPPING_GET_NAME_HWM(id), name, &length) == SUSI_STATUS_SUCCESS)
							{
								hwm_AddItem(pHWMInfo, type, name, tag, unit, value);
								//printf(">Add Item: (%s, %s, %s, %s, %f)\n", type, name, tag, unit, value);
							}
						}
					}
					else
					{
						item->value = value;
						//printf(">Update Item: (%s, %s, %s, %s, %f)\n", type, item->name, tag, unit, value);
					}
				}
			}
			bRet = true;
		}
	}
	else if(SusiLibType == SA_LIB_3)
	{
		int offset = 48;
		if(pSusiHWMAvailable)
		{
			if(pSusiHWMAvailable()<=0)
			{
				bRet = false; 
				return bRet;
			}
		}
		if(pSusiHWMGetFanSpeed)
		{
			unsigned short u16FanSupport=0;
			if(pSusiHWMGetFanSpeed(0,0,&u16FanSupport))
			{
				short i=0;
				for(i=0; i<16; i++)
				{
					unsigned short id = (unsigned short)(1<<i);
					if ((id & u16FanSupport) != 0)
					{
						unsigned short value;
						if (!pSusiHWMGetFanSpeed(id, &value, NULL))
						{
							hwm_item_t* item = NULL;
							char tag[DEF_HWMTAG_LENGTH];
							sprintf(tag, "V%d", offset+i);
							item = hwm_FindItem(pHWMInfo, tag);
							if(item == NULL)
							{
								char name[DEF_HWMNAME_LENGTH];
								strcpy(name, SUSI3FanDefTagArray[i]);
								hwm_AddItem(pHWMInfo, type, name, tag, unit, value);
								//printf(">Add Item: (%s, %s, %s, %s, %d)\n", type, name, tag, unit, value);
							}
							else
							{
								item->value = value;
								//printf(">Update Item: (%s, %s, %s, %s, %d)\n", type, item->name, tag, unit, value);
							}

						}
					}
				}
			}
			bRet = true;
		}
	}
	return bRet;
}

bool hwm_GetHWMCurrentInfo(hwm_info_t * pHWMInfo)
{
	bool bRet = false;
	char type[DEF_HWMTYPE_LENGTH] = DEF_SENSORTYPE_CURRENT;
	char unit[DEF_HWMUNIT_LENGTH] = DEF_UNIT_CURRENT;
	if(!pHWMInfo) return bRet;
	if(SusiLibType == SA_LIB_3PTY)
	{
		if(pHWMInfo->total<=0)
			HWM3PartyGetHWMPlatformInfo(pHWMInfo);
		bRet = HWM3PartyGetHWMCurrentInfo(pHWMInfo);
	}
	else if(SusiLibType == SA_LIB_4)
	{
		if(pSusiBoardGetValue)
		{
			int tempUnitMax = SUSI_ID_HWM_CURRENT_MAX;
			uint32_t iRet = SUSI_STATUS_SUCCESS;
			uint32_t tempValue = 0;
			int i = 0;
			int id = 0;
			for(i=0; i<tempUnitMax; i++)
			{
				tempValue = DEF_INVALID_VALUE;
				id = SUSI_ID_HWM_CURRENT_BASE + i;
				iRet = pSusiBoardGetValue(id, &tempValue);
				if(iRet == SUSI_STATUS_SUCCESS)
				{
					hwm_item_t* item = NULL;
					char tag[DEF_HWMTAG_LENGTH];
					float value =  (float)tempValue/1000;
					sprintf(tag, "V%d", SUSI_ID_MAPPING_GET_NAME_HWM(id));
					item = hwm_FindItem(pHWMInfo, tag);
					if(item == NULL)
					{

						char name[DEF_HWMNAME_LENGTH];
						if(pSusiBoardGetStringA)
						{
							uint32_t length = sizeof(name);
							if(pSusiBoardGetStringA(SUSI_ID_MAPPING_GET_NAME_HWM(id), name, &length) == SUSI_STATUS_SUCCESS)
							{
								hwm_AddItem(pHWMInfo, type, name, tag, unit, value);
								//printf(">Add Item: (%s, %s, %s, %s, %f)\n", type, name, tag, unit, value);
							}
						}
					}
					else
					{
						item->value = value;
						//printf(">Update Item: (%s, %s, %s, %s, %f)\n", type, item->name, tag, unit, value);
					}
				}
			}
			bRet = true;
		}
	}
	else if(SusiLibType == SA_LIB_3)
	{
		bRet = true;
	}
	return bRet;
}

bool hwm_GetHWMCaseOpenInfo(hwm_info_t * pHWMInfo)
{
	bool bRet = false;
	char type[DEF_HWMTYPE_LENGTH] = DEF_SENSORTYPE_CASEOPEN;
	char unit[DEF_HWMUNIT_LENGTH] = DEF_UNIT_CASEOPEN;
	if(!pHWMInfo) return bRet;
	if(SusiLibType == SA_LIB_3PTY)
	{
		if(pHWMInfo->total<=0)
			HWM3PartyGetHWMPlatformInfo(pHWMInfo);
		bRet = HWM3PartyGetHWMCaseOpenInfo(pHWMInfo);
	}
	else if(SusiLibType == SA_LIB_4)
	{
		if(pSusiBoardGetValue)
		{
			int tempUnitMax = SUSI_ID_HWM_CASEOPEN_MAX;
			uint32_t iRet = SUSI_STATUS_SUCCESS;
			uint32_t tempValue = 0;
			int i = 0;
			int id = 0;
			for(i=0; i<tempUnitMax; i++)
			{
				tempValue = DEF_INVALID_VALUE;
				id = SUSI_ID_HWM_CASEOPEN_BASE + i;
				iRet = pSusiBoardGetValue(id, &tempValue);
				if(iRet == SUSI_STATUS_SUCCESS)
				{
					hwm_item_t* item = NULL;
					char tag[DEF_HWMTAG_LENGTH];
					float value =  (float)tempValue/1000;
					sprintf(tag, "V%d", SUSI_ID_MAPPING_GET_NAME_HWM(id));
					item = hwm_FindItem(pHWMInfo, tag);
					if(item == NULL)
					{

						char name[DEF_HWMNAME_LENGTH];
						if(pSusiBoardGetStringA)
						{
							uint32_t length = sizeof(name);
							if(pSusiBoardGetStringA(SUSI_ID_MAPPING_GET_NAME_HWM(id), name, &length) == SUSI_STATUS_SUCCESS)
							{
								hwm_AddItem(pHWMInfo, type, name, tag, unit, value);
								//printf(">Add Item: (%s, %s, %s, %s, %f)\n", type, name, tag, unit, value);
							}
						}
					}
					else
					{
						item->value = value;
						//printf(">Update Item: (%s, %s, %s, %s, %f)\n", type, item->name, tag, unit, value);
					}
				}
			}
			bRet = true;
		}
	}	
	else if(SusiLibType == SA_LIB_3)
	{
		bRet = true;
	}
	return bRet;
}

bool hwm_GetHWMInfo(hwm_info_t * pHWMInfo)
{
	bool bRet = false;
	if(!pHWMInfo) return bRet;
	bRet = true;
	bRet &= hwm_GetHWMTempInfo(pHWMInfo);
	bRet &= hwm_GetHWMVoltInfo(pHWMInfo);
	bRet &= hwm_GetHWMFanInfo(pHWMInfo);
	bRet &= hwm_GetHWMCurrentInfo(pHWMInfo);
	bRet &= hwm_GetHWMCaseOpenInfo(pHWMInfo);

	return bRet;
}

bool hwm_ReleaseHWMInfo(hwm_info_t * pHWMInfo)
{
	bool bRet = false;
	hwm_item_t *item = NULL;
	hwm_item_t *target = NULL;
	if(!pHWMInfo) return bRet;
	item = hwm_LastItem(pHWMInfo);
	while(item != NULL)
	{
		target = item;
		item = item->prev;
		if(target!=NULL)
		{
			pHWMInfo->total--;
			free(target);
			target = NULL;
		}
	}
	return bRet;
}