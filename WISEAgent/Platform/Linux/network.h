#ifndef _CAGENT_NETWORK_H_
#define _CAGENT_NETWORK_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <fcntl.h>

#define socket_handle  int

#ifndef INVALID_SOCKET
#define INVALID_SOCKET  ~0
#endif

#ifndef BOOL
#define BOOL  	int 
#define  FALSE  0
#define  TRUE   1
#endif

//************************Define private fuction return code************************
typedef enum {
   network_success = 0,               // No error. 
   network_no_init, 
   network_callback_null,
   network_callback_error,
   network_no_connnect,
   network_connect_error,
   network_init_error,   
   network_waitsock_timeout = 0x10,
   network_waitsock_error,
   network_act_unrecognized,
   network_terminate_error,
   network_send_data_error,
   network_reg_action_comm_error,
   network_reg_callback_comm_error,
   network_report_agentinfo_error,  
   network_send_action_data_error,
   network_send_callback_response_data_error,
   network_reg_willmsg_error,
   network_send_willmsg_error,
   // S--Added by wang.nan--S
   network_mindray_main_error,
   // E--Added by wang.nan--E
} network_status_t;

typedef enum{
   network_waitsock_read     = 0x01,
   network_waitsock_write    = 0x02,
   network_waitsock_rw       = 0x03,
}network_waitsock_mode_t;

int app_get_host_name(char * phostname, int size);
int app_get_ip(char * ipaddr, int size);
int app_get_mac(char * macstr);
int app_get_mac_ex(char * macstr);
int app_get_mac_list(char macsStr[][20], int n);
int app_get_socket_ip(int socket, char* clientip, int size);
BOOL app_os_SAWakeOnLan(char * mac, int size);
#endif

