#ifdef MTA_TR104SUPPORT
#include "TR104.h"
#include <syscfg/syscfg.h>
#include "sysevent/sysevent.h"
/* WebConfig framework commented out - RBUS-only build, no CCSP/DBUS */
/* Force disable CCSP support for RBUS-only builds */
    #ifdef CCSP_SUPPORT_ENABLED
        #undef CCSP_SUPPORT_ENABLED
    #endif
#include "webconfig_framework.h"
#include <msgpack.h>
#include <inttypes.h>


char mta_provision_status[2][20] = {"MTA_PROVISIONED", "MTA_NON_PROVISIONED"};
char MTA_status[5][20] = {"MTA_INIT", "MTA_START", "MTA_COMPLETE", "MTA_ERROR", "MTA_REJECTED"};

#define TR104_SUBDOC_COUNT 1
#define MTA_PROVISIONED "MTA_PROVISIONED"
#define MTA_MAX_DETAIL_SIZE 512

int CosaDmlTR104DataSet(char* pString,int bootup);

FILE* fptr_dummy;
FILE* fptr;
int rc;
int isbootup=0;
typedef struct
{
    bool DevEnabled;
}VoIPProfile_Table_t;
typedef struct
{
    char LocalTimeZone[64];
}POTS_Table_t;
typedef struct
{
    bool Enable;
}FXS_Table_t;
typedef struct
{
/* NETWORK_TABLE_MEMBER_COUNT has to be updated accordance with number of members of the structure */
#define NETWORK_TABLE_MEMBER_COUNT 2
    char ProxyServer[64];
    int ProxyServerPort;
}Network_Table_t;
typedef struct
{
    char Domain[64];
}Operator_Table_t;
typedef struct
{
/* CLIENT_TABLE_MEMBER_COUNT has to be updated accordance with number of members of the structure */
#define CLIENT_TABLE_MEMBER_COUNT 3
    char IMPUId[64];
    char AuthUserName[64];
    char AuthPassword[64];
}Client_Table_t;
typedef struct
{
    bool CallWaitingEnable;
}Set_Table_t;
typedef struct
{
    int profile_count;
    VoIPProfile_Table_t* profileTable;
    int pots_count;
    POTS_Table_t* potsTable;
    int fxs_count;
    FXS_Table_t* fxsTable;
    int network_count;
    Network_Table_t* networkTable;
    int operator_count;
    Operator_Table_t* operatorTable;
    int client_count;
    Client_Table_t* clientTable;
    int set_count;
    Set_Table_t* setTable;
}VoiceService_Table_t;
typedef struct
{
    void     *data;
    int      voiceservice_count;
    VoiceService_Table_t* voiceserviceTable;
    char      subdoc_name[32];
    uint64_t  version;
    uint16_t  transaction_id;
}TR104_WebConfig_t;

void TR104_free_TR181_resources(int total_params,char** aParamDetail)
{
    int i=0;
    for(i=0;i<total_params;i++)
    {
        if(aParamDetail[i])
        {
            CcspTraceDebug(("%s:%d freeing aParamDetail[%d] at %p with size = %zu\n",__FUNCTION__,__LINE__,i,aParamDetail[i],sizeof(aParamDetail[i])));
            free(aParamDetail[i]);
            aParamDetail[i]=NULL;
        }
    }
    if(aParamDetail)
    {
        CcspTraceDebug(("%s:%d freeing aParamDetail at %p with size = %zu\n",__FUNCTION__,__LINE__,aParamDetail,sizeof(aParamDetail)));
        free(aParamDetail);
        aParamDetail=NULL;
    }
}


void TR104_Process_free_resources(void *arg)
{
    CcspTraceDebug(("Entering: %s\n",__FUNCTION__));
    if (arg == NULL) {
        CcspTraceError(("%s: Input Data is NULL\n",__FUNCTION__));
        return;
    }
    execData *blob_exec_data  = (execData*) arg;
    TR104_WebConfig_t* pTR104_WebConfig = (TR104_WebConfig_t*)blob_exec_data->user_data;
    if(blob_exec_data)
    {
        CcspTraceDebug(("%s:%d freeing blob_exec_data at %p with size=%zu\n",__FUNCTION__,__LINE__,blob_exec_data,sizeof(blob_exec_data)));
        free(blob_exec_data);
        blob_exec_data=NULL;
    }
    CcspTraceDebug(("free all the memory allocated dynamically under pTR104_WebConfig\n"));
    if(pTR104_WebConfig!=NULL)
    {
        int i;
        for (i=0;i<pTR104_WebConfig->voiceservice_count;i++)
        {
            if(pTR104_WebConfig->voiceserviceTable)
            {
                CcspTraceDebug(("check pTR104_WebConfig->voiceserviceTable[%d].profileTable is null or not\n",i));
                if(pTR104_WebConfig->voiceserviceTable[i].profileTable)
                {
                    CcspTraceDebug(("freeing pTR104_WebConfig->voiceserviceTable[%d].profileTable at %p with size = %zu\n",i,pTR104_WebConfig->voiceserviceTable[i].profileTable,sizeof(pTR104_WebConfig->voiceserviceTable[i].profileTable)));
                    free(pTR104_WebConfig->voiceserviceTable[i].profileTable);
                    pTR104_WebConfig->voiceserviceTable[i].profileTable=NULL;
                }
                CcspTraceDebug(("check pTR104_WebConfig->voiceserviceTable[%d].potsTable is null or not\n",i));
                if(pTR104_WebConfig->voiceserviceTable[i].potsTable)
                {
                    CcspTraceDebug(("freeing pTR104_WebConfig->voiceserviceTable[%d].potsTable at %p with size=%zu\n",i,pTR104_WebConfig->voiceserviceTable[i].potsTable,sizeof(pTR104_WebConfig->voiceserviceTable[i].potsTable)));
                    free(pTR104_WebConfig->voiceserviceTable[i].potsTable);
                    pTR104_WebConfig->voiceserviceTable[i].potsTable=NULL;
                }
                CcspTraceDebug(("check pTR104_WebConfig->voiceserviceTable[%d].fxsTable is null or not\n",i));
                if(pTR104_WebConfig->voiceserviceTable[i].fxsTable)
                {
                    CcspTraceDebug(("freeing pTR104_WebConfig->voiceserviceTable[%d].fxsTable at %p with size=%zu\n",i,pTR104_WebConfig->voiceserviceTable[i].fxsTable,sizeof(pTR104_WebConfig->voiceserviceTable[i].fxsTable)));
                    free(pTR104_WebConfig->voiceserviceTable[i].fxsTable);
                    pTR104_WebConfig->voiceserviceTable[i].fxsTable=NULL;
                }
                CcspTraceDebug(("check pTR104_WebConfig->voiceserviceTable[%d].neteworkTable is null or not\n",i));
                if(pTR104_WebConfig->voiceserviceTable[i].networkTable)
                {
                    CcspTraceDebug(("freeing pTR104_WebConfig->voiceserviceTable[%d].networkTable at %p with size=%zu\n",i,pTR104_WebConfig->voiceserviceTable[i].networkTable,sizeof(pTR104_WebConfig->voiceserviceTable[i].networkTable)));
                    free(pTR104_WebConfig->voiceserviceTable[i].networkTable);
                    pTR104_WebConfig->voiceserviceTable[i].networkTable=NULL;

                }
                CcspTraceDebug(("check pTR104_WebConfig->voiceserviceTable[%d].operatorTable is null or not\n",i));
                if(pTR104_WebConfig->voiceserviceTable[i].operatorTable)
                {
                    CcspTraceDebug(("freeing pTR104_WebConfig->voiceserviceTable[%d].operatorTable at %p with size=%zu\n",i,pTR104_WebConfig->voiceserviceTable[i].operatorTable,sizeof(pTR104_WebConfig->voiceserviceTable[i].operatorTable)));
                    free(pTR104_WebConfig->voiceserviceTable[i].operatorTable);
                    pTR104_WebConfig->voiceserviceTable[i].operatorTable=NULL;
                }
                CcspTraceDebug(("check pTR104_WebConfig->voiceserviceTable[%d].clientTable is null or not\n",i));
                if(pTR104_WebConfig->voiceserviceTable[i].clientTable)
                {
                    CcspTraceDebug(("freeing pTR104_WebConfig->voiceserviceTable[%d].clientTable at %p with size = %zu\n",i,pTR104_WebConfig->voiceserviceTable[i].clientTable,sizeof(pTR104_WebConfig->voiceserviceTable[i].clientTable)));
                    free(pTR104_WebConfig->voiceserviceTable[i].clientTable);
                    pTR104_WebConfig->voiceserviceTable[i].clientTable=NULL;
                }
                CcspTraceDebug(("check pTR104_WebConfig->voiceserviceTable[%d].setTable is null or not\n",i));
                if(pTR104_WebConfig->voiceserviceTable[i].setTable)
                {
                    CcspTraceDebug(("freeing pTR104_WebConfig->voiceserviceTable[%d].setTable at %p with size = %zu\n",i,pTR104_WebConfig->voiceserviceTable[i].setTable,sizeof(pTR104_WebConfig->voiceserviceTable[i].setTable)));
                    free(pTR104_WebConfig->voiceserviceTable[i].setTable);
                    pTR104_WebConfig->voiceserviceTable[i].setTable=NULL;
                }
            }
        }
        CcspTraceDebug(("freeing pTR104_WebConfig->voiceserviceTable at %p with size = %zu\n",pTR104_WebConfig->voiceserviceTable,sizeof(pTR104_WebConfig->voiceserviceTable)));
        free(pTR104_WebConfig->voiceserviceTable);
        pTR104_WebConfig->voiceserviceTable=NULL;
        CcspTraceDebug(("freeing pTR104_WebConfig at %p with size = %zu\n",pTR104_WebConfig,sizeof(pTR104_WebConfig)));
        free(pTR104_WebConfig);
        pTR104_WebConfig=NULL;
        CcspTraceInfo(("%s:Success in Clearing TR104 webconfig resources\n",__FUNCTION__));
    }
}

