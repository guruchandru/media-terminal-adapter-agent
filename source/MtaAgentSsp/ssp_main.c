/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

#define _GNU_SOURCE
#include <string.h>

#ifdef __GNUC__
#if (!defined _BUILD_ANDROID) && (!defined _NO_EXECINFO_H_)
#include <execinfo.h>
#endif
#endif

#include "ssp_global.h"
#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif
#include "stdlib.h"
/* WebConfig framework - skip if causes compilation issues */
#if defined(FEATURE_SUPPORT_WEBCONFIG) && !defined(SKIP_WEBCONFIG_FOR_BUILD)
    /* Force disable CCSP support for RBUS-only builds */
    #ifdef CCSP_SUPPORT_ENABLED
        #undef CCSP_SUPPORT_ENABLED
    #endif
    #include "webconfig_framework.h"
#endif
#ifdef USE_PCD_API_EXCEPTION_HANDLING
#include "pcdapi.h"
#endif
//#include <docsis_ext_interface.h>
//#include "safec_lib_common.h"
#include <safec_lib.h>
#include "syscfg/syscfg.h"
#include "secure_wrapper.h"
#ifdef MTA_TR104SUPPORT
#define MTAAGENT_SYSEVENT 1
#include "TR104.h"
#define TR104_ENABLE "TR104enable"
int CosaDmlTR104DataSet(char *pString,int bootup);
#endif

#include "mta_log.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
/* RBUS implementation */
#include "../TR-181/middle_layer_src/mta_rbus_handlers.h"

#define MTA_LOG_FILE "/rdklogs/logs/MtaLog.txt.0"
#define MTA_CCSP_INIT_FILE_BOOTUP "/tmp/mta_ccsp_initialized_bootup"
#define MAX_SUBSYSTEM_SIZE 32
#define NUM_SUBSYSTEM_TYPES (sizeof(gSubsystem_type_table)/sizeof(gSubsystem_type_table[0]))

/* Initialize logging to file */
static void init_mta_logging(void) {
    /* Create directory if it doesn't exist */
    if (mkdir("/rdklogs", 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "Failed to create /rdklogs directory: %s\n", strerror(errno));
    }
    if (mkdir("/rdklogs/logs", 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "Failed to create /rdklogs/logs directory: %s\n", strerror(errno));
    }
    
    /* Open log file */
    g_mta_logfile = fopen(MTA_LOG_FILE, "a");
    if (!g_mta_logfile) {
        fprintf(stderr, "Failed to open log file %s: %s\n", MTA_LOG_FILE, strerror(errno));
    } else {
        setvbuf(g_mta_logfile, NULL, _IOLBF, 0); /* Line buffered */
        fprintf(stderr, "MTA logging initialized: %s\n", MTA_LOG_FILE);
        fprintf(g_mta_logfile, "\n=== MTA Agent Log Started ===\n");
        fflush(g_mta_logfile);
    }
}

#include "cap.h"
static cap_user appcaps;

/* Legacy common-library global variables commented out for JSON-driven RBUS approach */
/* PDSLH_CPE_CONTROLLER_OBJECT     pDslhCpeController      = NULL; */
/* PCOMPONENT_COMMON_DM            g_pComponent_Common_Dm  = NULL; */
/* PCCSP_FC_CONTEXT                pMtaFcContext           = (PCCSP_FC_CONTEXT            )NULL; */
/* PCCSP_CCD_INTERFACE             pMtaCcdIf               = (PCCSP_CCD_INTERFACE         )NULL; */
/* PCCC_MBI_INTERFACE              pMtaMbiIf               = (PCCC_MBI_INTERFACE          )NULL; */
char                            g_Subsystem[MAX_SUBSYSTEM_SIZE]         = {0};
BOOL                            g_bActive               = FALSE;

int consoleDebugEnable = 0;
FILE* debugLogFile;

