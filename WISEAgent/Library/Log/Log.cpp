#ifdef LOG4Z
#include "log4z.h"
using namespace zsummer::log4z;
static ILog4zManager* g_logger = NULL;
#endif
#include "platform.h"
#include "common.h"
#include "Log.h"

//static void WriteLogFile(LOGHANDLE logHandle, LOGMODE logMode, LogLevel logLevel, const char * format, va_list ap);
#ifdef LOG4Z
static void _WriteLog(LOGHANDLE logHandle, LoggerId id, ENUM_LOG_LEVEL level, char* log)
{
	if(!logHandle)
		return;
	g_logger = (ILog4zManager*)logHandle;
	if (g_logger->prePushLog(id,level))
	{
		char logBuf[LOG4Z_LOG_BUF_SIZE];
		zsummer::log4z::Log4zStream ss(logBuf, LOG4Z_LOG_BUF_SIZE);
		ss << log;
		g_logger->pushLog(id, level, logBuf, NULL, NULL);
	}
}
#endif
static void WriteLogFile(LOGHANDLE logHandle, int id, LOGMODE logMode, LogLevel logLevel, const char * format, va_list ap)
{
    char logStr[2048] = {0};
    if(logHandle == NULL) return;
    _vsnprintf(logStr, sizeof(logStr), format, ap);
#ifdef LOG4Z
	switch(logLevel)
    {
	case Debug:
	  {
		_WriteLog(logHandle, (LoggerId)id, LOG_LEVEL_DEBUG, logStr);
		break;
	  }
    case Normal:
	  {
		_WriteLog(logHandle, (LoggerId)id, LOG_LEVEL_INFO, logStr);
		break;
	  }
	case Warning:
      {
         _WriteLog(logHandle, (LoggerId)id, LOG_LEVEL_WARN, logStr);
         break;
      }
   case Error:
      {
         _WriteLog(logHandle, (LoggerId)id, LOG_LEVEL_ERROR, logStr);
         break;
      }
   case Alarm:
      {
         _WriteLog(logHandle, (LoggerId)id, LOG_LEVEL_ALARM, logStr);
         break;
      }
   case Fatal:
      {
         _WriteLog(logHandle, (LoggerId)id, LOG_LEVEL_FATAL, logStr);
         break;
      }
   default:
      break;
   }
#else
	char logLine[4096] = {0};
	time_t curTime = 0;
	struct tm *pCurTm = NULL;
    char timeStr[32] = {0};
    char levelStr[16] = {0};

	curTime = time(0);
    pCurTm = localtime(&curTime);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", pCurTm);
    switch(logLevel)
    {
	case Debug:
    case Normal:
	  {
		memcpy(levelStr, "Normal", sizeof("Normal"));
		break;
	  }
	case Warning:
      {
         memcpy(levelStr, "Warning", sizeof("Warning"));
         break;
      }
   case Error:
   case Alarm:
   case Fatal:
      {
         memcpy(levelStr, "Error]", sizeof("Error"));
         break;
      }
   default:
      break;
   }

   sprintf_s(logLine, sizeof(logLine), "[%s][%s]: %s\r\n",timeStr, levelStr, logStr);

   if(logMode & LOG_MODE_CONSOLE_OUT)
   {
      printf("%s", logLine);
   }

   if(logMode & LOG_MODE_FILE_OUT)
   {
      fputs(logLine, (FILE *)logHandle);
      fflush((FILE *)logHandle);
   }
#endif
}

LOGHANDLE InitLog(char * logFileName)
{
	LOGHANDLE logHandle = NULL;
#ifdef LOG4Z
	if(!g_logger)
	{
		//char path [MAX_PATH] = {0};
		//char filename[MAX_PATH] = {0};
		char configpath[MAX_PATH] = {0};
		//split_path_file(logFileName, path, filename);
		path_combine(configpath, logFileName, "logger.ini");
		ILog4zManager::getRef().config(configpath);
		logHandle = g_logger=ILog4zManager::getPtr();
		//logHandle = (LOGHANDLE)g_logger->findLogger("agent");
		//start log4z
		g_logger->start();
		//hot update configure
		g_logger->setAutoUpdate(10);
	}
#else
   if(logFileName == NULL) return logHandle;
   {
      FILE * logFile = NULL;
      logFile = fopen(logFileName, "ab");
      if(logFile != NULL) logHandle = (LOGHANDLE)logFile;
   }
#endif
   return logHandle;
}
#ifdef LOG4Z
int GetLogID(LOGHANDLE logHandle,char * logname)
{
	LoggerId id = -1;
	
	if(logHandle)
	{
		g_logger = (ILog4zManager*)logHandle;
		id = g_logger->findLogger(logname);
		if(id<0)
			id = g_logger->findLogger("agent");
	}
	return id;
}
#endif
void UninitLog(LOGHANDLE logHandle)
{
#ifdef LOG4Z
	if(logHandle)
		((ILog4zManager*)logHandle)->stop();
#else
   fclose((FILE *)logHandle);
#endif
}

void WriteLog(LOGHANDLE logHandle, LOGMODE logMode, LogLevel level, const char * format, ...)
{
	int id = -1;
	va_list ap;
    va_start(ap, format);
#ifdef LOG4Z
	id = GetLogID(logHandle, "agent");
#endif
    WriteLogFile(logHandle, id, logMode, level, format, ap);
    va_end(ap);
}

void WriteIndividualLog(LOGHANDLE logHandle, char* group, LOGMODE logMode, LogLevel level, const char * format, ...)
{
	int id = -1;
	va_list ap;
    va_start(ap, format);
#ifdef LOG4Z
	id = GetLogID(logHandle, group);
#endif
    WriteLogFile(logHandle, id, logMode, level, format, ap);
    va_end(ap);
}