#ifndef _CAGENT_TOPIC_H_
#define _CAGENT_TOPIC_H_

typedef int (*topic_msg_cb_func_t)(char* topic, void *payload, void *pRev1, void* pRev2);

typedef struct topic_entry
{    
	char name[128];
	topic_msg_cb_func_t callback_func;
	struct topic_entry *prev;
	struct topic_entry *next;
} topic_entry_st;

struct topic_entry * Topic_FirstTopic();
struct topic_entry * Topic_LastTopic();
struct topic_entry * Topic_AddTopic(char const * topicname, topic_msg_cb_func_t cbfunc);
void Topic_RemoveTopic(char* topicname);
struct topic_entry * Topic_FindTopic(char const * topicname);

#endif