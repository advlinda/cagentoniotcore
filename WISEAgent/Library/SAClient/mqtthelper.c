#include "mqtthelper.h"
#include "platform.h"
#include "common.h"

struct message {
	struct mosquitto			*mosq;
	struct mosquitto_message	*msg;
};

struct queue {
    CAGENT_MUTEX_TYPE   lock;
    CAGENT_COND_TYPE    wait_room;
    CAGENT_COND_TYPE    wait_data;
    unsigned int		size;
    unsigned int		head;
    unsigned int		tail;
    struct message		**queue;
};

struct recv_ctx{
   void				*threadHandler;
   bool				isThreadRunning;
   struct queue		*msgqueue;
};

MQTT_CONNECT_CALLBACK on_mqtt_connect_cb = NULL;
MQTT_LOSTCONNECT_CALLBACK on_mqtt_lostconnect_cb = NULL;
MQTT_DISCONNECT_CALLBACK on_mqtt_disconnect_cb = NULL;
MQTT_MESSAGE_CALLBACK on_mqtt_message_func = NULL;
static struct recv_ctx g_recvthreadctx;
static CAGENT_MUTEX_TYPE g_publishmutex;

bool message_create(struct message *const msg, struct mosquitto *const mosq, struct mosquitto_message *const msgdata)
{
	if(!msg)
		return false;
	msg->mosq = mosq;

	msg->msg = malloc(sizeof (struct mosquitto_message) + 1);
    if (!msg->msg)
        return false;

	memset(msg->msg, 0, sizeof (struct mosquitto_message) + 1);
	if(mosquitto_message_copy(msg->msg, msgdata) == 0)
		return true;
	else
		return false;
}

void message_free(struct message *const msg)
{
	if(!msg)
		return;

	msg->mosq = NULL;

	 if (!msg->msg)
        return;

	mosquitto_message_free(&msg->msg);
	msg->msg = NULL;

	free(msg);
}

bool queue_init(struct queue *const q, const unsigned int slots)
{
    if (!q || slots < 1U)
        return false;

    q->queue = malloc(sizeof (struct message *) * (size_t)(slots + 1));
    if (!q->queue)
        return false;

    q->size = slots + 1U; 
    q->head = 0U;
    q->tail = 0U;
    if(app_os_mutex_setup(&q->lock)<0)
	{
		free(q->queue);
		q->queue = NULL;
		return false;
	}
	if(app_os_cond_setup(&q->wait_room)<0)
	{
		free(q->queue);
		q->queue = NULL;
		return false;
	}
	if(app_os_cond_setup(&q->wait_data)<0)
	{
		free(q->queue);
		q->queue = NULL;
		return false;
	}
    return true;
}

void queue_uninit(struct queue *const q)
{
	struct message *msg;
	app_os_mutex_lock(&q->lock);
    /*if (q->head == q->tail)
	{
		app_os_mutex_unlock(&q->lock);
		return;
	}*/
	while (q->head != q->tail)
	{
		msg = q->queue[q->tail];
		q->queue[q->tail] = NULL;
		q->tail = (q->tail + 1U) % q->size;
		message_free(msg);
		msg = NULL;
	}
	free(q->queue);
	q->queue = NULL;

	//app_os_cond_signal(&q->wait_data);
	//app_os_cond_signal(&q->wait_room);

	q->size = 0U;
    q->head = 0U;
    q->tail = 0U;
	
    app_os_mutex_unlock(&q->lock);
    app_os_mutex_cleanup(&q->lock);
    app_os_cond_cleanup(&q->wait_room);
    app_os_cond_cleanup(&q->wait_data);
}

struct message *queue_get(struct queue *const q)
{
	struct message *msg = NULL;
	int time = 500;
	int ret = 0;
	if(!q)
		return msg;
    app_os_mutex_lock(&q->lock);
    while (q->head == q->tail)
	{
        ret = app_os_cond_timed_wait(&q->wait_data, &q->lock, &time);
		if(ret != 0)
		{
			app_os_mutex_unlock(&q->lock);
			return msg;
		}
	}

    msg = q->queue[q->tail];
    q->queue[q->tail] = NULL;
    q->tail = (q->tail + 1U) % q->size;

    app_os_cond_signal(&q->wait_room);

    app_os_mutex_unlock(&q->lock);
    return msg;
}

