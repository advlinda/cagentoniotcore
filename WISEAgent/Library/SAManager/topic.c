#include "topic.h"
#include "platform.h"

static struct samanager_topic_entry *subscribe_topics = NULL;

struct samanager_topic_entry * samanager_Topic_FirstTopic()
{
	struct samanager_topic_entry *target = subscribe_topics;
	return target;
}

struct samanager_topic_entry * samanager_Topic_LastTopic()
{
	struct samanager_topic_entry *topic = subscribe_topics;
	struct samanager_topic_entry *target = NULL;
	//printf("Find Last\n"); 
	while(topic != NULL)
	{
		//printf("Topic Name: %s\n", topic->name);
		target = topic;
		topic = topic->next;
	}
	return target;
}

struct samanager_topic_entry * samanager_Topic_AddTopic(char const * topicname, samanager_topic_msg_cb_func_t cbfunc)
{
	struct samanager_topic_entry *topic = NULL;
	
	topic = (struct samanager_topic_entry *)malloc(sizeof(*topic));
	
	if (topic == NULL)
		return NULL;
	
	//snprintf(topic->name, strlen(topic->name), "%s", topicname);

	strncpy(topic->name, topicname, strlen(topicname)+1);
	topic->callback_func = cbfunc;
	topic->next = NULL;	
	topic->prev = NULL;	

	if(subscribe_topics == NULL)
	{
		subscribe_topics = topic;
	} else {
		struct samanager_topic_entry *lasttopic = samanager_Topic_LastTopic();
		//printf("Last Topic Name: %s\n", lasttopic->name);
		lasttopic->next = topic;
		topic->prev = lasttopic;
	}
	return topic;
}

void samanager_Topic_RemoveTopic(char* topicname)
{
	struct samanager_topic_entry *topic = subscribe_topics;
	struct samanager_topic_entry *target = NULL;
	//printf("Remove Topic\n");
	while(topic != NULL)
	{
		//printf("Topic Name: %s\n", topic->name);
		if(strcmp(topic->name, topicname) == 0)
		{
			if(subscribe_topics == topic)
				subscribe_topics = topic->next;
			if(topic->prev != NULL)
				topic->prev->next = topic->next;
			if(topic->next != NULL)
				topic->next->prev = topic->prev;
			target = topic;
			break;
		}
		topic = topic->next;
	}
	if(target!=NULL)
	{
		free(target);
		target = NULL;
	}
}

struct samanager_topic_entry * samanager_Topic_FindTopic(char const * topicname)
{
	struct samanager_topic_entry *topic = subscribe_topics;
	struct samanager_topic_entry *target = NULL;

	//printf("Find Topic\n");
	while(topic != NULL)
	{
		//printf("Topic Name: %s\n", topic->name);
		if(strcmp(topic->name, topicname) == 0)
		{
			target = topic;
			break;
		}
		topic = topic->next;
	}
	return target;
}