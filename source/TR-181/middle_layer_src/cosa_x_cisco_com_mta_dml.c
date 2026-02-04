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

// #ifdef CONFIG_TI_PACM
/**************************************************************************

    module: cosa_x_cisco_com_mta_dml.c

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    description:

        This file implementes back-end apis for the COSA Data Model Library

    -------------------------------------------------------------------

    environment:

        platform independent

    -------------------------------------------------------------------

    author:

        COSA XML TOOL CODE GENERATOR 1.0

    -------------------------------------------------------------------

    revision:

        01/05/2012    initial revision.

**************************************************************************/

/* Legacy platform includes commented out for JSON-driven RBUS approach */
/* #include "ansc_platform.h" */
#include "mta_compat_types.h"  /* Include compatibility layer */
#include "plugin_main_apis.h"
/* Legacy common-library includes commented out for JSON-driven RBUS approach */
/* #include "safec_lib_common.h" */
#include <safec_lib.h>
#include "cosa_x_cisco_com_mta_apis.h"
#include "cosa_x_cisco_com_mta_dml.h"
#include "cosa_x_cisco_com_mta_internal.h"
/* Legacy DBUS headers commented out for RBUS approach */
/* #include "ccsp_base_api.h" */
/* #include "messagebus_interface_helper.h" */
#include "syscfg/syscfg.h"

extern ULONG g_currentBsUpdate;

#define MAX_LINE_REG 256

#define IS_UPDATE_ALLOWED_IN_DM(paramName, requestorStr) ({                                                                                        \
    if ( g_currentBsUpdate == DSLH_CWMP_BS_UPDATE_firmware ||                                                                                      \
         (g_currentBsUpdate == DSLH_CWMP_BS_UPDATE_rfcUpdate && (strcmp(requestorStr, BS_SOURCE_RFC_STR) != 0)))                                   \
    {                                                                                                                                              \
       CcspTraceWarning(("Do NOT allow override of param: %s bsUpdate = %lu, requestor = %s\n", paramName, g_currentBsUpdate, requestorStr));      \
       return FALSE;                                                                                                                               \
    }                                                                                                                                              \
})

// If the requestor is RFC but the param was previously set by webpa, do not override it.
#define IS_UPDATE_ALLOWED_IN_JSON(paramName, requestorStr, UpdateSource) ({                                                                        \
   if ((strcmp(requestorStr, BS_SOURCE_RFC_STR) == 0) && (strcmp(UpdateSource, BS_SOURCE_WEBPA_STR) == 0))                                         \
   {                                                                                                                                               \
      CcspTraceWarning(("Do NOT allow override of param: %s requestor = %lu updateSource = %s\n", paramName, g_currentWriteEntity, UpdateSource)); \
      return FALSE;                                                                                                                                \
   }                                                                                                                                               \
})

/***********************************************************************
 IMPORTANT NOTE:

 According to TR69 spec:
 On successful receipt of a SetParameterValues RPC, the CPE MUST apply 
 the changes to all of the specified Parameters atomically. That is, either 
 all of the value changes are applied together, or none of the changes are 
 applied at all. In the latter case, the CPE MUST return a fault response 
 indicating the reason for the failure to apply the changes. 
 
 The CPE MUST NOT apply any of the specified changes without applying all 
 of them.

 In order to set parameter values correctly, the back-end is required to
 hold the updated values until "Validate" and "Commit" are called. Only after
 all the "Validate" passed in different objects, the "Commit" will be called.
 Otherwise, "Rollback" will be called instead.

 The sequence in COSA Data Model will be:

 SetParamBoolValue/SetParamIntValue/SetParamUlongValue/SetParamStringValue
 -- Backup the updated values;

 if( Validate_XXX())
 {
     Commit_XXX();    -- Commit the update all together in the same object
 }
 else
 {
     Rollback_XXX();  -- Remove the update at backup;
 }
 
***********************************************************************/
/***********************************************************************

 APIs for Object:

    X_CISCO_COM_MTA_V6.

    *  X_CISCO_COM_MTA_V6_GetParamUlongValue
    *  X_CISCO_COM_MTA_V6_GetParamStringValue
    *  X_CISCO_COM_MTA_V6_Validate
    *  X_CISCO_COM_MTA_V6_Commit
    *  X_CISCO_COM_MTA_V6_Rollback

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        X_CISCO_COM_MTA_V6_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
X_CISCO_COM_MTA_V6_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    COSA_MTA_DHCPv6_INFO              Info;
    memset(&Info, 0, sizeof(COSA_MTA_DHCPv6_INFO));

    /* collect value */
    if (CosaDmlMTAGetDHCPV6Info(NULL, &Info) != ANSC_STATUS_SUCCESS)
        return FALSE;

    if (strcmp(ParamName, "LeaseTimeRemaining") == 0)
    {
        *puLong = Info.LeaseTimeRemaining;

        return TRUE;
    }
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        X_CISCO_COM_MTA_V6_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
X_CISCO_COM_MTA_V6_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(pUlSize);
    COSA_MTA_DHCPv6_INFO              Info;
    memset(&Info, 0, sizeof(COSA_MTA_DHCPv6_INFO));

    /* collect value */
    if (CosaDmlMTAGetDHCPV6Info(NULL, &Info) != ANSC_STATUS_SUCCESS)
         return -1;

    /* check the parameter name and return the corresponding value */
    if (strcmp(ParamName, "IPV6Address") == 0)
    {
        AnscCopyString(pValue, Info.IPV6Address);

        return 0;
    }

    if (strcmp(ParamName, "Prefix") == 0)
    {
        AnscCopyString(pValue, Info.Prefix);

        return 0;
    }

    if (strcmp(ParamName, "BootFileName") == 0)
    {
        AnscCopyString(pValue, Info.BootFileName);

        return 0;
    }

    if (strcmp(ParamName, "FQDN") == 0)
    {
        AnscCopyString(pValue, Info.FQDN);

        return 0;
    }

    if (strcmp(ParamName, "Gateway") == 0)
    {
        AnscCopyString(pValue, Info.Gateway);

        return 0;
    }

    if (strcmp(ParamName, "RebindTimeRemaining") == 0)
    {
        AnscCopyString(pValue, Info.RebindTimeRemaining);

        return 0;
    }

    if (strcmp(ParamName, "RenewTimeRemaining") == 0)
    {
        AnscCopyString(pValue, Info.RenewTimeRemaining);

        return 0;
    }

    if (strcmp(ParamName, "PrimaryDNS") == 0)
    {
        AnscCopyString(pValue, Info.PrimaryDNS);

        return 0;
    }

    if (strcmp(ParamName, "SecondaryDNS") == 0)
    {
        AnscCopyString(pValue, Info.SecondaryDNS);

        return 0;
    }

    if (strcmp(ParamName, "DHCPOption3") == 0)
    {
        AnscCopyString(pValue, Info.DHCPOption3);

        return 0;
    }

    if (strcmp(ParamName, "DHCPOption6") == 0)
    {
        AnscCopyString(pValue, Info.DHCPOption6);

        return 0;
    }

    if (strcmp(ParamName, "DHCPOption7") == 0)
    {
        AnscCopyString(pValue, Info.DHCPOption7);

        return 0;
    }

    if (strcmp(ParamName, "DHCPOption8") == 0)
    {
        AnscCopyString(pValue, Info.DHCPOption8);

        return 0;
    }

    if (strcmp(ParamName, "PCVersion") == 0)
    {
        AnscCopyString(pValue, Info.PCVersion);

        return 0;
    }

    if (strcmp(ParamName, "MACAddress") == 0)
    {
        AnscCopyString(pValue, Info.MACAddress);

        return 0;
    }

    if (strcmp(ParamName, "PrimaryDHCPv6Server") == 0)
    {
        AnscCopyString(pValue, Info.PrimaryDHCPv6Server);

        return 0;
    }

    if (strcmp(ParamName, "SecondaryDHCPv6Server") == 0)
    {
        AnscCopyString(pValue, Info.SecondaryDHCPv6Server);

        return 0;
    }

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        X_CISCO_COM_MTA_V6_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation. 

                ULONG*                      puLength
                The output length of the param name. 

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
X_CISCO_COM_MTA_V6_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(pReturnParamName);
    UNREFERENCED_PARAMETER(puLength);

    return TRUE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        X_CISCO_COM_MTA_V6_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
X_CISCO_COM_MTA_V6_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        X_CISCO_COM_MTA_V6_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a 
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
X_CISCO_COM_MTA_V6_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    return 0;
}