bool queue_put(struct queue *const q, struct message *const msg)
{
	int time = 500;
	int ret = 0;
	if(!q)
		return false;
    app_os_mutex_lock(&q->lock);
	while ((q->head + 1U) % q->size == q->tail)
	{
        ret = app_os_cond_timed_wait(&q->wait_room, &q->lock, &time);
		if(ret != 0)
		{
			app_os_mutex_unlock(&q->lock);
			return false;
		}
	}

    q->queue[q->head] = msg;
	
	q->head = (q->head + 1U) % q->size;

    app_os_cond_signal(&q->wait_data);

    app_os_mutex_unlock(&q->lock);
    return true;
}

static CAGENT_PTHREAD_ENTRY(MessageQueueThread, args)
{
	struct recv_ctx *precvContex = (struct recv_ctx *)args;
	int interval = 100;
	struct message *msg = NULL;
	while(precvContex->isThreadRunning)
	{
		app_os_sleep(interval);
		msg = queue_get(precvContex->msgqueue);
		if(!msg)
			continue;
		if(on_mqtt_message_func != NULL)
		{
			on_mqtt_message_func(msg->mosq, NULL, msg->msg);
		}
		message_free(msg);
		msg = NULL;
	}

	app_os_thread_exit(0);
	return 0;
}

void MQTT_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	if(rc == mqtt_success)
	{
		if(on_mqtt_connect_cb != NULL)
			on_mqtt_connect_cb();
	}
	else
	{
		if(on_mqtt_lostconnect_cb != NULL)
			on_mqtt_lostconnect_cb();
	}
}

void MQTT_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	if(rc == mqtt_success)
	{
		if(on_mqtt_disconnect_cb != NULL)
			on_mqtt_disconnect_cb();
	}
	else
	{
		if(on_mqtt_lostconnect_cb != NULL)
			on_mqtt_lostconnect_cb();
	}
}

void MQTT_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{	
	struct message *newmsg = NULL;
	if(g_recvthreadctx.msgqueue)
	{
		newmsg = malloc(sizeof(struct message));
		message_create(newmsg, mosq, msg);
		if(!queue_put(g_recvthreadctx.msgqueue, newmsg))
		{
			message_free(newmsg);
			newmsg = NULL;
		}
	}
	else
	{
		if(on_mqtt_message_func != NULL)
		{
			on_mqtt_message_func(mosq, obj, msg);
		}
	}
}

struct mosquitto * MQTT_Initialize(char const * devid)
{
	struct mosquitto *mosq = NULL;
	memset(&g_recvthreadctx, 0, sizeof(struct recv_ctx));
	if(app_os_mutex_setup(&g_publishmutex)!=0)
	{
		return mosq;
	}

	mosquitto_lib_init();
	mosq = mosquitto_new(devid, true, NULL);
	if (!mosq)
		return mosq;
	mosquitto_connect_callback_set(mosq, MQTT_connect_callback);
	mosquitto_disconnect_callback_set(mosq, MQTT_disconnect_callback);
	mosquitto_message_callback_set(mosq, MQTT_message_callback);

	
	g_recvthreadctx.msgqueue = malloc(sizeof( struct queue));
	if(g_recvthreadctx.msgqueue)
	{
		if(!queue_init(g_recvthreadctx.msgqueue, 1000))
		{
			free(g_recvthreadctx.msgqueue);
			g_recvthreadctx.msgqueue = NULL;
		}
		else
		{
			g_recvthreadctx.isThreadRunning = true;
			if (app_os_thread_create(&g_recvthreadctx.threadHandler, MessageQueueThread, &g_recvthreadctx) != 0)
			{
				g_recvthreadctx.isThreadRunning = false;
				queue_uninit(g_recvthreadctx.msgqueue);
				free(g_recvthreadctx.msgqueue);
				g_recvthreadctx.msgqueue = NULL;
			}
		}
	}
	
	return mosq;
}

