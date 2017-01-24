#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#include <mds_udp_ipc.h>

#define MAX_MSG_LOOP_CNT                20
#define MSG_LOOP_DEFAULT_TIMEOUT_SEC    10

// thread argument
typedef struct msg_loop_arg
{
    int port_num;
    int thread_index;
	int (*msg_recv_proc)(const unsigned char* recv_msg, const int recv_msg_len, unsigned char* resp_msg, int* resp_msg_len);
}MSG_LOOP_ARG_T;

// thread management
typedef struct mgr_msg_loop
{
    int port_num;
    int thread_run;
    pthread_t thread_var_svr_msg_loop;
    MSG_LOOP_ARG_T loop_arg;
}MGR_MSG_LOOP_T;

static MGR_MSG_LOOP_T g_mgr_msg_loop[MAX_MSG_LOOP_CNT];

static void _loop_mgr_init()
{
    static int init_flag = 0;
    int i = 0;
    if (init_flag == 1)
        return;
    
    for ( i = 0 ; i < MAX_MSG_LOOP_CNT; i++)
        memset(&g_mgr_msg_loop[i], 0x00, sizeof(MGR_MSG_LOOP_T));
    
    init_flag = 1;
}
static int _get_thread_idx(int port_num)
{
    int i = 0;
    int msg_loop_idx = -1;

    for ( i = 0 ; i < MAX_MSG_LOOP_CNT ; i++)
    {
        if ( g_mgr_msg_loop[i].port_num == port_num ) 
        {
            msg_loop_idx = i;
            break;
        }
    }

    if ( msg_loop_idx == -1 )
    {
        for ( i = 0 ; i < MAX_MSG_LOOP_CNT ; i++)
        {
            if ( g_mgr_msg_loop[i].port_num == 0 ) 
            {
                g_mgr_msg_loop[i].port_num = port_num;
                msg_loop_idx = i;
                break;
            }
        }
    }

    return msg_loop_idx;
}