/***********************************************************************

 APIs for Object:

    X_CISCO_COM_MTA.

    *  X_CISCO_COM_MTA_GetParamBoolValue
    *  X_CISCO_COM_MTA_GetParamIntValue
    *  X_CISCO_COM_MTA_GetParamUlongValue
    *  X_CISCO_COM_MTA_GetParamStringValue
    *  X_CISCO_COM_MTA_SetParamBoolValue
    *  X_CISCO_COM_MTA_SetParamIntValue
    *  X_CISCO_COM_MTA_SetParamUlongValue
    *  X_CISCO_COM_MTA_SetParamStringValue
    *  X_CISCO_COM_MTA_Validate
    *  X_CISCO_COM_MTA_Commit
    *  X_CISCO_COM_MTA_Rollback

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        X_CISCO_COM_MTA_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
X_CISCO_COM_MTA_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject     = (PCOSA_DATAMODEL_MTA )g_pCosaBEManager->hMTA;
    PCOSA_MTA_PKTC                  pPktc         = (PCOSA_MTA_PKTC      )&pMyObject->Pktc;
    PCOSA_DML_MTA_LOG               pCfg          = (PCOSA_DML_MTA_LOG   )&pMyObject->MtaLog;
        /* Coverity Issue Fix - CID:57316 : UnInitialised Variable */
        BOOL                            btemp = FALSE;
    errno_t                         rc            = -1;
    int                             ind           = -1;
	
    /* check the parameter name and return the corresponding value */

    rc = strcmp_s("pktcMtaDevEnabled", strlen("pktcMtaDevEnabled"), ParamName , &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK)) 
    {
        /* collect value */
        /*Coverity Fix CID: 55776 CHECKED_RETURN */
        if( CosaDmlMTAGetPktc(NULL, pPktc) != ANSC_STATUS_SUCCESS )
         {
              CcspTraceWarning(("%s-CosaDmlMTAGetPktc is not success\n",__FUNCTION__));
               free(pPktc);
		return FALSE;
         }
         
        *pBool = pPktc->pktcMtaDevEnabled;
        return TRUE;
    }


    rc = strcmp_s("DSXLogEnable", strlen("DSXLogEnable"), ParamName, &ind);
    ERR_CHK(rc);
    if( (!ind) && (rc == EOK))
    {
        /* collect value */
        CosaDmlMTAGetDSXLogEnable(NULL, &btemp);
		*pBool = btemp;
        return TRUE;
    }

    rc = strcmp_s("ClearDSXLog", strlen("ClearDSXLog"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *pBool = FALSE;
        return TRUE;
    }

    rc = strcmp_s("CallSignallingLogEnable", strlen("CallSignallingLogEnable"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK)) 
    {
        /* collect value */
		CosaDmlMTAGetCallSignallingLogEnable(NULL, &btemp);
		*pBool = btemp;
        return TRUE;
    }

    rc = strcmp_s("ClearCallSignallingLog", strlen("ClearCallSignallingLog"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *pBool = FALSE;
        return TRUE;
    }

    rc = strcmp_s("EnableDECTLog", strlen("EnableDECTLog"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *pBool = pCfg->EnableDECTLog;
        return TRUE;
    }

    rc = strcmp_s("EnableMTALog", strlen("EnableMTALog"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *pBool = pCfg->EnableMTALog;
        return TRUE;
    }

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        X_CISCO_COM_MTA_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
X_CISCO_COM_MTA_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(ParamName);
    UNREFERENCED_PARAMETER(pInt);

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        X_CISCO_COM_MTA_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
X_CISCO_COM_MTA_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    COSA_MTA_DHCP_INFO              Info;
    memset(&Info, 0, sizeof(COSA_MTA_DHCP_INFO));

    PCOSA_DATAMODEL_MTA             pMyObject     = (PCOSA_DATAMODEL_MTA )g_pCosaBEManager->hMTA;
    PCOSA_MTA_PKTC                  pPktc         = (PCOSA_MTA_PKTC      )&pMyObject->Pktc;
    errno_t                         rc            = -1;
    int                             ind           = -1;

    rc = strcmp_s("MTAResetCount", strlen("MTAResetCount"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        if (CosaDmlMtaGetResetCount(NULL,MTA_RESET,puLong) != ANSC_STATUS_SUCCESS)
        return FALSE;

        return TRUE;
    }

    rc = strcmp_s("LineResetCount", strlen("LineResetCount"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        if (CosaDmlMtaGetResetCount(NULL,LINE_RESET,puLong) != ANSC_STATUS_SUCCESS)
        return FALSE;

        return TRUE;
    }

    /* check the parameter name and return the corresponding value */   
    if ((strncmp("pktc", ParamName, strlen("pktc"))) == 0)
    {
        /* collect value */
        if (CosaDmlMTAGetPktc(NULL, pPktc) != ANSC_STATUS_SUCCESS) 
        {
            return FALSE;
        }
        
        ind = -1;
        rc =  -1;
        rc = strcmp_s("pktcSigDefCallSigTos",  strlen("pktcSigDefCallSigTos"), ParamName, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            *puLong = pPktc->pktcSigDefCallSigTos;
            return TRUE;
        }
               
        rc = strcmp_s("pktcSigDefMediaStreamTos", strlen("pktcSigDefMediaStreamTos"), ParamName, &ind);
        ERR_CHK(rc);
        
        if((!ind) && (rc == EOK))
        {
            *puLong = pPktc->pktcSigDefMediaStreamTos;
            return TRUE;
        }
        rc = strcmp_s("pktcMtaDevRealmOrgName", strlen("pktcMtaDevRealmOrgName"), ParamName, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            *puLong = pPktc->pktcMtaDevRealmOrgName;
            return TRUE;
        }
        rc = strcmp_s("pktcMtaDevCmsKerbRealmName", strlen("pktcMtaDevCmsKerbRealmName"), ParamName, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            *puLong = pPktc->pktcMtaDevCmsKerbRealmName;
            return TRUE;
        }
        rc = strcmp_s("pktcMtaDevCmsIpsecCtrl", strlen("pktcMtaDevCmsIpsecCtrl"), ParamName, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            *puLong = pPktc->pktcMtaDevCmsIpsecCtrl;
            return TRUE;
        }
        rc = strcmp_s("pktcMtaDevCmsSolicitedKeyTimeout", strlen("pktcMtaDevCmsSolicitedKeyTimeout"), ParamName, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            *puLong = pPktc->pktcMtaDevCmsSolicitedKeyTimeout;
            return TRUE;
        }
        rc = strcmp_s("pktcMtaDevRealmPkinitGracePeriod", strlen("pktcMtaDevRealmPkinitGracePeriod"), ParamName, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            *puLong = pPktc->pktcMtaDevRealmPkinitGracePeriod;
            return TRUE;
        }    
    }
    else 
    {
        if (CosaDmlMTAGetDHCPInfo(NULL, &Info) != ANSC_STATUS_SUCCESS)
        {
            return FALSE;
        }
        rc = strcmp_s("IPAddress", strlen("IPAddress"), ParamName, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            *puLong = Info.IPAddress.Value;
            return TRUE;
        }
        
        rc = strcmp_s("SubnetMask", strlen("SubnetMask"), ParamName, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            *puLong = Info.SubnetMask.Value;
            return TRUE;
        }
        rc = strcmp_s("Gateway", strlen("Gateway"), ParamName, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            *puLong = Info.Gateway.Value;
            return TRUE;
        }
        rc = strcmp_s("LeaseTimeRemaining", strlen("LeaseTimeRemaining"), ParamName, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            *puLong = Info.LeaseTimeRemaining;
            return TRUE;
        }
        rc = strcmp_s("PrimaryDNS", strlen("PrimaryDNS"), ParamName, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            *puLong = Info.PrimaryDNS.Value;
            return TRUE;
        }
        rc = strcmp_s("SecondaryDNS", strlen("SecondaryDNS"), ParamName, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            *puLong = Info.SecondaryDNS.Value;
            return TRUE;
        }
        rc = strcmp_s("PrimaryDHCPServer", strlen("PrimaryDHCPServer"), ParamName, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            *puLong = Info.PrimaryDHCPServer.Value;
            // AnscCopyString(pValue, Info.PrimaryDHCPServer);
            return TRUE;
        }
        rc = strcmp_s("SecondaryDHCPServer", strlen("SecondaryDHCPServer"), ParamName, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            *puLong = Info.SecondaryDHCPServer.Value;
            // AnscCopyString(pValue, Info.SecondaryDHCPServer);
            return TRUE;
        }
    }

    rc = strcmp_s("ClearLineStats", strlen("ClearLineStats"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        *puLong = 0;

	return TRUE;
    }

    /*AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName));*/ 
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        X_CISCO_COM_MTA_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
X_CISCO_COM_MTA_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    COSA_MTA_DHCP_INFO              Info;
    memset(&Info, 0, sizeof(COSA_MTA_DHCP_INFO));

    errno_t                         rc   = -1;
    int                             ind  = -1;
    
    
    if (CosaDmlMTAGetDHCPInfo(NULL, &Info) != ANSC_STATUS_SUCCESS)
            return -1;
    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("BootFileName", strlen("BootFileName"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        rc = strcpy_s(pValue, *pUlSize, Info.BootFileName);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        return 0;
    }
    
    rc = strcmp_s("FQDN", strlen("FQDN"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        rc = strcpy_s(pValue, *pUlSize, Info.FQDN);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }
    
    rc = strcmp_s("RebindTimeRemaining", strlen("RebindTimeRemaining"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        rc = strcpy_s(pValue, *pUlSize, Info.RebindTimeRemaining);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RenewTimeRemaining", strlen("RenewTimeRemaining"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        rc = strcpy_s(pValue, *pUlSize, Info.RenewTimeRemaining);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("DHCPOption3", strlen("DHCPOption3"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        rc = strcpy_s(pValue, *pUlSize, Info.DHCPOption3);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("DHCPOption6", strlen("DHCPOption6"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        rc = strcpy_s(pValue, *pUlSize, Info.DHCPOption6);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("DHCPOption7", strlen("DHCPOption7"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        rc = strcpy_s(pValue, *pUlSize, Info.DHCPOption7);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("DHCPOption8", strlen("DHCPOption8"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        rc = strcpy_s(pValue, *pUlSize, Info.DHCPOption8);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("PCVersion", strlen("PCVersion"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        rc = strcpy_s(pValue, *pUlSize, Info.PCVersion);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("MACAddress", strlen("MACAddress"), ParamName, &ind);
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        rc = strcpy_s(pValue, *pUlSize, Info.MACAddress);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        X_CISCO_COM_MTA_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
X_CISCO_COM_MTA_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject     = (PCOSA_DATAMODEL_MTA )g_pCosaBEManager->hMTA;
    PCOSA_MTA_PKTC                  pPktc         = (PCOSA_MTA_PKTC      )&pMyObject->Pktc;
    PCOSA_DML_MTA_LOG               pCfg          = (PCOSA_DML_MTA_LOG   )&pMyObject->MtaLog;
    errno_t                         rc            = -1;
    int                             ind           = -1;

    /* check the parameter name and set the corresponding value */

    rc = strcmp_s("pktcMtaDevEnabled", strlen("pktcMtaDevEnabled"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        pPktc->pktcMtaDevEnabled = bValue;
        return TRUE;
    }

    rc = strcmp_s("ClearDSXLog", strlen("ClearDSXLog"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        CosaDmlMTAClearDSXLog(NULL,bValue);
        return TRUE;
    }

    rc = strcmp_s("DSXLogEnable", strlen("DSXLogEnable"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        CosaDmlMTASetDSXLogEnable(NULL, bValue);
        return TRUE;
    }

    rc = strcmp_s("CallSignallingLogEnable", strlen("CallSignallingLogEnable"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
		CosaDmlMTASetCallSignallingLogEnable(NULL, bValue);
        return TRUE;
    }

    rc = strcmp_s("ClearCallSignallingLog", strlen("ClearCallSignallingLog"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))

    {
         CosaDmlMTAClearCallSignallingLog(NULL,bValue);
         return TRUE;
    }

    rc = strcmp_s("EnableDECTLog", strlen("EnableDECTLog"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))

    {
        /* save update to backup */
        pCfg->EnableDECTLog = bValue;
        return TRUE;
    }

    rc = strcmp_s("EnableMTALog", strlen("EnableMTALog"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* save update to backup */
        pCfg->EnableMTALog = bValue;
        return TRUE;
    }

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        X_CISCO_COM_MTA_SetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int                         iValue
            );

    description:

        This function is called to set integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int                         iValue
                The updated integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
X_CISCO_COM_MTA_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(ParamName);
    UNREFERENCED_PARAMETER(iValue);

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        X_CISCO_COM_MTA_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
X_CISCO_COM_MTA_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject     = (PCOSA_DATAMODEL_MTA )g_pCosaBEManager->hMTA;
    PCOSA_MTA_PKTC                  pPktc         = (PCOSA_MTA_PKTC      )&pMyObject->Pktc;
    errno_t                         rc            = -1;
    int                             ind           = -1;

    /* check the parameter name and set the corresponding value */

    rc = strcmp_s("pktcSigDefCallSigTos", strlen("pktcSigDefCallSigTos"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        pPktc->pktcSigDefCallSigTos = uValue;

        return TRUE;
    }

    rc = strcmp_s("pktcSigDefMediaStreamTos", strlen("pktcSigDefMediaStreamTos"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK)) 
    {
        pPktc->pktcSigDefMediaStreamTos = uValue;

        return TRUE;
    }

    rc = strcmp_s("ClearLineStats", strlen("ClearLineStats"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
	CosaDmlMtaClearCalls( uValue );

        return TRUE;
    }

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        X_CISCO_COM_MTA_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
X_CISCO_COM_MTA_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    /* check the parameter name and set the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(ParamName);
    UNREFERENCED_PARAMETER(pString);

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        X_CISCO_COM_MTA_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation. 

                ULONG*                      puLength
                The output length of the param name. 

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
X_CISCO_COM_MTA_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(pReturnParamName);
    UNREFERENCED_PARAMETER(puLength);

    return TRUE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        X_CISCO_COM_MTA_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
X_CISCO_COM_MTA_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject     = (PCOSA_DATAMODEL_MTA )g_pCosaBEManager->hMTA;
    PCOSA_MTA_PKTC                  pPktc         = (PCOSA_MTA_PKTC      )&pMyObject->Pktc;

    CosaDmlMTASetPktc(NULL, pPktc);

    return 0;
}


/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        X_CISCO_COM_MTA_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a 
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
X_CISCO_COM_MTA_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject     = (PCOSA_DATAMODEL_MTA )g_pCosaBEManager->hMTA;
    PCOSA_MTA_PKTC                  pPktc         = (PCOSA_MTA_PKTC      )&pMyObject->Pktc;
    /*Coverity Fix CID:51944 CHECKED_RETURN */
    if( CosaDmlMTAGetPktc(NULL, pPktc) != ANSC_STATUS_SUCCESS )
        CcspTraceWarning(("%s- CosaDmlMTAGetPktc not success\n",__FUNCTION__)); 

    return 0;
}


/***********************************************************************

 APIs for Object:

    Device.X_CISCO_COM_MTA.LineTable.{i}.

    *  LineTable_GetEntryCount
    *  LineTable_GetEntry
    *  LineTable_GetParamBoolValue
    *  LineTable_GetParamIntValue
    *  LineTable_GetParamUlongValue
    *  LineTable_GetParamStringValue
    *  LineTable_SetParamBoolValue
    *  LineTable_SetParamIntValue
    *  LineTable_SetParamUlongValue
    *  LineTable_SetParamStringValue
    *  LineTable_Validate
    *  LineTable_Commit
    *  LineTable_Rollback

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        LineTable_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
LineTable_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject     = (PCOSA_DATAMODEL_MTA     )g_pCosaBEManager->hMTA;
    return pMyObject->LineTableCount;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        LineTable_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
LineTable_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject     = (PCOSA_DATAMODEL_MTA     )g_pCosaBEManager->hMTA;
    PCOSA_MTA_LINETABLE_INFO        pLineTable    = (PCOSA_MTA_LINETABLE_INFO)pMyObject->pLineTable;

    /*CosaDmlMTALineTableGetEntry(NULL, nIndex, &(pLineTable[nIndex]));*/

    pLineTable[nIndex].InstanceNumber = nIndex + 1;

    *pInsNumber = nIndex + 1;
    return &(pLineTable[nIndex]); /* return the handle */
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        LineTable_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
LineTable_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    errno_t                         rc            = -1;
    int                             ind           = -1;
    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("TriggerDiagnostics", strlen("TriggerDiagnostics"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *pBool = FALSE;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        LineTable_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
LineTable_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(ParamName);
    UNREFERENCED_PARAMETER(pInt);

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        LineTable_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
LineTable_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    PCOSA_MTA_LINETABLE_INFO        pInfo = (PCOSA_MTA_LINETABLE_INFO)hInsContext;

    ULONG                           nIndex = pInfo->InstanceNumber - 1;
    errno_t                         rc            = -1;
    int                             ind           = -1;

    CosaDmlMTALineTableGetEntry(NULL, nIndex, pInfo);

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("LineNumber", strlen("LineNumber"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pInfo->LineNumber;

        return TRUE;
    }

    rc = strcmp_s("Status", strlen("Status"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pInfo->Status;

        return TRUE;
    }

    rc = strcmp_s("CAPort", strlen("CAPort"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pInfo->CAPort;

        return TRUE;
    }

    rc = strcmp_s("MWD", strlen("MWD"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pInfo->MWD;

        return TRUE;
    }

    rc = strcmp_s("OverCurrentFault", strlen("OverCurrentFault"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pInfo->OverCurrentFault;

        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        LineTable_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
LineTable_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCOSA_MTA_LINETABLE_INFO        pInfo = (PCOSA_MTA_LINETABLE_INFO)hInsContext;

    ULONG                           nIndex = pInfo->InstanceNumber - 1;
    errno_t                         rc     = -1;
    int                             ind    = -1;
    CosaDmlMTALineTableGetEntry(NULL, nIndex, pInfo);

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("HazardousPotential", strlen("HazardousPotential"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
       
        rc = strcpy_s(pValue, *pUlSize, pInfo->HazardousPotential);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("ForeignEMF", strlen("ForeignEMF"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pInfo->ForeignEMF);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        } 
        
        return 0;
    }

    rc = strcmp_s("ResistiveFaults", strlen("ResistiveFaults"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pInfo->ResistiveFaults);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("ReceiverOffHook", strlen("ReceiverOffHook"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pInfo->ReceiverOffHook);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RingerEquivalency", strlen("RingerEquivalency"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pInfo->RingerEquivalency);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("CAName", strlen("CAName"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pInfo->CAName);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        LineTable_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
LineTable_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    PCOSA_MTA_LINETABLE_INFO        pInfo = (PCOSA_MTA_LINETABLE_INFO)hInsContext;

    ULONG                           nIndex = pInfo->InstanceNumber - 1;
    errno_t                         rc            = -1;
    int                             ind           = -1;

    /* check the parameter name and set the corresponding value */
    // printf("\n\n ==> LineTable_SetParamBoolValue ParamName=%s \n\n", ParamName);
    rc = strcmp_s("TriggerDiagnostics", strlen("TriggerDiagnostics"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
		// printf("\n\n ==> LineTable_SetParamBoolValue bValue=%d \n", bValue);
        /* save update to backup */
        if ( bValue == TRUE )
        {
             CosaDmlMTATriggerDiagnostics(nIndex);
        }
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        LineTable_SetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int                         iValue
            );

    description:

        This function is called to set integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int                         iValue
                The updated integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
LineTable_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(ParamName);
    UNREFERENCED_PARAMETER(iValue);

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        LineTable_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
LineTable_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(ParamName);
    UNREFERENCED_PARAMETER(uValue);

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        LineTable_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
LineTable_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    /* check the parameter name and set the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(ParamName);
    UNREFERENCED_PARAMETER(pString);

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        LineTable_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation. 

                ULONG*                      puLength
                The output length of the param name. 

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
LineTable_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(pReturnParamName);
    UNREFERENCED_PARAMETER(puLength);

    return TRUE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        LineTable_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
LineTable_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        LineTable_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a 
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
LineTable_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    return 0;
}

/***********************************************************************

 APIs for Object:

    Device.X_CISCO_COM_MTA.LineTable.{i}.CALLP.

    *  CALLP_GetParamStringValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        CALLP_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
CALLP_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    COSA_MTA_CAPPL                  CALLP;
    memset(&CALLP, 0, sizeof(COSA_MTA_CAPPL));
    PCOSA_MTA_LINETABLE_INFO        pInfo = (PCOSA_MTA_LINETABLE_INFO)hInsContext;
    ULONG                           nIndex = pInfo->InstanceNumber - 1;
    errno_t                         rc     = -1;
    int                             ind    = -1; 
    CosaDmlMTALineTableGetEntry(NULL, nIndex, pInfo);
    
    if (CosaDmlMTAGetCALLP(hInsContext, &CALLP) != ANSC_STATUS_SUCCESS)
    {
        return -1;
    }
    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("LCState", strlen("LCState"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))    
    {       
        rc = strcpy_s(pValue, *pUlSize, CALLP.LCState);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        } 
        
        return 0;
    }

    rc = strcmp_s("CallPState", strlen("CallPState"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        rc = strcpy_s(pValue, *pUlSize, CALLP.CallPState);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("LoopCurrent", strlen("LoopCurrent"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        rc = strcpy_s(pValue, *pUlSize, CALLP.LoopCurrent);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }
    
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/***********************************************************************

 APIs for Object:

    Device.X_CISCO_COM_MTA.LineTable.{i}.VQM.

    *  VQM_GetParamBoolValue
    *  VQM_GetParamIntValue
    *  VQM_GetParamUlongValue
    *  VQM_GetParamStringValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        VQM_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
VQM_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    errno_t                         rc            = -1;
    int                             ind           = -1;

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("ResetStats", strlen("ResetStats"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *pBool = FALSE;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        VQM_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
VQM_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    errno_t                         rc            = -1;
    int                             ind           = -1;

    /* check the parameter name and set the corresponding value */
    rc = strcmp_s("ResetStats", strlen("ResetStats"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK)) 
    {
        /* save update to backup */
        if ( bValue == TRUE )
        {
            CosaDmlMTAVQMResetStats(NULL);
        }
        return TRUE;
    }


    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        VQM_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation. 

                ULONG*                      puLength
                The output length of the param name. 

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
VQM_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(pReturnParamName);
    UNREFERENCED_PARAMETER(puLength);

    return TRUE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        VQM_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
VQM_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        VQM_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a 
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
VQM_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    return 0;
}


/***********************************************************************

 APIs for Object:

    Device.X_CISCO_COM_MTA.LineTable.{i}.VQM.Calls.{i}.

    *  Calls_GetEntryCount
    *  Calls_GetEntry
    *  Calls_IsUpdated
    *  Calls_Synchronize
    *  Calls_GetParamUlongValue
    *  Calls_GetParamStringValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Calls_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
Calls_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PCOSA_MTA_LINETABLE_INFO        pMyObject = (PCOSA_MTA_LINETABLE_INFO)hInsContext;
    
    return pMyObject->CallsNumber;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        Calls_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
Calls_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
    PCOSA_MTA_LINETABLE_INFO        pMyObject = (PCOSA_MTA_LINETABLE_INFO)hInsContext;

    if (nIndex < pMyObject->CallsNumber)
    {
        *pInsNumber  = nIndex + 1;

        return &pMyObject->pCalls[nIndex];
    }

    return NULL; /* return the handle */
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Calls_IsUpdated
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is checking whether the table is updated or not.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     TRUE or FALSE.

**********************************************************************/
BOOL
Calls_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PCOSA_MTA_LINETABLE_INFO        pMyObject = (PCOSA_MTA_LINETABLE_INFO)hInsContext;

    if ( !pMyObject->CallsUpdateTime ) 
    {
        pMyObject->CallsUpdateTime = AnscGetTickInSeconds();

        return TRUE;
    }
    
    if ( pMyObject->CallsUpdateTime >= TIME_NO_NEGATIVE(AnscGetTickInSeconds() - MTA_REFRESH_INTERVAL) )
    {
        return FALSE;
    }
    else 
    {
        pMyObject->CallsUpdateTime = AnscGetTickInSeconds();

        return TRUE;
    }
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Calls_Synchronize
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to synchronize the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Calls_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PCOSA_MTA_LINETABLE_INFO        pMyObject = (PCOSA_MTA_LINETABLE_INFO)hInsContext;
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;

    if ( pMyObject->pCalls )
    {
        AnscFreeMemory(pMyObject->pCalls);
        pMyObject->pCalls = NULL;
    }
    
    pMyObject->CallsNumber = 0;
    
    returnStatus = CosaDmlMTAGetCalls
        (
            (ANSC_HANDLE)NULL, 
            pMyObject->InstanceNumber,
            &pMyObject->CallsNumber,
            &pMyObject->pCalls
        );

    if ( returnStatus != ANSC_STATUS_SUCCESS )
    {
        pMyObject->pCalls = NULL;
        pMyObject->CallsNumber = 0;
    }
    
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Calls_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Calls_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PCOSA_MTA_CALLS                 pCalls = (PCOSA_MTA_CALLS)hInsContext;
    errno_t                         rc            = -1;
    int                             ind           = -1;

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("JitterBufferAdaptive", strlen("JitterBufferAdaptive"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *pBool = pCalls->JitterBufferAdaptive;
        return TRUE;
    }

    rc = strcmp_s("Originator", strlen("Originator"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *pBool = pCalls->Originator;
        return TRUE;
    }

    rc = strcmp_s("RemoteJitterBufferAdaptive", strlen("RemoteJitterBufferAdaptive"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *pBool = pCalls->RemoteJitterBufferAdaptive;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Calls_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Calls_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    PCOSA_MTA_CALLS                 pCalls = (PCOSA_MTA_CALLS)hInsContext;
    errno_t                         rc            = -1;
    int                             ind           = -1;
    
    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("RemoteIPAddress", strlen("RemoteIPAddress"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pCalls->RemoteIPAddress.Value; 

        return TRUE;
    }

    rc = strcmp_s("CallDuration", strlen("CallDuration"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pCalls->CallDuration; 

        return TRUE;
    }

    

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Calls_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
Calls_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCOSA_MTA_CALLS                 pCalls = (PCOSA_MTA_CALLS)hInsContext;
    errno_t                         rc       = -1;
    int                             ind      = -1;

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("Codec", strlen("Codec"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->Codec);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteCodec", strlen("RemoteCodec"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteCodec);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("CallEndTime", strlen("CallEndTime"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->CallEndTime);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("CallStartTime", strlen("CallStartTime"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->CallStartTime);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("CWErrorRate", strlen("CWErrorRate"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->CWErrorRate);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("PktLossConcealment", strlen("PktLossConcealment"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->PktLossConcealment);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemotePktLossConcealment", strlen("RemotePktLossConcealment"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemotePktLossConcealment);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }
	/********************zqiu >>**********************************************/

    rc = strcmp_s("CWErrors", strlen("CWErrors"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->CWErrors);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("SNR", strlen("SNR"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->SNR);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
               
        return 0;
    }

    rc = strcmp_s("MicroReflections", strlen("MicroReflections"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->MicroReflections);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("DownstreamPower", strlen("DownstreamPower"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->DownstreamPower);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }
 
    rc = strcmp_s("UpstreamPower", strlen("UpstreamPower"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->UpstreamPower);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("EQIAverage", strlen("EQIAverage"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->EQIAverage);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;

    }

    rc = strcmp_s("EQIMinimum", strlen("EQIMinimum"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->EQIMinimum);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        } 
        
        return 0;
    }

    rc = strcmp_s("EQIMaximum", strlen("EQIMaximum"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->EQIMaximum);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }


    rc = strcmp_s("EQIInstantaneous", strlen("EQIInstantaneous"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->EQIInstantaneous);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("MOS-LQ", strlen("MOS-LQ"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->MOS_LQ);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("MOS-CQ", strlen("MOS-CQ"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->MOS_CQ);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("EchoReturnLoss", strlen("EchoReturnLoss"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->EchoReturnLoss);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("SignalLevel", strlen("SignalLevel"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->SignalLevel);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("NoiseLevel", strlen("NoiseLevel"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->NoiseLevel);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("LossRate", strlen("LossRate"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->LossRate);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("DiscardRate", strlen("DiscardRate"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->DiscardRate);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("BurstDensity", strlen("BurstDensity"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->BurstDensity);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("GapDensity", strlen("GapDensity"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->GapDensity);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("BurstDuration", strlen("BurstDuration"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->BurstDuration);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("GapDuration", strlen("GapDuration"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->GapDuration);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RoundTripDelay", strlen("RoundTripDelay"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RoundTripDelay);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("Gmin", strlen("Gmin"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->Gmin);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;

    }

    rc = strcmp_s("RFactor", strlen("RFactor"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RFactor);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("ExternalRFactor", strlen("ExternalRFactor"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->ExternalRFactor);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("JitterBufRate", strlen("JitterBufRate"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->JitterBufRate);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("JBNominalDelay", strlen("JBNominalDelay"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->JBNominalDelay);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("JBMaxDelay", strlen("JBMaxDelay"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->JBMaxDelay);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;

    }

    rc = strcmp_s("JBAbsMaxDelay", strlen("JBAbsMaxDelay"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */        
        rc = strcpy_s(pValue, *pUlSize, pCalls->JBAbsMaxDelay);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("TxPackets", strlen("TxPackets"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->TxPackets);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("TxOctets", strlen("TxOctets"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->TxOctets);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RxPackets", strlen("RxPackets"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RxPackets);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RxOctets", strlen("RxOctets"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RxOctets);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("PacketLoss", strlen("PacketLoss"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->PacketLoss);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("IntervalJitter", strlen("IntervalJitter"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->IntervalJitter);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteIntervalJitter", strlen("RemoteIntervalJitter"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteIntervalJitter);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteMOS-LQ", strlen("RemoteMOS-LQ"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteMOS_LQ);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteMOS-CQ", strlen("RemoteMOS-CQ"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteMOS_CQ);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteEchoReturnLoss", strlen("RemoteEchoReturnLoss"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteEchoReturnLoss);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteSignalLevel", strlen("RemoteSignalLevel"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteSignalLevel);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteNoiseLevel", strlen("RemoteNoiseLevel"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteNoiseLevel);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }


    rc = strcmp_s("RemoteLossRate", strlen("RemoteLossRate"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteLossRate);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteDiscardRate", strlen("RemoteDiscardRate"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteDiscardRate);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteBurstDensity", strlen("RemoteBurstDensity"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteBurstDensity);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteGapDensity", strlen("RemoteGapDensity"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */        
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteGapDensity);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }


    rc = strcmp_s("RemoteBurstDuration", strlen("RemoteBurstDuration"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteBurstDuration);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }


    rc = strcmp_s("RemoteGapDuration", strlen("RemoteGapDuration"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteGapDuration);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteRoundTripDelay", strlen("RemoteRoundTripDelay"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteRoundTripDelay);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteGmin", strlen("RemoteGmin"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteGmin);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteRFactor", strlen("RemoteRFactor"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteRFactor);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteExternalRFactor", strlen("RemoteExternalRFactor"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteExternalRFactor);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteJitterBufRate", strlen("RemoteJitterBufRate"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteJitterBufRate);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteJBNominalDelay", strlen("RemoteJBNominalDelay"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteJBNominalDelay);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteJBMaxDelay", strlen("RemoteJBMaxDelay"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteJBMaxDelay);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RemoteJBAbsMaxDelay", strlen("RemoteJBAbsMaxDelay"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pCalls->RemoteJBAbsMaxDelay);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }
	/********************zqiu <<**********************************************/
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/***********************************************************************

 APIs for Object:

    Device.X_CISCO_COM_MTA.ServiceClass.{i}.

    *  ServiceClass_GetEntryCount
    *  ServiceClass_GetEntry
    *  ServiceClass_IsUpdated
    *  ServiceClass_Synchronize
    *  ServiceClass_GetParamStringValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        ServiceClass_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
ServiceClass_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;
    
    return pMyObject->ServiceClassNumber;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        ServiceClass_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
ServiceClass_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;

    if (nIndex < pMyObject->ServiceClassNumber)
    {
        *pInsNumber  = nIndex + 1;

        return &pMyObject->pServiceClass[nIndex];
    }

    return NULL; /* return the handle */
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        ServiceClass_IsUpdated
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is checking whether the table is updated or not.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     TRUE or FALSE.

**********************************************************************/
BOOL
ServiceClass_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;

    if ( !pMyObject->ServiceClassUpdateTime ) 
    {
        pMyObject->ServiceClassUpdateTime = AnscGetTickInSeconds();

        return TRUE;
    }
    
    if ( pMyObject->ServiceClassUpdateTime >= TIME_NO_NEGATIVE(AnscGetTickInSeconds() - MTA_REFRESH_INTERVAL) )
    {
        return FALSE;
    }
    else 
    {
        pMyObject->ServiceClassUpdateTime = AnscGetTickInSeconds();

        return TRUE;
    }
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        ServiceClass_Synchronize
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to synchronize the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
ServiceClass_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject    = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;

    if ( pMyObject->pServiceClass )
    {
        AnscFreeMemory(pMyObject->pServiceClass);
        pMyObject->pServiceClass = NULL;
    }
    
    pMyObject->ServiceClassNumber = 0;
    
    returnStatus = CosaDmlMTAGetServiceClass
        (
            (ANSC_HANDLE)NULL, 
            &pMyObject->ServiceClassNumber,
            &pMyObject->pServiceClass
        );

    if ( returnStatus != ANSC_STATUS_SUCCESS )
    {
        pMyObject->pServiceClass = NULL;
        pMyObject->ServiceClassNumber = 0;
    }
    
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        ServiceClass_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
ServiceClass_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject     = (PCOSA_DATAMODEL_MTA )g_pCosaBEManager->hMTA;
    PCOSA_MTA_SERVICE_CLASS         pServiceClass = (PCOSA_MTA_SERVICE_CLASS)&pMyObject->pServiceClass;
    errno_t                         rc            = -1;
    int                             ind           = -1;

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("ServiceClassName", strlen("ServiceClassName"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pServiceClass->ServiceClassName);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/***********************************************************************

 APIs for Object:

    Device.X_CISCO_COM_MTA.ServiceFlow.{i}.

    *  ServiceFlow_GetEntryCount
    *  ServiceFlow_GetEntry
    *  ServiceFlow_IsUpdated
    *  ServiceFlow_Synchronize
    *  ServiceFlow_GetParamUlongValue
    *  ServiceFlow_GetParamStringValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        ServiceFlow_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
ServiceFlow_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;
    
    return pMyObject->ServiceFlowNumber;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        ServiceFlow_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
ServiceFlow_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;

    if (nIndex < pMyObject->ServiceFlowNumber)
    {
        *pInsNumber  = nIndex + 1;

        return &pMyObject->pServiceFlow[nIndex];
    }

    return NULL; /* return the handle */
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        ServiceFlow_IsUpdated
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is checking whether the table is updated or not.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     TRUE or FALSE.

**********************************************************************/
BOOL
ServiceFlow_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;

    if ( !pMyObject->ServiceFlowUpdateTime ) 
    {
        pMyObject->ServiceFlowUpdateTime = AnscGetTickInSeconds();

        return TRUE;
    }
    
    if ( pMyObject->ServiceFlowUpdateTime >= TIME_NO_NEGATIVE(AnscGetTickInSeconds() - MTA_REFRESH_INTERVAL) )
    {
        return FALSE;
    }
    else 
    {
        pMyObject->ServiceFlowUpdateTime = AnscGetTickInSeconds();

        return TRUE;
    }
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        ServiceFlow_Synchronize
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to synchronize the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
ServiceFlow_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject    = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;

    if ( pMyObject->pServiceFlow )
    {
        AnscFreeMemory(pMyObject->pServiceFlow);
        pMyObject->pServiceFlow = NULL;
    }
    
    pMyObject->ServiceFlowNumber = 0;
    
    returnStatus = CosaDmlMTAGetServiceFlow
        (
            (ANSC_HANDLE)NULL, 
            &pMyObject->ServiceFlowNumber,
            &pMyObject->pServiceFlow
        );

    if ( returnStatus != ANSC_STATUS_SUCCESS )
    {
        pMyObject->pServiceFlow = NULL;
        pMyObject->ServiceFlowNumber = 0;
    }
    
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        ServiceFlow_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
ServiceFlow_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PCOSA_MTA_SERVICE_FLOW          pFlow = (PCOSA_MTA_SERVICE_FLOW)hInsContext;
    errno_t                         rc            = -1;
    int                             ind           = -1;

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("DefaultFlow", strlen("DefaultFlow"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *pBool = pFlow->DefaultFlow;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        ServiceFlow_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
ServiceFlow_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCOSA_MTA_SERVICE_FLOW          pFlow = (PCOSA_MTA_SERVICE_FLOW)hInsContext;
    errno_t                         rc       = -1;
    int                             ind      = -1;

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("TrafficType", strlen("TrafficType"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pFlow->TrafficType);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }
    
    rc = strcmp_s("Direction", strlen("Direction"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pFlow->Direction);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("ServiceClassName", strlen("ServiceClassName"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pFlow->ServiceClassName);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        ServiceFlow_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
ServiceFlow_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    PCOSA_MTA_SERVICE_FLOW          pFlow = (PCOSA_MTA_SERVICE_FLOW)hInsContext;
    errno_t                         rc            = -1;
    int                             ind           = -1;
    
    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("SFID", strlen("SFID"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pFlow->SFID; 

        return TRUE;
    }

    rc = strcmp_s("ScheduleType", strlen("ScheduleType"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pFlow->ScheduleType; 

        return TRUE;
    }

rc = strcmp_s("NomGrantInterval", strlen("NomGrantInterval"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pFlow->NomGrantInterval; 

        return TRUE;
    }

    rc = strcmp_s("UnsolicitGrantSize", strlen("UnsolicitGrantSize"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pFlow->UnsolicitGrantSize; 

        return TRUE;
    }

    rc = strcmp_s("TolGrantJitter", strlen("TolGrantJitter"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pFlow->TolGrantJitter; 

        return TRUE;
    }

    rc = strcmp_s("NomPollInterval", strlen("NomPollInterval"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pFlow->NomPollInterval; 

        return TRUE;
    }

    rc = strcmp_s("MinReservedPkt", strlen("MinReservedPkt"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pFlow->MinReservedPkt; 

        return TRUE;
    }

    rc = strcmp_s("MaxTrafficRate", strlen("MaxTrafficRate"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pFlow->MaxTrafficRate; 

        return TRUE;
    }

    rc = strcmp_s("MinReservedRate", strlen("MinReservedRate"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pFlow->MinReservedRate; 

        return TRUE;
    }

    rc = strcmp_s("MaxTrafficBurst", strlen("MaxTrafficBurst"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pFlow->MaxTrafficBurst; 

        return TRUE;
    }

    rc = strcmp_s("NumberOfPackets", strlen("NumberOfPackets"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pFlow->NumberOfPackets; 

        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/***********************************************************************

 APIs for Object:

    Device.X_CISCO_COM_MTA.Dect.

    *  Dect_GetParamBoolValue
    *  Dect_GetParamIntValue
    *  Dect_GetParamUlongValue
    *  Dect_GetParamStringValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Dect_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Dect_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    errno_t                         rc            = -1;
    int                             ind           = -1;

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("Enable", strlen("Enable"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        CosaDmlMTADectGetEnable(hInsContext, pBool);
        return TRUE;
    }

    rc = strcmp_s("RegistrationMode", strlen("RegistrationMode"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        CosaDmlMTADectGetRegistrationMode(hInsContext, pBool);
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Dect_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Dect_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(ParamName);
    UNREFERENCED_PARAMETER(pInt);

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Dect_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Dect_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    errno_t                         rc            = -1;
    int                             ind           = -1;

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("RegisterDectHandset", strlen("RegisterDectHandset"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = 0;
        return TRUE;
    }

    rc = strcmp_s("DeregisterDectHandset", strlen("DeregisterDectHandset"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = 0;
        return TRUE;
    }
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Dect_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
Dect_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCOSA_DATAMODEL_MTA             pMyObject     = (PCOSA_DATAMODEL_MTA )g_pCosaBEManager->hMTA;
    PCOSA_MTA_DECT                  pDect         = (PCOSA_MTA_DECT      )&pMyObject->Dect;
    errno_t                         rc       = -1;
    int                             ind      = -1;

    if (CosaDmlMTAGetDect(hInsContext, pDect) != ANSC_STATUS_SUCCESS) 
    {
        return FALSE;
    }    
            
    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("HardwareVersion", strlen("HardwareVersion"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))    
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pDect->HardwareVersion);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("RFPI", strlen("RFPI"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pDect->RFPI);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("SoftwareVersion", strlen("SoftwareVersion"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */

        rc = strcpy_s(pValue, *pUlSize, pDect->SoftwareVersion);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("PIN", strlen("PIN"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        CosaDmlMTAGetDectPIN(hInsContext, pDect->PIN);
        rc = strcpy_s(pValue, *pUlSize, pDect->PIN);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

BOOL
Dect_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    errno_t                         rc            = -1;
    int                             ind           = -1;

    /* check the parameter name and set the corresponding value */
    rc = strcmp_s("Enable", strlen("Enable"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        CosaDmlMTADectSetEnable(hInsContext, bValue);
        return TRUE;
    }

    rc = strcmp_s("RegistrationMode", strlen("RegistrationMode"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        CosaDmlMTADectSetRegistrationMode(hInsContext, bValue);
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Dect_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Dect_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    PCOSA_DATAMODEL_MTA             pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;
    errno_t                         rc            = -1;
    int                             ind           = -1;

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("RegisterDectHandset", strlen("RegisterDectHandset"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        CosaDmlMTADectRegisterDectHandset(hInsContext, uValue);
        return TRUE;
    }

    rc = strcmp_s("DeregisterDectHandset", strlen("DeregisterDectHandset"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        if ( ANSC_STATUS_SUCCESS == CosaDmlMTADectDeregisterDectHandset(hInsContext, uValue) )
        {
            pMyObject->HandsetsUpdateTime = 0;
        }
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

BOOL
Dect_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    errno_t                         rc            = -1;
    int                             ind           = -1;

    /* check the parameter name and set the corresponding value */
    rc = strcmp_s("PIN", strlen("PIN"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        CosaDmlMTASetDectPIN(hInsContext, pString);
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Dect_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation. 

                ULONG*                      puLength
                The output length of the param name. 

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
Dect_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(pReturnParamName);
    UNREFERENCED_PARAMETER(puLength);

    return TRUE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Dect_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Dect_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Dect_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a 
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Dect_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    return 0;
}

/***********************************************************************

 APIs for Object:

    Device.X_CISCO_COM_MTA.Dect.Handsets.{i}

    *  Handsets_GetEntryCount
    *  Handsets_GetEntry
    *  Handsets_GetParamBoolValue
    *  Handsets_GetParamIntValue
    *  Handsets_GetParamUlongValue
    *  Handsets_GetParamStringValue
    *  Handsets_SetParamBoolValue
    *  Handsets_SetParamIntValue
    *  Handsets_SetParamUlongValue
    *  Handsets_SetParamStringValue
    *  Handsets_Validate
    *  Handsets_Commit
    *  Handsets_Rollback

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Handsets_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
Handsets_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject    = (PCOSA_DATAMODEL_MTA    )g_pCosaBEManager->hMTA;
    return pMyObject->HandsetsNumber;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        Handsets_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
Handsets_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject    = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;

    if (nIndex < pMyObject->HandsetsNumber)
    {
        *pInsNumber  = nIndex + 1;

        return &pMyObject->pHandsets[nIndex];
    }

    return NULL;

}

BOOL
Handsets_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;

    if ( !pMyObject->HandsetsUpdateTime ) 
    {
        pMyObject->HandsetsUpdateTime = AnscGetTickInSeconds();

        return TRUE;
    }
    
    if ( pMyObject->HandsetsUpdateTime >= TIME_NO_NEGATIVE(AnscGetTickInSeconds() - HANDSETS_REFRESH_INTERVAL) )
    {
        return FALSE;
    }
    else 
    {
        pMyObject->HandsetsUpdateTime = AnscGetTickInSeconds();

        return TRUE;
    }
}

ULONG
Handsets_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject    = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;

    if ( pMyObject->pHandsets )
    {
        AnscFreeMemory(pMyObject->pHandsets);
        pMyObject->pHandsets = NULL;
    }
    
    pMyObject->HandsetsNumber = 0;
    
    returnStatus = CosaDmlMTAGetHandsets
        (
            (ANSC_HANDLE)NULL, 
            &pMyObject->HandsetsNumber,
            &pMyObject->pHandsets
        );

    if ( returnStatus != ANSC_STATUS_SUCCESS )
    {
        pMyObject->pHandsets = NULL;
        pMyObject->HandsetsNumber = 0;
    }
    
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Handsets_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Handsets_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PCOSA_MTA_HANDSETS_INFO         pInfo = (PCOSA_MTA_HANDSETS_INFO)hInsContext;
    errno_t                         rc            = -1;
    int                             ind           = -1;

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("Status", strlen("Status"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    
    {
        /* collect value */
        *pBool = pInfo->Status;

        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Handsets_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Handsets_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(ParamName);
    UNREFERENCED_PARAMETER(pInt);

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Handsets_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Handsets_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    /* check the parameter name and return the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(ParamName);
    UNREFERENCED_PARAMETER(puLong);

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Handsets_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
Handsets_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCOSA_MTA_HANDSETS_INFO         pInfo = (PCOSA_MTA_HANDSETS_INFO)hInsContext;
    errno_t                         rc       = -1;
    int                             ind      = -1;

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("LastActiveTime", strlen("LastActiveTime"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pInfo->LastActiveTime);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("HandsetName", strlen("HandsetName"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pInfo->HandsetName);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("HandsetFirmware", strlen("HandsetFirmware"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pInfo->HandsetFirmware);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("OperatingTN", strlen("OperatingTN"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        // At present only TN1 will be assigned to DECT phones
        //AnscCopyString(pValue, pInfo->OperatingTN);
        rc = strcpy_s(pValue, *pUlSize, "TN1");
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    rc = strcmp_s("SupportedTN", strlen("SupportedTN"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pInfo->SupportedTN);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Handsets_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Handsets_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(ParamName);
    UNREFERENCED_PARAMETER(bValue);

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Handsets_SetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int                         iValue
            );

    description:

        This function is called to set integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int                         iValue
                The updated integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Handsets_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(ParamName);
    UNREFERENCED_PARAMETER(iValue);

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Handsets_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Handsets_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(ParamName);
    UNREFERENCED_PARAMETER(uValue);

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Handsets_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Handsets_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    PCOSA_MTA_HANDSETS_INFO         pInfo = (PCOSA_MTA_HANDSETS_INFO)hInsContext;
    errno_t                         rc       = -1;
    int                             ind      = -1;
    

    /* check the parameter name and set the corresponding value */
    rc = strcmp_s("OperatingTN", strlen("OperatingTN"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        rc = strcpy_s(pInfo->OperatingTN, sizeof(pInfo->OperatingTN), pString);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return  FALSE;
        } 
        
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Handsets_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation. 

                ULONG*                      puLength
                The output length of the param name. 

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
Handsets_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(pReturnParamName);
    UNREFERENCED_PARAMETER(puLength);

    return TRUE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Handsets_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Handsets_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PCOSA_MTA_HANDSETS_INFO         pInfo = (PCOSA_MTA_HANDSETS_INFO)hInsContext;

    CosaDmlMTASetHandsets(NULL, pInfo);

    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Handsets_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a 
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Handsets_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    return 0;
}

/***********************************************************************

 APIs for Object:

    Device.X_CISCO_COM_MTA.DSXLog.{i}.

    *  DSXLog_GetEntryCount
    *  DSXLog_GetEntry
    *  DSXLog_IsUpdated
    *  DSXLog_Synchronize
    *  DSXLog_GetParamStringValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        DSXLog_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
DSXLog_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;
    
    return pMyObject->DSXLogNumber;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        DSXLog_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
DSXLog_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;

    if (nIndex < pMyObject->DSXLogNumber)
    {
        *pInsNumber  = nIndex + 1;

        return &pMyObject->pDSXLog[nIndex];
    }

    return NULL; /* return the handle */
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        DSXLog_IsUpdated
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is checking whether the table is updated or not.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     TRUE or FALSE.

**********************************************************************/
BOOL
DSXLog_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;

    if ( !pMyObject->DSXLogUpdateTime ) 
    {
        pMyObject->DSXLogUpdateTime = AnscGetTickInSeconds();

        return TRUE;
    }
    
    if ( pMyObject->DSXLogUpdateTime >= TIME_NO_NEGATIVE(AnscGetTickInSeconds() - MTA_REFRESH_INTERVAL) )
    {
        return FALSE;
    }
    else 
    {
        pMyObject->DSXLogUpdateTime = AnscGetTickInSeconds();

        return TRUE;
    }
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        DSXLog_Synchronize
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to synchronize the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
DSXLog_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject    = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;

    if ( pMyObject->pDSXLog )
    {
        AnscFreeMemory(pMyObject->pDSXLog);
        pMyObject->pDSXLog = NULL;
    }
    
    pMyObject->DSXLogNumber = 0;
    
    returnStatus = CosaDmlMTAGetDSXLogs
        (
            (ANSC_HANDLE)NULL, 
            &pMyObject->DSXLogNumber,
            &pMyObject->pDSXLog
        );

    if ( returnStatus != ANSC_STATUS_SUCCESS )
    {
        pMyObject->pDSXLog = NULL;
        pMyObject->DSXLogNumber = 0;
    }
    
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        DSXLog_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
DSXLog_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    //PCOSA_DATAMODEL_MTA             pMyObject = (PCOSA_DATAMODEL_MTA )g_pCosaBEManager->hMTA;
    //PCOSA_MTA_DSXLOG                pDSXLog   = (PCOSA_MTA_DSXLOG)&pMyObject->pDSXLog;
    PCOSA_MTA_DSXLOG                pDSXLog    = (PCOSA_MTA_DSXLOG)hInsContext;
    errno_t                         rc       = -1;
    int                             ind      = -1;

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("Time", strlen("Time"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pDSXLog->Time);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }
    rc = strcmp_s("Description", strlen("Description"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        rc = strcpy_s(pValue, *pUlSize, pDSXLog->Description);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        return 0;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        DSXLog_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
DSXLog_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    errno_t                         rc            = -1;
    int                             ind           = -1;

    //PCOSA_DATAMODEL_MTA             pMyObject = (PCOSA_DATAMODEL_MTA )g_pCosaBEManager->hMTA;
    //PCOSA_MTA_DSXLOG                pDSXLog   = (PCOSA_MTA_DSXLOG)&pMyObject->pDSXLog;
    PCOSA_MTA_DSXLOG                pDSXLog    = (PCOSA_MTA_DSXLOG)hInsContext;
    
    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("ID", strlen("ID"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pDSXLog->ID; 

        return TRUE;
    }

    rc = strcmp_s("Level", strlen("Level"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pDSXLog->Level; 

        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}


/***********************************************************************

 APIs for Object:

    X_CISCO_COM_MLD.Group.{i}.

    *  MTALog_GetEntryCount
    *  MTALog_GetEntry
    *  MTALog_IsUpdated
    *  MTALog_Synchronize
    *  MTALog_GetParamUlongValue
    *  MTALog_GetParamStringValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        MTALog_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
MTALog_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA      pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;
    
    return pMyObject->MtaLogNumber;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        MTALog_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
MTALog_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA      pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;

    if (nIndex < pMyObject->MtaLogNumber)
    {
        *pInsNumber  = nIndex + 1;

        return &pMyObject->pMtaLog[nIndex];
    }

    return NULL; /* return the handle */
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        MTALog_IsUpdated
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is checking whether the table is updated or not.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     TRUE or FALSE.

**********************************************************************/
BOOL
MTALog_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA      pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;

    if ( !pMyObject->MtaLogUpdateTime ) 
    {
        pMyObject->MtaLogUpdateTime = AnscGetTickInSeconds();

        return TRUE;
    }
    
    if ( pMyObject->MtaLogUpdateTime >= TIME_NO_NEGATIVE(AnscGetTickInSeconds() - MTA_REFRESH_INTERVAL) )
    {
        return FALSE;
    }
    else 
    {
        pMyObject->MtaLogUpdateTime = AnscGetTickInSeconds();

        return TRUE;
    }
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        MTALog_Synchronize
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to synchronize the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
MTALog_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA      pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;
    ANSC_STATUS                     ret       = ANSC_STATUS_SUCCESS;
    ULONG                           i         = 0;

    if ( pMyObject->pMtaLog )
    {
        for(i=0; i<pMyObject->MtaLogNumber; i++)
        {
            if ( pMyObject->pMtaLog[i].pDescription)
            {
                AnscFreeMemory(pMyObject->pMtaLog[i].pDescription);
                pMyObject->pMtaLog[i].pDescription = NULL;
            }
        }

        AnscFreeMemory(pMyObject->pMtaLog);
        pMyObject->pMtaLog = NULL;
    }
    
    pMyObject->MtaLogNumber = 0;
    
    ret = CosaDmlMtaGetMtaLog
        (
            (ANSC_HANDLE)NULL, 
            &pMyObject->MtaLogNumber,
            &pMyObject->pMtaLog
        );

    if ( ret != ANSC_STATUS_SUCCESS )
    {
        pMyObject->pMtaLog = NULL;
        pMyObject->MtaLogNumber = 0;
    }
    
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        MTALog_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
MTALog_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    PCOSA_DML_MTALOG_FULL           pConf        = (PCOSA_DML_MTALOG_FULL)hInsContext;
    errno_t                         rc            = -1;
    int                             ind           = -1;
    
    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("Index", strlen("Index"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pConf->Index; 

        return TRUE;
    }

    rc = strcmp_s("EventID", strlen("EventID"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pConf->EventID; 

        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        MTALog_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 4095 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
MTALog_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCOSA_DML_MTALOG_FULL           pConf        = (PCOSA_DML_MTALOG_FULL)hInsContext;
    errno_t                         rc           = -1;
    int                             ind          = -1;

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("Description", strlen("Description"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))    
    {
    
        /* collect value */
        if ( _ansc_strlen(pConf->pDescription) > *pUlSize )
        {
            *pUlSize = _ansc_strlen(pConf->pDescription);
            return 1;
        }
        
        AnscCopyString(pValue, pConf->pDescription);
        return 0;
    }

    rc = strcmp_s("Time", strlen("Time"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        if ( _ansc_strlen(pConf->Time) >= *pUlSize )
        {
            *pUlSize = _ansc_strlen(pConf->Time);
            return 1;
        }
        
        AnscCopyString(pValue, pConf->Time);
        return 0;
    }
    if (strcmp(ParamName, "EventLevel") == 0)
    {
        /* collect value */
        if ( _ansc_strlen(pConf->EventLevel) >= *pUlSize )
        {
            *pUlSize = _ansc_strlen(pConf->EventLevel);
            return 1;
        }
        
        AnscCopyString(pValue, pConf->EventLevel);
        return 0;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}


/***********************************************************************

 APIs for Object:

    X_CISCO_COM_MLD.Group.{i}.

    *  DECTLog_GetEntryCount
    *  DECTLog_GetEntry
    *  DECTLog_IsUpdated
    *  DECTLog_Synchronize
    *  DECTLog_GetParamUlongValue
    *  DECTLog_GetParamStringValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        DECTLog_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
DECTLog_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA      pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;
    
    return pMyObject->DectLogNumber;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        DECTLog_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
DECTLog_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA      pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;

    if (nIndex < pMyObject->DectLogNumber)
    {
        *pInsNumber  = nIndex + 1;

        return &pMyObject->pDectLog[nIndex];
    }

    return NULL; /* return the handle */
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        DECTLog_IsUpdated
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is checking whether the table is updated or not.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     TRUE or FALSE.

**********************************************************************/
BOOL
DECTLog_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA      pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;

    if ( !pMyObject->DectLogUpdateTime ) 
    {
        pMyObject->DectLogUpdateTime = AnscGetTickInSeconds();

        return TRUE;
    }
    
    if ( pMyObject->DectLogUpdateTime >= TIME_NO_NEGATIVE(AnscGetTickInSeconds() - MTA_REFRESH_INTERVAL) )
    {
        return FALSE;
    }
    else 
    {
        pMyObject->DectLogUpdateTime = AnscGetTickInSeconds();

        return TRUE;
    }
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        DECTLog_Synchronize
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to synchronize the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
DECTLog_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA      pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;
    ANSC_STATUS                     ret           = ANSC_STATUS_SUCCESS;

    if ( pMyObject->pDectLog )
    {
        AnscFreeMemory(pMyObject->pDectLog);
        pMyObject->pDectLog = NULL;
    }
    
    pMyObject->DectLogNumber = 0;
    
    ret = CosaDmlMtaGetDectLog
        (
            (ANSC_HANDLE)NULL, 
            &pMyObject->DectLogNumber,
            &pMyObject->pDectLog
        );

    if ( ret != ANSC_STATUS_SUCCESS )
    {
        pMyObject->pDectLog = NULL;
        pMyObject->DectLogNumber = 0;
    }
    
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        DECTLog_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
DECTLog_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    PCOSA_DML_DECTLOG_FULL          pConf        = (PCOSA_DML_DECTLOG_FULL)hInsContext;
    errno_t                         rc            = -1;
    int                             ind           = -1;    
    
    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("Index", strlen("Index"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pConf->Index; 

        return TRUE;
    }

    rc = strcmp_s("EventID", strlen("EventID"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        *puLong = pConf->EventID; 

        return TRUE;
    }

    if (strcmp(ParamName, "EventLevel") == 0)
    {
        /* collect value */
        *puLong = pConf->EventLevel; 

        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        DECTLog_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 4095 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
DECTLog_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCOSA_DML_DECTLOG_FULL          pConf        = (PCOSA_DML_DECTLOG_FULL)hInsContext;
    errno_t                         rc           = -1;
    int                             ind          = -1;
    /* check the parameter name and return the corresponding value */    
    rc = strcmp_s("Description", strlen("Description"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK)) 
    {
        /* collect value */
        if ( _ansc_strlen(pConf->Description) >= *pUlSize )
        {
            *pUlSize = _ansc_strlen(pConf->Description);
            return 1;
        }
        
        AnscCopyString(pValue, pConf->Description);
        return 0;
    }

    rc = strcmp_s("Time", strlen("Time"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        if ( _ansc_strlen(pConf->Time) >= *pUlSize )
        {
            *pUlSize = _ansc_strlen(pConf->Time);
            return 1;
        }
        
        AnscCopyString(pValue, pConf->Time);
        return 0;
    }


    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}


// #ifdef CONFIG_TI_BBU

/***********************************************************************

 APIs for Object:

    Battery.

    *  Battery_GetParamBoolValue
    *  Battery_GetParamIntValue
    *  Battery_GetParamUlongValue
    *  Battery_GetParamStringValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Battery_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Battery_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    errno_t                         rc            = -1;
    int                             ind           = -1;

    // PCOSA_DATAMODEL_MTA     pMyObject = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;
    // PCOSA_DML_CM_LOG               pCfg      = &pMyObject->CmLog;

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("Installed", strlen("Installed"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        if (CosaDmlMtaBatteryGetInstalled(NULL, pBool) != ANSC_STATUS_SUCCESS)
            return FALSE;

        return TRUE;
    }    

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Battery_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Battery_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(ParamName);
    UNREFERENCED_PARAMETER(pInt);

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Battery_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Battery_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    errno_t                         rc            = -1;
    int                             ind           = -1;

    
    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("TotalCapacity", strlen("TotalCapacity"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
       /* collect value */
        if (CosaDmlMtaBatteryGetTotalCapacity(NULL, puLong) != ANSC_STATUS_SUCCESS)
            return FALSE; 

        return TRUE;
    }

    rc = strcmp_s("ActualCapacity", strlen("ActualCapacity"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        if (CosaDmlMtaBatteryGetActualCapacity(NULL, puLong) != ANSC_STATUS_SUCCESS)
            return FALSE;

        return TRUE;
    }

    rc = strcmp_s("RemainingCharge", strlen("RemainingCharge"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        if (CosaDmlMtaBatteryGetRemainingCharge(NULL, puLong) != ANSC_STATUS_SUCCESS)
            return FALSE;

        return TRUE;
    }

    rc = strcmp_s("RemainingTime", strlen("RemainingTime"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        if (CosaDmlMtaBatteryGetRemainingTime(NULL, puLong) != ANSC_STATUS_SUCCESS)
            return FALSE;

        return TRUE;
    }

    rc = strcmp_s("NumberofCycles", strlen("NumberofCycles"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        if (CosaDmlMtaBatteryGetNumberofCycles(NULL, puLong) != ANSC_STATUS_SUCCESS)
            return FALSE;

        return TRUE;
    }


    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Battery_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
Battery_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA      pMTA   = (PCOSA_DATAMODEL_MTA)g_pCosaBEManager->hMTA;
    PCOSA_DML_BATTERY_INFO          pInfo = (PCOSA_DML_BATTERY_INFO)&pMTA->MtaBatteryInfo;
    errno_t                         rc    = -1;
    int                             ind   = -1;        

    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("PowerStatus", strlen("PowerStatus"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        if (CosaDmlMtaBatteryGetPowerStatus(NULL, pValue, pUlSize) != ANSC_STATUS_SUCCESS)
            return -1;

        return 0;
    }

    rc = strcmp_s("Condition", strlen("Condition"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        if (CosaDmlMtaBatteryGetCondition(NULL, pValue, pUlSize) != ANSC_STATUS_SUCCESS)
            return -1;

        return 0;
    }

    rc = strcmp_s("Status", strlen("Status"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        if (CosaDmlMtaBatteryGetStatus(NULL, pValue, pUlSize) != ANSC_STATUS_SUCCESS)
            return -1;

        return 0;
    }

    rc = strcmp_s("Life", strlen("Life"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
        if (CosaDmlMtaBatteryGetLife(NULL, pValue, pUlSize) != ANSC_STATUS_SUCCESS)
            return -1;

        return 0;
    }

     if (CosaDmlMtaBatteryGetInfo(NULL, pInfo) != ANSC_STATUS_SUCCESS)
     {
         return -1;
     }
     
    rc = strcmp_s("ModelNumber", strlen("ModelNumber"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        rc = strcpy_s(pValue, *pUlSize, pInfo->ModelNumber);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        *pUlSize = _ansc_strlen(pValue);

        return 0;
    }

    rc = strcmp_s("SerialNumber", strlen("SerialNumber"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {            
        rc = strcpy_s(pValue, *pUlSize, pInfo->SerialNumber);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        
        *pUlSize = _ansc_strlen(pValue);

        return 0;
    }

    rc = strcmp_s("PartNumber", strlen("PartNumber"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        rc = strcpy_s(pValue, *pUlSize, pInfo->PartNumber);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        

        *pUlSize = _ansc_strlen(pValue);

        return 0;
    }

    rc = strcmp_s("ChargerFirmwareRevision", strlen("ChargerFirmwareRevision"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {        
        rc = strcpy_s(pValue, *pUlSize, pInfo->ChargerFirmwareRevision);
        if (rc != EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        

        *pUlSize = _ansc_strlen(pValue);

        return 0;
    }

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

// #endif /* CONFIG_TI_BBU */

// #endif /* CONFIG_TI_PACM */

/***********************************************************************

 APIs for Object:

    X_RDKCENTRAL_COM_MTA.

    *  X_RDKCENTRAL_COM_MTA_GetParamUlongValue
    *  X_RDKCENTRAL_COM_MTA_GetParamStringValue

***********************************************************************/

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        X_RDKCENTRAL_COM_MTA_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
X_RDKCENTRAL_COM_MTA_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
        UNREFERENCED_PARAMETER(hInsContext);
        errno_t                         rc            = -1;
        int                             ind           = -1;

	/* check the parameter name and return the corresponding value */
        rc = strcmp_s("Ipv4DhcpStatus", strlen("Ipv4DhcpStatus"), ParamName, &ind );
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
	{
		ULONG Ipv4DhcpStatus = 0, 
		      Ipv6DhcpStatus = 0;
		
        CosaDmlMtaGetDhcpStatus( &Ipv4DhcpStatus, &Ipv6DhcpStatus );
		*puLong = Ipv4DhcpStatus;
		return TRUE;
	}

        rc = strcmp_s("Ipv6DhcpStatus", strlen("Ipv6DhcpStatus"), ParamName, &ind );
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
	{
		ULONG Ipv4DhcpStatus = 0, 
		      Ipv6DhcpStatus = 0;
		
        CosaDmlMtaGetDhcpStatus( &Ipv4DhcpStatus, &Ipv6DhcpStatus );
		*puLong = Ipv6DhcpStatus;
		return TRUE;
	}

        rc = strcmp_s("ConfigFileStatus", strlen("ConfigFileStatus"), ParamName, &ind );
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
	{
        CosaDmlMtaGetConfigFileStatus( puLong );
		return TRUE;
	}

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        X_RDKCENTRAL_COM_MTA_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
X_RDKCENTRAL_COM_MTA_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
       UNREFERENCED_PARAMETER(hInsContext);
       errno_t                         rc            = -1;
       int                             ind           = -1;

	/* check the parameter name and return the corresponding value */
    rc = strcmp_s("LineRegisterStatus", strlen("LineRegisterStatus"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
	    /* CID 91923 Calling risky function fix */
	    CHAR acLineRegisterStatus[ MAX_LINE_REG ] = { 0 };

        /* collect value */

		CosaDmlMtaGetLineRegisterStatus( acLineRegisterStatus );
        if ( AnscSizeOfString( acLineRegisterStatus ) < *pUlSize)
        {
            char* pAcLineRegisterStatus = acLineRegisterStatus;
            rc = strcpy_s(pValue, *pUlSize, pAcLineRegisterStatus);
            if (rc != EOK)
            {
                ERR_CHK(rc);
                return -1;
            }
            
	    return 0;
        }
        else
        {
            *pUlSize = AnscSizeOfString( acLineRegisterStatus )+1;
            return 1;
        }
    }

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

BOOL VoiceService_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
{
    UNREFERENCED_PARAMETER(hInsContext);
    BOOL ret = FALSE;
#ifdef MTA_TR104SUPPORT

        char value[8] = {'\0'};
        if((syscfg_get(NULL,"TR104enable", value, sizeof(value)) == 0) && (strcmp(value, "true") == 0))
        {
            if (strcmp(ParamName, "Data") == 0)
            {
                if ( 0 == CosaDmlTR104DataSet(pString,0) )
                {
                    CcspTraceInfo(("%s Success in parsing web config blob.. with pString=%s\n",__FUNCTION__,pString));
                    ret = TRUE;
                }
                else
                {
                    CcspTraceError(("%s Failed to parse webconfig blob..\n",__FUNCTION__));
                }
            }
        }
        else
        {
            CcspTraceWarning(("Skipping webconfig parsing as RFC for TR104 is false\n"));
        }

#else
    UNREFERENCED_PARAMETER(ParamName);
    UNREFERENCED_PARAMETER(pString);
#endif
    return ret;
}

ULONG VoiceService_GetParamStringValue ( ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize )
{
    UNREFERENCED_PARAMETER(hInsContext);

    if (strcmp(ParamName, "Data") == 0)
    {
        snprintf(pValue, *pUlSize, "%s", "");
        return 0;
    }

    return -1;
}

#define BS_SOURCE_WEBPA_STR "webpa"
#define BS_SOURCE_RFC_STR "rfc"
#define  PARTNER_ID_LEN  64

char * getRequestorString()
{
   switch(g_currentWriteEntity)
   {
      case 0x0A: //CCSP_COMPONENT_ID_WebPA from webpa_internal.h(parodus2ccsp)
      case 0x0B: //CCSP_COMPONENT_ID_XPC
         return BS_SOURCE_WEBPA_STR;

      case 0x08: //DSLH_MPA_ACCESS_CONTROL_CLI
      case 0x10: //DSLH_MPA_ACCESS_CONTROL_CLIENTTOOL
         return BS_SOURCE_RFC_STR;

      default:
         return "unknown";
   }
}

char * getTime()
{
    time_t timer;
    static char buffer[50];
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 50, "%Y-%m-%d %H:%M:%S ", tm_info);
    return buffer;
}

/********************
X_RDKCENTRAL-COM_EthernetWAN_MTA
***************/
BOOL EthernetWAN_MTA_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         pInt
    )
{
        UNREFERENCED_PARAMETER(hInsContext);
        errno_t                         rc            = -1;
        int                             ind           = -1;

        PCOSA_DATAMODEL_MTA             pMyObject     = (PCOSA_DATAMODEL_MTA )g_pCosaBEManager->hMTA;
        rc = strcmp_s("StartupIPMode", strlen("StartupIPMode"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
        {
           char isEthEnabled[64]={'\0'};
	   char buff[8] = {'\0'};
           char * requestorStr = getRequestorString();
           char * currentTime = getTime();

           IS_UPDATE_ALLOWED_IN_DM(ParamName, requestorStr);
           IS_UPDATE_ALLOWED_IN_JSON(ParamName, requestorStr, pMyObject->pmtaprovinfo->StartupIPMode.UpdateSource);

          if( (0 == syscfg_get( NULL, "eth_wan_enabled", isEthEnabled, sizeof(isEthEnabled))) &&
                      ((isEthEnabled[0] != '\0') && (strncmp(isEthEnabled, "true", strlen("true")) == 0)))
            {
		       snprintf(buff,sizeof(buff),"%d",pInt);
               if (syscfg_set_commit(NULL, "StartupIPMode", buff) != 0) {
                   AnscTraceWarning(("syscfg_set failed\n"));
               }
               CosaDmlMTASetStartUpIpMode(pMyObject->pmtaprovinfo, pInt);
               
               rc = strcpy_s(pMyObject->pmtaprovinfo->StartupIPMode.UpdateSource,sizeof( pMyObject->pmtaprovinfo->StartupIPMode.UpdateSource ), requestorStr);
               if (rc != EOK)
               {
                   ERR_CHK(rc);
                   return  FALSE;
               }
               

               char PartnerID[PARTNER_ID_LEN] = {0};
               if((CCSP_SUCCESS == getPartnerId(PartnerID) ) && (PartnerID[ 0 ] != '\0') )
                   UpdateJsonParam("Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.StartupIPMode",PartnerID, buff, requestorStr, currentTime);

               // set startup ip mode
               CosaSetMTAHal(pMyObject->pmtaprovinfo);
               return TRUE;
            } 
        }
    return FALSE;
}

BOOL
EthernetWAN_MTA_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    errno_t                         rc            = -1;
    int                             ind           = -1;

    /* check the parameter name and return the corresponding value */
    PCOSA_DATAMODEL_MTA             pMyObject     = (PCOSA_DATAMODEL_MTA )g_pCosaBEManager->hMTA;
     if(pMyObject == NULL)
    {
        AnscTraceWarning(("Unsupported parameter - MTA Object not available"));
        return FALSE;
    }

    if(pMyObject->pmtaprovinfo == NULL)
    {
        AnscTraceWarning(("EthernetWAN_MTA_GetParamIntValue: MTA NOT provisioned - Unable to retrieve %s\n",ParamName ));
        return FALSE;
    }

    rc = strcmp_s("StartupIPMode", strlen("StartupIPMode"), ParamName, &ind );
    ERR_CHK(rc);
    if((!ind) && (rc == EOK))
    {
        /* collect value */
            char isEthEnabled[64]={'\0'};

          if( (0 == syscfg_get( NULL, "eth_wan_enabled", isEthEnabled, sizeof(isEthEnabled))) &&
                      ((isEthEnabled[0] != '\0') && (strncmp(isEthEnabled, "true", strlen("true")) == 0)))
            {
               *pInt = pMyObject->pmtaprovinfo->StartupIPMode.ActiveValue;
                return TRUE;
            } else {
                AnscTraceWarning(("Eth_wan not enabled : Invalid request\n"));
                return FALSE;
             }
    }
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

ULONG
EthernetWAN_MTA_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
  /* check the parameter name and return the corresponding value */
    UNREFERENCED_PARAMETER(hInsContext);
    PCOSA_DATAMODEL_MTA             pMyObject         = (PCOSA_DATAMODEL_MTA )g_pCosaBEManager->hMTA;
    char                            isEthEnabled[64]  ={'\0'};
    errno_t                         rc                = -1;
    int                             ind               = -1;

    if(pMyObject->pmtaprovinfo == NULL)
    {
        AnscTraceWarning(("EthernetWAN_MTA_GetParamStringValue: MTA NOT provisioned - Unable to retrieve %s\n",ParamName ));
        return -1;
    }

    if( (0 == syscfg_get( NULL, "eth_wan_enabled", isEthEnabled, sizeof(isEthEnabled))) &&
                      ((isEthEnabled[0] != '\0') && (strncmp(isEthEnabled, "true", strlen("true")) == 0)))   
    {         
            rc = strcmp_s("IPv4PrimaryDhcpServerOptions", strlen("IPv4PrimaryDhcpServerOptions"), ParamName, &ind );
            ERR_CHK(rc);
            if((!ind) && (rc == EOK))
            {
                rc = strcpy_s(pValue, *pUlSize, pMyObject->pmtaprovinfo->IPv4PrimaryDhcpServerOptions.ActiveValue);
                if (rc != EOK)
                {
                    ERR_CHK(rc);
                    return -1;
                }
                
                return 0;
            }

            rc = strcmp_s("IPv4SecondaryDhcpServerOptions", strlen("IPv4SecondaryDhcpServerOptions"), ParamName, &ind );
            ERR_CHK(rc);
            if((!ind) && (rc == EOK))
            {
                rc = strcpy_s(pValue, *pUlSize, pMyObject->pmtaprovinfo->IPv4SecondaryDhcpServerOptions.ActiveValue);
                if (rc != EOK)
                {
                    ERR_CHK(rc);
                    return -1;
                }
                
                return 0;
            }

            rc = strcmp_s("IPv6PrimaryDhcpServerOptions", strlen("IPv6PrimaryDhcpServerOptions"), ParamName, &ind );
            ERR_CHK(rc);
            if((!ind) && (rc == EOK))
            {
                rc = strcpy_s(pValue, *pUlSize, pMyObject->pmtaprovinfo->IPv6PrimaryDhcpServerOptions.ActiveValue);
                if (rc != EOK)
                {
                    ERR_CHK(rc);
                    return -1;
                }
                
                return 0;
            }

            rc = strcmp_s("IPv6SecondaryDhcpServerOptions", strlen("IPv6SecondaryDhcpServerOptions"), ParamName, &ind );
            ERR_CHK(rc);
            if((!ind) && (rc == EOK))
            {
               rc = strcpy_s(pValue, *pUlSize, pMyObject->pmtaprovinfo->IPv6SecondaryDhcpServerOptions.ActiveValue);
                if (rc != EOK)
                {
                    ERR_CHK(rc);
                    return -1;
                }
                
                return 0;
            }
 
   } else {
                AnscTraceWarning(("Eth_wan not enabled : Invalid request\n"));
                return -1;
    }
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}


BOOL
EthernetWAN_MTA_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
          UNREFERENCED_PARAMETER(hInsContext);
          MTA_IP_TYPE_TR ip_type;
          PCOSA_DATAMODEL_MTA             pMyObject     = (PCOSA_DATAMODEL_MTA )g_pCosaBEManager->hMTA;
          char isEthEnabled[64]={'\0'};
          char * requestorStr = getRequestorString();
          char * currentTime = getTime();
          errno_t rc         = -1;
          int     ind        = -1;

          IS_UPDATE_ALLOWED_IN_DM(ParamName, requestorStr);

          if( (0 == syscfg_get( NULL, "eth_wan_enabled", isEthEnabled, sizeof(isEthEnabled))) &&
                      ((isEthEnabled[0] != '\0') && (strncmp(isEthEnabled, "true", strlen("true")) == 0)))
            {
               ip_type = MTA_IPV4_TR;
               rc = strcmp_s("IPv4PrimaryDhcpServerOptions", strlen("IPv4PrimaryDhcpServerOptions"), ParamName, &ind );
               ERR_CHK(rc);
              if((!ind) && (rc == EOK))
                  {
                      IS_UPDATE_ALLOWED_IN_JSON(ParamName, requestorStr, pMyObject->pmtaprovinfo->IPv4PrimaryDhcpServerOptions.UpdateSource);

                      if (syscfg_set_commit(NULL, "IPv4PrimaryDhcpServerOptions", pString) != 0)
                          {
                              AnscTraceWarning(("syscfg_set failed\n"));
                          }
                      CosaDmlMTASetPrimaryDhcpServerOptions(pMyObject->pmtaprovinfo, pString, ip_type);

                      rc = strcpy_s( pMyObject->pmtaprovinfo->IPv4PrimaryDhcpServerOptions.UpdateSource,sizeof( pMyObject->pmtaprovinfo->IPv4PrimaryDhcpServerOptions.UpdateSource ) , requestorStr);
                      if (rc != EOK)
                      {
                          ERR_CHK(rc);
                          return FALSE;
                      } 
                      

                      char PartnerID[PARTNER_ID_LEN] = {0};
                      if((CCSP_SUCCESS == getPartnerId(PartnerID) ) && (PartnerID[ 0 ] != '\0') )
                          UpdateJsonParam("Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv4PrimaryDhcpServerOptions",PartnerID, pString, requestorStr, currentTime);

                      return TRUE;
                  }

              rc = strcmp_s("IPv4SecondaryDhcpServerOptions", strlen("IPv4SecondaryDhcpServerOptions"), ParamName, &ind );
              ERR_CHK(rc);
              if((!ind) && (rc == EOK))   
                  {
                      IS_UPDATE_ALLOWED_IN_JSON(ParamName, requestorStr, pMyObject->pmtaprovinfo->IPv4SecondaryDhcpServerOptions.UpdateSource);

                      if (syscfg_set_commit(NULL, "IPv4SecondaryDhcpServerOptions", pString) != 0)
                          {
                              AnscTraceWarning(("syscfg_set failed\n"));
                          }
                      CosaDmlMTASetSecondaryDhcpServerOptions(pMyObject->pmtaprovinfo, pString, ip_type);

                      rc = strcpy_s(pMyObject->pmtaprovinfo->IPv4SecondaryDhcpServerOptions.UpdateSource, sizeof( pMyObject->pmtaprovinfo->IPv4SecondaryDhcpServerOptions.UpdateSource ), requestorStr );
                      if (rc != EOK)
                      {
                          ERR_CHK(rc);
                          return FALSE;
                      }
                      
                      char PartnerID[PARTNER_ID_LEN] = {0};
                      if((CCSP_SUCCESS == getPartnerId(PartnerID) ) && (PartnerID[ 0 ] != '\0') )
                          UpdateJsonParam("Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv4SecondaryDhcpServerOptions",PartnerID, pString, requestorStr, currentTime);

                      return TRUE;
                  }

              ip_type = MTA_IPV6_TR;

              rc = strcmp_s("IPv6PrimaryDhcpServerOptions", strlen("IPv6PrimaryDhcpServerOptions"), ParamName, &ind );
              ERR_CHK(rc);
              if((!ind) && (rc == EOK))
                  {
                      IS_UPDATE_ALLOWED_IN_JSON(ParamName, requestorStr, pMyObject->pmtaprovinfo->IPv6PrimaryDhcpServerOptions.UpdateSource);

                      if (syscfg_set_commit(NULL, "IPv6PrimaryDhcpServerOptions", pString) != 0)
                          {
                              AnscTraceWarning(("syscfg_set failed\n"));
                          }
                      CosaDmlMTASetPrimaryDhcpServerOptions(pMyObject->pmtaprovinfo, pString, ip_type);

                      rc = strcpy_s(pMyObject->pmtaprovinfo->IPv6PrimaryDhcpServerOptions.UpdateSource, sizeof( pMyObject->pmtaprovinfo->IPv6PrimaryDhcpServerOptions.UpdateSource ), requestorStr );
                      if (rc != EOK)
                      {
                          ERR_CHK(rc);
                          return FALSE;
                      }
                      
                      char PartnerID[PARTNER_ID_LEN] = {0};
                      if((CCSP_SUCCESS == getPartnerId(PartnerID) ) && (PartnerID[ 0 ] != '\0') )
                          UpdateJsonParam("Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv6PrimaryDhcpServerOptions",PartnerID, pString, requestorStr, currentTime);

                      return TRUE;
                  }

               rc = strcmp_s("IPv6SecondaryDhcpServerOptions", strlen("IPv6SecondaryDhcpServerOptions"), ParamName, &ind );
               ERR_CHK(rc);
               if((!ind) && (rc == EOK))
                  {
                      IS_UPDATE_ALLOWED_IN_JSON(ParamName, requestorStr, pMyObject->pmtaprovinfo->IPv6SecondaryDhcpServerOptions.UpdateSource);

                      if (syscfg_set_commit(NULL, "IPv6SecondaryDhcpServerOptions", pString) != 0)
                          {
                              AnscTraceWarning(("syscfg_set failed\n"));
                          }
                      CosaDmlMTASetSecondaryDhcpServerOptions(pMyObject->pmtaprovinfo, pString, ip_type);

                      rc = strcpy_s( pMyObject->pmtaprovinfo->IPv6SecondaryDhcpServerOptions.UpdateSource,sizeof( pMyObject->pmtaprovinfo->IPv6SecondaryDhcpServerOptions.UpdateSource ), requestorStr );
                      if (rc != EOK)
                      {
                          ERR_CHK(rc);
                          return FALSE;
                      }
                      

                      char PartnerID[PARTNER_ID_LEN] = {0};
                      if((CCSP_SUCCESS == getPartnerId(PartnerID) ) && (PartnerID[ 0 ] != '\0') )
                          UpdateJsonParam("Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv6SecondaryDhcpServerOptions",PartnerID, pString, requestorStr, currentTime);

                      return TRUE;
                  }
                 return FALSE;

           } else {
                AnscTraceWarning(("Eth_wan not enabled : Invalid request\n"));
                return FALSE;
           }
            /*Coverity Fix CID:59709 MISSING_RETURN */
            return FALSE;

}

BOOL
X_RDKCENTRAL_COM_MTA_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{ 
        UNREFERENCED_PARAMETER(hInsContext);
        errno_t                         rc            = -1;
        int                             ind           = -1;
        rc = strcmp_s("pktcMtaDevResetNow", strlen("pktcMtaDevResetNow"), ParamName, &ind );
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
	{
		*pBool = false;
		return TRUE;
	}

	return FALSE;
}

BOOL
X_RDKCENTRAL_COM_MTA_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                       bValue
    )
{
        UNREFERENCED_PARAMETER(hInsContext);
        errno_t                         rc            = -1;
        int                             ind           = -1;
        
        rc = strcmp_s("pktcMtaDevResetNow", strlen("pktcMtaDevResetNow"), ParamName, &ind );       
               ERR_CHK(rc);
        if((!ind) && (rc == EOK))
	{
		//if(pBool && *pBool)
                if(bValue)
		{
			if(	ANSC_STATUS_SUCCESS == CosaDmlMtaResetNow(bValue)	)
			{                    
				AnscTraceWarning(("MTA Module is Reset ParamName: '%s'  Value: '%d' \n",ParamName,bValue));
				CosaDmlMtaProvisioningStatusGet();
				return TRUE;
			}
			else
			{                        
				AnscTraceWarning(("MTA Module is not Reset ParamName: '%s'  Value: '%d' \n",ParamName,bValue));
				return FALSE;
			}
		}
		else
		{
                	AnscTraceWarning(("parameter '%s' value: '%d' \n", ParamName,bValue));
			return TRUE;
		}
	}
	
	return FALSE;
}
