#ifndef _SAGENERAL_LOG_H_
#define _SAGENERAL_LOG_H_

#include <Log.h>

#define DEF_SAGENERAL_LOG_NAME    "AgentLog.txt"   //default log file name
#define SAGENERAL_LOG_ENABLE
//#define DEF_SAGENERAL_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_SAGENERAL_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_SAGENERAL_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)
LOGHANDLE SAGeneralLogHandle = NULL;
#ifdef SAGENERAL_LOG_ENABLE
#define SAGeneralLog(level, fmt, ...)  do { if (SAGeneralLogHandle != NULL)   \
	WriteLog(SAGeneralLogHandle, DEF_SAGENERAL_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define SAGeneralLog(level, fmt, ...)
#endif

#endif