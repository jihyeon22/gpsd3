#ifndef __GARDEN_TOOLS_H__
#define __GARDEN_TOOLS_H__

extern int garden_force_stop;
void mds_gps_tools_nmea_callback(long long int timestamp, const char *nmea, int length);


// 아래처럼 –d 옵션을 주셔서 설정하시면 cold reset이 됩니다.
// -d 옵션을 주지 않으시면 warm reset입니다.
// ./garden_app -r 0 -l 365 -t 86400 -m 1 -n 1 -c /etc/gps.conf -S 1,lte-internet.sktelecom.com,0 -d 0xFFFFFFFF

#define GPS_TYPE_WARM_START    1
#define GPS_TYPE_COLD_START    2

// gps type
#define GPS_TYPE_AGPS           0
#define GPS_TYPE_SGPS           1
#define GPS_TYPE_SGPS_WITH_XTRA 2


#define GPS_CMD_TYPE_AGPS_WARM_START    1
//const char cmd_agps_warm_start[] = "dummy_name -r 0 -l 365 -m 1 -n 1 -c /etc/gps.conf -S 1,lte-internet.sktelecom.com,0 -d 0";
const char cmd_agps_warm_start[] = "dummy_name -r 0 -m 1 -n 1 -E 0 -c /etc/gps.conf -S 1,lte-internet.sktelecom.com,0 -d 0";

// -l 옵션과 -t 옵션자체는 의미가없다.
//  -> 해당옵션은 세션을 몇번 열것인가...에 대한 옵션이다.
//  -> 세션을 열고 닫는행위할경우 일종의 warm reset 과 같이 작용하는것 같다.
//  -> 세션을 열고 닫을 때 마다. gps 데이터가 조금씩 빠진다.  초기 몇초간~수십초 데이가 빠진상태로 들어온다.
//  -> 굳이 세션을 매번 열고 닫을 필요가 없다.
// -t 옵션의 경우..
//  -> 세션을 닫을 시점을 결정하는 옵션이다.
//  -> -t 60 초라고 세팅한다면 60초 후에 해당 세션을 닫는다. 닫고나면 세션을 새로 열고 세션카운트를 증가시킨다.
//  -> gps 프로그램을 실행시킬경우 중간 중단없이 계속 실행시키길것이기 때문에 해당옵션은 의미가 없다.
#define GPS_CMD_TYPE_AGPS_COLD_START    2
const char cmd_agps_cold_start[] = "dummy_name -r 0 -m 1 -n 1 -E 0 -c /etc/gps.conf -S 1,lte-internet.sktelecom.com,0 -d FFFFFFFF";
//./garden_app -r 0 -l 365 -t 86400 -m 0 -n 1 -c /etc/gps.conf -d 0xFFFFFFFF -q 1

#define GPS_CMD_TYPE_SGPS_WARM_START    3
const char cmd_sgps_warm_start[] = "dummy_name -r 0 -m 0 -E 0 -n 1 -c /etc/gps.conf -d 0";

#define GPS_CMD_TYPE_SGPS_COLD_START    4
const char cmd_sgps_cold_start[] = "dummy_name -r 0 -m 0 -n 1 -E 0 -c /etc/gps.conf -d FFFFFFFF";

#define GPS_CMD_TYPE_SGPS_WITH_XTRA_WARM_START    5
const char cmd_sgps_with_xtra_warm_start[] = "dummy_name -r 0 -m 0 -E 0 -n 1 -c /etc/gps.conf -d 0 -q 1";

#define GPS_CMD_TYPE_SGPS_WITH_XTRA_COLD_START    6
const char cmd_sgps_with_xtra_cold_start[] = "dummy_name -r 0 -m 0 -n 1 -E 0 -c /etc/gps.conf -d FFFFFFFF -q 1";


#endif // __GARDEN_TOOLS_H__