int param_count(TR104_WebConfig_t* pTR104_WebConfig)
{
    CcspTraceDebug(("Entering %s\n",__FUNCTION__));
    int count=0,i;
    CcspTraceDebug(("voiceservice_count=%d\n",pTR104_WebConfig->voiceservice_count));
    for(i=0;i<pTR104_WebConfig->voiceservice_count;i++)
    {
        CcspTraceDebug(("profile_count=%d\npots_count=%d\nfxs_count=%d\nnetwok_count=%d\nclient_count=%d\noperator_count=%d\nset_count=%d\n",pTR104_WebConfig->voiceserviceTable[i].profile_count,pTR104_WebConfig->voiceserviceTable[i].pots_count,pTR104_WebConfig->voiceserviceTable[i].fxs_count,(pTR104_WebConfig->voiceserviceTable[i].network_count*NETWORK_TABLE_MEMBER_COUNT),(pTR104_WebConfig->voiceserviceTable[i].client_count*CLIENT_TABLE_MEMBER_COUNT),pTR104_WebConfig->voiceserviceTable[i].operator_count,pTR104_WebConfig->voiceserviceTable[i].set_count));
        count+=pTR104_WebConfig->voiceserviceTable[i].profile_count+pTR104_WebConfig->voiceserviceTable[i].fxs_count+(pTR104_WebConfig->voiceserviceTable[i].network_count*NETWORK_TABLE_MEMBER_COUNT)+(pTR104_WebConfig->voiceserviceTable[i].client_count*CLIENT_TABLE_MEMBER_COUNT)+pTR104_WebConfig->voiceserviceTable[i].operator_count+pTR104_WebConfig->voiceserviceTable[i].set_count+pTR104_WebConfig->voiceserviceTable[i].pots_count;
    }
    CcspTraceInfo(("total number of parameters received from blob=%d\n",count));
    return count;
}

