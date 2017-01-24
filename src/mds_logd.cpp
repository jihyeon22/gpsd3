/**
 *       @file  logd_clnt.c
 *      @brief  logd service client program stub
 *
 * Detailed description starts here.
 *
 *     @author  Yoonki (IoT), yoonki@mdstec.com
 *
 *   @internal
 *     Created  2013년 02월 27일
 *    Revision  $Id: doxygen.templates,v 1.3 2010/07/06 09:20:12 mehner Exp $
 *    Compiler  gcc/g++
 *     Company  MDS Technologt, R.Korea
 *   Copyright  Copyright (c) 2013, Yoonki
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * =====================================================================================
 */

#include "mds_logd.h" // Service Common Header 

#include <memory.h>
#include <errno.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static char process_name[32] = {0};
static char *pname = NULL;

static int _logfd = -1;
static struct sockaddr_un logd_addr;

void
mds_logd_init(void)
{
	char tmp[128] = {0};
	char *tr, *tr_prev;
	FILE *fp;

	sprintf(tmp, "/proc/%d/cmdline", (int) getpid());
	fp = fopen(tmp, "r");
	if(fp) {
		char *temp_bp = NULL;
		
		if(fgets(tmp, sizeof(tmp), fp) != NULL) {
			tr_prev = tr = strtok_r(tmp, "/", &temp_bp);
			for(;;) {
				tr = strtok_r(NULL, "/", &temp_bp);
				if(tr == NULL) {
					break;
				}
				tr_prev = tr;
			}
			strcpy(process_name, tr_prev);
			pname = process_name;
		}
		fclose(fp);
	}

	memset(&logd_addr, 0, sizeof(logd_addr));
	logd_addr.sun_family = AF_UNIX;
	strcpy(logd_addr.sun_path, SOCKNAME);
}

int
mds_logd_open(void)
{
	_logfd = socket(PF_FILE, SOCK_DGRAM, 0);
	if(_logfd == -1) {
		perror("socket() err");
	}

	return _logfd;
}

void
mds_logd(int svc, int level, const char *format, ...)
{
	char tmp[1024] = {0};
	struct log_message l_msg;
	int l_msg_size;
	//int i = 0;

	/** pthread_mutex_lock이 정상적으로 동작안함 락이 안풀림 */
	//fprintf(stderr, "%s logd +++\n", pname);
	//fprintf(stderr, "%s logd get lock - ", pname);
	if(_logfd == -1) {
		if((_logfd = socket(PF_FILE, SOCK_DGRAM, 0)) < 0) {
			//fprintf(stderr, "%s logd free 1 lock\n", pname);
			return;
		}
	}
	//fprintf(stderr, "%s logd free 1 lock\n", pname);

	memset(&l_msg, 0, sizeof(struct log_message));

	va_list va;
	va_start(va, format);
	vsprintf(tmp, format, va);
	va_end(va);

	//fprintf(stderr, "%s logd - 1 -\n", pname);

	l_msg.type = 0;		// 0: normal, 1: file dump
	l_msg.service = svc;
	l_msg.level = level;

	if(pname != NULL) {
		strcpy(l_msg.pname, pname);
	}
	else {
		strcpy(l_msg.pname, "none");
	}

	l_msg_size = strlen(tmp) + sizeof(l_msg.type) + sizeof(l_msg.service) + sizeof(l_msg.level) + sizeof(l_msg.pname);

	strcpy(l_msg.message, tmp);
	

//	for(i = 0; i < 5; i++) 
	{
		if(sendto(_logfd, &l_msg, l_msg_size, MSG_DONTWAIT,
		          (struct sockaddr*)&logd_addr, sizeof(logd_addr)) == l_msg_size) {
//			break;
		}
//		usleep(10000);
	}

	//fprintf(stderr, "%s logd ---\n", pname);
}

void mds_logd_dump(void)
{
	/* take debug dump for killed module */
}

