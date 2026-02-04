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


/**************************************************************************

    module: cosa_x_cisco_com_mta_api.c

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    description:

        This file implementes back-end apis for the COSA Data Model Library

        *  CosaDmlMTAInit
    -------------------------------------------------------------------

    environment:

        platform independent

    -------------------------------------------------------------------

    author:

        COSA XML TOOL CODE GENERATOR 1.0

    -------------------------------------------------------------------

    revision:

        01/11/2011    initial revision.

**************************************************************************/

#include <cjson/cJSON.h>

//!!!  This code assumes that all data structures are the SAME in middle-layer APIs and HAL layer APIs
//!!!  So it uses casting from one to the other
#include "cosa_x_cisco_com_mta_apis.h"
#include "mta_hal.h"
#include "syscfg/syscfg.h"
/* Legacy common-library includes commented out for JSON-driven RBUS approach */
/* #include "safec_lib_common.h" */
#include <safec_lib.h>
#include "mta_compat_types.h"  /* Include compatibility layer */
#include "sysevent/sysevent.h"
#include "ctype.h"


// #include "cosa_x_cisco_com_mta_internal.h"

#define PARTNERS_INFO_FILE              "/nvram/partners_defaults.json"
#define BOOTSTRAP_INFO_FILE             "/opt/secure/bootstrap.json"
#define BOOTSTRAP_INFO_FILE_BACKUP      "/nvram/bootstrap.json"
#define CLEAR_TRACK_FILE                "/nvram/ClearUnencryptedData_flags"
#define NVRAM_BOOTSTRAP_CLEARED         (1 << 0)
#define MAX_LINE_REG 256

#ifdef MTA_TR104SUPPORT

int CosaDmlTR104DataSet(char *pString,int bootup);

#define MAX_BASE64_LEN 8192

int mtaReapplytr104Conf(void)
{
	
        int length;
	char *buffer;
	FILE *fptr;

	fptr = fopen("/nvram/.vsb64.txt","rb");
        CcspTraceDebug(("%s Entering \n",__FUNCTION__));
        
	if (fptr == NULL) {    
		CcspTraceDebug(("file not present \n"));
		return ANSC_STATUS_FAILURE;
	}
         
	fseek(fptr, 0, SEEK_END);
	length = ftell (fptr);
        if (length < 0) {
            CcspTraceDebug(("Error getting file length"));
            fclose(fptr);
            return ANSC_STATUS_FAILURE;  
        } else {
           CcspTraceDebug(("Current file position: %d\n", length));
        }
	fseek(fptr, 0, SEEK_SET);
	buffer = (char*)malloc( length+1 );
	if (buffer == NULL){
            fclose(fptr);
	    return ANSC_STATUS_FAILURE;
        }

	if (buffer) {
		size_t bytesRead = fread(buffer, 1, length, fptr);
		if ( bytesRead != (size_t) length ){
			if ( bytesRead < (size_t) length){
				CcspTraceDebug((" The complete file is not read \n"));
			}
			if (ferror(fptr)) {
				CcspTraceDebug((" Error in reading the file  \n"));
			}
			fclose(fptr);
			free(buffer);
			return ANSC_STATUS_FAILURE;
		}

		buffer[bytesRead] = '\0';
		CcspTraceDebug(("buffer=%s\n", buffer));

        /* Coverity fix CID : 513281 Insecure data handling */
        if (buffer[0] == '\0')
        {
            CcspTraceDebug(("Buffer is empty or invalid\n"));
            free(buffer);
            fclose(fptr);
            return ANSC_STATUS_FAILURE;
        }

		CcspTraceInfo(("%s:Calling CosaDmlTR104DataSet \n", __func__));
		CosaDmlTR104DataSet(buffer, 1);
		free(buffer);
	}
       
        CcspTraceDebug(("%s Exiting \n",__FUNCTION__));
	fclose(fptr);
	return ANSC_STATUS_SUCCESS;
  
}

#endif // MTA_TR104SUPPORT