pErr TR104_Process_Webconfig_Request(void *Data)
{
    CcspTraceDebug(("Entering %s\n",__FUNCTION__));
    pErr execRetVal=NULL;
    int total_params=0,i,j,index=0;
    char tr104_applied_state[10] = {0};
    int counter = 0;
    if (Data == NULL)
    {
        CcspTraceError(("%s: Input Data is NULL\n",__FUNCTION__));
        return execRetVal;
    }
    TR104_WebConfig_t* pTR104_WebConfig = (TR104_WebConfig_t*) Data;
    MTAMGMT_MTA_PROVISION_STATUS provisionStatus = 0;
    MTAMGMT_MTA_STATUS operationalStatus = 0;
  
    int tr104_sysevent_fd;
    token_t tr104_sysevent_token;

    tr104_sysevent_fd = sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, "TR104_Apply", &tr104_sysevent_token);
    if (tr104_sysevent_fd < 0){
            CcspTraceError(("%s sysevent_open failed \n",__FUNCTION__));
            return NULL;
    }
  
    execRetVal = (pErr ) malloc (sizeof(Err));
    memset(execRetVal,0,(sizeof(Err)));
    CcspTraceDebug(("%s:%d allocated memory for execRetVal at %p\n",__FUNCTION__,__LINE__,execRetVal));
    for(i=0;i<10;i++)
    {
        if(mta_hal_getMtaProvisioningStatus(&provisionStatus) != 0)
        {
            CcspTraceError(("%s %d mta_hal_getMtaProvisioningStatus fail\n",__FUNCTION__,i));
            execRetVal->ErrorCode = TR104_HAL_FAILURE;
            sleep(30);
        }
        else
        {
            if( strcmp(mta_provision_status[provisionStatus],MTA_PROVISIONED) )
            {
                CcspTraceInfo(("%s %d provisionStatus = %s \n",__FUNCTION__,i,mta_provision_status[provisionStatus]));
                execRetVal->ErrorCode = BLOB_EXEC_FAILURE;
                sleep(30);
            }
            else
            {
                break;
            }
        }
        if(i==9)
        {
            CcspTraceError(("Tried for 10 times but it seems box is not MTA provisioned/ api is not able to provide the information\n"));
            return execRetVal;
        }
    }
    total_params=param_count(pTR104_WebConfig);
    char **aParamDetail = NULL;
    aParamDetail=(char**)malloc(sizeof(char*)*total_params);
    if(aParamDetail==NULL)
    {
        CcspTraceError(("malloc failure/memory not assigned\n"));
        execRetVal->ErrorCode = BLOB_EXEC_FAILURE;
        return execRetVal;

    }
    CcspTraceDebug(("%s:%d allocated memory for aParamDetail at %p with size = %zu\n",__FUNCTION__,__LINE__,aParamDetail,sizeof(aParamDetail)));
    CcspTraceInfo(("aParamDetail has been allocated memory for %d char pointers\n",total_params));
    for(i=0;i<pTR104_WebConfig->voiceservice_count;i++)
    {

        for(j=0;j<pTR104_WebConfig->voiceserviceTable[i].pots_count;j++)
        {
            aParamDetail[index] = (char*)malloc(MTA_MAX_DETAIL_SIZE*sizeof(char));
            if( aParamDetail[index] != NULL )
            {
                CcspTraceDebug(("%s:%d allocated memory for aParamDetail[%d] at %p with size = %zu\n",__FUNCTION__,__LINE__,index,aParamDetail[index],sizeof(aParamDetail[index])));
                /* CID 190022  Calling risky function fix */
                snprintf(aParamDetail[index], MTA_MAX_DETAIL_SIZE, "Device.Services.VoiceService.%d.POTS.X_RDKCENTRAL-COM_LocalTimeZone",i+1);
                snprintf(aParamDetail[index]+strlen(aParamDetail[index]), MTA_MAX_DETAIL_SIZE, ",%s,%s","string",pTR104_WebConfig->voiceserviceTable[i].potsTable[j].LocalTimeZone);
                CcspTraceDebug(("%s:%d aParamDetail[%d]=%s\n",__FUNCTION__,__LINE__,index,aParamDetail[index]));
                index++;
            }
            else
            {
                TR104_free_TR181_resources(total_params,aParamDetail);
                return execRetVal;
            }
        }
        for(j=0;j<pTR104_WebConfig->voiceserviceTable[i].network_count;j++)
        {
            aParamDetail[index] = (char*)malloc(MTA_MAX_DETAIL_SIZE*sizeof(char));
            if( aParamDetail[index] != NULL )
            {
                CcspTraceDebug(("%s:%d allocated memory for aParamDetail[%d] at %p with size = %zu\n",__FUNCTION__,__LINE__,index,aParamDetail[index],sizeof(aParamDetail[index])));
                snprintf(aParamDetail[index], MTA_MAX_DETAIL_SIZE, "Device.Services.VoiceService.%d.SIP.Network.%d.ProxyServer",i+1,j+1);
                snprintf(aParamDetail[index]+strlen(aParamDetail[index]), MTA_MAX_DETAIL_SIZE,",%s,%s","string",pTR104_WebConfig->voiceserviceTable[i].networkTable[j].ProxyServer);
                CcspTraceDebug(("%s:%d aParamDetail[%d]=%s\n",__FUNCTION__,__LINE__,index,aParamDetail[index]));
                index++;
            }
            else
            {
                TR104_free_TR181_resources(total_params,aParamDetail);
                return execRetVal;
            }
            aParamDetail[index] = (char*)malloc(MTA_MAX_DETAIL_SIZE*sizeof(char));
            if( aParamDetail[index] != NULL )
            {
                CcspTraceDebug(("%s:%d allocated memory for aParamDetail[%d] at %p with size = %zu\n",__FUNCTION__,__LINE__,index,aParamDetail[index],sizeof(aParamDetail[index])));
                snprintf(aParamDetail[index], MTA_MAX_DETAIL_SIZE, "Device.Services.VoiceService.%d.SIP.Network.%d.ProxyServerPort",i+1,j+1);
                snprintf(aParamDetail[index]+strlen(aParamDetail[index]), MTA_MAX_DETAIL_SIZE,",%s,%d","unsignedInt",pTR104_WebConfig->voiceserviceTable[i].networkTable[j].ProxyServerPort);
                CcspTraceDebug(("%s:%d aParamDetail[%d]=%s\n",__FUNCTION__,__LINE__,index,aParamDetail[index]));
                index++;
            }
            else
            {
                TR104_free_TR181_resources(total_params,aParamDetail);
                return execRetVal;
            }
        }
        for(j=0;j<pTR104_WebConfig->voiceserviceTable[i].operator_count;j++)
        {
            aParamDetail[index] = (char*)malloc(MTA_MAX_DETAIL_SIZE*sizeof(char));
            if( aParamDetail[index] != NULL )
            {
                CcspTraceDebug(("%s:%d allocated memory for aParamDetail[%d] at %p with size = %zu\n",__FUNCTION__,__LINE__,index,aParamDetail[index],sizeof(aParamDetail[index])));
                snprintf(aParamDetail[index], MTA_MAX_DETAIL_SIZE,"Device.Services.VoiceService.%d.SIP.X_RDKCENTRAL-COM_Operator.%d.Domain",i+1,j+1);
                snprintf(aParamDetail[index]+strlen(aParamDetail[index]), MTA_MAX_DETAIL_SIZE,",%s,%s","string",pTR104_WebConfig->voiceserviceTable[i].operatorTable[j].Domain);
                CcspTraceDebug(("%s:%d aParamDetail[%d]=%s\n",__FUNCTION__,__LINE__,index,aParamDetail[index]));
                index++;
            }
            else
            {
                TR104_free_TR181_resources(total_params,aParamDetail);
                return execRetVal;
            }
        }
        for(j=0;j<pTR104_WebConfig->voiceserviceTable[i].client_count;j++)
        {
            aParamDetail[index] = (char*)malloc(MTA_MAX_DETAIL_SIZE*sizeof(char));
            if( aParamDetail[index] != NULL )
            {
                CcspTraceDebug(("%s:%d allocated memory for aParamDetail[%d] at %p with size = %zu\n",__FUNCTION__,__LINE__,index,aParamDetail[index],sizeof(aParamDetail[index])));
                snprintf(aParamDetail[index], MTA_MAX_DETAIL_SIZE,"Device.Services.VoiceService.%d.SIP.Client.%d.X_RDKCENTRAL-COM_IMPU.IMPUId",i+1,j+1);
                snprintf(aParamDetail[index]+strlen(aParamDetail[index]), MTA_MAX_DETAIL_SIZE,",%s,%s","string",pTR104_WebConfig->voiceserviceTable[i].clientTable[j].IMPUId);
                CcspTraceDebug(("%s:%d aParamDetail[%d]=%s\n",__FUNCTION__,__LINE__,index,aParamDetail[index]));
                index++;
            }
            else
            {
                TR104_free_TR181_resources(total_params,aParamDetail);
                return execRetVal;
            }
            aParamDetail[index] = (char*)malloc(MTA_MAX_DETAIL_SIZE*sizeof(char));
            if( aParamDetail[index] != NULL )
            {
                CcspTraceDebug(("%s:%d allocated memory for aParamDetail[%d] at %p with size = %zu\n",__FUNCTION__,__LINE__,index,aParamDetail[index],sizeof(aParamDetail[index])));
                snprintf(aParamDetail[index], MTA_MAX_DETAIL_SIZE,"Device.Services.VoiceService.%d.SIP.Client.%d.AuthUserName",i+1,j+1);
                snprintf(aParamDetail[index]+strlen(aParamDetail[index]), MTA_MAX_DETAIL_SIZE,",%s,%s","string",pTR104_WebConfig->voiceserviceTable[i].clientTable[j].AuthUserName);
                CcspTraceDebug(("%s:%d aParamDetail[%d]=%s\n",__FUNCTION__,__LINE__,index,aParamDetail[index]));
                index++;
            }
            else
            {
                TR104_free_TR181_resources(total_params,aParamDetail);
                return execRetVal;
            }
            aParamDetail[index] = (char*)malloc(MTA_MAX_DETAIL_SIZE*sizeof(char));
            if( aParamDetail[index] != NULL )
            {
                CcspTraceDebug(("%s:%d allocated memory for aParamDetail[%d] at %p with size = %zu\n",__FUNCTION__,__LINE__,index,aParamDetail[index],sizeof(aParamDetail[index])));
                snprintf(aParamDetail[index], MTA_MAX_DETAIL_SIZE,"Device.Services.VoiceService.%d.SIP.Client.%d.AuthPassword",i+1,j+1);
                snprintf(aParamDetail[index]+strlen(aParamDetail[index]), MTA_MAX_DETAIL_SIZE,",%s,%s","string",pTR104_WebConfig->voiceserviceTable[i].clientTable[j].AuthPassword);
                CcspTraceDebug(("%s:%d aParamDetail[%d]=%s\n",__FUNCTION__,__LINE__,index,aParamDetail[index]));
                index++;
            }
            else
            {
                TR104_free_TR181_resources(total_params,aParamDetail);
                return execRetVal;
            }
        }
        for(j=0;j<pTR104_WebConfig->voiceserviceTable[i].set_count;j++)
        {
            aParamDetail[index] = (char*)malloc(MTA_MAX_DETAIL_SIZE*sizeof(char));
            if( aParamDetail[index] != NULL )
            {
                CcspTraceDebug(("%s:%d allocated memory for aParamDetail[%d] at %p with size = %zu\n",__FUNCTION__,__LINE__,index,aParamDetail[index],sizeof(aParamDetail[index])));
                snprintf(aParamDetail[index], MTA_MAX_DETAIL_SIZE,"Device.Services.VoiceService.%d.CallControl.CallingFeatures.Set.%d.CallWaitingEnable",i+1,j+1);
                if(pTR104_WebConfig->voiceserviceTable[i].setTable[j].CallWaitingEnable == true)
                {
                    CcspTraceDebug(("%s:%d sending value true to hal\n",__FUNCTION__,__LINE__));
                    snprintf(aParamDetail[index]+strlen(aParamDetail[index]), MTA_MAX_DETAIL_SIZE, ",%s,%s", "boolean","true");
                    CcspTraceDebug(("%s:%d aParamDetail[%d]=%s\n",__FUNCTION__,__LINE__,index,aParamDetail[index]));
                }
                else if(pTR104_WebConfig->voiceserviceTable[i].setTable[j].CallWaitingEnable == false)
                {
                    CcspTraceDebug(("%s:%d sending value false to hal\n",__FUNCTION__,__LINE__));
                    snprintf(aParamDetail[index]+strlen(aParamDetail[index]), MTA_MAX_DETAIL_SIZE, ",%s,%s","boolean","false");
                    CcspTraceDebug(("%s:%d aParamDetail[%d]=%s\n",__FUNCTION__,__LINE__,index,aParamDetail[index]));
                }
                index++;
            }
            else
            {
                TR104_free_TR181_resources(total_params,aParamDetail);
                return execRetVal;
            }
        }
        for(j=0;j<pTR104_WebConfig->voiceserviceTable[i].profile_count;j++)
        {
            aParamDetail[index] = (char*)malloc(MTA_MAX_DETAIL_SIZE*sizeof(char));
            if( aParamDetail[index] != NULL )
            {
                CcspTraceDebug(("%s:%d allocated memory for aParamDetail[%d] at %p with size = %zu\n",__FUNCTION__,__LINE__,index,aParamDetail[index],sizeof(aParamDetail[index])));
                snprintf(aParamDetail[index], MTA_MAX_DETAIL_SIZE,"Device.Services.VoiceService.%d.VoIPProfile.%d.X_RDKCENTRAL-COM_Device.DevEnabled",i+1,j+1);
                if(pTR104_WebConfig->voiceserviceTable[i].profileTable[j].DevEnabled == true)
                {
                    CcspTraceDebug(("%s:%d sending value true to hal\n",__FUNCTION__,__LINE__));
                    snprintf(aParamDetail[index]+strlen(aParamDetail[index]), MTA_MAX_DETAIL_SIZE, ",%s,%s","boolean","true");
                }
                else if(pTR104_WebConfig->voiceserviceTable[i].profileTable[j].DevEnabled == false)
                {
                    CcspTraceDebug(("%s:%d sending value false to hal\n",__FUNCTION__,__LINE__));
                    snprintf(aParamDetail[index]+strlen(aParamDetail[index]), MTA_MAX_DETAIL_SIZE,",%s,%s","boolean","false");
                }
                CcspTraceDebug(("%d:aParamDetail[%d]=%s\n",__LINE__,index,aParamDetail[index]));
                index++;
            }
            else
            {
                TR104_free_TR181_resources(total_params,aParamDetail);
                return execRetVal;
            }
        }
        for(j=0;j<pTR104_WebConfig->voiceserviceTable[i].fxs_count;j++)
        {
            aParamDetail[index] = (char*)malloc(MTA_MAX_DETAIL_SIZE*sizeof(char));
            if( aParamDetail[index] != NULL )
            {
                CcspTraceDebug(("%s:%d allocated memory for aParamDetail[%d] at %p with size = %zu\n",__FUNCTION__,__LINE__,index,aParamDetail[index],sizeof(aParamDetail[index])));
                snprintf(aParamDetail[index], MTA_MAX_DETAIL_SIZE,"Device.Services.VoiceService.%d.POTS.FXS.%d.Enable",i+1,j+1);
                if(pTR104_WebConfig->voiceserviceTable[i].fxsTable[j].Enable == true)
                {
                    CcspTraceDebug(("%s:%d sending value true to hal\n",__FUNCTION__,__LINE__));
                    snprintf(aParamDetail[index]+strlen(aParamDetail[index]), MTA_MAX_DETAIL_SIZE, ",%s,%s","boolean","true");
                }
                else if(pTR104_WebConfig->voiceserviceTable[i].fxsTable[j].Enable == false)
                {
                    CcspTraceDebug(("%s:%d sending value false to hal\n",__FUNCTION__,__LINE__));
                    snprintf(aParamDetail[index]+strlen(aParamDetail[index]), MTA_MAX_DETAIL_SIZE, ",%s,%s","boolean","false");
                }
                CcspTraceDebug(("%d:aParamDetail[%d]=%s\n",__LINE__,index,aParamDetail[index]));
                index++;
            }
            else
            {
                TR104_free_TR181_resources(total_params,aParamDetail);
                return execRetVal;
            }
        }
    }
  
    do 
    {
        if( RETURN_OK != mta_hal_getMtaOperationalStatus(&operationalStatus)) 
	{
	    CcspTraceInfo((" %s: mta_hal_getMtaOperationalStatus Fail \n ",__FUNCTION__));

        }
        // True when MTA operational status if complete or Rejected
        else if( !strcmp(MTA_status[operationalStatus],"MTA_COMPLETE")  || !strcmp(MTA_status[operationalStatus],"MTA_REJECTED")) 
	{
	    CcspTraceInfo(("%s operationalStatus = %s \n",__FUNCTION__,MTA_status[operationalStatus ]));
	    break;
	}
        sleep(5);
        counter = counter + 5;
    } while (counter < 600);
    
    // Changed the time from 150 seconds to 600 seconds 
    for(j=0;j<20;j++)
    {
        CcspTraceInfo(("calling HAL set for %d time\n",j));
        if (0 != mta_hal_setTR104parameterValues(aParamDetail, &total_params))
        {
            CcspTraceError(("ccspMtaAgentTR104Hal: Set Failed\n"));
            execRetVal->ErrorCode = TR104_HAL_FAILURE;
            //retry logic is not needed if configuration is pushed through webconfig server
            if(isbootup==1)
            {
                if(j==19)
                {
                    CcspTraceError(("hal api set for 20 times.So giving up as of now\n"));
                    TR104_free_TR181_resources(total_params,aParamDetail);
		    /* CID 190026 Unchecked return value from library */
		    int retrmv =  remove("/nvram/.vsb64_temp.txt");
		    if(retrmv != 0)
		    {
			    CcspTraceError(("Error in removing /nvram/.vsb64_temp.txt \n"));
		    }
                    return execRetVal;
                }
                sleep(30);
            }
            else
            {
                TR104_free_TR181_resources(total_params,aParamDetail);
		/* CID 190026 Unchecked return value from library */
		int retrmv =  remove("/nvram/.vsb64_temp.txt");
		if(retrmv != 0)
		{
			CcspTraceError(("Error in removing /nvram/.vsb64_temp.txt \n"));
		}
                CcspTraceError(("hal api set failed\n"));
                return execRetVal;
            }
        }
        else
        {
            break;
        }
    }
    CcspTraceInfo(("mta_hal_setTR104parameterValues returned success with total_params=%d\n",total_params));
    execRetVal->ErrorCode = BLOB_EXEC_SUCCESS;

    sysevent_set(tr104_sysevent_fd, tr104_sysevent_token, "tr104_applied", "true", 0);
    sysevent_get(tr104_sysevent_fd, tr104_sysevent_token, "tr104_applied", tr104_applied_state, sizeof(tr104_applied_state));
    
    CcspTraceInfo(("%s tr104_applied_state = %s \n",__FUNCTION__,tr104_applied_state));
  
    if (0 <= tr104_sysevent_fd) {
        sysevent_close(tr104_sysevent_fd, tr104_sysevent_token);
        tr104_sysevent_fd = -1;
    }
    tr104_sysevent_token = 0;

    if(isbootup==1)
    {
      CcspTraceDebug(("%s:no need to check for temporary file and copying it to original file\n",__FUNCTION__));
    }
    else
    {
        fptr = fopen("/nvram/.vsb64.txt","w");
        if(!fptr)
        {
            CcspTraceError(("%s:original file not opened\n",__FUNCTION__));
        }
        else
        {
            char* buffer=0;
            int length;
            FILE* fptr_dummy= fopen("/nvram/.vsb64_temp.txt","rb");
            if (fptr_dummy)
            {
                fseek (fptr_dummy, 0, SEEK_END);
                length = ftell (fptr_dummy);
                if (length >= 0) {
                  fseek (fptr_dummy, 0, SEEK_SET);
                  buffer = (char*)malloc(length+1);
                  if (buffer)
                  {
                    /* Coverity Fix CID : 190019 Ignoring number of bytes read */
                    size_t bytesRead = fread (buffer, 1, length, fptr_dummy); 
                    if (bytesRead == (size_t)length) 
                    {
                        buffer[length] = '\0'; // Ensure string is null-terminated
                        fputs(buffer,fptr);
                    } 
                    else 
                    {
                        CcspTraceError(("%s: fread incomplete. Expected %d bytes, got %zu bytes.\n", __FUNCTION__, length, bytesRead));
                    }
                  }
                CcspTraceDebug(("%s:base64 has been copied to original file with length=%u\n",__FUNCTION__,length));
                }
                fclose(fptr_dummy);
		/* CID 190026 Unchecked return value from library */
		int retrmv =  remove("/nvram/.vsb64_temp.txt");
		if(retrmv != 0)
		{
			CcspTraceError(("Error in removing /nvram/.vsb64_temp.txt \n"));
		}
                free(buffer);
            }
            else
            {
                CcspTraceError(("%s:temporary file have some problem\n",__FUNCTION__));
            }
            fclose(fptr);
        }
    }
    TR104_free_TR181_resources(total_params,aParamDetail);
    return execRetVal;
}
int WebConfig_Process_profile_Params(VoIPProfile_Table_t* e,msgpack_object_map *map)
{
    CcspTraceDebug(("Entering %s\n",__FUNCTION__));
    msgpack_object_kv *p = map->ptr;

    //Validate param
    if( NULL == p )
    {
        CcspTraceError(("%s Invalid Pointer\n", __FUNCTION__));
        return RETURN_ERR;
    }
    if (strncmp(p->key.via.str.ptr,"DevEnabled",p->key.via.str.size) == 0)
    {
        if (p->val.type == MSGPACK_OBJECT_BOOLEAN)
        {
            if (p->val.via.boolean)
            {
                e->DevEnabled = true;
                CcspTraceDebug(("DevEnabled given true to set\n"));
            }
            else
            {
                e->DevEnabled = false;
                CcspTraceDebug(("DevEnabled given false to set\n"));
            }
        }
        else
        {
            CcspTraceError(("object sent not a boolean\n"));
            return VALIDATION_FALIED;
        }

    }
    else
    {
        CcspTraceError(("object name mismatch\n"));
        return VALIDATION_FALIED;
    }
    return RETURN_OK;
}
int WebConfig_Process_pots_Params(POTS_Table_t* e,msgpack_object_map *map)
{
    CcspTraceDebug(("Entering %s...\n",__FUNCTION__));
    msgpack_object_kv *p = map->ptr;

    //Validate param
    if( NULL == p )
    {
        CcspTraceError(("%s Invalid Pointer\n", __FUNCTION__));
        return RETURN_ERR;
    }
    if(strncmp(p->key.via.str.ptr,"LocalTimeZone",p->key.via.str.size) == 0)
    {
        if (p->val.type == MSGPACK_OBJECT_STR)
        {
            CcspTraceDebug(("copying the string value\n"));
            strncpy(e->LocalTimeZone,p->val.via.str.ptr,p->val.via.str.size);
            e->LocalTimeZone[p->val.via.str.size]='\0';
            CcspTraceDebug(("done strcpy\n"));
        }
        else
        {
            CcspTraceError(("object type is not string\n"));
            return VALIDATION_FALIED;
        }
    }
    else
    {
        CcspTraceError(("object name mismatch\n"));
        return VALIDATION_FALIED;
    }
    return RETURN_OK;
}
int WebConfig_Process_fxs_Params(FXS_Table_t* e,msgpack_object_map *map)
{
    CcspTraceDebug(("Entering %s...\n",__FUNCTION__));
    msgpack_object_kv *p = map->ptr;

    //Validate param
    if( NULL == p )
    {
        CcspTraceError(("%s Invalid Pointer\n", __FUNCTION__));
        return RETURN_ERR;
    }
    if (strncmp(p->key.via.str.ptr,"Enable",p->key.via.str.size) == 0)
    {
        if (p->val.type == MSGPACK_OBJECT_BOOLEAN)
        {
            if (p->val.via.boolean)
            {
                CcspTraceDebug(("assigning FXS Enable value as true\n"));
                e->Enable = true;
            }
            else
            {
                CcspTraceDebug(("assigning FXS Enable value as false\n"));
                e->Enable = false;
            }
        }
        else
        {
            CcspTraceError(("%s: object type not a boolean\n",__FUNCTION__));
            return VALIDATION_FALIED;
        }
    }
    else
    {
        CcspTraceError(("%s: object name mismatch\n",__FUNCTION__));
        return VALIDATION_FALIED;
    }
    return RETURN_OK;
}
int WebConfig_Process_network_Params(Network_Table_t* e,msgpack_object_map *map)
{
    CcspTraceDebug(("Entering %s...\n",__FUNCTION__));
    msgpack_object_kv *p = map->ptr;
    int i;
    //Validate param
    if( NULL == p )
    {
        CcspTraceError(("%s Invalid Pointer\n", __FUNCTION__));
        return RETURN_ERR;
    }
    CcspTraceDebug(("map->size=%d\n",(int)map->size));
    for(i = 0;i < (int)map->size; i++)
    {
        if (strncmp(p->key.via.str.ptr,"RegistrarServer",p->key.via.str.size) == 0)
        {
        }
        else if(strncmp(p->key.via.str.ptr,"RegistrarServerPort",p->key.via.str.size) == 0)
        {
        }
        else if(strncmp(p->key.via.str.ptr,"ProxyServer",p->key.via.str.size) == 0)
        {
            if (p->val.type == MSGPACK_OBJECT_STR)
            {
                strncpy(e->ProxyServer,p->val.via.str.ptr,p->val.via.str.size);
                e->ProxyServer[p->val.via.str.size]='\0';
                CcspTraceDebug(("String %s copied to proxyServer\n",e->ProxyServer));
            }
            else
            {
                CcspTraceError(("%s: object type is not string\n",__FUNCTION__));
                return VALIDATION_FALIED;
            }
        }
        else if(strncmp(p->key.via.str.ptr,"ProxyServerPort",p->key.via.str.size) == 0)
        {
            if(p->val.type == MSGPACK_OBJECT_POSITIVE_INTEGER)
            {
                CcspTraceDebug(("assigning the positive integer value to ProxyServerPort\n"));
                e->ProxyServerPort=p->val.via.u64;
            }
            else if(p->val.type == MSGPACK_OBJECT_NEGATIVE_INTEGER)
            {
                CcspTraceDebug(("assigning the negative integer value to ProxyServerPort\n"));
                e->ProxyServerPort=p->val.via.i64;
            }
            else
            {
                CcspTraceDebug(("ProxyServerPort neither +ve nor -ve\n"));
            }
        }
        else
        {
            CcspTraceError(("object name mismatch\n"));
            return VALIDATION_FALIED;
        }
        p++;
    }
    return RETURN_OK;
}
int WebConfig_Process_operator_Params(Operator_Table_t* e,msgpack_object_map *map)
{
    CcspTraceDebug(("Entering %s...\n",__FUNCTION__));
    msgpack_object_kv *p = map->ptr;

    //Validate param
    if( NULL == p )
    {
        CcspTraceError(("%s Invalid Pointer\n", __FUNCTION__));
        return RETURN_ERR;
    }
    CcspTraceDebug(("comparing Domain\n"));
    if (strncmp(p->key.via.str.ptr,"Domain",p->key.via.str.size) == 0)
    {
        if (p->val.type == MSGPACK_OBJECT_STR)
        {
            CcspTraceDebug(("copying the string value\n"));
            strncpy(e->Domain,p->val.via.str.ptr,p->val.via.str.size);
            e->Domain[p->val.via.str.size]='\0';
            CcspTraceDebug(("done strcpy\n"));
        }
        else
        {
            CcspTraceError(("object type is not string\n"));
            return VALIDATION_FALIED;
        }
    }
    else
    {
        CcspTraceError(("object name mismatch\n"));
        return VALIDATION_FALIED;
    }
    return RETURN_OK;
}

