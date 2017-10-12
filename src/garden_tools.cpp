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

static int gps_thread_stat = 0;

// ------------------------------------------------
static int _devide_argument(char* buff, int buff_len, char* argv[]);
static int _gps_start(int stat);
static int _gps_stop();
void mds_api_stackdump_init(void);

static int _gps_default_type = GPS_TYPE_AGPS;

#define MAX_RET_BUFF_SIZE 1024

static int xtra_gps_file_download_req_cnt = 0;
static int xtra_gps_file_download_fail_cnt = 0;

static int _isExistFile(const char *file, int timeout)
{
    while(timeout--) {
        if(access(file, F_OK) == 0) {
            return 0;
        }
        printf(".\n");
        sleep(1);
    }
    
    printf("%s> %s can not open!!!!\n", __func__, file);

    return -1;
}
int mds_gps_tool_xtra_gps_file_chk()
{
    int ret = 0;
    if ( _isExistFile(XTRA_DATA_FILE_NAME, 1) == 0 )
    {
        xtra_gps_file_download_req_cnt++;
    }
    else 
    {
        xtra_gps_file_download_fail_cnt++;
    }

    if ( ( xtra_gps_file_download_req_cnt > GPS_XTRA_DOWNLOAD_MAX_CNT ) || ( xtra_gps_file_download_fail_cnt > (GPS_XTRA_DOWNLOAD_MAX_CNT * 2 )) )
        ret = 0;
    else 
        ret = 1;

    MDS_LOGI(eSVC_GPS, " >> xtra gps download => chk [%d]/[%d] fail [%d]/[%d] => ret [%d]\r\n", xtra_gps_file_download_req_cnt,GPS_XTRA_DOWNLOAD_MAX_CNT,  xtra_gps_file_download_fail_cnt, GPS_XTRA_DOWNLOAD_MAX_CNT*2, ret);
    return ret;
}

int mds_gps_tool_xtra_gps_file_chk_clr()
{
    MDS_LOGI(eSVC_GPS, " >> xtra gps file clear!!!\r\n");
    xtra_gps_file_download_req_cnt = 0;
    xtra_gps_file_download_fail_cnt = 0;

    unlink(XTRA_DATA_FILE_NAME);
}


int mds_api_remove_etc_char(const char *s, char* target, int target_len)
{
	int cnt = 0;

	while (*s)
	{
		//printf("strlen [%c]\r\n" ,*s);
		if ( ( *s >= 33 ) && ( *s <= 126 ) )
		{
            target[cnt] = *s;
            //printf("target[%d] => [0x%x][%c]\r\n", cnt, target[cnt], target[cnt]);
			cnt++;
			
			if (cnt > target_len)
				return -1;
		}
		s++;
	}
	//printf("strlen count [%d]\r\n" ,cnt);
	return cnt;
}

