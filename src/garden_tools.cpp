#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mds_logd.h"
#include "garden_tools.h"
#include "mds_udp_ipc.h"
#include "gps_ipc.h"

//#include "test_android_gps.h"
// ------------------------------------------------
int garden_force_stop = 0;
int garden_entry (int argc, char *argv[]);
pthread_t garden_mgr_tid = 0;
// ------------------------------------------------
static int _devide_argument(char* buff, int buff_len, char* argv[]);
static int _gps_start(int stat);
static int _gps_stop();
void mds_api_stackdump_init(void);

#define MAX_RET_BUFF_SIZE 1024

#define DEBUG_PRINT_INTERVAL_SEC    10
// ------------------------------------------------
// nmea call back
// ------------------------------------------------
void mds_gps_tools_nmea_callback(long long int timestamp, const char *nmea, int length)
{
    static char send_buff[1024] = {0,};
    static int send_to_size = 0;

    static int print_interval = 0;

    int cur_sec = timestamp % 10000;
    static int last_sec = cur_sec;
    cur_sec = cur_sec / 1000;
    printf("mds_gps_tools_nmea_callback++\r\n");
    if ( last_sec != cur_sec )
    {
        // send to data..
        last_sec = cur_sec;
        // send data...
         //MDS_LOGI(eSVC_GPS, "[GPSMGR] gps data send : length %d \r\n",send_to_size);
         udp_ipc_broadcast_send(UDP_IPC_PORT__GPS_NMEA_DATA_CH, (unsigned char*)send_buff, send_to_size);

        send_to_size = 0;
        memset(send_buff, 0x00, 1024);
    }
    else
    {
         if(nmea != 0)
         {
            if ( (strstr(nmea, "GPGSV") != NULL ) || (strstr(nmea, "GPRMC") != NULL ) || (strstr(nmea, "GPGGA") != NULL ) || (strstr(nmea, "GPGSA") != NULL ) )
                send_to_size += sprintf(send_buff+send_to_size, "%s\r", nmea);

            if (strstr(nmea, "GPRMC") != NULL )
            {
                if ( print_interval++ % DEBUG_PRINT_INTERVAL_SEC)
                {
                    MDS_LOGI(eSVC_GPS, "[GPSMGR] %s\r\n",nmea);
                    print_interval = 0;
                }
            }
         }
    }

}

// ------------------------------------------------
// ------------------------------------------------
void *gps_tool_thread (void *args)
{
    int stop_time = 9999;
    while(stop_time--)
    {
        //MDS_LOGI(eSVC_GPS,"gps test thread... %d\r\n", stop_time);
        sleep(3);
    }
    //garden_force_stop = 0;
}

void *garden_mgr_thread (void *args)
{
    int gps_type = *((int *) args);
    free(args);

    MDS_LOGI(eSVC_GPS,"gps mgr thread start type :: %d\r\n", gps_type);

    _gps_start(gps_type);
    MDS_LOGI(eSVC_GPS,"gps mgr thread end type :: %d\r\n", gps_type);

}

int gps_start(int gps_type)
{
    if ( garden_mgr_tid != 0)
        return -1;
    int* gpt_type_p = (int*)malloc(sizeof(int));
    *gpt_type_p = gps_type;

    pthread_create(&garden_mgr_tid, NULL, garden_mgr_thread, (void* )gpt_type_p);
}

void gps_stop()
{
    MDS_LOGI(eSVC_GPS,"gps mgr thread stop start...\r\n");
    if ( garden_mgr_tid == 0)
        return ;
    _gps_stop();
    void *ignored;
    pthread_join (garden_mgr_tid, &ignored);
    garden_mgr_tid = 0;
    MDS_LOGI(eSVC_GPS,"gps mgr thread stop end...\r\n");
}