int WebConfig_Process_client_Params(Client_Table_t* e,msgpack_object_map *map)
{
    CcspTraceDebug(("Entering %s...\n",__FUNCTION__));
    int  i;
    msgpack_object_kv *p = map->ptr;

    //Validate param
    if( NULL == p )
    {
        CcspTraceError(("%s Invalid Pointer\n", __FUNCTION__));
        return RETURN_ERR;
    }
    CcspTraceDebug(("map->size=%d\n",(int)map->size));
    for(i = 0;i < (int)map->size; i++)
    {
        if (strncmp(p->key.via.str.ptr,"IMPUId",p->key.via.str.size) == 0)
        {
            CcspTraceDebug(("IMPUId comaprision success for i=%d\n",i));
            if (p->val.type == MSGPACK_OBJECT_STR)
            {
                CcspTraceDebug(("copying the value to IMPUID\n"));
                strncpy(e->IMPUId,p->val.via.str.ptr,p->val.via.str.size);
                e->IMPUId[p->val.via.str.size]='\0';
                CcspTraceDebug(("done strcpy %s for IMPUId\n",e->IMPUId));
            }
            else
            {
                CcspTraceError(("object type is not string\n"));
                return VALIDATION_FALIED;
            }
        }
        else if(strncmp(p->key.via.str.ptr,"AuthUserName",p->key.via.str.size) == 0)
        {
            CcspTraceDebug(("AuthUserName comaprision success for i=%d\n",i));
            if (p->val.type == MSGPACK_OBJECT_STR)
            {
                CcspTraceDebug(("copying the string value to AuthUserName\n"));
                strncpy(e->AuthUserName,p->val.via.str.ptr,p->val.via.str.size);
                e->AuthUserName[p->val.via.str.size]='\0';
                CcspTraceDebug(("done strcpy %s for AuthUserName\n",e->AuthUserName));
            }
            else
            {
                CcspTraceError(("object type is not string\n"));
                return VALIDATION_FALIED;
            }
        }
        else if(strncmp(p->key.via.str.ptr,"AuthPassword",p->key.via.str.size) == 0)
        {
            CcspTraceDebug(("AuthPassword comaprision success for i=%d\n",i));
            if (p->val.type == MSGPACK_OBJECT_STR)
            {
                CcspTraceDebug(("copying the string value to AuthPassword\n"));
                strncpy(e->AuthPassword,p->val.via.str.ptr,p->val.via.str.size);
                e->AuthPassword[p->val.via.str.size]='\0';
                CcspTraceDebug(("done strcpy %s for AuthPassword\n",e->AuthPassword));
            }
            else
            {
                CcspTraceError(("object type is not string\n"));
                return VALIDATION_FALIED;
            }
        }
        else
        {
            CcspTraceError(("object name mismatch\n"));
            return VALIDATION_FALIED;
        }
        CcspTraceDebug(("i=%d\n",i));
        p++;
    }
    return RETURN_OK;
}