int  cmd_dispatch(int  command)
{
    /* Legacy messagebus/DML code commented out for RBUS approach - no longer needed */
    /* char*                           pParamNames[]      = {"Device.X_CISCO_COM_MTA."}; */
    /* parameterValStruct_t**          ppReturnVal        = NULL; */
    /* int                             ulReturnValCount   = 0; */
    /* int                             i                  = 0; */
    int ret = 0;

    switch ( command )
    {
            case	'e' :

            CcspTraceInfo(("Initializing MTA Agent with RBUS...\n"));

            /* New RBUS-based initialization */
            if (mta_rbus_init(RBUS_COMPONENT_NAME_MTA) != 0) {
                CcspTraceError(("Failed to initialize RBUS\n"));
                return -1;
            }

            g_bActive = TRUE;
            CcspTraceInfo(("MTA RBUS Module loaded successfully...\n"));

            /* Legacy messagebus/DML approach (commented out) */
            break;

            case    'r' :

            /* Legacy CcspCcMbi_GetParameterValues will be replaced with rbus_get() */
            #if 0
            CcspCcMbi_GetParameterValues
                (
                    DSLH_MPA_ACCESS_CONTROL_ACS,
                    pParamNames,
                    1,
                    &ulReturnValCount,
                    &ppReturnVal,
                    NULL
                );

            for ( i = 0; i < ulReturnValCount; i++ )
            {
                CcspTraceWarning(("Parameter %d name: %s value: %s \n", i+1, ppReturnVal[i]->parameterName, ppReturnVal[i]->parameterValue));
            }
            #endif
            
                break;

        case    'm':

                AnscPrintComponentMemoryTable(pComponentName);

                break;

        case    't':

                AnscTraceMemoryTable();

                break;

        case    'c':
                /* Terminate RBUS */
                if (g_bActive) {
                    mta_rbus_terminate();
                    g_bActive = FALSE;
                }
                
                /* Legacy cancel function (commented out) */
                /* ssp_cancel_pnm(gpPnmStartCfg); */

                break;

        default:
            break;
    }

    return ret;
}

static void _print_stack_backtrace(void)
{
#ifdef __GNUC__
#if (!defined _BUILD_ANDROID) && (!defined _NO_EXECINFO_H_) && (!defined _COSA_SIM_)
        void* tracePtrs[100];
        char** funcNames = NULL;
        int i, count = 0;

        int fd;
        const char* path = "/nvram/MTaAgentSsp_backtrace";
        fd = open(path, O_RDWR | O_CREAT);
        if (fd < 0)
        {
            CcspTraceError(("failed to open backtrace file: %s", path));
            return;
        }

        count = backtrace( tracePtrs, 100 );

        backtrace_symbols_fd( tracePtrs, count, fd);
   
        close(fd);

        funcNames = backtrace_symbols( tracePtrs, count );

        if ( funcNames ) {
            // Print the stack trace
            for( i = 0; i < count; i++ )
                CcspTraceInfo(("%s\n", funcNames[i] ));

            // Free the string pointers
            free( funcNames );
        }
#endif
#endif
}

static void daemonize(void) {
	switch (fork()) {
	case 0:
		break;
	case -1:
		// Error
		CcspTraceError(("Error daemonizing (fork)! %d - %s\n", errno, strerror(
				errno)));
		exit(0);
		break;
	default:
		_exit(0);
	}

	if (setsid() < 	0) {
		CcspTraceError(("Error demonizing (setsid)! %d - %s\n", errno, strerror(errno)));
		exit(0);
	}

    /*
     *  What is the point to change current directory?
     *
    chdir("/");
     */

#ifndef  _DEBUG

	int fd;
	fd = open("/dev/null", O_RDONLY);
	if (fd != 0) {
		dup2(fd, 0);
		close(fd);
	}
	fd = open("/dev/null", O_WRONLY);
	if (fd != 1) {
		dup2(fd, 1);
		close(fd);
	}
	fd = open("/dev/null", O_WRONLY);
	if (fd != 2) {
		dup2(fd, 2);
		close(fd);
	}
#endif
}

void sig_handler(int sig)
{
    if ( sig == SIGINT ) {
    	signal(SIGINT, sig_handler); /* reset it to this function */
    	CcspTraceError(("SIGINT received!\n"));
        exit(0);
    }
    else if ( sig == SIGUSR1 ) {
    	signal(SIGUSR1, sig_handler); /* reset it to this function */
    	CcspTraceWarning(("SIGUSR1 received!\n"));
    }
    else if ( sig == SIGUSR2 ) {
    	CcspTraceWarning(("SIGUSR2 received!\n"));
    }
    else if ( sig == SIGCHLD ) {
    	signal(SIGCHLD, sig_handler); /* reset it to this function */
    	CcspTraceWarning(("SIGCHLD received!\n"));
    }
    else if ( sig == SIGPIPE ) {
    	signal(SIGPIPE, sig_handler); /* reset it to this function */
    	CcspTraceWarning(("SIGPIPE received!\n"));
    }
    else if ( sig == SIGTERM )
    {
        CcspTraceInfo(("SIGTERM received!\n"));
        exit(0);
    }
    else if ( sig == SIGKILL )
    {
        CcspTraceInfo(("SIGKILL received!\n"));
        exit(0);
    }
	else if ( sig == SIGALRM ) {
    	signal(SIGALRM, sig_handler); /* reset it to this function */
    	CcspTraceInfo(("SIGALRM received!\n"));		
    }
    else {
    	/* get stack trace first */
    	_print_stack_backtrace();
    	CcspTraceError(("Signal %d received, exiting!\n", sig));
    	exit(0);
    }

}

