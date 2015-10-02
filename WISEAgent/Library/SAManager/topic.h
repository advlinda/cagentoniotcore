#ifndef _SAMANAGER_TOPIC_H_
#define _SAMANAGER_TOPIC_H_

typedef int (*samanager_topic_msg_cb_func_t)(char * const topic, void* const data, const unsigned int datalen, void *pRev1, void* pRev2);

typedef struct samanager_topic_entry
{    
	char name[128];
	samanager_topic_msg_cb_func_t callback_func;
	struct samanager_topic_entry *prev;
	struct samanager_topic_entry *next;
} samanager_topic_entry_st;

struct samanager_topic_entry * samanager_Topic_FirstTopic();
struct samanager_topic_entry * samanager_Topic_LastTopic();
struct samanager_topic_entry * samanager_Topic_AddTopic(char const * topicname, samanager_topic_msg_cb_func_t cbfunc);
void samanager_Topic_RemoveTopic(char* topicname);
struct samanager_topic_entry * samanager_Topic_FindTopic(char const * topicname);

#endif