int WebConfig_Process_set_Params(Set_Table_t* e,msgpack_object_map *map)
{
    CcspTraceDebug(("Entering %s...\n",__FUNCTION__));
    msgpack_object_kv *p = map->ptr;

    //Validate param
    if( NULL == p )
    {
        CcspTraceError(("%s Invalid Pointer\n", __FUNCTION__));
        return RETURN_ERR;
    }
    CcspTraceDebug(("comparing CallWaitingEnable\n"));
    if (strncmp(p->key.via.str.ptr,"CallWaitingEnable",p->key.via.str.size) == 0)
    {
        if (p->val.type == MSGPACK_OBJECT_BOOLEAN)
        {
            if (p->val.via.boolean)
            {
                CcspTraceDebug(("assigning true value to enable\n"));
                e->CallWaitingEnable = true;
            }
            else
            {
                CcspTraceDebug(("assigning false to enable\n"));
                e->CallWaitingEnable = false;
            }
        }
        else
        {
            CcspTraceError(("object type is not boolean\n"));
            return VALIDATION_FALIED;
        }
    }
    else
    {
        CcspTraceError(("object name mismatch\n"));
        return VALIDATION_FALIED;
    }
    return RETURN_OK;
}

int WebConfig_Process_voiceservice_Params(VoiceService_Table_t* e, msgpack_object_map *map)
{
    CcspTraceDebug(("Entering %s...\n",__FUNCTION__));
    int i=0, j=0;
    msgpack_object_kv* p = map->ptr;

    //Validate param
    if( NULL == p )
    {
        CcspTraceError(("%s Invalid Pointer\n", __FUNCTION__));
        return RETURN_ERR;
    }
    CcspTraceDebug(("map->size=%d\n",(int)map->size));
    for(i = 0;i < (int)map->size; i++)
    {
        if (MSGPACK_OBJECT_ARRAY == p->val.type )
        {
            if( strncmp(p->key.via.str.ptr, "VoIPProfile",p->key.via.str.size) == 0)
            {
                CcspTraceDebug(("VoIPProfile comparision success at i=%d\n",i));
                e->profile_count=p->val.via.array.size;
                e->profileTable = (VoIPProfile_Table_t*)malloc(sizeof(VoIPProfile_Table_t)*e->profile_count);
                if(e->profileTable == NULL)
                {
                    e->profile_count=0;
                    return RETURN_ERR;
                }
                CcspTraceDebug(("%s:%d allocated memory for profileTable at %p with size = %zu\n",__FUNCTION__,__LINE__,e->profileTable,sizeof(e->profileTable)));
                CcspTraceDebug(("Profile Table Count=%d\n",e->profile_count));
                memset( e->profileTable,0,sizeof(VoIPProfile_Table_t)*e->profile_count);
                for(j=0;j<e->profile_count;j++)
                {
                    if( MSGPACK_OBJECT_MAP != p->val.via.array.ptr[j].type )
                    {
                        CcspTraceError(("%s %d - Invalid OBJECT \n",__FUNCTION__,__LINE__));
                        return VALIDATION_FALIED;
                    }
                    CcspTraceDebug(("calling WebConfig_Process_profile_Params for %d time\n",j+1));
                    rc = WebConfig_Process_profile_Params(&e->profileTable[j], &p->val.via.array.ptr[j].via.map);
                    if( RETURN_OK != rc )
                    {
                        return rc;
                    }
                }
            }
            else if(strncmp(p->key.via.str.ptr, "FXS",p->key.via.str.size) == 0)
            {
                CcspTraceDebug(("FXS comparision success at i=%d\n",i));
                e->fxs_count=p->val.via.array.size;
                e->fxsTable=(FXS_Table_t*)malloc(sizeof(FXS_Table_t)*e->fxs_count);
                if(e->fxsTable==NULL)
                {
                    e->fxs_count=0;
                    return RETURN_ERR;
                }
                CcspTraceDebug(("FXS Table count = %d\n",e->fxs_count));
                memset(e->fxsTable,0,sizeof(FXS_Table_t)*e->fxs_count);
                CcspTraceDebug(("%s:%d allocated memory for fxsTable at %p with size = %zu\n",__FUNCTION__,__LINE__,e->fxsTable,sizeof(e->fxsTable)));
                for(j=0;j<e->fxs_count;j++)
                {
                    if( MSGPACK_OBJECT_MAP != p->val.via.array.ptr[j].type )
                    {
                        CcspTraceError(("%s %d - Invalid OBJECT \n",__FUNCTION__,__LINE__));
                        return VALIDATION_FALIED;
                    }
                    CcspTraceDebug(("calling WebConfig_Process_fxs_Params for %d time\n",j+1));
                    rc = WebConfig_Process_fxs_Params(&e->fxsTable[j], &p->val.via.array.ptr[j].via.map);
                    if( RETURN_OK != rc)
                    {
                        return rc;
                    }
                }
            }
            else if(strncmp(p->key.via.str.ptr, "Network",p->key.via.str.size) == 0)
            {
                CcspTraceDebug(("Network comparision success at i=%d\n",i));
                e->network_count=p->val.via.array.size;
                e->networkTable=(Network_Table_t*)malloc(sizeof(Network_Table_t)*e->network_count);
                if(e->networkTable==NULL)
                {
                    e->network_count=0;
                    return RETURN_ERR;
                }
                CcspTraceDebug(("Network Table count = %d\n",e->network_count));
                memset(e->networkTable,0,sizeof(Network_Table_t)*e->network_count);
                CcspTraceDebug(("%s:%d allocated memory for networkTable at %p with size = %zu\n",__FUNCTION__,__LINE__,e->networkTable,sizeof(e->networkTable)));
                for(j=0;j<e->network_count;j++)
                {
                    if( MSGPACK_OBJECT_MAP != p->val.via.array.ptr[j].type )
                    {
                        CcspTraceError(("%s %d - Invalid OBJECT \n",__FUNCTION__,__LINE__));
                        return VALIDATION_FALIED;
                    }
                    CcspTraceDebug(("calling WebConfig_Process_network_Params for %d time\n",j+1));
                    rc = WebConfig_Process_network_Params(&e->networkTable[j], &p->val.via.array.ptr[j].via.map);
                    if( RETURN_OK != rc)
                    {
                        return rc;
                    }
                }
            }
            else if(strncmp(p->key.via.str.ptr, "Operator",p->key.via.str.size) == 0)
            {
                CcspTraceDebug(("Operator comparision success at i=%d\n",i));
                e->operator_count=p->val.via.array.size;
                e->operatorTable=(Operator_Table_t*)malloc(sizeof(Operator_Table_t)*e->operator_count);
                if(e->operatorTable==NULL)
                {
                    e->operator_count=0;
                    return RETURN_ERR;
                }
                CcspTraceDebug(("Operator Table count = %d\n",e->operator_count));
                memset(e->operatorTable,0,sizeof(Operator_Table_t)*e->operator_count);
                CcspTraceDebug(("%s:%d allocated memory for operatorTable at %p with size = %zu\n",__FUNCTION__,__LINE__,e->operatorTable,sizeof(e->operatorTable)));
                for(j=0;j<e->operator_count;j++)
                {
                    if( MSGPACK_OBJECT_MAP != p->val.via.array.ptr[j].type )
                    {
                        CcspTraceError(("%s %d - Invalid OBJECT \n",__FUNCTION__,__LINE__));
                        return VALIDATION_FALIED;
                    }
                    CcspTraceDebug(("calling WebConfig_Process_operator_Params for %d time\n",j+1));
                    rc = WebConfig_Process_operator_Params(&e->operatorTable[j], &p->val.via.array.ptr[j].via.map);
                    if( RETURN_OK != rc)
                    {
                        return rc;
                    }
                }
            }
            else if(strncmp(p->key.via.str.ptr, "Client",p->key.via.str.size) == 0)
            {
                CcspTraceDebug(("Client comparision success at i=%d\n",i));
                e->client_count=p->val.via.array.size;
                e->clientTable=(Client_Table_t*)malloc(sizeof(Client_Table_t)*e->client_count);
                if(e->clientTable==NULL)
                {
                    e->client_count=0;
                    return RETURN_ERR;
                }
                CcspTraceDebug(("Client Table count = %d\n",e->client_count));
                memset(e->clientTable,0,sizeof(Client_Table_t)*e->client_count);
                CcspTraceDebug(("%s:%d allocated memory for clientTable at %p with size = %zu\n",__FUNCTION__,__LINE__,e->clientTable,sizeof(e->clientTable)));
                CcspTraceDebug(("Client Table memset completed\n"));
                for(j=0;j<e->client_count;j++)
                {
                    if( MSGPACK_OBJECT_MAP != p->val.via.array.ptr[j].type )
                    {
                        CcspTraceError(("%s %d - Invalid OBJECT \n",__FUNCTION__,__LINE__));
                        return VALIDATION_FALIED;
                    }
                    CcspTraceDebug(("calling WebConfig_Process_client_Params for %d time\n",j+1));
                    rc = WebConfig_Process_client_Params(&e->clientTable[j], &p->val.via.array.ptr[j].via.map);
                    if( RETURN_OK != rc)
                    {
                        return rc;
                    }
                }
            }
            else if(strncmp(p->key.via.str.ptr, "Line",p->key.via.str.size) == 0)
            {
            }
            else if(strncmp(p->key.via.str.ptr, "Set",p->key.via.str.size) == 0)
            {
                CcspTraceDebug(("Set comparision success at i=%d\n",i));
                e->set_count=p->val.via.array.size;
                e->setTable=(Set_Table_t*)malloc(sizeof(Set_Table_t)*e->set_count);
                if(e->setTable==NULL)
                {
                    e->set_count=0;
                    return RETURN_ERR;
                }
                CcspTraceDebug(("Set Table count = %d\n",e->set_count));
                memset(e->setTable,0,sizeof(Set_Table_t)*e->set_count);
                CcspTraceDebug(("%s:%d allocated memory for setTable at %p with size = %zu\n",__FUNCTION__,__LINE__,e->setTable,sizeof(e->setTable)));
                for(j=0;j<e->set_count;j++)
                {
                    if( MSGPACK_OBJECT_MAP != p->val.via.array.ptr[j].type )
                    {
                        CcspTraceError(("%s %d - Invalid OBJECT \n",__FUNCTION__,__LINE__));
                        return VALIDATION_FALIED;
                    }
                    CcspTraceDebug(("calling WebConfig_Process_set_Params for %d time\n",j+1));
                    rc = WebConfig_Process_set_Params(&e->setTable[j], &p->val.via.array.ptr[j].via.map);
                    if( RETURN_OK != rc)
                    {
                        return rc;
                    }
                }
            }
            else
            {
                CcspTraceError(("Object name mismatch\n"));
                return VALIDATION_FALIED;
            }
        }
        else
        {
            if(strncmp(p->key.via.str.ptr,"POTS",p->key.via.str.size) == 0)
            {
                CcspTraceDebug(("POTS comparision success at i=%d\n",i));
                e->potsTable = (POTS_Table_t*)malloc(sizeof(POTS_Table_t));
                if(e->potsTable==NULL)
                {
                    return RETURN_ERR;
                }
                e->pots_count=1;
                CcspTraceDebug(("%s:%d allocated memory for potsTable at %p with size = %zu\n",__FUNCTION__,__LINE__,e->potsTable,sizeof(POTS_Table_t)));
                memset( e->potsTable,0,sizeof(POTS_Table_t));
                if( MSGPACK_OBJECT_MAP != p->val.type )
                {
                    CcspTraceError(("%s %d - Invalid OBJECT \n",__FUNCTION__,__LINE__));
                    return VALIDATION_FALIED;
                }
                CcspTraceDebug(("calling WebConfig_Process_pots_Params\n"));
                rc = WebConfig_Process_pots_Params(&e->potsTable[0],&p->val.via.map);
                if( RETURN_OK != rc )
                {
                    return rc;
                }
            }
        }
        p++;
    }
    return RETURN_OK;
}
int CosaDmlTR104DataSet(char* pString,int bootup)
{
    CcspTraceDebug(("Entering %s...\n",__FUNCTION__));
    size_t offset = 0;
    msgpack_unpacked msg;
    msgpack_unpack_return mp_rv;
    msgpack_object_map *map = NULL;
    msgpack_object_kv* map_ptr  = NULL;
    execData *execDataPf = NULL;
    TR104_WebConfig_t *pWebConfig = NULL;
    int i = 0,j;
    unsigned char *webConf = NULL;
    int webSize = 0;
    webConf = AnscBase64Decode((PUCHAR)pString, (ULONG*)&webSize);
    if(!webConf)
    {
        CcspTraceError(("%s: Failed to decode webconfig blob..\n",__FUNCTION__));
        return RETURN_ERR;
    }

    msgpack_unpacked_init( &msg );
    webSize +=  1;

    /* The outermost wrapper MUST be a map. */
    mp_rv = msgpack_unpack_next( &msg, (const char*) webConf, webSize, &offset );
    if (mp_rv != MSGPACK_UNPACK_SUCCESS) {
        CcspTraceError(("%s: Failed to unpack voiceservice msg blob. Error %d",__FUNCTION__,mp_rv));
        msgpack_unpacked_destroy( &msg );
        AnscFreeMemory(webConf);
        return RETURN_ERR;
    }

    CcspTraceInfo(("%s:Msg unpack success. Offset is %zu pString=%s\n", __FUNCTION__,offset,pString));
    msgpack_object obj = msg.data;

    map = &msg.data.via.map;

    map_ptr = obj.via.map.ptr;
    if ((!map) || (!map_ptr)) {
        CcspTraceError(("Failed to get object map\n"));
        msgpack_unpacked_destroy( &msg );
        AnscFreeMemory(webConf);
        return RETURN_ERR;
    }

    if (msg.data.type != MSGPACK_OBJECT_MAP) {
        CcspTraceError(("%s: Invalid msgpack type",__FUNCTION__));
        msgpack_unpacked_destroy( &msg );
        AnscFreeMemory(webConf);
        return VALIDATION_FALIED;
    }

    /* Allocate memory for TR104 structure*/

    pWebConfig = (TR104_WebConfig_t *) malloc(sizeof(TR104_WebConfig_t));
    CcspTraceDebug(("%s:%d allocated memory for pWebConfig at %p with size = %zu\n",__FUNCTION__,__LINE__,pWebConfig,sizeof(pWebConfig)));
    if ( pWebConfig == NULL )
    {
        CcspTraceError(("%s: TR104 Struct malloc error\n",__FUNCTION__));
        msgpack_unpacked_destroy( &msg );
        AnscFreeMemory(webConf);
        return RETURN_ERR;
    }

    memset( pWebConfig, 0, sizeof(TR104_WebConfig_t) );

    /* Parsing Config Msg String to TR104 Structure */
    CcspTraceInfo(("%s Parsing Config Msg String to TR104 Structure\n",__FUNCTION__));
    for (i = 0;i < (int)map->size;i++)
    {
        if (strncmp(map_ptr->key.via.str.ptr, "subdoc_name", map_ptr->key.via.str.size) == 0) {
            if (map_ptr->val.type == MSGPACK_OBJECT_STR) {
                strncpy(pWebConfig->subdoc_name, map_ptr->val.via.str.ptr, map_ptr->val.via.str.size);
                CcspTraceInfo(("subdoc name %s\n", pWebConfig->subdoc_name));
            }
        }
        else if (strncmp(map_ptr->key.via.str.ptr, "version", map_ptr->key.via.str.size) == 0) {
            if (map_ptr->val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
                pWebConfig->version = (uint64_t) map_ptr->val.via.u64;
                CcspTraceInfo(("Version type %d version %" PRIu64 "\n",map_ptr->val.type,pWebConfig->version));
            }
        }
        else if (strncmp(map_ptr->key.via.str.ptr, "transaction_id", map_ptr->key.via.str.size) == 0) {
            if (map_ptr->val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
                pWebConfig->transaction_id = (uint16_t) map_ptr->val.via.u64;
                CcspTraceInfo(("Tx id type %d tx id %d\n",map_ptr->val.type,pWebConfig->transaction_id));
            }
        }
        else if (strncmp(map_ptr->key.via.str.ptr, "VoiceService",map_ptr->key.via.str.size) == 0)
        {
            pWebConfig->voiceservice_count = map_ptr->val.via.array.size;
            pWebConfig->voiceserviceTable = (VoiceService_Table_t*)malloc(sizeof(VoiceService_Table_t)*pWebConfig->voiceservice_count);
            if(pWebConfig->voiceserviceTable == NULL)
            {
                pWebConfig->voiceservice_count = 0;
                /* Coverity Fix CID : 564371 Resource Leak */
                msgpack_unpacked_destroy( &msg );
                AnscFreeMemory(webConf);
                free(pWebConfig);
                return RETURN_ERR;
            }
            CcspTraceDebug(("%s:%d allocated memory for pWebConfig->voiceserviceTable at %p with size = %zu\n",__FUNCTION__,__LINE__,pWebConfig->voiceserviceTable,sizeof(pWebConfig->voiceserviceTable)));
            CcspTraceDebug(("voiceserviceTable count = %d\n",pWebConfig->voiceservice_count));
            memset(pWebConfig->voiceserviceTable,0,sizeof(VoiceService_Table_t)*pWebConfig->voiceservice_count);
            for(j=0;j<pWebConfig->voiceservice_count;j++)
            {
                CcspTraceDebug(("calling WebConfig_Process_voiceservice_Params for %d time\n",j+1));
                rc = WebConfig_Process_voiceservice_Params(&pWebConfig->voiceserviceTable[j],&map_ptr->val.via.array.ptr[j].via.map);
            }
        }

        ++map_ptr;
    }

    //Free allocated resource
    msgpack_unpacked_destroy( &msg );
    //Push blob request after collection
    execDataPf = (execData*) malloc (sizeof(execData));
    CcspTraceDebug(("%s:%d allocated memory for execDataPf at %p\n",__FUNCTION__,__LINE__,execDataPf));

    if (rc != RETURN_OK)
    {
        CcspTraceError(("%s:Failed to copy voiceservice params\n",__FUNCTION__));
        execDataPf->user_data = (void*) pWebConfig;
        TR104_Process_free_resources(execDataPf);
        AnscFreeMemory(webConf);
        return rc;
    }
    if ( execDataPf != NULL )
    {
        memset(execDataPf, 0, sizeof(execData));
        execDataPf->txid = pWebConfig->transaction_id;
        execDataPf->version = pWebConfig->version;
        execDataPf->numOfEntries = 1;
        strncpy(execDataPf->subdoc_name,pWebConfig->subdoc_name, sizeof(execDataPf->subdoc_name)-1);
        execDataPf->user_data = (void*) pWebConfig;
        execDataPf->executeBlobRequest = TR104_Process_Webconfig_Request;
        execDataPf->freeResources = TR104_Process_free_resources;
        isbootup=bootup;
        if(bootup == 1)
	{
            /* CID 190028 Resource leak fix */
            pErr execRetVal = NULL;
	    execRetVal = TR104_Process_Webconfig_Request(pWebConfig);
	    if(execRetVal != NULL)
	    {
		    CcspTraceDebug(("%s:%d Webconfig request error :  %s\n",__FUNCTION__,__LINE__,execRetVal->ErrorMsg));
		    free(execRetVal);
		    execRetVal = NULL;
	    }
            TR104_Process_free_resources(execDataPf);
        }
        else
        {
            fptr_dummy = fopen("/nvram/.vsb64_temp.txt","w");
            if(!fptr_dummy)
            {
                CcspTraceError(("%s temporary file not created\n",__FUNCTION__));
            }
            else
            {
                fputs(pString,fptr_dummy);
                CcspTraceDebug(("voice service base64=%s\n",pString));
                fclose(fptr_dummy);
            }
            PushBlobRequest(execDataPf);
            CcspTraceInfo(("%s PushBlobRequest Complete\n",__FUNCTION__));
        }
    }
    AnscFreeMemory(webConf);
    return RETURN_OK;
}
unsigned int getTr104DataBlobVersion(char *pSubDoc)
{
    char  subdoc_ver[64] = {0},buf[64] = {0};
    unsigned int version = 0;
    snprintf(buf,sizeof(buf), "%s_version", pSubDoc);
    if ( syscfg_get( NULL, buf, subdoc_ver, sizeof(subdoc_ver)) == 0 )
    {
        version = strtoul(subdoc_ver, NULL, 10);
        CcspTraceInfo(("%s TR104 data %s blob version %s\n",__FUNCTION__, pSubDoc, subdoc_ver));
        return version;
    }
    CcspTraceInfo(("%s Failed to get TR104 Data %s blob version\n",__FUNCTION__, pSubDoc));
    return 0;
}
int setTr104DataBlobVersion(char *pSubDoc, unsigned int version)
{
    char subdoc_ver[64] = {0},buf[64] = {0};
    snprintf(subdoc_ver, sizeof(subdoc_ver), "%u", version);
    snprintf(buf, sizeof(buf), "%s_version", pSubDoc);
    //Set blob version to DB

    if (syscfg_set_commit( NULL, buf, subdoc_ver ) != 0 )
    {
        return -1;
    }
    CcspTraceInfo(("%s TR104 data %s blob version %s set successfully\n",__FUNCTION__, pSubDoc, subdoc_ver));
    return 0;
}
ANSC_STATUS init_TR104_web_config()
{
    char *sub_docs[TR104_SUBDOC_COUNT+1]= {"voiceservice",(char *) 0 };
    blobRegInfo *blobData = NULL,*blobDataPointer = NULL;
    int i;

    //Allocate memory for blob registration structure
    blobData = (blobRegInfo*) malloc(TR104_SUBDOC_COUNT * sizeof(blobRegInfo));

    //Validate Memory
    if (blobData == NULL)
    {
        CcspTraceError(("%s: Failed to allocate memory\n",__FUNCTION__));
        return RETURN_ERR;
    }
    memset(blobData, 0, TR104_SUBDOC_COUNT*sizeof(blobRegInfo));
    CcspTraceDebug(("%s:%d allocated memory for blobData at %p with size %zu\n",__FUNCTION__,__LINE__,blobData,sizeof(blobData)));
    blobDataPointer = blobData;
    for ( i = 0; i < TR104_SUBDOC_COUNT; i++ )
    {
        strncpy(blobDataPointer->subdoc_name, sub_docs[i], sizeof(blobDataPointer->subdoc_name) - 1);
        blobDataPointer++;
    }
    blobDataPointer = blobData;

    getVersion versionGet = getTr104DataBlobVersion;
    setVersion versionSet = setTr104DataBlobVersion;
    register_sub_docs(blobData, TR104_SUBDOC_COUNT, versionGet, versionSet);
  
    CcspTraceInfo(("%s TR104 Webconfig Subdoc Registration Complete\n",__FUNCTION__));
    return ANSC_STATUS_SUCCESS;
}
#endif