FILE *log_fd = NULL;
// ------------------------------------------------
// nmea call back
// ------------------------------------------------
void mds_gps_tools_nmea_callback(long long int timestamp, const char *nmea, int length)
{
    static char send_buff[1024] = {0,};
    static int send_to_size = 0;

    static int print_interval = 0;
	
	char tmp_filter_buff[1024] = {0,};
	char tmp_filter_len = 0;
	
    int cur_sec = timestamp % 10000;
    static int last_sec = cur_sec;
    cur_sec = cur_sec / 1000;

    printf("mds_gps_tools_nmea_callback++\r\n");
    
//	fprintf(log_fd, " >> %s", nmea);

	if(nmea != NULL)
	{
		//if ( (strstr(nmea, "GPGSV") != NULL ) || (strstr(nmea, "GPRMC") != NULL ) || (strstr(nmea, "GPGGA") != NULL ) || (strstr(nmea, "GPGSA") != NULL ) 
		if ( strstr(nmea, "$GP") == NULL)
			return;
		
		if ( ( send_to_size + length ) > 1024 )
		{
			MDS_LOGE(eSVC_GPS, "[GPSMGR] gps buffer is full\r\n");
			send_to_size = 0;
			memset(send_buff, 0x00, 1024);
		}
		
		send_to_size += sprintf(send_buff+send_to_size, "%s\r", nmea);

		if (strstr(nmea, "GPRMC") != NULL )
		{
			if ( print_interval++ > DEBUG_PRINT_INTERVAL_SEC)
			{
				MDS_LOGI(eSVC_GPS, "[GPSMGR] %s\r\n",nmea);
				print_interval = 0;
			}
		}
	}

    //if ( last_sec != cur_sec ) // TIME SYNC FAIL... 
    if (strstr(nmea, "GPGGA") != NULL )
    {
        // send to data..
        last_sec = cur_sec;
        // send data...
        // MDS_LOGI(eSVC_GPS, "[GPSMGR] gps data send : length %d \r\n",send_to_size);
        udp_ipc_broadcast_send(UDP_IPC_PORT__GPS_NMEA_DATA_CH, (unsigned char*)send_buff, send_to_size);

//      fprintf(log_fd, "==============================================================");
//      fprintf(log_fd, "%s", send_buff);
//      fprintf(log_fd, "==============================================================");

        send_to_size = 0;
        memset(send_buff, 0x00, 1024);
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

    gps_thread_stat = 1;

    MDS_LOGI(eSVC_GPS,"gps mgr thread start type :: %d\r\n", gps_type);

    _gps_start(gps_type);

    gps_thread_stat = 0;

    MDS_LOGI(eSVC_GPS,"gps mgr thread end type :: %d\r\n", gps_type);

}

int gps_start(int gps_type)
{
    if ( gps_thread_stat != 0)
        return -1;
    int* gps_type_p = (int*)malloc(sizeof(int));

    switch ( _gps_default_type )
    {
        case GPS_TYPE_AGPS:
        {
            if ( gps_type ==  GPS_TYPE_WARM_START)
            {
                MDS_LOGT(eSVC_GPS,"GPS START :: AGPS / WARM start  \r\n");
                *gps_type_p = GPS_CMD_TYPE_AGPS_WARM_START;
            }
            else if ( gps_type ==  GPS_TYPE_COLD_START)
            {
                MDS_LOGT(eSVC_GPS,"GPS START :: AGPS / COLD start \r\n");
                *gps_type_p = GPS_CMD_TYPE_AGPS_COLD_START;
            }
            break;
        }
        case GPS_TYPE_SGPS:
        {
            if ( gps_type ==  GPS_TYPE_WARM_START)
            {
                MDS_LOGT(eSVC_GPS,"GPS START :: SGPS / WARM start \r\n");
                *gps_type_p = GPS_CMD_TYPE_SGPS_WARM_START;
            }
            else if ( gps_type ==  GPS_TYPE_COLD_START)
            {
                MDS_LOGT(eSVC_GPS,"GPS START :: SGPS / COLD start \r\n");
                *gps_type_p = GPS_CMD_TYPE_SGPS_COLD_START;
            }
            break;
        }
        case GPS_TYPE_SGPS_WITH_XTRA:
        {
            if ( gps_type ==  GPS_TYPE_WARM_START)
            {
                MDS_LOGT(eSVC_GPS,"GPS START :: SGPS_XTRA / WARM start \r\n");
                *gps_type_p = GPS_CMD_TYPE_SGPS_WITH_XTRA_WARM_START;
            }
            else if ( gps_type ==  GPS_TYPE_COLD_START)
            {
                MDS_LOGT(eSVC_GPS,"GPS START :: SGPS_XTRA / COLD start \r\n");
                *gps_type_p = GPS_CMD_TYPE_SGPS_WITH_XTRA_COLD_START;
            }
            break;
        }
        default :
        {
            if ( gps_type ==  GPS_TYPE_WARM_START)
                *gps_type_p = GPS_CMD_TYPE_AGPS_WARM_START;
            else if ( gps_type ==  GPS_TYPE_COLD_START)
                *gps_type_p = GPS_CMD_TYPE_AGPS_COLD_START;
            break;
        }
    }


   // *gps_type_p = gps_type;

    pthread_create(&garden_mgr_tid, NULL, garden_mgr_thread, (void* )gps_type_p);
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

    gps_thread_stat = 0;

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

        if ( gps_thread_stat == 0 )
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

#define GPS_THREAD_INVALID_CHK_CNT  10

int main(int argc, char* argv[])
{
	int pid, sid;
	int count = 0;

    int no_start_flag = 0;

	pid = fork();
	while(pid < 0)
	{
		perror("fork error : ");
		pid = fork();
		if(count == 10) {
			exit(0);
		}
		sleep(10);
		count++;
	}

	if(pid > 0) {
		exit(EXIT_SUCCESS);
	}

	sid = setsid();
	if(sid < 0) {
		exit(EXIT_FAILURE);
	}


	chdir("/");

    pthread_t tid;
    //const char cmd_start_agps[] = "-r 0 -l 365 -t 86400 -m 1 -n 1 -c /etc/gps.conf -S 1,lte-internet.sktelecom.com,0";
    
    // ------------------------------------------------
    // debug msg out mute... 
    //   - prebuilt library has so many debug print.

    log_fd = stderr;

	close(0);
	close(1);
	//close(2);

	stdin = freopen("/dev/null", "r", stdin);
	stdout = freopen("/dev/null", "w", stdout);
	//stderr = freopen("/dev/null", "rw", stderr);

    
        
    // ------------------------------------------------

    mds_logd_init();
    mds_api_stackdump_init();

    MDS_LOGT(eSVC_GPS,"Program start... \r\n");
    pthread_create (&tid, NULL, gps_tool_thread, NULL);
    
    // default warm boot
    if ( argc == 1 )
    {
        _gps_default_type = GPS_TYPE_AGPS;
        gps_start(GPS_TYPE_WARM_START);
    }

    // 하위버젼 호환성 : 기존에는 argument 가 2개였다.
    if ( argc == 2 )
    {
        _gps_default_type = GPS_TYPE_AGPS;
        if ( strcmp(argv[1],"warm") == 0 )
            gps_start(GPS_TYPE_WARM_START);
        else if ( strcmp(argv[1],"cold") == 0 )
            gps_start(GPS_TYPE_COLD_START);
        else if ( strcmp(argv[1],"nostart") == 0 )
            no_start_flag = 1; // nothing..
    }

    if ( argc == 3 )
    {
        if ( strcmp(argv[1],"agps") == 0 )
            _gps_default_type = GPS_TYPE_AGPS;
        else if ( strcmp(argv[1],"sgps") == 0 )
            _gps_default_type = GPS_TYPE_SGPS;
        else if ( strcmp(argv[1],"sgps_xtra") == 0 )
            _gps_default_type = GPS_TYPE_SGPS_WITH_XTRA;

        if ( strcmp(argv[2],"warm") == 0 )
            gps_start(GPS_TYPE_WARM_START);
        else if ( strcmp(argv[2],"cold") == 0 )
            gps_start(GPS_TYPE_COLD_START);
        else if ( strcmp(argv[2],"nostart") == 0 )
            no_start_flag = 1; // nothing..
/*
        if ( strcmp(argv[2],"agps") == 0 )
            _gps_default_type = GPS_TYPE_AGPS;
        else if ( strcmp(argv[2],"sgps") == 0 )
            _gps_default_type = GPS_TYPE_SGPS;
        else if ( strcmp(argv[2],"sgps_xtra") == 0 )
            _gps_default_type = GPS_TYPE_SGPS_WITH_XTRA;

        if ( strcmp(argv[1],"warm") == 0 )
            gps_start(GPS_TYPE_WARM_START);
        else if ( strcmp(argv[1],"cold") == 0 )
            gps_start(GPS_TYPE_COLD_START);
        else if ( strcmp(argv[1],"nostart") == 0 )
            no_start_flag = 1; // nothing..
*/
    }

    MDS_LOGT(eSVC_GPS,"gps start type [%d]  \r\n" , _gps_default_type);

    udp_ipc_server_start(UDP_IPC_PORT__GPS_MGR_CH, msg_recv_proc_gps_tools);

    count = 0;

    while(1)
    {
        // gps thread 가 동작하고 있지 않다면 무조건 다시 시작시킨다.
        // thread 가 동작하고있지 않으면 뭔가 이상한상태다.
        // 단, no start 로 시작하면 굳이 자동 실행시킬 이유가 없다.
        if (( gps_thread_stat == 0 ) && ( no_start_flag == 0 ))
        {
            MDS_LOGT(eSVC_GPS,"GPS THREAD is invalid stat [%d]  \r\n" , count);
            count++;
        }

        if ( count > GPS_THREAD_INVALID_CHK_CNT)
        {
            count = 0;
            gps_start(GPS_TYPE_WARM_START);
        }
        
        sleep(1);
    }
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
        case GPS_CMD_TYPE_AGPS_WARM_START:
        {
            sprintf(tmp_buff, "%s", cmd_agps_warm_start);
            MDS_LOGT(eSVC_GPS,"gps mgr cold start:: input argument %s\r\n", tmp_buff);
            tools_argc = _devide_argument(tmp_buff, 1024, tools_argv);
            break;
        }
        case GPS_CMD_TYPE_AGPS_COLD_START:
        {
            sprintf(tmp_buff, "%s", cmd_agps_cold_start);
            MDS_LOGT(eSVC_GPS,"gps mgr warm start:: input argument %s\r\n", tmp_buff);
            tools_argc = _devide_argument(tmp_buff, 1024, tools_argv);
            mds_gps_tool_xtra_gps_file_chk_clr();
            break;
        }
        case GPS_CMD_TYPE_SGPS_WARM_START:
        {
            sprintf(tmp_buff, "%s", cmd_sgps_warm_start);
            MDS_LOGT(eSVC_GPS,"gps mgr sgps warm start:: input argument %s\r\n", tmp_buff);
            tools_argc = _devide_argument(tmp_buff, 1024, tools_argv);
            break;
        }
        case GPS_CMD_TYPE_SGPS_COLD_START:
        {
            sprintf(tmp_buff, "%s", cmd_sgps_cold_start);
            MDS_LOGT(eSVC_GPS,"gps mgr sgps warm start:: input argument %s\r\n", tmp_buff);
            tools_argc = _devide_argument(tmp_buff, 1024, tools_argv);
            mds_gps_tool_xtra_gps_file_chk_clr();
            break;
        }
        case GPS_CMD_TYPE_SGPS_WITH_XTRA_WARM_START:
        {
            sprintf(tmp_buff, "%s", cmd_sgps_with_xtra_warm_start);
            MDS_LOGT(eSVC_GPS,"gps mgr sgps warm start:: input argument %s\r\n", tmp_buff);
            tools_argc = _devide_argument(tmp_buff, 1024, tools_argv);
            break;
        }
        case GPS_CMD_TYPE_SGPS_WITH_XTRA_COLD_START:
        {
            sprintf(tmp_buff, "%s", cmd_sgps_with_xtra_cold_start);
            MDS_LOGT(eSVC_GPS,"gps mgr sgps warm start:: input argument %s\r\n", tmp_buff);
            tools_argc = _devide_argument(tmp_buff, 1024, tools_argv);
            mds_gps_tool_xtra_gps_file_chk_clr();
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
