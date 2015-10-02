#include "common.h"
#include "NamedPipeServer.h"
#include "IPCCommData.h"

PIPEINSTLIST  PipeInstList = NULL;
char*		  FIFOName = NULL;
//----------------------------Watch object list function define--------------------------
static PIPEINSTLIST CreatePipeInstList();
static void DestroyPipeInstList(PIPEINSTLIST pipeInstList);
static PPIPEINSTNODE InsertPipeInstNode(PIPEINSTLIST pipeInstList, void * hPipe);
static PPIPEINSTNODE FindPipeInstNodeWithCommID(PIPEINSTLIST pipeInstList, unsigned long commID);
static PPIPEINSTNODE FindPipeInstNodeWithHandle(PIPEINSTLIST pipeInstList, void * hPipe);
static bool DeleteAllPipeInstNode(PIPEINSTLIST pipeInstList);
//---------------------------------------------------------------------------------------

//---------------------------Other function define---------------------------------------
static bool LaunchPipeInst(PIPEINSTLIST pipeInstList);
static bool UnlaunchPipeInst(PIPEINSTLIST pipeInstList);
//---------------------------------------------------------------------------------------

static PIPEINSTLIST CreatePipeInstList()
{
   PPIPEINSTNODE head = NULL;
   head = (PPIPEINSTNODE)malloc(sizeof(PIPEINSTNODE));
   if(head) 
   {
      head->next = NULL;
      head->pipeInst.commID = 0;
      head->pipeInst.hPipe = NULL;
      head->pipeInst.isConnected = false;
      head->pipeInst.pipeListenThreadHandle = NULL;
      head->pipeInst.pipeListenThreadRun = false;
      head->pipeInst.pipeRecvThreadHandle = NULL;
      head->pipeInst.pipeRecvThreadRun = false;
      head->pipeInst.pipeRecvCB = NULL;
   }
   return head;
}

static void DestroyPipeInstList(PIPEINSTLIST pipeInstList)
{
   PPIPEINSTNODE head = pipeInstList;
   if(NULL == head) return;
   DeleteAllPipeInstNode(head);
   free(head); 
   head = NULL;
}

static PPIPEINSTNODE InsertPipeInstNode(PIPEINSTLIST pipeInstList, void * hPipe)
{
   PPIPEINSTNODE pRet = NULL;
   PPIPEINSTNODE newNode = NULL;
   PPIPEINSTNODE findNode = NULL;
   PPIPEINSTNODE head = pipeInstList;
   if(head == NULL || hPipe == NULL) return pRet;
   findNode = FindPipeInstNodeWithHandle(head, hPipe);
   if(findNode == NULL)
   {
      newNode = (PPIPEINSTNODE)malloc(sizeof(PIPEINSTNODE));
      newNode->pipeInst.commID = 0;
      newNode->pipeInst.hPipe = hPipe;
      newNode->pipeInst.isConnected = true;
      newNode->pipeInst.pipeListenThreadHandle = NULL;
      newNode->pipeInst.pipeListenThreadRun = false;
      newNode->pipeInst.pipeRecvThreadHandle = NULL;
      newNode->pipeInst.pipeRecvThreadRun = false;
      newNode->pipeInst.pipeRecvCB = NULL;

      newNode->next = head->next;
      head->next = newNode;
      pRet = newNode;
   }
   return pRet;
}

static PPIPEINSTNODE FindPipeInstNodeWithCommID(PIPEINSTLIST pipeInstList, unsigned long commID)
{
   PPIPEINSTNODE head = pipeInstList;
   PPIPEINSTNODE findNode = NULL;
   if(head == NULL) return findNode;
   findNode = head->next;
   while(findNode)
   {
      if(findNode->pipeInst.commID == commID) break;
      else
      {
         findNode = findNode->next;
      }
   }

   return findNode;
}

static PPIPEINSTNODE FindPipeInstNodeWithHandle(PIPEINSTLIST pipeInstList, void * hPipe)
{
   PPIPEINSTNODE head = pipeInstList;
   PPIPEINSTNODE findNode = NULL;
   if(head == NULL) return findNode;
   findNode = head->next;
   while(findNode)
   {
      if(findNode->pipeInst.hPipe == hPipe) break;
      else
      {
         findNode = findNode->next;
      }
   }

   return findNode;
}

static bool DeleteAllPipeInstNode(PIPEINSTLIST pipeInstList)
{
   bool bRet = false;
   PPIPEINSTNODE delNode = NULL;
   PPIPEINSTNODE head = pipeInstList;
   if(head == NULL) return bRet;

   delNode = head->next;
   while(delNode)
   {
      head->next = delNode->next;

      if(delNode->pipeInst.pipeRecvThreadHandle)
      {
         delNode->pipeInst.pipeRecvThreadRun = false;
         app_os_thread_join(delNode->pipeInst.pipeRecvThreadHandle);
         delNode->pipeInst.pipeRecvThreadHandle = NULL;
      }

      if(delNode->pipeInst.hPipe)
      {
         close(delNode->pipeInst.hPipe);
         delNode->pipeInst.hPipe = NULL;
      }

      free(delNode);
      delNode = head->next;
   }

   bRet = true;
   return bRet;
}