#ifndef INCLUDE_BREAKPAD
static int is_core_dump_opened(void)
{
    FILE *fp;
    char path[256];
    char line[1024];
    char *start, *tok, *sp;
#define TITLE   "Max core file size"

    snprintf(path, sizeof(path), "/proc/%d/limits", getpid());
    if ((fp = fopen(path, "rb")) == NULL)
        return 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        if ((start = strstr(line, TITLE)) == NULL)
            continue;

        start += strlen(TITLE);
        if ((tok = strtok_r(start, " \t\r\n", &sp)) == NULL)
            break;

        fclose(fp);
        return (tok[0] == '0' && tok[1] == '\0') ? 0 : 1;
    }

    fclose(fp);
    return 0;
}
#endif

int write_pid_file(const char *file)
{
    FILE *fp;
    pid_t pid;

    if ((fp = fopen(file, "w")) == NULL)
        return -1;

    pid = getpid();
    fprintf(fp, "%d\n", pid);
    fclose(fp);
    return 0;
}

static bool drop_root()
{
    bool retval = false;
    CcspTraceInfo(("NonRoot feature is enabled, dropping root privileges for CcspMtaAgent process\n"));
    appcaps.caps = NULL;
    appcaps.user_name = NULL;

    if(init_capability() != NULL) {
        if(drop_root_caps(&appcaps) != -1) {
            if(update_process_caps(&appcaps) != -1) {
                read_capability(&appcaps);
                retval = true;
            }
        }
    }
    return retval;
}

int main(int argc, char* argv[])
{
    int                             cmdChar            = 0;
    BOOL                            bRunAsDaemon       = TRUE;
    int                             idx                = 0;
    errno_t                         rc                 = 1;
    int                             ind                = -1;
    int                             ret                = 0;

    /* Initialize logging to file */
    init_mta_logging();
    
    CcspTraceInfo(("MTA Agent SSP Starting...\n"));

    debugLogFile = stderr;

    // Buffer characters till newline for stdout and stderr
    setlinebuf(stdout);
    setlinebuf(stderr);

    /* Set the global pComponentName */
    //pComponentName = CCSP_COMPONENT_NAME_MTA;

#if defined(_DEBUG) && defined(_COSA_SIM_)
    AnscSetTraceLevel(CCSP_TRACE_LEVEL_INFO);
#endif

    for (idx = 1; idx < argc; idx++)
    {
        rc = strcmp_s("-subsys",strlen("-subsys"),argv[idx],&ind);
        ERR_CHK(rc);
        if ((!ind) && (rc == EOK))
        {
            rc = strcpy_s(g_Subsystem,sizeof(g_Subsystem),argv[idx+1]);
            if(rc != EOK)
            {
               ERR_CHK(rc);
               return ANSC_STATUS_FAILURE;
            }
            CcspTraceWarning(("\nSubsystem is %s\n", g_Subsystem));
         }
        else
        {
           rc = strcmp_s("-c",strlen("-c"),argv[idx],&ind);
           ERR_CHK(rc);
           
            if ((!ind) && (rc == EOK))
            {
               bRunAsDaemon = FALSE;
            }
         }
    }

    if(!drop_root()) {
         CcspTraceInfo(("drop_root method failed!\n"));
    }

    if ( bRunAsDaemon )
        daemonize();

    /*This is used for ccsp recovery manager */
    if (write_pid_file("/var/tmp/CcspMtaAgent.pid") != 0)
        fprintf(stderr, "%s: fail to write PID file\n", argv[0]);

#ifdef INCLUDE_BREAKPAD
    breakpad_ExceptionHandler();
    signal(SIGUSR1, sig_handler);   
#else
    if (is_core_dump_opened())
    {
        signal(SIGUSR1, sig_handler);
        CcspTraceWarning(("Core dump is opened, do not catch signal\n"));
    }
    else
    {
        CcspTraceWarning(("Core dump is NOT opened, backtrace if possible\n"));
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);

    signal(SIGSEGV, sig_handler);
    signal(SIGBUS, sig_handler);
    signal(SIGKILL, sig_handler);
    signal(SIGFPE, sig_handler);
    signal(SIGILL, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGHUP, sig_handler);
	signal(SIGALRM, sig_handler);
    }
