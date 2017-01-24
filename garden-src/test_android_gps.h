/* Copyright (c) 2011-2014, Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#ifndef _TEST_ANDROID_GPS_H
#define _TEST_ANDROID_GPS_H

#include "hardware/gps.h"
#include "xtra_system_interface.h"
#include "location_callbacks_garden.h"

extern int LEGACY;

#define TRUE     1
#define FALSE    0


typedef enum
{
    ACTION_NONE,
    ACTION_QUIT,
    ACTION_OPEN_ATL,
    ACTION_CLOSE_ATL,
    ACTION_FAIL_ATL,
    ACTION_NI_NOTIFY,
    ACTION_XTRA_DATA,
    ACTION_XTRA_TIME,
    ACTION_NLP_RESPONSE,
    ACTION_PHONE_CONTEXT_UPDATE
} test_thread_action_e_type;

#define XTRA_DATA_BUF_LEN 200000
#define XTRA_DATA_FILE_NAME "localxtra.bin"
#define XTRA_DEFAULT_SERVER1 "http://xtrapath1.izatcloud.net/xtra2.bin"
#define XTRA_DEFAULT_SERVER2 "http://xtrapath2.izatcloud.net/xtra2.bin"
#define XTRA_DEFAULT_SERVER3 "http://xtrapath3.izatcloud.net/xtra2.bin"
#define XTRA_USER_AGENT_STRING "LE/1.2.3/OEM/Model/Board/Carrier" // Test user agent string
#define XTRA_NTP_DEFAULT_SERVER1 "time.gpsonextra.net"

typedef struct time_s {
    GpsUtcTime time;
    int64_t timeReference;
    int uncertainty;
} XtraTime;

/** Agps Command Line options **/
typedef struct agps_command_line_options {
    int16_t agpsType; // Agps type
    const char * apn; // apn
    int16_t agpsBearerType; // Agps bearer type.
/* values for struct members from here down are initialized by reading gps.conf
   file using -c options to garden */
    unsigned long suplVer;
    char suplHost[256];
    int suplPort;
    char c2kHost[256];
    int c2kPort;
} AgpsCommandLineOptionsType;


/** Structure that holds the command line options given to main **/
typedef struct command_line_options {
    uint32_t r; // recurrence type
    int l; // Number of sessions to loop through.
    int t; // time to stop fix in seconds
    int s; // Stacks to test.
    int b; // Legacy TRUE OR FALSE.
    int ulpTestCaseNumber; // Run specified ULP test case number
    uint32_t deleteAidingDataMask; // Specifies Hexadecimal mask for deleting aiding data.
    uint32_t positionMode; // Specifies Position mode.
    uint32_t interval; // Time in milliseconds between fixes.
    uint32_t accuracy; // Accuracy in meters
    uint32_t responseTime; // Requested time to first fix in milliseconds
    GpsLocation location; // Only latitude, longiture and accuracy are used in this structure.
    int networkInitiatedResponse[256]; // To store the response pattern
    int niCount; // Number of back to back Ni tests
    int niResPatCount; // Number of elements in the response pattern
    AgpsCommandLineOptionsType agpsData; // Agps Data
    int isSuccess; // Success, Failure, ...
    int rilInterface; // Ril Interface
    XtraClientConfigType xtraClientConfig; // Xtra System configuration
    char gpsConfPath[256]; // Path to config file
    int disableAutomaticTimeInjection; // Flag to indicate whether to disable or enable automatic time injection
    int niSuplFlag; // Flag to indicate that tests being conducted is ni supl
    int printNmea; // Print nmea string
    int satelliteDetails; // Print detailed info on satellites in view.
    int fixThresholdInSecs; // User specified time to first fix threshold in seconds
    int zppTestCaseNumber; // Run specified ULP test case number
    int apiToTest; // Defaults to HAL API
    int enableXtra; // Flag to enable/disable Xtra
    int enableCheckForEngOff; // Flag to enable/disable CheckForEngineOff
} CommandLineOptionsType;

void test_xtra_client_data_callback(char * data,int length);
void test_xtra_client_time_callback(int64_t utcTime, int64_t timeReference, int uncertainty);

int garden_entry (int argc, char *argv[]);

#endif // _TEST_ANDROID_GPS_H
