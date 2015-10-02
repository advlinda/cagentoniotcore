#include "common.h"
#include "platform.h"
#include "Keepalive.h"
#include "SAManagerLog.h"

//#ifdef WIN32
#include "NamedPipeClient.h"
//#endif

LOGHANDLE g_keepalivelogger = NULL;
struct kepalive_ctx g_kepalivectx;

typedef struct handler_countdown
{    
	char name[128];
	int count;
	int limit;
	struct handler_countdown *prev;
	struct handler_countdown *next;
} handler_countdown_st;

static handler_countdown_st *handlers_countdown = NULL;

handler_countdown_st * GetLastHandlerCD()
{
	handler_countdown_st *handlers = handlers_countdown;
	handler_countdown_st *target = NULL;
	//printf("Find Last\n"); 
	while(handlers != NULL)
	{
		//printf("Topic Name: %s\n", topic->name);
		target = handlers;
		handlers = handlers->next;
	}
	return target;
}

handler_countdown_st * AddhHandlerCD(char const * name, int limit)
{
	handler_countdown_st *handler = NULL;
	
	if (name == NULL)
		return NULL;
	
	handler = (struct handler_countdown_st *)malloc(sizeof(handler_countdown_st));

	strncpy(handler->name, name, strlen(name)+1);
	handler->count = 0;
	handler->limit = limit;
	handler->next = NULL;	
	handler->prev = NULL;	

	if(handlers_countdown == NULL)
	{
		handlers_countdown = handler;
	} else {
		handler_countdown_st *lastone = GetLastHandlerCD();
		//printf("Last Topic Name: %s\n", lasttopic->name);
		lastone->next = handler;
		handler->prev = lastone;
	}
	return handler;
}

void RemoveHandlerCD(char* name)
{
	handler_countdown_st *handler = handlers_countdown;
	handler_countdown_st *target = NULL;
	//printf("Remove Topic\n");
	while(handler != NULL)
	{
		//printf("Topic Name: %s\n", topic->name);
		if(strcmp(handler->name, name) == 0)
		{
			if(handlers_countdown == handler)
				handlers_countdown = handler->next;
			if(handler->prev != NULL)
				handler->prev->next = handler->next;
			if(handler->next != NULL)
				handler->next->prev = handler->prev;
			target = handler;
			break;
		}
		handler = handler->next;
	}
	if(target!=NULL)
	{
		free(target);
		target = NULL;
	}
}

handler_countdown_st * FindHandlerCD(char const * name)
{
	handler_countdown_st *handler = handlers_countdown;
	handler_countdown_st *target = NULL;

	//printf("Find Topic\n");
	while(handler != NULL)
	{
		//printf("Topic Name: %s\n", topic->name);
		if(strcmp(handler->name, name) == 0)
		{
			target = handler;
			break;
		}
		handler = handler->next;
	}
	return target;
}

void CheckHandlerStatus(struct kepalive_ctx* ctx)
{
	Handler_Loader_Interface *pInterfaceTmp = NULL;
	if(!ctx)
		return;
	pInterfaceTmp = ctx->pHandlerList->items;
	while(pInterfaceTmp)
	{
		HANDLER_THREAD_STATUS pOutStatus;
		bool bRestart = false;
		if(pInterfaceTmp->Workable == false)
		{
			pInterfaceTmp = pInterfaceTmp->next;
			continue;
		}
		if(pInterfaceTmp->type != user_handler)
		{
			pInterfaceTmp = pInterfaceTmp->next;
			continue;
		}

		if(!ctx->isThreadRunning)
			break;

		if(pInterfaceTmp->Handler_Get_Status_API)
		{
			handler_result result = pInterfaceTmp->Handler_Get_Status_API(&pOutStatus);
			if(result == handler_success)
			{
				handler_countdown_st *phandlercd = FindHandlerCD(pInterfaceTmp->Name);
				if(pOutStatus == handler_status_busy)
				{
					if(!phandlercd)
					{
						phandlercd = AddhHandlerCD(pInterfaceTmp->Name, 3);
					}
					phandlercd->count++;
					SAManagerLog(g_keepalivelogger, Warning, "Handler %s is busy count %d!", pInterfaceTmp->Name, phandlercd->count);
					if(phandlercd->count >= phandlercd->limit)
					{
						bRestart = true;
						RemoveHandlerCD(pInterfaceTmp->Name);
					}
				}
				else if(phandlercd)
				{
					RemoveHandlerCD(pInterfaceTmp->Name);
				}
			}
		}
		
		if(!ctx->isThreadRunning)
			break;

		if(bRestart)
		{
			if(pInterfaceTmp->Handler_Stop_API)
			{
				SAManagerLog(g_keepalivelogger, Warning, "Stop Handler %s!", pInterfaceTmp->Name);
				pInterfaceTmp->Handler_Stop_API();
			}

			if(!ctx->isThreadRunning)
				break;

			if(pInterfaceTmp->Handler_Start_API)
			{
				SAManagerLog(g_keepalivelogger, Warning, "Restart Handler %s!", pInterfaceTmp->Name);
				pInterfaceTmp->Handler_Start_API();
			}
		}
		pInterfaceTmp = pInterfaceTmp->next;
	}
}

