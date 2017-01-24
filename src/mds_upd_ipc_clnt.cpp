#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>


#include <mds_udp_ipc.h>


#define REUSE_CLNT_SOCK

static int g_sock[MAX_REUSE_SOCK_CNT] = {0,};

static int _get_sock_idx(int port_num)
{
    int i = 0;
    
    static int g_sock_mgr[MAX_REUSE_SOCK_CNT] = {0,};

    for ( i = 0 ; i < MAX_REUSE_SOCK_CNT ; i ++ )
    {
        if ( g_sock_mgr[i] == port_num )
        {
            return i;
        }
    }
    
    for ( i = 0 ; i < MAX_REUSE_SOCK_CNT ; i ++ )
    {
        if ( g_sock_mgr[i] == 0 )
        {
            g_sock_mgr[i] = port_num;
            return i;
        }
    }
    
    return 0;
    
}

static int _init_ipc_sock(int recv_timeout, int port_num)
{
    int sock;
    int option = 1;
    
    struct timeval tval;
    
    int idx = _get_sock_idx(port_num);
    printf("[DBUG:MSG] %s() : port idx [%d]\r\n", __func__, idx);
    
    if ( g_sock[idx] > 0 )
        return g_sock[idx];
    
    tval.tv_sec = recv_timeout;
    tval.tv_usec = 0;
    
    sock  = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if( -1 == sock)
    {
        printf("[DBUG:MSG] %s() : sock create fail..\r\n",__func__);
        return -1;
    }


    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &option, sizeof(option));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof(struct timeval));
    
    //setsockopt(sock, SOL_SOCKET, SO_DONTROUTE, &option, sizeof(option));
    g_sock[idx] = sock;
    
    return g_sock[idx];
}

static int _close_ipc_sock(int port_num)
{
    int idx = _get_sock_idx(port_num);
    printf("[DBUG:MSG] %s() : port idx [%d]\r\n", __func__, idx);
    
    if ( g_sock[idx] > 0 )
        close(g_sock[idx]);
    g_sock[idx] = -1;
    return 0;
}

// udp.. borad cast msg..
void udp_ipc_broadcast_send(int port_num, const unsigned char* send_data, const int send_data_size)
{

    int sock;
    struct sockaddr_in server_addr;
    //socklen_t server_addr_size = 0;
    //unsigned char buff_rcv[IPC_BUFF_SIZE];

    sock = _init_ipc_sock(0, port_num);
    
    if ( sock < 0 )
    {
        printf("[DBUG:ERROR] %s() : sock open error", __func__);
        _close_ipc_sock(port_num);
        return;
    }
    
    memset( &server_addr, 0, sizeof( server_addr));
    server_addr.sin_family     = AF_INET;
    server_addr.sin_port       = htons( port_num );
    server_addr.sin_addr.s_addr= inet_addr( SVR_LOCALHOST_BROADCAST );
    //server_addr.sin_addr.s_addr=  htonl(INADDR_BROADCAST);
    
    if ( sendto(sock, send_data, send_data_size, MSG_DONTWAIT, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 )
    {
        printf("[DBUG:ERROR] %s() : sock write error", __func__);
        _close_ipc_sock(port_num);
        return;
    }
    
    // 정상종료..
#ifndef REUSE_CLNT_SOCK
    _close_ipc_sock(port_num);
#endif
    
}


int udp_ipc_broadcast_send_recv(int port_num, int time_out, const unsigned char* send_data, const int send_data_size, unsigned char* recv_buff, int recv_buff_size)
{
    int sock;
    struct sockaddr_in server_addr;
    socklen_t server_addr_size = 0;
    //char buff_rcv[IPC_BUFF_SIZE];

    char svr_ip_buffer[256] = {0,};
    
    int recv_size = 0;
    
    sock = _init_ipc_sock(time_out, port_num);
    
    if ( sock < 0 )
    {
        printf("[DBUG:ERROR] %s() : sock open error", __func__);
        _close_ipc_sock(port_num);
        return -1;
    }
    
    memset( &server_addr, 0, sizeof( server_addr));
    server_addr.sin_family     = AF_INET;
    server_addr.sin_port       = htons( port_num );
    server_addr.sin_addr.s_addr= inet_addr( SVR_LOCALHOST_BROADCAST );
    
    if ( sendto(sock, send_data, send_data_size, MSG_DONTWAIT, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 )
    {
        printf("[DBUG:ERROR] %s() : sock write error", __func__);
        _close_ipc_sock(port_num);
        return -1;
    }
    
    server_addr_size = sizeof(server_addr);
    recv_size = recvfrom( sock, recv_buff, recv_buff_size, 0 , (struct sockaddr*)&server_addr, &server_addr_size);
    
    if ( recv_size > 0 )
    {
        inet_ntop(AF_INET, &(server_addr.sin_addr), svr_ip_buffer, 256);
        printf("[DBUG:MSG] %s() - (%s): read [%s] / [%d] \r\n", __func__ , svr_ip_buffer, recv_buff, recv_size );
    }

#ifndef REUSE_CLNT_SOCK
    _close_ipc_sock(port_num);
#endif

    return recv_size;
}