void MQTT_Uninitialize(struct mosquitto *mosq)
{
	struct topic_entry *iter_topic = NULL;
	struct topic_entry *tmp_topic = NULL;
	app_os_mutex_cleanup(&g_publishmutex);

	on_mqtt_connect_cb = NULL;
	on_mqtt_lostconnect_cb = NULL;
	on_mqtt_disconnect_cb = NULL;
	on_mqtt_message_func = NULL;

	if(mosq == NULL)
		return;

	mosquitto_connect_callback_set(mosq, NULL);
	mosquitto_disconnect_callback_set(mosq, NULL);
	mosquitto_message_callback_set(mosq, NULL);

	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	
	if(g_recvthreadctx.isThreadRunning == true)
	{
		g_recvthreadctx.isThreadRunning = false;
		app_os_thread_join(g_recvthreadctx.threadHandler);
		g_recvthreadctx.threadHandler = NULL;

		app_os_sleep(500);
	}

	if(g_recvthreadctx.msgqueue)
	{
		queue_uninit(g_recvthreadctx.msgqueue);
		free(g_recvthreadctx.msgqueue);
		g_recvthreadctx.msgqueue = NULL;
	}

}

void MQTT_Callback_Set(struct mosquitto *mosq, void* connect_cb, void* lostconnect_cb, void* disconnect_cb, void* message_cb)
{
	if(mosq == NULL)
		return;
	on_mqtt_connect_cb = connect_cb;
	on_mqtt_lostconnect_cb = lostconnect_cb;
	on_mqtt_disconnect_cb = disconnect_cb;
	on_mqtt_message_func = message_cb;
}

int MQTT_SetTLS(struct mosquitto *mosq, const char *cafile, const char *capath, const char *certfile, const char *keyfile, int (*pw_callback)(char *buf, int size, int rwflag, void *userdata))
{
	int result = MOSQ_ERR_SUCCESS;
	if(mosq == NULL)
		return MOSQ_ERR_INVAL;
	mosquitto_tls_insecure_set(mosq, true);
	result = mosquitto_tls_set(mosq, cafile, capath, certfile, keyfile, pw_callback);
	return result;
}

int MQTT_SetTLSPSK(struct mosquitto *mosq, const char *psk, const char *identity, const char *ciphers)
{
	int result = MOSQ_ERR_SUCCESS;
	if(mosq == NULL)
		return MOSQ_ERR_INVAL;
	result = mosquitto_tls_psk_set(mosq, psk, identity, ciphers) ;
	return result;
}

int MQTT_Connect(struct mosquitto *mosq, char const * ip, int port, char const * username, char const * password, int keepalive, char* willtopic, const void *willmsg, int msglen )
{
	int result = MOSQ_ERR_SUCCESS;
	if(mosq == NULL)
		return MOSQ_ERR_INVAL;
	if( username!= NULL && password != NULL)
		mosquitto_username_pw_set(mosq,username,password);

	mosquitto_will_clear(mosq);

	if(willmsg != NULL) {
		mosquitto_will_set(mosq, willtopic, msglen, willmsg, 0, false);
	}
	result = mosquitto_connect(mosq, ip, port, keepalive);
	if(result == MOSQ_ERR_SUCCESS)
	{
		mosquitto_loop_start(mosq);
	}
	return result;
}

void MQTT_Disconnect(struct mosquitto *mosq)
{
	if(mosq == NULL)
		return;
	mosquitto_loop(mosq, 0, 1);
	if(mosquitto_disconnect(mosq) == MOSQ_ERR_SUCCESS)
		printf("MQTT Disconnected\n");
	mosquitto_loop(mosq, 0, 1);
	mosquitto_loop_stop(mosq, false);
}

int MQTT_Publish(struct mosquitto *mosq,  char* topic, const void *msg, int msglen, int qos, bool retain)
{
	int result = MOSQ_ERR_SUCCESS;
	if(mosq == NULL)
		return MOSQ_ERR_INVAL;
	app_os_mutex_lock(&g_publishmutex);
		
	//if(mosquitto_loop(mosq, -1, 1)!=MOSQ_ERR_SUCCESS)
	//	return false;
	result = mosquitto_publish(mosq, NULL, topic, msglen, msg, qos, retain);
	app_os_mutex_unlock(&g_publishmutex);
	return result;
}

int MQTT_Subscribe(struct mosquitto *mosq,  char* topic, int qos)
{
	int result = MOSQ_ERR_SUCCESS;
	if(mosq == NULL)
		return MOSQ_ERR_INVAL;
	result = mosquitto_subscribe(mosq, NULL, topic, qos);
	return result;
}

void MQTT_Unsubscribe(struct mosquitto *mosq,  char* topic)
{
	if(mosq == NULL)
		return;

	mosquitto_unsubscribe(mosq, NULL, topic);
}

int MQTT_GetSocket(struct mosquitto *mosq)
{
	if(mosq == NULL)
		return -1;
	return mosquitto_socket(mosq);
}