ANSC_STATUS
CosaDmlMTAInit
    (
        ANSC_HANDLE                 hDml,
        PANSC_HANDLE                phContext
    )
{
    UNREFERENCED_PARAMETER(hDml);
    UNREFERENCED_PARAMETER(phContext);

   // PCOSA_DATAMODEL_MTA      pMyObject    = (PCOSA_DATAMODEL_MTA)phContext;

    if ( mta_hal_InitDB() == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMTAGetDHCPInfo
    (
        ANSC_HANDLE                 hContext,
        PCOSA_MTA_DHCP_INFO         pInfo
    )
{

    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_GetDHCPInfo((PMTAMGMT_MTA_DHCP_INFO)pInfo) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMTAGetDHCPV6Info
    (
        ANSC_HANDLE                 hContext,
        PCOSA_MTA_DHCPv6_INFO       pInfo
    )
{

    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_GetDHCPV6Info((PMTAMGMT_MTA_DHCPv6_INFO)pInfo) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMTAGetPktc
    (
        ANSC_HANDLE                 hContext,
        PCOSA_MTA_PKTC              pPktc
    )
{
    UNREFERENCED_PARAMETER(hContext);
    UNREFERENCED_PARAMETER(pPktc);

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
CosaDmlMTASetPktc
    (
        ANSC_HANDLE                 hContext,
        PCOSA_MTA_PKTC              pPktc
    )
{
    UNREFERENCED_PARAMETER(hContext);
    UNREFERENCED_PARAMETER(pPktc);

    return ANSC_STATUS_SUCCESS;
}

ULONG
CosaDmlMTALineTableGetNumberOfEntries
    (
        ANSC_HANDLE                 hContext
    )
{
    UNREFERENCED_PARAMETER(hContext);
    return mta_hal_LineTableGetNumberOfEntries();
}

ANSC_STATUS
CosaDmlMTALineTableGetEntry
    (
        ANSC_HANDLE                 hContext,
        ULONG                       ulIndex,
        PCOSA_MTA_LINETABLE_INFO    pEntry
    )

{

    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_LineTableGetEntry(ulIndex, (PMTAMGMT_MTA_LINETABLE_INFO)pEntry) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMTATriggerDiagnostics
    (
        ULONG                         nIndex
    )
{ 
    if ( mta_hal_TriggerDiagnostics(nIndex) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE; 
}

ANSC_STATUS
CosaDmlMTAGetServiceClass
    (
        ANSC_HANDLE                 hContext,
        PULONG                      pulCount,
        PCOSA_MTA_SERVICE_CLASS     *ppCfg
    )
{
    UNREFERENCED_PARAMETER(hContext);
    UNREFERENCED_PARAMETER(ppCfg);
    *pulCount = 0;

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
CosaDmlMTAGetServiceFlow
    (
        ANSC_HANDLE                 hContext,
        PULONG                      pulCount,
        PCOSA_MTA_SERVICE_FLOW      *ppCfg
    )
{
    UNREFERENCED_PARAMETER(hContext);
    *pulCount = 0;
    PMTAMGMT_MTA_SERVICE_FLOW pInfo = NULL;
    if ( mta_hal_GetServiceFlow(pulCount, &pInfo) != RETURN_OK )
    {
         /*Coverity Fix CID:58790 RESOURCE_LEAK */
          if(pInfo != NULL)
          {
             free(pInfo);
              return ANSC_STATUS_FAILURE;
          }
    }

    if (*pulCount > 0) {
        if( (*ppCfg = (PCOSA_MTA_SERVICE_FLOW)AnscAllocateMemory(sizeof(MTAMGMT_MTA_SERVICE_FLOW)*(*pulCount))) == NULL )
        {
            AnscTraceWarning(("AllocateMemory error %s, %d\n", __FUNCTION__, __LINE__));
            free(pInfo);
            return ANSC_STATUS_FAILURE;  
        }
        AnscCopyMemory(*ppCfg, pInfo, sizeof(MTAMGMT_MTA_SERVICE_FLOW)*(*pulCount));
        free(pInfo);
    }
    else
    {
        /* Coverity Fix CID : 58790, RESOURCE_LEAK */
        if(pInfo != NULL)
        {
            free(pInfo);
        }
    }
    return ANSC_STATUS_SUCCESS;

    /*
    if ( mta_hal_GetServiceFlow(pulCount, (PMTAMGMT_MTA_SERVICE_FLOW *)ppCfg) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
    */
}

ANSC_STATUS
CosaDmlMTADectGetEnable
    (
        ANSC_HANDLE                 hContext,
        BOOLEAN                     *pBool
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if (mta_hal_DectGetEnable(pBool) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}
        
ANSC_STATUS
CosaDmlMTADectSetEnable
    (
        ANSC_HANDLE                 hContext,
        BOOLEAN                     bBool
    )
{
     UNREFERENCED_PARAMETER(hContext);
     if (mta_hal_DectSetEnable(bBool) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;

}

ANSC_STATUS
CosaDmlMTADectGetRegistrationMode
    (
        ANSC_HANDLE                 hContext,
        BOOLEAN                     *pBool
    )
{
     UNREFERENCED_PARAMETER(hContext);
     if (mta_hal_DectGetRegistrationMode(pBool) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;

}

ANSC_STATUS
CosaDmlMTADectSetRegistrationMode
    (
        ANSC_HANDLE                 hContext,
        BOOLEAN                     bBool
    )
{
     UNREFERENCED_PARAMETER(hContext);
     if (mta_hal_DectSetRegistrationMode(bBool) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;

}

ANSC_STATUS
CosaDmlMTADectRegisterDectHandset
    (
        ANSC_HANDLE                 hContext,
        ULONG                       uValue    
    )
{
    fprintf(stderr, "%s is not implemented!\n", __FUNCTION__);
    UNREFERENCED_PARAMETER(hContext);
    UNREFERENCED_PARAMETER(uValue);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
CosaDmlMTADectDeregisterDectHandset
    (
        ANSC_HANDLE                 hContext,
        ULONG                       uValue
    )
{

    UNREFERENCED_PARAMETER(hContext);
    if (mta_hal_DectDeregisterDectHandset(uValue) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;


}

ANSC_STATUS
CosaDmlMTAGetDect
    (
        ANSC_HANDLE                 hContext,
        PCOSA_MTA_DECT              pDect
    )
{

     UNREFERENCED_PARAMETER(hContext);
     if (mta_hal_GetDect((PMTAMGMT_MTA_DECT)pDect) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;


}

ANSC_STATUS
CosaDmlMTAGetDectPIN
    (
        ANSC_HANDLE                 hContext,
        char                        *pPINString
    )
{

     UNREFERENCED_PARAMETER(hContext);
     if (mta_hal_GetDectPIN(pPINString) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
     else
        return ANSC_STATUS_FAILURE;


}

ANSC_STATUS
CosaDmlMTASetDectPIN
    (
        ANSC_HANDLE                 hContext,
        char                        *pPINString
    )
{

     UNREFERENCED_PARAMETER(hContext);
     if (mta_hal_SetDectPIN(pPINString) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;

}

ANSC_STATUS
CosaDmlMTAGetHandsets
    (
        ANSC_HANDLE                 hContext,
        PULONG                      pulCount,
        PCOSA_MTA_HANDSETS_INFO     *ppHandsets
    )
{
    UNREFERENCED_PARAMETER(hContext);
    PMTAMGMT_MTA_HANDSETS_INFO pInfo = NULL;
    if ( mta_hal_GetHandsets( pulCount, &pInfo) != RETURN_OK )
    {
        if(pInfo)
        {
            free(pInfo);
            pInfo = NULL;
        }
        return ANSC_STATUS_FAILURE;
    }
 
    if (*pulCount > 0) {
        if( (*ppHandsets = AnscAllocateMemory(DECT_MAX_HANDSETS * sizeof(MTAMGMT_MTA_HANDSETS_INFO))) == NULL )
        {
            AnscTraceWarning(("AllocateMemory error %s, %d\n", __FUNCTION__, __LINE__));
            free(pInfo);
            return ANSC_STATUS_FAILURE;  
        }
        AnscCopyMemory(*ppHandsets, pInfo, sizeof(MTAMGMT_MTA_HANDSETS_INFO)*DECT_MAX_HANDSETS);
        free(pInfo);
    }
    return ANSC_STATUS_SUCCESS;

}

ANSC_STATUS
CosaDmlMTASetHandsets
    (
        ANSC_HANDLE                 hContext,
        PCOSA_MTA_HANDSETS_INFO     pHandsets
    )
{
   fprintf(stderr, "%s is not implemented!\n", __FUNCTION__);
   UNREFERENCED_PARAMETER(hContext);
   UNREFERENCED_PARAMETER(pHandsets);
   return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
CosaDmlMTAVQMResetStats
    (
        ANSC_HANDLE                 hContext
    )
{
    UNREFERENCED_PARAMETER(hContext);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
CosaDmlMTAGetCalls
    (
        ANSC_HANDLE                 hContext,
        ULONG                       InstanceNumber, /* LineTable's instance number */
        PULONG                      pulCount,
        PCOSA_MTA_CALLS             *ppCfg
    )
{ 
    UNREFERENCED_PARAMETER(hContext);
    *pulCount = 0;
    PMTAMGMT_MTA_CALLS pInfo = NULL;
    if ( mta_hal_GetCalls(InstanceNumber, pulCount, &pInfo) != RETURN_OK )
    {
        if(pInfo)
        {
            free(pInfo);
            pInfo = NULL;
        }

        return ANSC_STATUS_FAILURE;
    }

    if (*pulCount > 0) {
        if( (*ppCfg = (PCOSA_MTA_CALLS)AnscAllocateMemory(sizeof(MTAMGMT_MTA_CALLS)*(*pulCount))) == NULL )
        {
            AnscTraceWarning(("AllocateMemory error %s, %d\n", __FUNCTION__, __LINE__));
            free(pInfo);
            return ANSC_STATUS_FAILURE;  
        }
        AnscCopyMemory(*ppCfg, pInfo, sizeof(MTAMGMT_MTA_CALLS)*(*pulCount));
        free(pInfo);
    }
    return ANSC_STATUS_SUCCESS;
    
    /*
    if ( mta_hal_GetCalls(InstanceNumber, (PMTAMGMT_MTA_CALLS *)ppCfg, pulCount) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else 
        return ANSC_STATUS_FAILURE;
    */
}

ANSC_STATUS
CosaDmlMTAGetCALLP
    (
        ANSC_HANDLE                 hContext,
        PCOSA_MTA_CAPPL             pCallp
    )
{
    PCOSA_MTA_LINETABLE_INFO pLineTable = (PCOSA_MTA_LINETABLE_INFO)hContext;

    if ( mta_hal_GetCALLP(pLineTable->LineNumber, (PMTAMGMT_MTA_CALLP)pCallp) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMTAGetDSXLogs
    (
        ANSC_HANDLE                 hContext,
        PULONG                      pulCount,
        PCOSA_MTA_DSXLOG            *ppDSXLog
    )
{
    UNREFERENCED_PARAMETER(hContext);
    *pulCount = 0;
    PMTAMGMT_MTA_DSXLOG pInfo = NULL;
    if ( mta_hal_GetDSXLogs(pulCount, &pInfo) != RETURN_OK )
    {

        /* Coverity Fix CID:76397 RESOURCE_LEAK */
        if( pInfo != NULL )
        {
            free(pInfo);
           return ANSC_STATUS_FAILURE;
       }
    }

    if (*pulCount > 0) {
        if( (*ppDSXLog = (PCOSA_MTA_DSXLOG)AnscAllocateMemory(sizeof(MTAMGMT_MTA_DSXLOG)*(*pulCount))) == NULL )
        {
            AnscTraceWarning(("AllocateMemory error %s, %d\n", __FUNCTION__, __LINE__));
             free(pInfo);
            return ANSC_STATUS_FAILURE;  
        }
        /* CID 160180 Dereference after null check fix */
	if(pInfo != NULL)
	{
		AnscCopyMemory(*ppDSXLog, pInfo, sizeof(MTAMGMT_MTA_DSXLOG)*(*pulCount));
		free(pInfo);
	}
    }
    else
    {
        /* Coverity Fix CID : 76397, RESOURCE_LEAK */
        if( pInfo != NULL )
        {
            free(pInfo);
        }
    }
    return ANSC_STATUS_SUCCESS;

    /*
    if ( mta_hal_GetDSXLogs(pulCount, (PMTAMGMT_MTA_DSXLOG *)ppDSXLog) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
    */
}

ANSC_STATUS
CosaDmlMTAGetDSXLogEnable
    (
        ANSC_HANDLE                 hContext,
        BOOLEAN                     *pBool
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_GetDSXLogEnable(pBool) == RETURN_OK ) 
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMTASetDSXLogEnable
    (
        ANSC_HANDLE                 hContext,
        BOOLEAN                     Bool
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_SetDSXLogEnable(Bool) == RETURN_OK ) 
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMTAClearDSXLog
    (
        ANSC_HANDLE                 hContext,
        BOOLEAN                     ClearLog
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_ClearDSXLog(ClearLog) == RETURN_OK ) 
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

// CallSignalling apis
ANSC_STATUS
CosaDmlMTAGetCallSignallingLogEnable
	(
        ANSC_HANDLE                 hContext,
        BOOLEAN                     *pBool
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_GetCallSignallingLogEnable(pBool) == RETURN_OK ) 
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMTASetCallSignallingLogEnable
    (
        ANSC_HANDLE                 hContext,
        BOOLEAN                     Bool
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_SetCallSignallingLogEnable(Bool) == RETURN_OK ) 
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMTAClearCallSignallingLog
    (
        ANSC_HANDLE                 hContext,
	BOOLEAN                     ClearLog
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_ClearCallSignallingLog(ClearLog) == RETURN_OK ) 
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

// CallSignalling apis ends

ANSC_STATUS
CosaDmlMtaBatteryGetInstalled
    (
        ANSC_HANDLE                 hContext,
        PBOOL                       pValue
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_BatteryGetInstalled(pValue) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMtaBatteryGetTotalCapacity
    (
        ANSC_HANDLE                 hContext,
        PULONG                      pValue
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_BatteryGetTotalCapacity(pValue) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}
    
ANSC_STATUS
CosaDmlMtaBatteryGetActualCapacity
    (
        ANSC_HANDLE                 hContext,
        PULONG                      pValue
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_BatteryGetActualCapacity(pValue) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMtaBatteryGetRemainingCharge
    (
        ANSC_HANDLE                 hContext,
        PULONG                      pValue
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_BatteryGetRemainingCharge(pValue) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMtaBatteryGetRemainingTime
    (
        ANSC_HANDLE                 hContext,
        PULONG                      pValue
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_BatteryGetRemainingTime(pValue) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMtaBatteryGetNumberofCycles
    (
        ANSC_HANDLE                 hContext,
        PULONG                      pValue
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_BatteryGetNumberofCycles(pValue) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMtaBatteryGetPowerStatus
    (
        ANSC_HANDLE                 hContext,
        PCHAR                       pValue,
        PULONG                      pSize
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_BatteryGetPowerStatus(pValue, pSize) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMtaBatteryGetCondition
    (
        ANSC_HANDLE                 hContext,
        PCHAR                       pValue,
        PULONG                      pSize
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_BatteryGetCondition(pValue, pSize) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMtaBatteryGetStatus
    (
        ANSC_HANDLE                 hContext,
        PCHAR                       pValue,
        PULONG                      pSize
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_BatteryGetStatus(pValue, pSize) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMtaBatteryGetLife
    (
        ANSC_HANDLE                 hContext,
        PCHAR                       pValue,
        PULONG                      pSize
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_BatteryGetLife(pValue, pSize) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
}

ANSC_STATUS
CosaDmlMtaBatteryGetInfo
    (
        ANSC_HANDLE                 hContext,
        PCOSA_DML_BATTERY_INFO      pInfo
    )
{
    UNREFERENCED_PARAMETER(hContext);
    if ( mta_hal_BatteryGetInfo((PMTAMGMT_MTA_BATTERY_INFO)pInfo) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else 
        return ANSC_STATUS_FAILURE;
}


ANSC_STATUS
CosaDmlMtaGetMtaLog
    (
        ANSC_HANDLE                 hContext,
        PULONG                      pulCount,
        PCOSA_DML_MTALOG_FULL       *ppConf        
    )    
{
    UNREFERENCED_PARAMETER(hContext);
    *pulCount = 0;
    PMTAMGMT_MTA_MTALOG_FULL pInfo = NULL;
    if ( mta_hal_GetMtaLog(pulCount, &pInfo) != RETURN_OK )
    {
        /* Coverity Fix CID:79698 RESOURCE_LEAK */
        if(pInfo != NULL)
        {     free(pInfo);
          return ANSC_STATUS_FAILURE;
        }
    }

    if (*pulCount > 0) {
        if( (*ppConf = (PCOSA_DML_MTALOG_FULL)AnscAllocateMemory(sizeof(MTAMGMT_MTA_MTALOG_FULL)*(*pulCount))) == NULL )
        {
            AnscTraceWarning(("AllocateMemory error %s, %d\n", __FUNCTION__, __LINE__));
            /* Coverity Fix CID:79698 RESOURCE_LEAK */
            free(pInfo);
            return ANSC_STATUS_FAILURE;  
        }
        AnscCopyMemory(*ppConf, pInfo, sizeof(MTAMGMT_MTA_MTALOG_FULL)*(*pulCount));
        {
            unsigned int i;
            for (i=0; i<*pulCount; i++) {
                if (pInfo[i].pDescription) {
                    (*ppConf)[i].pDescription = AnscCloneString(pInfo[i].pDescription);
                    free(pInfo[i].pDescription);
                }
                else (*ppConf)[i].pDescription = NULL;
            }
        }
        free(pInfo);
    }
    else
    {
        /* Coverity Fix CID : 79698 RESOURCE_LEAK */
        if(pInfo != NULL)
        {     
            free(pInfo);
        }
    }
    return ANSC_STATUS_SUCCESS;

/*
    if ( mta_hal_GetMtaLog(pulCount, (PMTAMGMT_MTA_MTALOG_FULL *)ppConf) == RETURN_OK )
        return ANSC_STATUS_SUCCESS;
    else
        return ANSC_STATUS_FAILURE;
*/

}

ANSC_STATUS
CosaDmlMtaGetDectLog
    (
        ANSC_HANDLE                 hContext,
        PULONG                      pulCount,
        PCOSA_DML_DECTLOG_FULL      *ppConf        
    )    
{
    UNREFERENCED_PARAMETER(hContext);
    UNREFERENCED_PARAMETER(ppConf);

    *pulCount = 0;
    //*ppConf = (PCOSA_DML_DECTLOG_FULL)AnscAllocateMemory( sizeof(DectLog) );

    //AnscCopyMemory(*ppConf, &DectLog, sizeof(DectLog) );

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
CosaDmlMtaGetResetCount
    (
        ANSC_HANDLE                 hContext,
	MTA_RESET_TYPE              type,
        ULONG                       *pValue
    )
{
	UNREFERENCED_PARAMETER(hContext);
        switch(type)
	{
		case MTA_RESET:{
				mta_hal_Get_MTAResetCount(pValue);
			 }
			break;
		case LINE_RESET:{
				mta_hal_Get_LineResetCount(pValue);
			}
			break;
		default:{
			 AnscTraceWarning(("Invalide type %s, %d\n", __FUNCTION__, __LINE__));
			}
	}
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
CosaDmlMtaClearCalls
    (
		ULONG InstanceNumber
    )
{
	mta_hal_ClearCalls( InstanceNumber );

    return ANSC_STATUS_SUCCESS;
}
// #endif /* CONFIG_TI_PACM */

ANSC_STATUS
CosaDmlMtaGetDhcpStatus
    (
		PULONG pIpv4DhcpStatus,
		PULONG pIpv6DhcpStatus
    )
{
	*pIpv4DhcpStatus = MTA_ERROR;
	*pIpv6DhcpStatus = MTA_ERROR;

#ifdef _CBR_PRODUCT_REQ_
	MTAMGMT_MTA_STATUS output_pIpv4status = MTA_ERROR;
	MTAMGMT_MTA_STATUS output_pIpv6status = MTA_ERROR;
        if ( RETURN_OK == mta_hal_getDhcpStatus( &output_pIpv4status, &output_pIpv6status ) )
	{
		*pIpv4DhcpStatus = output_pIpv4status;
		*pIpv6DhcpStatus = output_pIpv6status;
	}
#endif /* _CBR_PRODUCT_REQ_ */

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
CosaDmlMtaGetConfigFileStatus
    (
		PULONG pConfigFileStatus
    )
{
        UNREFERENCED_PARAMETER(pConfigFileStatus);
#ifdef _CBR_PRODUCT_REQ_
     
         /*Coverity Fix CID 59560 */
         MTAMGMT_MTA_STATUS output_status = MTA_ERROR;
	if ( RETURN_OK == mta_hal_getConfigFileStatus( &output_status))
	{
		*pConfigFileStatus = output_status;
	}
#endif /* _CBR_PRODUCT_REQ_ */

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
CosaDmlMtaGetLineRegisterStatus
    (
		PCHAR pcLineRegisterStatus
    )
{
#ifdef _CBR_PRODUCT_REQ_
#define max_number_of_line 8
	MTAMGMT_MTA_STATUS output_status_array[ max_number_of_line ] = { 0 };
	char 			   aoutput_status_array_string [ 4 ][ 32 ] = { "Init", "Start", "Complete", "Error" };

	if ( RETURN_OK == mta_hal_getLineRegisterStatus( output_status_array, max_number_of_line ) )
	{
		/* CID 91923 Calling risky function fix */
		snprintf( pcLineRegisterStatus, MAX_LINE_REG, "%s,%s,%s,%s,%s,%s,%s,%s", aoutput_status_array_string[ output_status_array[ 0 ] ],
																  aoutput_status_array_string[ output_status_array[ 1 ] ],
																  aoutput_status_array_string[ output_status_array[ 2 ] ],
																  aoutput_status_array_string[ output_status_array[ 3 ] ],
																  aoutput_status_array_string[ output_status_array[ 4 ] ],
																  aoutput_status_array_string[ output_status_array[ 5 ] ],
																  aoutput_status_array_string[ output_status_array[ 6 ] ],
																  aoutput_status_array_string[ output_status_array[ 7 ] ] );
	}
	else
#endif /* _CBR_PRODUCT_REQ_ */
	{	
		/* CID 91923 Calling risky function fix */
		snprintf( pcLineRegisterStatus, MAX_LINE_REG,"%s", "" );
	}

    return ANSC_STATUS_SUCCESS;
}


ANSC_STATUS
CosaDmlMTASetStartUpIpMode
    (
        PCOSA_MTA_ETHWAN_PROV_INFO  pmtaethpro, 
        INT                         bInt
    )
{
     pmtaethpro->StartupIPMode.ActiveValue =bInt;
     return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
CosaDmlMTASetPrimaryDhcpServerOptions
    (
        PCOSA_MTA_ETHWAN_PROV_INFO  pmtaethpro,
        char                       *buf,
        MTA_IP_TYPE_TR              type
    )
{
    if (type == MTA_IPV4_TR)
    {
        snprintf(pmtaethpro->IPv4PrimaryDhcpServerOptions.ActiveValue, sizeof(pmtaethpro->IPv4PrimaryDhcpServerOptions.ActiveValue), "%s", buf);
    }
    else if (type == MTA_IPV6_TR)
    {
        snprintf(pmtaethpro->IPv6PrimaryDhcpServerOptions.ActiveValue, sizeof(pmtaethpro->IPv6PrimaryDhcpServerOptions.ActiveValue), "%s", buf);
    }

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
CosaDmlMTASetSecondaryDhcpServerOptions
    (
        PCOSA_MTA_ETHWAN_PROV_INFO  pmtaethpro,
        char                       *buf,
        MTA_IP_TYPE_TR              type
    )
{
    if (type == MTA_IPV4_TR)
    {
        snprintf(pmtaethpro->IPv4SecondaryDhcpServerOptions.ActiveValue, sizeof(pmtaethpro->IPv4SecondaryDhcpServerOptions.ActiveValue), "%s", buf);
    }
    else if (type == MTA_IPV6_TR)
    {
        snprintf(pmtaethpro->IPv6SecondaryDhcpServerOptions.ActiveValue, sizeof(pmtaethpro->IPv6SecondaryDhcpServerOptions.ActiveValue), "%s", buf);
    }

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
CosaDmlMtaResetNow
    (
		BOOLEAN	 bValue
    )
{
        UNREFERENCED_PARAMETER(bValue);
#if defined (_XB6_PRODUCT_REQ_) || defined (_CBR_PRODUCT_REQ_)
	if( RETURN_OK == mta_hal_devResetNow(bValue) )
#else
    if( 1 )
#endif /* _CBR_PRODUCT_REQ_ || _XB6_PRODUCT_REQ_*/
	{
		AnscTraceWarning(("MTA reset is successful \n"));
		return ANSC_STATUS_SUCCESS;
	}
	else
	{
		return ANSC_STATUS_FAILURE;
	}
}

/*Coverity fix CID 56740 argument type mismatch */
void * MtaProvisioningStatusGetFunc(void * arg)
{
	pthread_detach(pthread_self());
        CcspTraceInfo(("%s Entering \n",__FUNCTION__));

#if defined (_XB6_PRODUCT_REQ_) || defined (_CBR_PRODUCT_REQ_)
        /*Coverity Fix CID 62657 55626 */
        unsigned int ProvisioningStatus;
        int counter=0;
        char value[16] = {'\0'};
        bool tr104ApplySuccess = false;
        bool tr104Enable = false;
        MTAMGMT_MTA_PROVISION_STATUS provisionStatus = 0;
        char MTA_provision_status[2][20] = {"MTA_PROVISIONED", "MTA_NON_PROVISIONED"};
        int tr181_sysevent_fd;
        token_t tr181_sysevent_token;

        tr181_sysevent_fd = sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, "TR104_Apply", &tr181_sysevent_token);
        if (tr181_sysevent_fd < 0){
            CcspTraceError(("%s sysevent_open fail \n",__FUNCTION__));
            return NULL;
        }

        //Retreive the operating status only when the MTA provisioning status 
        //to non provisioning.

        do 
        {
               if( RETURN_OK != mta_hal_getMtaProvisioningStatus(&provisionStatus) ) 
	        {
	            CcspTraceInfo((" %s: Mta_halgetProvisioningstatus Fail \n ",__FUNCTION__));

	        }
                // True when MTA_NON_PROVISIONED and provisioning status value is returned.
                else if( strcmp(MTA_provision_status[provisionStatus],"MTA_PROVISIONED") ) 
	        {
	            CcspTraceInfo(("%s provisionStatus = %s \n",__FUNCTION__,MTA_provision_status[provisionStatus]));
	            break;
	        }
                sleep(5);
                counter = counter + 5;
	} while (counter < 240);
    
        if((syscfg_get(NULL,"TR104enable", value, sizeof(value)) == 0) && (strcmp(value, "true") == 0))
	{
	      tr104Enable = true;
	}
        CcspTraceDebug((" %s: tr104Enable: %s \n",__FUNCTION__, value ));
        memset_s(value,sizeof(value),0,sizeof(value));

        if (sysevent_get(tr181_sysevent_fd, tr181_sysevent_token, "tr104_applied", value, sizeof(value)) == 0)
        {
             tr104ApplySuccess = (strcmp(value, "true") == 0);
             CcspTraceDebug((" %s: tr104_applied: %s tr104ApplySuccess=%d \n",__FUNCTION__, value ,tr104ApplySuccess ));
        }
        else
        {
             CcspTraceDebug(("%s:%d tr104 not applied :value =%s \n",__FUNCTION__,__LINE__,value));
             tr104ApplySuccess = false;
        }
        CcspTraceInfo((" %s: tr104Enable: %s tr104ApplySuccess: %s \n",__FUNCTION__, tr104Enable ?"true":"false" ,tr104ApplySuccess ?"true":"false" ));
        
        
        if (0 <= tr181_sysevent_fd) {
            sysevent_close(tr181_sysevent_fd, tr181_sysevent_token);
            tr181_sysevent_fd = -1;
        }
        tr181_sysevent_token = 0;
  
	while(1)
	{
		if(RETURN_OK != mta_hal_getMtaOperationalStatus(&ProvisioningStatus) )
		{
			CcspTraceDebug((" %s: mta_hal_getMtaOperationalStatus Fail \n",__FUNCTION__));
			return NULL;
		}

		switch(ProvisioningStatus)
		{
			case COSA_MTA_INIT:
				CcspTraceDebug(("MTA init after reset  \n"));
				break;

			case COSA_MTA_START:
				CcspTraceDebug(("MTA provisiong started after reset \n"));
				break;


			case COSA_MTA_ERROR:
				CcspTraceDebug(("MTA provisioning failed after reset"));
				break;

			//Apply TR104 config if any of both the lines are Enabled / both the lines disabled
 
			case COSA_MTA_COMPLETE:
			case COSA_MTA_REJECTED:
				CcspTraceInfo((" %s:MTA is REjected/completed after reset \n",__FUNCTION__));
                        //ReApply TR104 config after reset only if MTA is TR104 capable and WC already applied before
                    
				if( tr104Enable && tr104ApplySuccess) 
                		{
#ifdef MTA_TR104SUPPORT                                       
                                	mtaReapplytr104Conf();
#endif // MTA_TR104SUPPORT
                    			CcspTraceInfo(("%s: TR104 is Enabled,Applying TR104 config. \n",__FUNCTION__));
               			}		  
				pthread_exit(NULL);
				return arg;


			default :
					break;
		}
		sleep(5);
		counter = counter+5;

		/*This thread is run for 180Sec*/
		if(counter == 180)
		{
			counter = 0;
			pthread_exit(NULL);
			return arg;
		}
        }
        
        /* Coverity Fix CID : 560416 Structurally dead code */
        //pthread_exit(NULL);
  
#endif /* _CBR_PRODUCT_REQ_ || _XB6_PRODUCT_REQ_*/
        CcspTraceInfo(("%s Exiting \n",__FUNCTION__));
        return arg;    
}



void CosaDmlMtaProvisioningStatusGet()
{
        int res = 0;
        pthread_t MtaProvisioningStatusGetThread;
        
        res = pthread_create(&MtaProvisioningStatusGetThread, NULL, MtaProvisioningStatusGetFunc, "MtaProvisioningStatusGetFunc");
	if(res != 0)
	{
	    CcspTraceError(("Create MtaProvisioningStatusGetThread error %d\n", res));
	    return;
	}

}

#define PARTNER_ID_LEN 64
ANSC_STATUS fillCurrentPartnerId
        (
                char*                       pValue,
        PULONG                      pulSize
    )
{
        char buf[PARTNER_ID_LEN] = {'\0'} ;
    	if(ANSC_STATUS_SUCCESS == syscfg_get( NULL, "PartnerID", buf, sizeof(buf)))
    	{
	    	/* CID 66248 Array compared against NULL fix */
                if( buf[0] != '\0' )
                {
                        /* Coverity Fix : STRING_NULL */
                        size_t n = strlen(buf);
                        snprintf(pValue, n + 1, "%s", buf);
                        *pulSize = AnscSizeOfString(pValue);
                        return ANSC_STATUS_SUCCESS;
                }
		/* CID 56187 Logically dead code fix with CID 66248 */		
                else
                        return ANSC_STATUS_FAILURE;
    	}
        else
                return ANSC_STATUS_FAILURE;

}

void FillParamUpdateSource(cJSON *partnerObj, char *key, char *paramUpdateSource)
{
    cJSON *paramObj = cJSON_GetObjectItem( partnerObj, key);
    if ( paramObj != NULL )
    {
        char *valuestr = NULL;
        cJSON *paramObjVal = cJSON_GetObjectItem(paramObj, "UpdateSource");
        if (paramObjVal)
            valuestr = paramObjVal->valuestring;
        if (valuestr != NULL)
        {
            AnscCopyString(paramUpdateSource, valuestr);
            valuestr = NULL;
        }
        else
        {
            CcspTraceWarning(("%s - %s UpdateSource is NULL\n", __FUNCTION__, key ));
        }
    }
    else
    {
        CcspTraceWarning(("%s - %s Object is NULL\n", __FUNCTION__, key ));
    }
}

void FillMissingSyscfgParam(cJSON *partnerObj, char *key, char *syscfgParam)
{
    int ret;
    char buf[64] = {0};

    ret = syscfg_get( NULL, syscfgParam, buf, sizeof(buf));

    if( ret != 0 || buf[0] == '\0' )
    {
	cJSON *paramObj = cJSON_GetObjectItem( partnerObj, key);
	if ( paramObj != NULL )
	{
	    char *valuestr = NULL;
	    cJSON *paramObjVal = cJSON_GetObjectItem(paramObj, "ActiveValue");
	    if (paramObjVal)
		valuestr = paramObjVal->valuestring;
	    if (valuestr != NULL)
	    {
		if (syscfg_set_commit(NULL, syscfgParam, valuestr) != 0)
		{
		    CcspTraceError(("%s - syscfg_set %s failed\n", __FUNCTION__,syscfgParam));
		}
	    }
	    else
	    {
		CcspTraceWarning(("%s - %s ActiveValue is NULL\n", __FUNCTION__, key ));
	    }
	}
	else
	{
	    CcspTraceWarning(("%s - %s Object is NULL\n", __FUNCTION__, key ));
	}
    }
}

void FillPartnerIDJournal
    (
        cJSON *json ,
        char *partnerID ,
        PCOSA_MTA_ETHWAN_PROV_INFO  pmtaethpro
    )
{
                cJSON *partnerObj = cJSON_GetObjectItem( json, partnerID );
                if( partnerObj != NULL)
                {     /*Coverity Fix CID 60029 70533 72638 53440 58371 Incompatible argument type */
                      FillParamUpdateSource(partnerObj, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.StartupIPMode", pmtaethpro->StartupIPMode.UpdateSource);
                      FillParamUpdateSource(partnerObj, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv4PrimaryDhcpServerOptions", pmtaethpro->IPv4PrimaryDhcpServerOptions.UpdateSource);
                      FillParamUpdateSource(partnerObj, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv4SecondaryDhcpServerOptions", pmtaethpro->IPv4SecondaryDhcpServerOptions.UpdateSource);
                      FillParamUpdateSource(partnerObj, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv6PrimaryDhcpServerOptions", pmtaethpro->IPv6PrimaryDhcpServerOptions.UpdateSource);
                      FillParamUpdateSource(partnerObj, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv6SecondaryDhcpServerOptions", pmtaethpro->IPv6SecondaryDhcpServerOptions.UpdateSource);

		      //Check if syscfg parameters are missing in db. If so update the syscfg parameter based on ActiveValue from bootstrap.json
		      FillMissingSyscfgParam(partnerObj, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.StartupIPMode","StartupIPMode");
		      FillMissingSyscfgParam(partnerObj, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv4PrimaryDhcpServerOptions","IPv4PrimaryDhcpServerOptions");
		      FillMissingSyscfgParam(partnerObj, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv4SecondaryDhcpServerOptions","IPv4SecondaryDhcpServerOptions");
		      FillMissingSyscfgParam(partnerObj, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv6PrimaryDhcpServerOptions","IPv6PrimaryDhcpServerOptions");
		      FillMissingSyscfgParam(partnerObj, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv6SecondaryDhcpServerOptions","IPv6SecondaryDhcpServerOptions");
                }
                else
                {
                      CcspTraceWarning(("%s - PARTNER ID OBJECT Value is NULL\n", __FUNCTION__ ));
                }
}

//Get the UpdateSource info from /opt/secure/bootstrap.json. This is needed to know for override precedence rules in set handlers
ANSC_STATUS
CosaMTAInitializeEthWanProvJournal
    (
        PCOSA_MTA_ETHWAN_PROV_INFO  pmtaethpro
    )
{
        char *data = NULL;
        cJSON *json = NULL;
        FILE *fileRead = NULL;
        char PartnerID[PARTNER_ID_LEN] = {0};
        ULONG size = PARTNER_ID_LEN - 1;
        int len =0;
        if (!pmtaethpro)
        {
                CcspTraceWarning(("%s-%d : NULL param\n" , __FUNCTION__, __LINE__ ));
                return ANSC_STATUS_FAILURE;
        }

         fileRead = fopen( BOOTSTRAP_INFO_FILE, "r" );
         if( fileRead == NULL )
         {
                 CcspTraceWarning(("%s-%d : Error in opening JSON file\n" , __FUNCTION__, __LINE__ ));
                 return ANSC_STATUS_FAILURE;
         }

         fseek( fileRead, 0, SEEK_END );
         len = ftell( fileRead );
	 /* Coverity Issue Fix - CID:74334 : Negative Returns */
	 if (len < 0)
	 {
		CcspTraceWarning(("%s-%d : Error finding size of JSON file\n" , __FUNCTION__, __LINE__ ));
                fclose(fileRead);
                 return ANSC_STATUS_FAILURE;
         }
	 fseek( fileRead, 0, SEEK_SET );
         data = ( char* )malloc( sizeof(char) * (len + 1) );
         if (data != NULL)
         {
                memset( data, 0, ( sizeof(char) * (len + 1) ));
               /*Coverity CID Fix :60325 CHECKED_RETURN */
               if( fread( data, 1, len, fileRead ) < (len*sizeof(char)) )
                     CcspTraceWarning(("%s-%d : fread  failed \n", __FUNCTION__, __LINE__));
	       /*CID :135523 String not null terminated (STRING_NULL) */
               data[len] = '\0';
			
         }
         else
         {
                 CcspTraceWarning(("%s-%d : Memory allocation failed \n", __FUNCTION__, __LINE__));
                 fclose( fileRead );
                 return ANSC_STATUS_FAILURE;
         }

         fclose( fileRead );

         if ( data == NULL )
         {
                CcspTraceWarning(("%s-%d : fileRead failed \n", __FUNCTION__, __LINE__));
                return ANSC_STATUS_FAILURE;
         }
	 /* CID 135523 String not null terminated fix */
         else if ( strnlen(data,len) != 0)
         {
                 json = cJSON_Parse( data );
                 if( !json )
                 {
                         CcspTraceWarning((  "%s : json file parser error : [%d]\n", __FUNCTION__,__LINE__));
                         free(data);
                         return ANSC_STATUS_FAILURE;
                 }
                 else
                 {
                         if(ANSC_STATUS_SUCCESS == fillCurrentPartnerId(PartnerID, &size))
                         {
                                if ( PartnerID[0] != '\0' )
                                {
                                        CcspTraceWarning(("%s : Partner = %s \n", __FUNCTION__, PartnerID));
                                        FillPartnerIDJournal(json, PartnerID, pmtaethpro);
                                }
                                else
                                {
                                        CcspTraceWarning(( "Reading Deafult PartnerID Values \n" ));
                                        /*Coverity Fix CID:67642 DC.STRING_BUFFER */
                                        strncpy(PartnerID, "comcast",sizeof(PartnerID)-1);
                                        FillPartnerIDJournal(json, PartnerID, pmtaethpro);
                                }
                        }
                        else{
                                CcspTraceWarning(("Failed to get Partner ID\n"));
                        }
                        cJSON_Delete(json);
                }
                free(data);
               
         }
         else
         {
                CcspTraceWarning(("BOOTSTRAP_INFO_FILE %s is empty\n", BOOTSTRAP_INFO_FILE));
                /* Coverity Fix CID: 58665 RESOURCE_LEAK */
                free(data);
                return ANSC_STATUS_FAILURE;
         }
         return ANSC_STATUS_SUCCESS;
}

static int writeToJson(char *data, char *file)
{
    FILE *fp;
    fp = fopen(file, "w");
    if (fp == NULL)
    {
        CcspTraceWarning(("%s : %d Failed to open file %s\n", __FUNCTION__,__LINE__,file));
        return -1;
    }

    fwrite(data, strlen(data), 1, fp);
    fclose(fp);
    return 0;
}

ANSC_STATUS UpdateJsonParamLegacy
	(
		char*                       pKey,
		char*			PartnerId,
		char*			pValue
    )
{
	cJSON *partnerObj = NULL;
	cJSON *json = NULL;
	FILE *fileRead = NULL;
	char * cJsonOut = NULL;
	char* data = NULL;
	 int len  = 0;
	 int configUpdateStatus = -1;
	 fileRead = fopen( PARTNERS_INFO_FILE, "r" );
	 if( fileRead == NULL ) 
	 {
		 CcspTraceWarning(("%s-%d : Error in opening JSON file\n" , __FUNCTION__, __LINE__ ));
		 return ANSC_STATUS_FAILURE;
	 }
	 
	 fseek( fileRead, 0, SEEK_END );
	 len = ftell( fileRead );
	 /* Coverity Issue Fix - CID:74334 : Negative Returns*/
	 if (len < 0)
	 {
		CcspTraceWarning(("%s-%d : Error finding size of JSON file\n" , __FUNCTION__, __LINE__ ));
                 fclose(fileRead);
                 return ANSC_STATUS_FAILURE;
         }
	 fseek( fileRead, 0, SEEK_SET );
	 data = ( char* )malloc( sizeof(char) * (len + 1) );
	 if (data != NULL) 
	 {
		memset( data, 0, ( sizeof(char) * (len + 1) ));
	 	if( fread( data, sizeof(char), len, fileRead ) < (len*sizeof(char)) )
                     CcspTraceWarning(("%s-%d : File read failed \n", __FUNCTION__, __LINE__));
	 } 
	 else 
	 {
		 CcspTraceWarning(("%s-%d : Memory allocation failed \n", __FUNCTION__, __LINE__));
		 fclose( fileRead );
		 return ANSC_STATUS_FAILURE;
	 }
	 
	 fclose( fileRead );
	 if ( data == NULL )
	 {
		CcspTraceWarning(("%s-%d : fileRead failed \n", __FUNCTION__, __LINE__));
		return ANSC_STATUS_FAILURE;
	 }
	 /* CID 135238 String not null terminated fix */
	 else if ( strnlen(data,len) != 0)
	 {
		 json = cJSON_Parse( data );
		 if( !json ) 
		 {
			 CcspTraceWarning((  "%s : json file parser error : [%d]\n", __FUNCTION__,__LINE__));
			 free(data);
			 return ANSC_STATUS_FAILURE;
		 } 
		 else
		 {
			 partnerObj = cJSON_GetObjectItem( json, PartnerId );
			 if ( NULL != partnerObj)
			 {
				 if (NULL != cJSON_GetObjectItem( partnerObj, pKey) )
				 {
					 cJSON_ReplaceItemInObject(partnerObj, pKey, cJSON_CreateString(pValue));
					 cJsonOut = cJSON_Print(json);
                     if(cJsonOut)
                     {
                        CcspTraceWarning(( "Updated json content is %s\n", cJsonOut));
                        configUpdateStatus = writeToJson(cJsonOut, PARTNERS_INFO_FILE);
                        free(cJsonOut);
                     }
					 if ( !configUpdateStatus)
					 {
						 CcspTraceWarning(( "Updated Value for %s partner\n",PartnerId));
						 CcspTraceWarning(( "Param:%s - Value:%s\n",pKey,pValue));
					 }
					 else
				 	{
						 CcspTraceWarning(( "Failed to update value for %s partner\n",PartnerId));
						 CcspTraceWarning(( "Param:%s\n",pKey));
			 			 cJSON_Delete(json);
                         /* Coverity Fix CID : 65542 Resource leak */
                         if (data)
                         {
			                 free(data);
                         }
						 return ANSC_STATUS_FAILURE;						
				 	}
				 }
				else
			 	{
			 		CcspTraceWarning(("%s - OBJECT  Value is NULL %s\n", pKey,__FUNCTION__ ));
			 		cJSON_Delete(json);
                    /* Coverity Fix CID : 65542 Resource leak */
                    if (data)
                    {
                        free(data);
                    }
			 		return ANSC_STATUS_FAILURE;
			 	}
			 
			 }
			 else
			 {
			 	CcspTraceWarning(("%s - PARTNER ID OBJECT Value is NULL\n", __FUNCTION__ ));
			 	cJSON_Delete(json);
                /* Coverity Fix CID : 65542 Resource leak */
                if (data)
                {
                    free(data);
                }
			 	return ANSC_STATUS_FAILURE;
			 }
			cJSON_Delete(json);
            /* Coverity Fix CID : 65542 Resource leak */
            if (data)
            {
               free(data);
            }
		 }
	  }
	  else
	  {
		CcspTraceWarning(("PARTNERS_INFO_FILE %s is empty\n", PARTNERS_INFO_FILE));
        /* Coverity Fix CID : 65542 Resource leak */
        if (data)
        {
            free(data);
        }
		return ANSC_STATUS_FAILURE;
	  }
	 return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS UpdateJsonParam
        (
                char*                       pKey,
                char*                   PartnerId,
                char*                   pValue,
                char*                   pSource,
                char*                   pCurrentTime
    )
{
        cJSON *partnerObj = NULL;
        cJSON *json = NULL;
        FILE *fileRead = NULL;
        char * cJsonOut = NULL;
        char* data = NULL;
         int len = 0;
         int configUpdateStatus = -1;
         fileRead = fopen( BOOTSTRAP_INFO_FILE, "r" );
         if( fileRead == NULL )
         {
                 CcspTraceWarning(("%s-%d : Error in opening JSON file\n" , __FUNCTION__, __LINE__ ));
                 return ANSC_STATUS_FAILURE;
         }

         fseek( fileRead, 0, SEEK_END );
         len = ftell( fileRead );
	 /* Coverity Issue Fix - CID:74334 : Negative Returns */
	 if (len < 0)
	 {
		CcspTraceWarning(("%s-%d : Error finding size of JSON file\n" , __FUNCTION__, __LINE__ ));
                 fclose(fileRead);
                 return ANSC_STATUS_FAILURE;
         }
         fseek( fileRead, 0, SEEK_SET );
         data = ( char* )malloc( sizeof(char) * (len + 1) );
         if (data != NULL)
         {
                memset( data, 0, ( sizeof(char) * (len + 1) ));
                if( fread( data, sizeof(char), len, fileRead ) < (len*sizeof(char)) )
                     CcspTraceWarning(("%s-%d : File read failed \n", __FUNCTION__, __LINE__));
         }
         else
         {
                 CcspTraceWarning(("%s-%d : Memory allocation failed \n", __FUNCTION__, __LINE__));
                 fclose( fileRead );
                 return ANSC_STATUS_FAILURE;
         }

         fclose( fileRead );
         if ( data == NULL )
         {
                CcspTraceWarning(("%s-%d : fileRead failed \n", __FUNCTION__, __LINE__));
                return ANSC_STATUS_FAILURE;
         }
	 /* CID 135285 String not null terminated fix */
         else if ( strnlen(data,len) != 0)
         {
                 json = cJSON_Parse( data );
                 if( !json )
                 {
                         CcspTraceWarning((  "%s : json file parser error : [%d]\n", __FUNCTION__,__LINE__));
                         free(data);
                         return ANSC_STATUS_FAILURE;
                 }
                 else
                 {
                         partnerObj = cJSON_GetObjectItem( json, PartnerId );
                         if ( NULL != partnerObj)
                         {
                                 cJSON *paramObj = cJSON_GetObjectItem( partnerObj, pKey);
                                 if (NULL != paramObj )
                                 {
                                         cJSON_ReplaceItemInObject(paramObj, "ActiveValue", cJSON_CreateString(pValue));
                                         cJSON_ReplaceItemInObject(paramObj, "UpdateTime", cJSON_CreateString(pCurrentTime));
                                         cJSON_ReplaceItemInObject(paramObj, "UpdateSource", cJSON_CreateString(pSource));

                                         cJsonOut = cJSON_Print(json);
                                         if(cJsonOut)
                                         {
                                            CcspTraceWarning(( "Updated json content is %s\n", cJsonOut));
                                            configUpdateStatus = writeToJson(cJsonOut, BOOTSTRAP_INFO_FILE);
                                            //Check CLEAR_TRACK_FILE and update in nvram, if needed.
                                            unsigned int flags = 0;
                                            FILE *fp = fopen(CLEAR_TRACK_FILE, "r");
                                            if (fp)
                                            {
                                                /* Coverity Fix : CHECKED_RETURN */
                                                if (fscanf(fp, "%u", &flags) != 1) {
                                                    CcspTraceWarning(("%s: Failed to read flags from %s, defaulting to 0\n", __FUNCTION__, CLEAR_TRACK_FILE));
                                                    flags = 0;
                                                }
                                                fclose(fp);
                                            }
                                            if ((flags & NVRAM_BOOTSTRAP_CLEARED) == 0)
                                            {
                                                CcspTraceWarning(("%s: Updating %s\n", __FUNCTION__, BOOTSTRAP_INFO_FILE_BACKUP));
                                                writeToJson(cJsonOut, BOOTSTRAP_INFO_FILE_BACKUP);
                                            }
                                            free(cJsonOut);
                                         }
                                         if ( !configUpdateStatus)
                                         {
                                                 CcspTraceWarning(( "Bootstrap config update: %s, %s, %s, %s \n", pKey, pValue, PartnerId, pSource));
                                         }
                                         else
                                        {
                                                 CcspTraceWarning(( "Failed to update value for %s partner\n",PartnerId));
                                                 CcspTraceWarning(( "Param:%s\n",pKey));
                                                 cJSON_Delete(json);
						 /* CID 72622 Resource leak fix */
						 free(data);
						 data = NULL;
                                                 return ANSC_STATUS_FAILURE;
                                        }
                                 }
                                else
                                {
                                        CcspTraceWarning(("%s - OBJECT  Value is NULL %s\n", pKey,__FUNCTION__ ));
                                        cJSON_Delete(json);
					/* CID 72622 Resource leak fix */
					free(data);
					data = NULL;
                                        return ANSC_STATUS_FAILURE;
                                }

                         }
                         else
                         {
                                CcspTraceWarning(("%s - PARTNER ID OBJECT Value is NULL\n", __FUNCTION__ ));
                                cJSON_Delete(json);
				/* CID 72622 Resource leak fix */
				free(data);
				data = NULL;
                                return ANSC_STATUS_FAILURE;
                         }
                        cJSON_Delete(json);
                 }
          }
          else
          {
                CcspTraceWarning(("BOOTSTRAP_INFO_FILE %s is empty\n", BOOTSTRAP_INFO_FILE));
                /* Coverity Fix CID : 72622 Resource leak */
                if (data)
                {
                    free(data);
                }
                return ANSC_STATUS_FAILURE;
          }

          //Also update in the legacy file /nvram/partners_defaults.json for firmware roll over purposes.
          UpdateJsonParamLegacy(pKey, PartnerId, pValue);
          /* Coverity Fix CID : 72622 Resource leak */
          if (data)
          {
              free(data);
          }
         return ANSC_STATUS_SUCCESS;
}