int msg_recv_proc_gps_tools(const unsigned char* recv_msg, const int recv_msg_len, unsigned char* resp_msg, int* resp_msg_len)
{
    char resp_buff[128] = {0,};
    int msg_size = 0;
    if (( recv_msg_len == 0 ) || (recv_msg == NULL))
    {
        //MDS_LOGI(eSVC_GPS,"%s()-(%d) : recv timeout...\r\n",__func__,__LINE__);
        return 0;
    }
    printf("msg_recv_proc_gps_tools call \r\n");
    //MDS_LOGI(eSVC_GPS,"%s()-(%d) : recv data [%s] / [%d] ...\r\n",__func__,__LINE__, recv_msg, recv_msg_len);

    // resp msg init.
    *resp_msg_len = 0;
    //resp_msg = (unsigned char*)resp_buff;
    
    if (strcmp((char*)recv_msg, GPS_IPC_MSG__GET_GPS_STAT) == 0)
    {
        int run_flag = 0;
        
        MDS_LOGT(eSVC_GPS,"%s()-(%d) : GPS_IPC_MSG__GET_GPS_STAT ...\r\n",__func__,__LINE__);

        if ( garden_mgr_tid == 0 )
            run_flag = 0;
        else
            run_flag = 1;
        
        *resp_msg_len = sprintf((char*)resp_msg, "%s,%d,OK", GPS_IPC_MSG__GET_GPS_STAT, run_flag);
        
        //memcpy(&resp_msg, resp_buff, msg_size);
    }
    else if (strcmp((char*)recv_msg, GPS_IPC_MSG__SET_GPS_COLD_START) == 0)
    {
        MDS_LOGT(eSVC_GPS,"%s()-(%d) : GPS_IPC_MSG__SET_GPS_COLD_START ...\r\n",__func__,__LINE__);
        gps_start(GPS_TYPE_COLD_START);
        *resp_msg_len = sprintf((char*)resp_msg,  "%s,OK", GPS_IPC_MSG__SET_GPS_COLD_START);
        
    }
    else if (strcmp((char*)recv_msg, GPS_IPC_MSG__SET_GPS_WARM_START) == 0)
    {
        MDS_LOGT(eSVC_GPS,"%s()-(%d) : GPS_IPC_MSG__SET_GPS_WARM_START ...\r\n",__func__,__LINE__);
        gps_start(GPS_TYPE_WARM_START);
        *resp_msg_len = sprintf((char*)resp_msg, "%s,OK", GPS_IPC_MSG__SET_GPS_WARM_START);
    }
    else if (strcmp((char*)recv_msg, GPS_IPC_MSG__SET_GPS_STOP) == 0)
    {
        MDS_LOGT(eSVC_GPS,"%s()-(%d) : GPS_IPC_MSG__SET_GPS_STOP ...\r\n",__func__,__LINE__);
        gps_stop();
        *resp_msg_len = sprintf((char*)resp_msg,  "%s,OK", GPS_IPC_MSG__SET_GPS_STOP);
    }
    else if (strcmp((char*)recv_msg, GPS_IPC_MSG__SET_GPS_RESET_COLD_BOOT) == 0)
    {
        MDS_LOGT(eSVC_GPS,"%s()-(%d) : GPS_IPC_MSG__SET_GPS_RESET_COLD_BOOT ...\r\n",__func__,__LINE__);
        gps_stop();
        gps_start(GPS_TYPE_COLD_START);
        *resp_msg_len = sprintf((char*)resp_msg,  "%s,OK", GPS_IPC_MSG__SET_GPS_RESET_COLD_BOOT);
    }
    else if (strcmp((char*)recv_msg, GPS_IPC_MSG__SET_GPS_RESET_WARM_BOOT) == 0)
    {
        MDS_LOGT(eSVC_GPS,"%s()-(%d) : GPS_IPC_MSG__SET_GPS_RESET_WARM_BOOT ...\r\n",__func__,__LINE__);
        gps_stop();
        gps_start(GPS_TYPE_WARM_START);
        *resp_msg_len = sprintf((char*)resp_msg, "%s,OK", GPS_IPC_MSG__SET_GPS_RESET_WARM_BOOT);
    }

    //MDS_LOGT(eSVC_GPS,"%s()-(%d) : ret [%s] / [%d] ...\r\n",__func__,__LINE__, resp_msg, *resp_msg_len);
    // case 2 not resp to clnt
    /*
    *resp_msg_len = 0;
    resp_msg = NULL;
    */
    return 0;
}