#endif

#ifdef USE_PCD_API_EXCEPTION_HANDLING
    printf("Registering PCD exception handler CcspMTAAgent\n");
    PCD_api_register_exception_handlers( argv[0], NULL );
#endif

    /* ETHWAN mode handling and MTA provisioning */
    char eth_wan_enabled[8] = {'\0'};
    for(rc=0;rc<6;rc++)
    {
        if(syscfg_get(NULL,"eth_wan_enabled",eth_wan_enabled,sizeof(eth_wan_enabled)) ==0 )
        {
            if (strcmp(eth_wan_enabled,"true") == 0)
            {
                CcspTraceInfo(("Box is in ETHWAN mode\n"));
                if(syscfg_set(NULL, "MTA_PROVISION","false") != 0)
                {
                    CcspTraceWarning(("%s: syscfg_set failed\n", __FUNCTION__));
                }
                else
                {
                    CcspTraceInfo(("%s: syscfg_set sucess\n", __FUNCTION__));
                }
            }
            else
            {
                CcspTraceInfo(("Box is in DOCSIS mode\n"));
            }
            break;
        }
        else
        {
            sleep(5);
            CcspTraceWarning(("%s: syscfg_get failed\n", __FUNCTION__));
        }
    }

    /* Initialize RBUS */
    ret = cmd_dispatch('e');
    if(ret != 0)
    {
        CcspTraceError(("Exit error - cmd_dispatch failed %s:%d\n", __FUNCTION__, __LINE__));
        exit(0);
    }

    /* Legacy Cdm_Init removed for RBUS approach */
    #if 0
    #ifdef _COSA_SIM_
    subSys = "";        /* PC simu use empty string as subsystem */
    #else
    subSys = NULL;      /* use default sub-system */
    #endif
    err = Cdm_Init(bus_handle, subSys, NULL, NULL, pComponentName);
    if (err != CCSP_SUCCESS)
    {
        fprintf(stderr, "Cdm_Init: %s\n", Cdm_StrError(err));
        exit(1);
    }
    #endif

    v_secure_system("touch /tmp/mta_initialized");

#ifdef ARRIS_XB3_PLATFORM_CHANGES
     v_secure_system("touch /rdklogs/logs/mtaEvents.log");
#endif

    CcspTraceWarning(("RDKB_SYSTEM_BOOT_UP_LOG : Entering MTA loop\n"));
    printf("Entering MTA loop\n");
