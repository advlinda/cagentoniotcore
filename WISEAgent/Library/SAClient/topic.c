#include "topic.h"
#include "platform.h"

static struct topic_entry *subscribe_topics = NULL;
struct topic_entry * Topic_FirstTopic()
{
	struct topic_entry *target = subscribe_topics;
	return target;
}

struct topic_entry * Topic_LastTopic()
{
	struct topic_entry *topic = subscribe_topics;
	struct topic_entry *target = NULL;
	//printf("Find Last\n"); 
	while(topic != NULL)
	{
		//printf("Topic Name: %s\n", topic->name);
		target = topic;
		topic = topic->next;
	}
	return target;
}

struct topic_entry * Topic_AddTopic(char const * topicname, topic_msg_cb_func_t cbfunc)
{
	struct topic_entry *topic = NULL;
	
	topic = (struct topic_entry *)malloc(sizeof(*topic));
	
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
		struct topic_entry *lasttopic = Topic_LastTopic();
		//printf("Last Topic Name: %s\n", lasttopic->name);
		lasttopic->next = topic;
		topic->prev = lasttopic;
	}
	return topic;
}

void Topic_RemoveTopic(char* topicname)
{
	struct topic_entry *topic = subscribe_topics;
	struct topic_entry *target = NULL;
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

struct topic_entry * Topic_FindTopic(char const * topicname)
{
	struct topic_entry *topic = subscribe_topics;
	struct topic_entry *target = NULL;

	//printf("Find Topic\n");
	while(topic != NULL)
	{
		//printf("Topic Name: %s\n", topic->name);
		if(strchr(topic->name, '+')>0)
		{
			if(strchr(topicname, '+')>0)
			{
				if(strcmp(topic->name, topicname) == 0)
				{
					target = topic;
					break;
				}
			}
			else
			{
				char *delim = "+";
				char *p = NULL;
				char tName[128] = {0};
				bool match = true;
				char* ss = topicname;
				strcpy(tName, topic->name);
				p = strtok(tName, delim);
				while(p)
				{
					ss = strstr(ss, p);
					if(ss > 0)
						ss += strlen(p);
					else
					{
						match = false;
						break;
					}
					p=strtok(NULL,delim);
				}
				if(match)
					target = topic;
			}
		}
		else
		{
			if(strcmp(topic->name, topicname) == 0)
			{
				target = topic;
				break;
			}
		}
		topic = topic->next;
	}
	return target;
}