static CAGENT_PTHREAD_ENTRY( KeepaliveThread, args)
{
	struct kepalive_ctx* ctx = (struct kepalive_ctx*)args;
//#ifdef WIN32
	PIPECLINETHANDLE pipeClientHandle = NULL;
	WATCHMSG watchMsg;
//#endif
	bool isLogConnectFail = true;
	bool bRet = true;
	int i=0;
	while(ctx->isThreadRunning)
	{
//#ifdef WIN32
		pipeClientHandle = NamedPipeClientConnect(DEF_PIPE_NAME, DEF_COMM_ID, NULL);
		if(pipeClientHandle)
		{
			app_os_sleep(1000); //On CentOS, NamedPipeClient Connect need more time
			SAManagerLog(g_keepalivelogger, Normal, "NamedPipe: %s, CommID: %d, IPC Connect successfully!", DEF_PIPE_NAME, DEF_COMM_ID);
			isLogConnectFail = true;
			memset(&watchMsg, 0, sizeof(WATCHMSG));
			watchMsg.commCmd = START_WATCH;
			watchMsg.commID = DEF_COMM_ID;
			watchMsg.commParams.starWatchInfo.objType = WIN_SERVICE;
			watchMsg.commParams.starWatchInfo.watchPID = getpid();
			bRet = NamedPipeClientSend(pipeClientHandle, (char *)&watchMsg, sizeof(WATCHMSG));
			if(!bRet)
			{
				SAManagerLog(g_keepalivelogger, Error, "NamedPipe: %s, CommID: %d, Start watch failed!", DEF_PIPE_NAME, DEF_COMM_ID);
				goto done;	
			}
			app_os_sleep(1000); //On CentOS, NamedPipeClient Send Packet need more time
			while(ctx->isThreadRunning)
			{
//#endif
				CheckHandlerStatus(ctx);
//#ifdef WIN32
				//memset(&watchMsg, 0, sizeof(WATCHMSG));
				watchMsg.commCmd = KEEPALIVE;
				watchMsg.commID = DEF_COMM_ID;
				bRet = NamedPipeClientSend(pipeClientHandle, (char *)&watchMsg, sizeof(WATCHMSG));
				if(!bRet)
				{
					//SAManagerLog(g_keepalivelogger, Error, "NamedPipe: %s, CommID: %d, Send keepalive failed!", DEF_PIPE_NAME, DEF_COMM_ID);
					goto done;
				}
//#endif
				app_os_sleep(DEF_KEEPALIVE_INTERVAL_S*1000);
//#ifdef WIN32
			}
			//memset(&watchMsg, 0, sizeof(WATCHMSG));
			watchMsg.commCmd = STOP_WATCH;
			watchMsg.commID = DEF_COMM_ID;
			bRet = NamedPipeClientSend(pipeClientHandle, (char *)&watchMsg, sizeof(WATCHMSG));
			if(!bRet) SAManagerLog(g_keepalivelogger, Error, "NamedPipe: %s, CommID: %d, Stop watch failed!", DEF_PIPE_NAME, DEF_COMM_ID);
			else SAManagerLog(g_keepalivelogger, Normal, "NamedPipe: %s, CommID: %d, Stop watch successfully!", DEF_PIPE_NAME, DEF_COMM_ID);       
		}
		else
		{
			if(isLogConnectFail)
			{//Continuous failure log only once
				SAManagerLog(g_keepalivelogger,Error, "NamedPipe: %s, CommID: %d, IPC Connect failed!", DEF_PIPE_NAME, DEF_COMM_ID);
				isLogConnectFail = false;
			}
		}		
		
	done:
		if(pipeClientHandle)NamedPipeClientDisconnect(pipeClientHandle);

		app_os_sleep(1000);
//#endif
	}
	
	app_os_thread_exit(0);
	return 0;
}

void keepalive_initialize(Handler_List_t *pLoaderList, void * logger)
{
	g_keepalivelogger = logger;
	memset(&g_kepalivectx, 0, sizeof(struct kepalive_ctx));
	if(g_kepalivectx.threadHandler)
	{
		app_os_thread_join(g_kepalivectx.threadHandler);
		g_kepalivectx.threadHandler = NULL;
	}
	g_kepalivectx.isThreadRunning = true;
	if(app_os_thread_create(&g_kepalivectx.threadHandler, KeepaliveThread, &g_kepalivectx)==0)
		g_kepalivectx.pHandlerList = pLoaderList;
}

void keepalive_uninitialize()
{
	if(g_kepalivectx.threadHandler)
	{
		g_kepalivectx.isThreadRunning = false;
		app_os_thread_join(g_kepalivectx.threadHandler);
		g_kepalivectx.threadHandler = NULL;
	}
}