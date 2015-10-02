#ifndef _SA_KEEPALIVE_H_
#define _SA_KEEPALIVE_H_
#include "susiaccess_handler_mgmt.h"

#ifdef WIN32
#define DEF_PIPE_NAME              "\\\\.\\pipe\\SAWatchdogCommPipe"
#else
#define DEF_PIPE_NAME              "/tmp/SAWatchdogFifo"
#endif
#define DEF_COMM_ID                (1)
#define DEF_KEEPALIVE_INTERVAL_S   (2)

#define DEF_MAX_SELF_CPU_USAGE     70
#define DEF_MAX_SELF_MEM_USAGE     80000   //KB

#define DEF_KEEPALIVE_TIME_S         3
#define DEF_KEEPALIVE_TRY_TIMES      3

typedef enum WatchObjType
{
   FORM_PROCESS,
   NO_FORM_PROCESS,
   WIN_SERVICE,
}WATCHOBJTYPE;

typedef enum WatchCmdKey{
   START_WATCH,
   KEEPALIVE,
   BUSY_WAIT,
   STOP_WATCH,
}WATCHCMDKEY;

typedef union WatchParams{
   unsigned long busyWaitTimeS;
   struct {
      WATCHOBJTYPE  objType;
      unsigned long watchPID;
   }starWatchInfo;
}WATCHPARAMS;

typedef struct WatchMessage{
   unsigned long  commID;
   WATCHCMDKEY  commCmd;
   WATCHPARAMS  commParams;
}WATCHMSG, *PWATCHMSG;

struct kepalive_ctx{
   void				 *threadHandler;
   bool				 isThreadRunning;
   Handler_List_t *pHandlerList;
};

void keepalive_initialize(Handler_List_t *pLoaderList, void * logger);
void keepalive_uninitialize();

#endif