// msg loop
void * _svr_msg_loop(void* arg)
{
    MSG_LOOP_ARG_T loop_arg = *((MSG_LOOP_ARG_T*) (arg));
    int thread_idx = loop_arg.thread_index;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    int option = 1;
    struct timeval tval;

    int svr_sock = 0;

    socklen_t  client_addr_size = 0;

    unsigned char buff_rcv[IPC_BUFF_SIZE] = {0,};
    unsigned char buff_resp[IPC_BUFF_SIZE] = {0,};
    int buff_resp_len = 0;
    svr_sock  = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // set time out sec
    tval.tv_sec = MSG_LOOP_DEFAULT_TIMEOUT_SEC;
    tval.tv_usec = 0;

    // set server addr..
    memset( &server_addr, 0, sizeof( server_addr));
    server_addr.sin_family     = AF_INET;
    server_addr.sin_port       = htons( loop_arg.port_num );
    // server_addr.sin_addr.s_addr= inet_addr( SVR_LOCALHOST_ADDR );
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // set sock option for borad cast
    if ( setsockopt(svr_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0 )
        printf( "[server:%d/%d/%d] set sckopt SOL_SOCKET error. \n", getpid(), thread_idx, loop_arg.port_num);
    if ( setsockopt(svr_sock, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof(struct timeval)) < 0 )
        printf( "[server:%d/%d/%d] set sckopt SO_RCVTIMEO error. \n", getpid(), thread_idx, loop_arg.port_num);

    if( bind( svr_sock, (struct sockaddr*)&server_addr, sizeof( server_addr) ) == -1 )
    {
        printf( "[server:%d/%d/%d] bind Error. \n", getpid(), thread_idx, loop_arg.port_num);
        close(svr_sock);
        return NULL;
    }

    printf( "[server:%d/%d/%d] start. \n", getpid(), thread_idx, loop_arg.port_num);

    while( g_mgr_msg_loop[loop_arg.thread_index].thread_run )
    {
        client_addr_size  = sizeof( client_addr );
        int read_size = 0;
        
        memset(&buff_rcv, 0x00, IPC_BUFF_SIZE);
        
        printf("debug : %s() - %d\r\n", __func__, __LINE__);
		printf( "[server %d] recv start...\n", getpid());
		
        if ( (read_size = recvfrom( svr_sock, buff_rcv, IPC_BUFF_SIZE, 0 , ( struct sockaddr*)&client_addr, &client_addr_size)) <= 0 )
        {
            printf( "[server %d] receive timeout\n", getpid());
            loop_arg.msg_recv_proc(NULL, 0, NULL, NULL);
        }
        else
        {
            char ip_buffer[256] = {0,};
            memset(buff_resp, 0x00, IPC_BUFF_SIZE);
            printf("debug : %s() - %d\r\n", __func__, __LINE__);
            inet_ntop(AF_INET, &(client_addr.sin_addr), ip_buffer, 256);
            printf("debug : %s() - %d\r\n", __func__, __LINE__);
            printf( "[server:%d/%d/%d] recv data --> [%s] / [%d]. \n", getpid(), thread_idx, loop_arg.port_num, buff_rcv, read_size);
            
            loop_arg.msg_recv_proc(buff_rcv, read_size, buff_resp, &buff_resp_len);
            printf("debug : %s() - %d\r\n", __func__, __LINE__);
            if (( buff_resp_len > 0 ) && (buff_resp != NULL))
            {
                printf("debug : %s() - %d\r\n", __func__, __LINE__);
                printf( "[server:%d/%d/%d] resp data 1 --> [%s] / [%d]. \r\n", getpid(), thread_idx, loop_arg.port_num, buff_resp, buff_resp_len);
                sendto(svr_sock, buff_resp, buff_resp_len, MSG_DONTWAIT, (struct sockaddr*)&client_addr, sizeof(client_addr));
				printf( "[server:%d/%d/%d] resp data 2 --> [%s] / [%d]. \r\n", getpid(), thread_idx, loop_arg.port_num, buff_resp, buff_resp_len);
                printf("debug : %s() - %d\r\n", __func__, __LINE__);
            }
            else
            {
                printf("debug : %s() - %d\r\n", __func__, __LINE__);
                printf( "[server:%d/%d/%d] no resp data \r\n", getpid(), thread_idx, loop_arg.port_num );
            }
        }
    }
    printf("debug : %s() - %d\r\n", __func__, __LINE__);
    printf( "[server:%d/%d/%d] end. \n", getpid(), thread_idx, loop_arg.port_num);
    close(svr_sock);

}


int udp_ipc_data_recv_timeout(int port_num, int timeout_sec, unsigned char* buff, int buff_len)
{
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    int option = 1;
    struct timeval tval;

    int svr_sock = 0;

    socklen_t  client_addr_size = 0;
    int read_size = 0;

    unsigned char buff_rcv[IPC_BUFF_SIZE] = {0,};
//    unsigned char buff_resp[IPC_BUFF_SIZE] = {0,};
//    int buff_resp_len = 0;
    svr_sock  = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // set time out sec
    tval.tv_sec = timeout_sec;
    tval.tv_usec = 0;

    // set server addr..
    memset( &server_addr, 0, sizeof( server_addr));
    server_addr.sin_family     = AF_INET;
    server_addr.sin_port       = htons( port_num );
    // server_addr.sin_addr.s_addr= inet_addr( SVR_LOCALHOST_ADDR );
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // set sock option for borad cast
    if ( setsockopt(svr_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0 )
        printf( "[recv data:%d/%d] set sckopt SOL_SOCKET error. \n", getpid(), port_num);
    if ( setsockopt(svr_sock, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof(struct timeval)) < 0 )
        printf( "[recv data:%d/%d] set sckopt SO_RCVTIMEO error. \n", getpid(), port_num);

    if( bind( svr_sock, (struct sockaddr*)&server_addr, sizeof( server_addr) ) == -1 )
    {
        printf( "[server:%d/%d] bind Error. \n", getpid(), port_num);
        close(svr_sock);
        return -1;
    }

    {
        client_addr_size  = sizeof( client_addr );
        
        printf( "[server:%d/%d] wait data.. timeout [%d]. \n", getpid(), port_num, timeout_sec);

        if ( (read_size = recvfrom( svr_sock, buff_rcv, IPC_BUFF_SIZE, 0 , ( struct sockaddr*)&client_addr, &client_addr_size)) <= 0 )
        {
            close(svr_sock);
            return -1;
        }
        else
        {
            if (( buff != NULL ) && (buff_len > read_size))
                memcpy(buff, buff_rcv, read_size);
            // char ip_buffer[256] = {0,};    
            //inet_ntop(AF_INET, &(client_addr.sin_addr), ip_buffer, 256);
            printf( "[server:%d/%d] recv data --> [%s] / [%d]. \n", getpid(), port_num, buff_rcv, read_size);
            
        }
    }
    close(svr_sock);
    return read_size;
}


void udp_ipc_server_start(int port_num, int (*msg_recv_proc)(const unsigned char* recv_msg, const int recv_msg_len, unsigned char* resp_msg, int* resp_msg_len))
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 300000);

    int msg_loop_idx = _get_thread_idx(port_num);
    
    _loop_mgr_init();
    
    //printf("msg_loop_idx is [%d]\r\n",msg_loop_idx);
    g_mgr_msg_loop[msg_loop_idx].loop_arg.port_num = port_num;
    g_mgr_msg_loop[msg_loop_idx].loop_arg.msg_recv_proc = msg_recv_proc;
    g_mgr_msg_loop[msg_loop_idx].loop_arg.thread_index = msg_loop_idx;

    g_mgr_msg_loop[msg_loop_idx].thread_run = 1;

    if(pthread_create(&g_mgr_msg_loop[msg_loop_idx].thread_var_svr_msg_loop, &attr, _svr_msg_loop, (void*) &g_mgr_msg_loop[msg_loop_idx].loop_arg) != 0) {
        printf("create pthread error!!\r\n");
		return;
	}
}

void udp_ipc_server_end(int port_num)
{
    int msg_loop_idx = _get_thread_idx(port_num);

    memset( &g_mgr_msg_loop[msg_loop_idx], 0x00, sizeof(MGR_MSG_LOOP_T));
//    g_mgr_msg_loop[msg_loop_idx].thread_run = 0;
    
}
