#ifndef __H_MDS_EXT_EVENT__
#define __H_MDS_EXT_EVENT__

#define UDP_IPC_PORT__TEST_APP          30991
#define UDP_IPC_PORT__CHK_GLOBAL_MEM	30992
#define UDP_IPC_PORT__CHK_APP_MEM       30993

#define UDP_IPC_PORT__GPS_NMEA_DATA_CH  30994
#define UDP_IPC_PORT__GPS_MGR_CH        30995
//#define UDP_IPC_PORT__APP2	            30993
#define IPC_BUFF_SIZE 1024

#define MAX_REUSE_SOCK_CNT      10

//#define SVR_LOCALHOST_ADDR			"127.0.0.1"  // force INADDR_ANY
#define SVR_LOCALHOST_BROADCAST 	"127.255.255.255"

// server side api
void udp_ipc_server_start(int port_num, int (*msg_recv_proc)(const unsigned char* recv_msg, const int recv_msg_len, unsigned char* resp_msg, int* resp_msg_len));
// int msg_recv_proc_test(const char* recv_msg, const int recv_msg_len, unsigned unsigned char* resp_msg, int* resp_msg_len);
void udp_ipc_server_end(int port_num);
int udp_ipc_data_recv_timeout(int port_num, int timeout_sec, unsigned char* buff, int buff_len);

// client side api
void udp_ipc_broadcast_send(int port_num, const unsigned char* send_data, const int send_data_size);
int udp_ipc_broadcast_send_recv(int port_num, int time_out, const unsigned char* send_data, const int send_data_size, unsigned char* recv_buff, int recv_buff_size);


#endif // __H_MDS_EXT_EVENT__