int main(int argc, char* argv[])
{

    pthread_t tid;
    //const char cmd_start_agps[] = "-r 0 -l 365 -t 86400 -m 1 -n 1 -c /etc/gps.conf -S 1,lte-internet.sktelecom.com,0";
    
    // ------------------------------------------------
    // debug msg out mute... 
    //   - prebuilt library has so many debug print.
    
	close(0);
	close(1);
	close(2);

	stdin = freopen("/dev/null", "r", stdin);
	stdout = freopen("/dev/null", "w", stdout);
	stderr = freopen("/dev/null", "rw", stderr);
    
    // ------------------------------------------------

    mds_logd_init();
    mds_api_stackdump_init();

    MDS_LOGT(eSVC_GPS,"Program start... \r\n");
    pthread_create (&tid, NULL, gps_tool_thread, NULL);
    
    if ( argc == 2)
    {
        if ( strcmp(argv[1],"warm") == 0 )
            gps_start(GPS_TYPE_WARM_START);
        else if ( strcmp(argv[1],"cold") == 0 )
            gps_start(GPS_TYPE_COLD_START);
    }
    
    // default warm boot
    if ( argc == 1 )
        gps_start(GPS_TYPE_WARM_START);


    udp_ipc_server_start(UDP_IPC_PORT__GPS_MGR_CH, msg_recv_proc_gps_tools);


    while(1)
        sleep(1);
    /*
    while(1)
    {
        gps_start(GPS_TYPE_WARM_START);

        int test_cnt = 120;
        while(test_cnt--)
        {
            // todo : manage gps server..
            MDS_LOGI(eSVC_GPS,"gps test stop cnt ... %d\r\n", test_cnt);
            sleep(1);
        }
        MDS_LOGI(eSVC_GPS,"gps grogram end...\r\n");
        gps_stop();
        test_cnt = 120;
        while(test_cnt--)
        {
            MDS_LOGI(eSVC_GPS,"gps test stop cnt 2... %d\r\n", test_cnt);
            sleep(1);
        }
        MDS_LOGI(eSVC_GPS,"gps grogram end...\r\n");
    }
    */

}





static int _gps_start(int gps_type)
{
    int tools_argc = 0;
    char* tools_argv[20] = {0,};

    char tmp_buff[1024] = {0,};

    switch (gps_type)
    {
        case GPS_TYPE_COLD_START:
        {
            sprintf(tmp_buff, "%s", cmd_agps_cold_start);
            MDS_LOGT(eSVC_GPS,"gps mgr cold start:: input argument %s\r\n", tmp_buff);
            tools_argc = _devide_argument(tmp_buff, 1024, tools_argv);
            break;
        }
        case GPS_TYPE_WARM_START:
        {
            sprintf(tmp_buff, "%s", cmd_agps_warm_start);
            MDS_LOGT(eSVC_GPS,"gps mgr warm start:: input argument %s\r\n", tmp_buff);
            tools_argc = _devide_argument(tmp_buff, 1024, tools_argv);
            break;
        }
        default : 
            MDS_LOGE(eSVC_GPS,"gps mgr :: invalid type %d\r\n", gps_type);
            return -1;
    }

    garden_force_stop = 1;
    garden_entry(tools_argc, tools_argv);

    return 0;
    // convert argument
}

static int _gps_stop()
{
    garden_force_stop = 0;
    return 0;
}







static int _devide_argument(char* buff, int buff_len, char* argv[])
{
    unsigned char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
    
    char    *base = 0;
    int     t_argc = 0;
   // char*   t_argv[10] = {0,};
    
    int i = 0;
    
    memcpy(ret_buff, buff, buff_len);
    memset(buff, 0x00, MAX_RET_BUFF_SIZE);
    
    base = buff;
    
    //t_argv[t_argc] = base;
    argv[t_argc] = base;
    t_argc++;
    
    for (i = 0 ; i < buff_len ; i++)
    {
        switch(ret_buff[i])
        {
            case ' ':
                *base = '\0';
                //t_argv[t_argc] = base + 1;
                argv[t_argc] = base + 1;
                t_argc++;
                break;
            default:
                *base = ret_buff[i];
                break;
        }
        base++;
    }
    
    /*
    for( i = 0 ; i < t_argc ; i++)
    {
        printf("1: [%d]/[%d] => [%s]\r\n", i, t_argc, t_argv[i]);
    }
    */
    
    return t_argc;
}