#ifdef MTA_TR104SUPPORT
    int retry=0,retry1=0;
    CcspTraceInfo(("TR104 supports in this build\n"));
  
   
  
    char value[8] = {'\0'};
    if (strcmp(eth_wan_enabled,"true") == 0)
    {
        CcspTraceInfo(("Device is in ETHWAN mode\n"));
        char Provision_status[8] = {'\0'};
        while(retry<5)
        {
            if( syscfg_get(NULL, TR104_ENABLE, value, sizeof(value)) == 0 )
            {
                if( strcmp(value, "true") == 0 )
                {
                    CcspTraceInfo(("RFC for TR104 is true\n"));

                        while(retry1<24)
                        {
                            if( syscfg_get(NULL, "MTA_PROVISION", Provision_status, sizeof(Provision_status)) == 0 && strcmp(Provision_status,"true") == 0 )
                            {
                                rc = 0;
retry:
                                sleep(4);
                                int tr104_err = TR104_open();
                                CcspTraceInfo(("TR104_open returned %d during %d iteration\n", tr104_err,retry1));
                                if(tr104_err == 0)
                                {
                                    CcspTraceInfo(("TR104_open returned %d and restarting CcspTr069PaSsp\n", tr104_err));
                                    v_secure_system("systemctl restart CcspTr069PaSsp &");
                                    FILE* fptr= fopen("/nvram/.vsb64.txt","rb");
                                    if (fptr)
                                    {
                                        CcspTraceDebug(("file present and about to read\n"));
                                        char* buffer=0;
                                        int length;
                                        fseek(fptr, 0, SEEK_END);
                                        length = ftell (fptr);
                                        fseek(fptr, 0, SEEK_SET);
                                        buffer = (char*)malloc(length+1);
                                        if (buffer)
                                        {
                                            /* Coverity Fix : CHECKED_RETURN */
                                            size_t bytesRead = fread(buffer, 1, length, fptr);
                                            if (bytesRead > 0) {
                                               buffer[bytesRead] = '\0'; 
                                               CcspTraceInfo(("buffer=%s\n", buffer));
                                               CosaDmlTR104DataSet(buffer, 1);
                                            } else {
                                               CcspTraceWarning(("fread failed or returned 0 bytes\n"));
                                            }
                                            free(buffer);
                                        }
                                        fclose(fptr);
                                    	break;
                                    }
                                    else
                                    {
                                      CcspTraceInfo(("file not present\n"));
                                    }
                                }
                                else
                                {
                                    if(rc > 6)
                                    {
                                        CcspTraceWarning(("TR104_open is failing for %d times so breaking the loop\n",rc));
                                        break;
                                    }
                                    rc++;
                                    goto retry;
                                }
                            }
                            retry1++;
                            sleep(10);
                        }
                }
                else
                {
                    CcspTraceInfo(("RFC for TR104 is false \n"));
                }
                break;
            }
            else
            {
                CcspTraceError(("syscfg_get is failed\n"));
                retry++;
            }
        }
    }
    else
    {
        while(retry<5)
        {
            if( syscfg_get(NULL, TR104_ENABLE, value, sizeof(value)) == 0 )
            {
                if( strcmp(value, "true") == 0 )
                {
                    CcspTraceInfo(("RFC for TR104 is true\n"));

                        while(retry1<24)
                        {
                            int tr104_err = TR104_open();
                            CcspTraceInfo(("TR104_open returned %d during %d iteration\n", tr104_err,retry1));
                            if(tr104_err == 0)
                            {
                                CcspTraceInfo(("TR104_open returned %d and restarting CcspTr069PaSsp\n", tr104_err));
                                v_secure_system("systemctl restart CcspTr069PaSsp &");
                                FILE* fptr= fopen("/nvram/.vsb64.txt","rb");
                                if (fptr)
                                {
                                    CcspTraceDebug(("file present and about to read\n"));
                                    char* buffer=0;
                                    int length;
                                    fseek (fptr, 0, SEEK_END);
                                    length = ftell (fptr);
                                    fseek (fptr, 0, SEEK_SET);
                                    buffer = (char*)malloc (length+1);
                                    if (buffer)
                                    {
                                        /* Coverity Fix : CHECKED_RETURN */
                                        size_t bytesRead = fread(buffer, 1, length, fptr);
                                        if (bytesRead > 0) {
                                            buffer[bytesRead] = '\0'; 
                                            CcspTraceInfo(("buffer=%s\n", buffer));
                                            CosaDmlTR104DataSet(buffer, 1);
                                        } else {
                                            CcspTraceWarning(("fread failed or returned 0 bytes\n"));
                                        }
                                        free(buffer);
                                    }
                                    fclose (fptr);
                                }
                                else
                                {
                                    CcspTraceDebug(("file not present\n"));
                                }
                                break;
                            }
                            retry1++;
                            sleep(10);
                        }
                }
                else
                {
                    CcspTraceInfo(("RFC for TR104 is false \n"));
                }
                break;
            }
            else
            {
                CcspTraceError(("syscfg_get is failed\n"));
                retry++;
            }
        }
    }
    check_component_crash("/tmp/mta_initialized");
#endif
    if ( bRunAsDaemon )
    {
        while(1)
        {
            sleep(30);
        }
    }
    else
    {
        while ( cmdChar != 'q' )
        {
            cmdChar = getchar();

        /*Coverity Fix CID:79619 CHECKED_RETURN */
            if (cmdChar == EOF ) 
           {  
                  CcspTraceWarning(("Error:%s:%d cmdChar return as EOF \n",__FUNCTION__,__LINE__));
                  
           }
           else
           {
                 cmd_dispatch(cmdChar);
           }
      
        }
    }

    /* Legacy Cdm_Term removed for RBUS approach */
    #if 0
    err = Cdm_Term();
    if (err != CCSP_SUCCESS)
    {
        fprintf(stderr, "Cdm_Term: %s\n", Cdm_StrError(err));
        exit(1);
    }
    #endif

    if ( g_bActive )
    {
        /* Terminate RBUS */
        mta_rbus_terminate();
        g_bActive = FALSE;
        
        /* Legacy cancel function (commented out) */
        /* ssp_cancel_pnm(gpPnmStartCfg); */
    }

    /* Close log file */
    if (g_mta_logfile) {
        fprintf(g_mta_logfile, "=== MTA Agent Log Ended ===\n");
        fclose(g_mta_logfile);
        g_mta_logfile = NULL;
    }
  
    return 0;
}


