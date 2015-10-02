#include "platform.h"
#include "common.h"
#include "ftphelper.h"
#include <Log.h>
#include "FtpDownload.h"
#include "FtpUpload.h"

#define DEF_FTP_LOG_NAME    "FtpLog"   //Updater log file name
#define FTP_LOG_ENABLE
//#define DEF_FTP_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_FTP_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_FTP_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)
LOGHANDLE ftpLogHandle;
#ifdef FTP_LOG_ENABLE
#define FtpLog(level, fmt, ...)  do { if (ftpLogHandle != NULL)   \
	WriteIndividualLog(ftpLogHandle, "ftphelper", DEF_FTP_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define FtpLog(level, fmt, ...)
#endif

static CAGENT_PTHREAD_ENTRY(FTPDownloadThreadStart, args)
{
	ftp_context_t * pFtpParams = (ftp_context_t *)args;

	if(pFtpParams)
	{
		char repMsg[1024] = {0};
		if(pFtpParams->ftphandler)
		{
			int iRet = 0;
			pFtpParams->isTransferring = true;
			iRet = FtpDownload(pFtpParams->ftphandler, pFtpParams->sFileUrl, pFtpParams->sLocalPath);
			if(iRet != 0)
			{
				memset(repMsg, 0, sizeof(repMsg));
				sprintf_s(repMsg, sizeof(repMsg), "%s", "File download error!");
				if(iRet > 0)
				{
					char errStr[256] = {0};
					FtpDownloadGetErrorStr(iRet, errStr);
					if(strlen(errStr)) 
					{
						sprintf_s(repMsg, sizeof(repMsg), "%s%s", repMsg, errStr);
					}
				}
				FtpLog(Error, "%s", repMsg);
			}
			else
			{
				memset(repMsg, 0, sizeof(repMsg));
				sprintf_s(repMsg, sizeof(repMsg), "%s", "File download OK!");
				FtpLog(Normal, "%s", repMsg);
			}
			pFtpParams->isTransferring = false;
			FtpDownloadCleanup(pFtpParams->ftphandler);
			pFtpParams->ftphandler = NULL;
		}
	}
	
	app_os_thread_exit(0);
	return 0;
}

static CAGENT_PTHREAD_ENTRY(FTPUploadThreadStart, args)
{
	ftp_context_t * pFtpParams = (ftp_context_t *)args;

	if(pFtpParams)
	{
		char repMsg[1024] = {0};
		if(pFtpParams->ftphandler)
		{
			int iRet = 0;
			pFtpParams->isTransferring = true;
			iRet = FtpUpload(pFtpParams->ftphandler,  pFtpParams->sLocalPath, pFtpParams->sFileUrl);
			if(iRet != 0)
			{
				memset(repMsg, 0, sizeof(repMsg));
				sprintf_s(repMsg, sizeof(repMsg), "%s", "File upload error!");
				if(iRet > 0)
				{
					char errStr[256] = {0};
					FtpUploadGetErrorStr(iRet, errStr);
					if(strlen(errStr)) 
					{
						sprintf_s(repMsg, sizeof(repMsg), "%s%s", repMsg, errStr);
					}
				}
				FtpLog(Error, "%s", repMsg);
			}
			else
			{
				memset(repMsg, 0, sizeof(repMsg));
				sprintf_s(repMsg, sizeof(repMsg), "%s", "File upload OK!");
				FtpLog(Normal, "%s", repMsg);
			}
			pFtpParams->isTransferring = false;
			FtpUploadCleanup(pFtpParams->ftphandler);
			pFtpParams->ftphandler = NULL;
		}
	}

	app_os_thread_exit(0);
	return 0;
}

void ftphelper_EnableLog(void* logHandle)
{
	ftpLogHandle = logHandle;
}

ftp_context_t* ftphelper_FtpDownload(char* ftpserver, int port, char* ftpuserName, char* ftpPassword, char* remotePath, char* localPath)
{
	ftp_context_t* contex = NULL;
	CAGENT_THREAD_HANDLE threadhandle = NULL;
	if(!ftpserver || strlen(ftpserver) <= 0) return contex;

	if(!localPath || strlen(localPath) <= 0) return contex;
	
#ifdef FTP_LOG_ENABLE
	//if(ftpLogHandle == NULL)
	//{
	//	char mdPath[MAX_PATH] = {0};
	//	char ftpLogPath[MAX_PATH] = {0};
	//	app_os_get_module_path(mdPath);
	//	//path_combine(ftpLogPath, mdPath, DEF_FTP_LOG_NAME);
	//	//sprintf_s(ftpLogPath, sizeof(ftpLogPath), "%s%s", mdPath, DEF_FTP_LOG_NAME);
	//	ftpLogHandle = InitLog(ftpLogPath);
	//}
#endif
	contex = malloc(sizeof(ftp_context_t));
	memset(contex, 0, sizeof(ftp_context_t));
	contex->iType = FTP_TYPE_DOWNLOAD;
	if((ftpuserName && strlen(ftpuserName)) && (ftpPassword && strlen(ftpPassword)))
	{
		sprintf_s(contex->sFileUrl, sizeof(contex->sFileUrl), "ftp://%s:%s@%s:%d%s", ftpuserName, ftpPassword,
			ftpserver, port, remotePath);
	}
	else
	{
		sprintf_s(contex->sFileUrl, sizeof(contex->sFileUrl), "ftp://%s:%d%s", ftpserver, port, remotePath);
	}
	FtpLog(Normal, "FTP URL: %s", contex->sFileUrl);

	strncpy(contex->sLocalPath, localPath, strlen(localPath)+1);
	FtpLog(Normal, "File Download Path:%s",contex->sLocalPath);

	/*Detected 292 byte memory leaks in cURL*/
	contex->ftphandler = FtpDownloadInit();
	if(!contex->ftphandler)
	{
		free(contex);
		contex = NULL;
		return contex;
	}
	FtpLog(Normal, "%s","File downloader initialize Ok!");

	if (app_os_thread_create(&threadhandle, FTPDownloadThreadStart, contex) != 0)
	{
		contex->isThreadRunning = false;
	}
	else
	{
		contex->isThreadRunning = true;
		contex->threadHandler = (void*)threadhandle;
	}
	return contex;
}

/*Not verified.*/
ftp_context_t* ftphelper_FtpUpload(char* ftpserver, int port, char* ftpuserName, char* ftpPassword, char* remotePath, char * localFile)
{
	ftp_context_t* contex = NULL;
	CAGENT_THREAD_HANDLE threadhandle = NULL;

	if(!ftpserver || strlen(ftpserver) <= 0) return contex;

	if(!localFile || strlen(localFile) <= 0) return contex;

#ifdef FTP_LOG_ENABLE
	/*if(ftpLogHandle == NULL)
	{
		char mdPath[MAX_PATH] = {0};
		char ftpLogPath[MAX_PATH] = {0};
		app_os_get_module_path(mdPath);
		sprintf_s(ftpLogPath, sizeof(ftpLogPath), "%s%s", mdPath, DEF_FTP_LOG_NAME);
		ftpLogHandle = InitLog(ftpLogPath);
	}*/
#endif

	contex = malloc(sizeof(ftp_context_t));
	memset(contex, 0, sizeof(ftp_context_t));
	contex->iType = FTP_TYPE_UPLOAD;
	if((ftpuserName && strlen(ftpuserName)) && (ftpPassword && strlen(ftpPassword)))
	{
		sprintf_s(contex->sFileUrl, sizeof(contex->sFileUrl), "ftp://%s:%s@%s:%d%s", ftpuserName, ftpPassword,
			ftpserver, port, remotePath);
	}
	else
	{
		sprintf_s(contex->sFileUrl, sizeof(contex->sFileUrl), "ftp://%s:%d%s", ftpserver, port, remotePath);
	}
	FtpLog(Normal, "FTP URL: %s", contex->sFileUrl);

	strncpy(contex->sLocalPath, localFile, strlen(localFile)+1);
	FtpLog(Normal, "File Upload Path:%s",contex->sLocalPath);

	contex->ftphandler = FtpUploadInit();
	if(!contex->ftphandler)
	{
		free(contex);
		contex = NULL;
		return contex;
	}
	FtpLog(Normal, "%s","File uploader initialize Ok!");
	
	if (app_os_thread_create(&threadhandle, FTPUploadThreadStart, contex) != 0)
	{
		contex->isThreadRunning = false;
	}
	else
	{
		contex->isThreadRunning = true;
		contex->threadHandler = (void*)threadhandle;
	}
	return contex;
}

void ftphelper_WaitTransferComplete(ftp_context_t* contex)
{
	CAGENT_THREAD_HANDLE threadhandle = NULL;
	if(!contex) return;
	if(contex->isThreadRunning == true)
	{
		//MonitorLog(g_loghandle, Normal, " %s> Wait Trhead stop!\n", MyTopic);	
		contex->isThreadRunning = false;
		threadhandle = (CAGENT_THREAD_HANDLE)contex->threadHandler;
		app_os_thread_join(threadhandle);
		threadhandle = contex->threadHandler = NULL;
	}
}

void ftphelper_FtpCleanup(ftp_context_t* contex)
{
	CAGENT_THREAD_HANDLE threadhandle = NULL;
	if(!contex) return;
	if(contex->isThreadRunning == true)
	{
		//MonitorLog(g_loghandle, Normal, " %s> Wait Trhead stop!\n", MyTopic);	
		contex->isThreadRunning = false;
		threadhandle = (CAGENT_THREAD_HANDLE)contex->threadHandler;
		app_os_thread_join(threadhandle);
		threadhandle = contex->threadHandler = NULL;
	}

	if(contex->ftphandler)
	{
		if(contex->iType == FTP_TYPE_DOWNLOAD)
		{
			FtpDownloadCleanup(contex->ftphandler);
		}
		else if(contex->iType == FTP_TYPE_UPLOAD)
		{
			FtpUploadCleanup(contex->ftphandler);
		}
		contex->ftphandler = NULL;
	}

	free(contex);
	contex = NULL;

#ifdef FTP_LOG_ENABLE
	/*if(ftpLogHandle != NULL) 
	{
		UninitLog(ftpLogHandle);
		ftpLogHandle = NULL;
	}*/
#endif
}

int ftphelper_FtpGetSpeedKBS(ftp_context_t* contex, float * speedKBS)
{
	if(contex->iType == FTP_TYPE_DOWNLOAD)
	{
		return FtpDownloadGetSpeedKBS(contex->ftphandler, speedKBS);
	}
	*speedKBS = 0;
	return -1;
}

int ftphelper_FTPGetPersent(ftp_context_t* contex, unsigned int * persent)
{
	if(contex->iType == FTP_TYPE_DOWNLOAD)
	{
		return FTPDownloadGetPersent(contex->ftphandler, persent);
	}
	*persent = 0;
	return -1;
}

int ftphelper_FTPGetCurSizeKB(ftp_context_t* contex, unsigned int * curSizeKB)
{
	if(contex->iType == FTP_TYPE_DOWNLOAD)
	{
		return FTPDownloadGetCurDLSizeKB(contex->ftphandler, curSizeKB);
	}
	*curSizeKB = 0;
	return -1;
}

int ftphelper_FTPGetStatus(ftp_context_t* contex, FTPSTATUS * dlStatus)
{
	if(contex->iType == FTP_TYPE_DOWNLOAD)
	{
		return FTPDownloadGetStatus(contex->ftphandler, (FTPDLSTATUS*)dlStatus);
	}
	*dlStatus = 0;
	return -1;
}

int ftphelper_FTPGetLastError(ftp_context_t* contex, char * errorBuf, int bufLen)
{
	if(contex->iType == FTP_TYPE_DOWNLOAD)
	{
		return FTPDownLoadGetLastError(contex->ftphandler, errorBuf, bufLen);
	}
	return -1;
}

void ftphelper_FtpGetErrorStr(ftp_context_t* contex, int errorCode, char * errorStr)
{
	if(contex->iType == FTP_TYPE_DOWNLOAD)
	{
		FtpDownloadGetErrorStr(errorCode, errorStr);
	}
	else if(contex->iType == FTP_TYPE_UPLOAD)
	{
		FtpUploadGetErrorStr(errorCode, errorStr);
	}
}