static CAGENT_PTHREAD_ENTRY(PipeInstRecvThreadStart, args)
{
   PPIPEINST  pPipeInst = (PPIPEINST)args;
   char recvBuf[DEF_IPC_OUTPUT_BUF_SIZE] = {0};
   long recvCnt = 0;
   bool bRet = false;
   while(pPipeInst->pipeRecvThreadRun)
   {
      if(pPipeInst->hPipe && pPipeInst->isConnected)
      {
         if(recvCnt > 0)
         {
            memset(recvBuf, 0, sizeof(recvBuf));
            recvCnt = 0;
         }
         recvCnt = read(pPipeInst->hPipe, recvBuf, sizeof(recvBuf));
         if(recvCnt > 0)
         {
            PIPCMSG pIPCMsg = (PIPCMSG)recvBuf;
            switch(pIPCMsg->msgType)
            {
            case INTER_MSG:
               {
                  IPCINTERMSG interMsg;
                  printf("read INTER_MSG: %d\r\n", recvCnt);
                  if(pIPCMsg->msgContentLen == sizeof(IPCINTERMSG))
                  {
                     memcpy(&interMsg, pIPCMsg->msgContent, sizeof(IPCINTERMSG));
                     if(interMsg.interCmdKey == IPC_CONNECT)
                     {
                        pPipeInst->commID = interMsg.interParams.commID;
                     }
                  }
                  break;
               }
            case USER_MSG:
               {
                  char * userRecvBuf = pIPCMsg->msgContent;
                  DWORD userRecvCnt = pIPCMsg->msgContentLen;
		  printf("read USER_MSG: %d\r\n", recvCnt);
                  if(pPipeInst->pipeRecvCB)
                  {
                     pPipeInst->pipeRecvCB(userRecvBuf, userRecvCnt);
                  }
                  break;
               }
            default: break;
            }
         }
      }
      app_os_sleep(10);
   }
   return 0;
}

static bool LaunchPipeInst(PIPEINSTLIST pipeInstList)
{
   bool bRet = true;
   if(pipeInstList == NULL) return false;
   {
      PPIPEINSTNODE  pCurPipeInstNode = pipeInstList->next;
      while(pCurPipeInstNode)
      {
         pCurPipeInstNode->pipeInst.pipeRecvThreadRun = true;
	 if (app_os_thread_create(&pCurPipeInstNode->pipeInst.pipeRecvThreadHandle, PipeInstRecvThreadStart, &pCurPipeInstNode->pipeInst) != 0)
         {
            pCurPipeInstNode->pipeInst.pipeRecvThreadRun = false;
            bRet = false;
            break;
         }
         pCurPipeInstNode = pCurPipeInstNode->next;
      }
   }
   return bRet;
}

static bool UnlaunchPipeInst(PIPEINSTLIST pipeInstList)
{
   bool bRet = false;
   if(pipeInstList == NULL) return bRet;
   {
      PPIPEINSTNODE  pCurPipeInstNode = pipeInstList->next;
      while(pCurPipeInstNode)
      {
         if(pCurPipeInstNode->pipeInst.pipeRecvThreadHandle)
         {
            pCurPipeInstNode->pipeInst.pipeRecvThreadRun = false;
	    app_os_thread_join(pCurPipeInstNode->pipeInst.pipeRecvThreadHandle);
            pCurPipeInstNode->pipeInst.pipeRecvThreadHandle = NULL;
         }
         pCurPipeInstNode = pCurPipeInstNode->next;
      }
      bRet = true;
   }
   return bRet;
}

int NamedPipeServerInit(char * pName, unsigned long instCnt)
{
   bool bRet = false;
   unsigned long i = 0;
   void * hPipe = NULL;
   if(pName == NULL || instCnt <= 0) return bRet;
   PipeInstList = CreatePipeInstList();
   if(PipeInstList == NULL) return bRet;
   FIFOName = strdup(pName);   
   mkfifo(pName, 0666);
   hPipe = open(pName, O_NONBLOCK|O_RDONLY);
   InsertPipeInstNode(PipeInstList, hPipe);
   bRet = LaunchPipeInst(PipeInstList);
done:
   if(!bRet) NamedPipeServerUninit();
   return bRet;
}

void NamedPipeServerUninit()
{
   UnlaunchPipeInst(PipeInstList);
   DestroyPipeInstList(PipeInstList);
   unlink(FIFOName);
   free(FIFOName);
   FIFOName = NULL;
}

int IsChannelConnect(unsigned long commID)
{
   bool bRet = false;
   PPIPEINSTNODE findNode = NULL;
   if(PipeInstList == NULL) return bRet;
   findNode = FindPipeInstNodeWithCommID(PipeInstList, commID);
   if(findNode)
   {
      bRet = findNode->pipeInst.isConnected;
   }
   return bRet;
}

int NamedPipeServerRegRecvCB(unsigned long commID, PPIPERECVCB pipeRecvCB)
{
   bool bRet = false;
   if(pipeRecvCB == NULL || PipeInstList == NULL) return bRet;
   if(commID > 0)
   {
      PPIPEINSTNODE findNode = FindPipeInstNodeWithCommID(PipeInstList, commID);
      if(findNode)
      {
         findNode->pipeInst.pipeRecvCB = pipeRecvCB;
         bRet = true;
      }
   }
   else
   {
      PPIPEINSTNODE curNode = PipeInstList->next;
      while(curNode)
      {
         curNode->pipeInst.pipeRecvCB = pipeRecvCB;
         curNode = curNode->next;
      }
      bRet = true;
   }
   return bRet;
}
