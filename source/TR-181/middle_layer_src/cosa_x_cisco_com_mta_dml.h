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

/* Include RBUS compatibility types */
#include "mta_compat_types.h"

// #ifdef CONFIG_TI_PACM
/**************************************************************************

    module: cosa_X_CISCO_COM_mta_dml.h

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    description:

        This file defines the apis for objects to support Data Model Library.

    -------------------------------------------------------------------


    author:

        COSA XML TOOL CODE GENERATOR 1.0

    -------------------------------------------------------------------

    revision:

        01/05/2012    initial revision.

**************************************************************************/


#ifndef  _COSA_MTA_DML_H
#define  _COSA_MTA_DML_H

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
BOOL
X_CISCO_COM_MTA_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );

BOOL
X_CISCO_COM_MTA_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    );

BOOL
X_CISCO_COM_MTA_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      pUlong
    );

ULONG
X_CISCO_COM_MTA_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
X_CISCO_COM_MTA_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    );

BOOL
X_CISCO_COM_MTA_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         value
    );

BOOL
X_CISCO_COM_MTA_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValuepUlong
    );

BOOL
X_CISCO_COM_MTA_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       strValue
    );

BOOL
X_CISCO_COM_MTA_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    );

ULONG
X_CISCO_COM_MTA_Commit
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
X_CISCO_COM_MTA_Rollback
    (
        ANSC_HANDLE                 hInsContext
    );

/***********************************************************************

 APIs for Object:

    X_CISCO_COM_MTA_V6.

    *  X_CISCO_COM_MTA_V6_GetParamUlongValue
    *  X_CISCO_COM_MTA_V6_GetParamStringValue
    *  X_CISCO_COM_MTA_V6_Validate
    *  X_CISCO_COM_MTA_V6_Commit
    *  X_CISCO_COM_MTA_V6_Rollback

***********************************************************************/

BOOL
X_CISCO_COM_MTA_V6_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      pUlong
    );

ULONG
X_CISCO_COM_MTA_V6_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
X_CISCO_COM_MTA_V6_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    );

ULONG
X_CISCO_COM_MTA_V6_Commit
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
X_CISCO_COM_MTA_V6_Rollback
    (
        ANSC_HANDLE                 hInsContext
    );

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
ULONG
MTALog_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );

ANSC_HANDLE
MTALog_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );

BOOL
MTALog_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
MTALog_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    );

BOOL
MTALog_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

ULONG
MTALog_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

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
ULONG
DECTLog_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );

ANSC_HANDLE
DECTLog_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );

BOOL
DECTLog_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
DECTLog_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    );

BOOL
DECTLog_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

ULONG
DECTLog_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

//#ifdef CONFIG_TI_BBU

/***********************************************************************

 APIs for Object:

    Battery.

    *  Battery_GetParamBoolValue
    *  Battery_GetParamIntValue
    *  Battery_GetParamUlongValue
    *  Battery_GetParamStringValue

***********************************************************************/
BOOL
Battery_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );

BOOL
Battery_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    );

BOOL
Battery_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

ULONG
Battery_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );
//#endif /* CONFIG_TI_BBU */

BOOL
X_RDKCENTRAL_COM_MTA_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

ULONG
X_RDKCENTRAL_COM_MTA_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
X_RDKCENTRAL_COM_MTA_GetParamBoolValue

    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );

BOOL
X_RDKCENTRAL_COM_MTA_SetParamBoolValue

    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                       bValue
    );
int CosaDmlTR104DataSet(char* pString,int bootup);
#endif /* _COSA_MTA_DML_H */

// #endif /* CONFIG_TI_PACM */
BOOL
LineTable_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );

BOOL
LineTable_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    );

BOOL
LineTable_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

ULONG
LineTable_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
LineTable_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    );

BOOL
LineTable_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    );

BOOL
LineTable_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    );

BOOL
LineTable_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    );

ULONG
LineTable_Commit
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
LineTable_Rollback
    (
        ANSC_HANDLE                 hInsContext
    );

BOOL
LineTable_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    );

ULONG
CALLP_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

ULONG
ServiceClass_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
VQM_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    );

ULONG
VQM_Commit
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
VQM_Rollback
    (
        ANSC_HANDLE                 hInsContext
    );

BOOL
Dect_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    );

BOOL
Dect_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    );

ULONG
Dect_Commit
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
Dect_Rollback
    (
        ANSC_HANDLE                 hInsContext
    );

BOOL
Handsets_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    );

BOOL
Handsets_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

ULONG
Handsets_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
Handsets_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    );

BOOL
Handsets_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    );

BOOL
Handsets_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    );

BOOL
Handsets_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    );

ULONG
Handsets_Rollback
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
LineTable_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
Calls_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );
ULONG
ServiceClass_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );
ULONG
ServiceFlow_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );
ULONG
Handsets_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );
ULONG
DSXLog_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );
ANSC_HANDLE
LineTable_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );
ANSC_HANDLE
Calls_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );
ANSC_HANDLE
ServiceClass_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );

ANSC_HANDLE
ServiceFlow_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );
ANSC_HANDLE
Handsets_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );
ANSC_HANDLE
DSXLog_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );
BOOL
LineTable_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );
BOOL
VQM_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );
BOOL
ServiceFlow_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );
ULONG
ServiceFlow_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
ServiceFlow_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

BOOL
DSXLog_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );
BOOL
LineTable_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );
BOOL
LineTable_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    );
BOOL
Calls_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    );
ULONG
Calls_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    );
BOOL
VQM_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    );
BOOL
Calls_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );
ULONG
Calls_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );
BOOL
ServiceClass_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    );
BOOL
DSXLog_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    );
BOOL
Handsets_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    );
BOOL
ServiceFlow_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    );
char* getRequestorString();

ULONG VoiceService_GetParamStringValue(
    ANSC_HANDLE hInsContext,
    char* ParamName,
    char* pValue,
    ULONG* pUlSize
);

BOOL
VoiceService_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    );

ULONG
DSXLog_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
Calls_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );
BOOL
Dect_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );
ULONG
Dect_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );
BOOL
Dect_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    );
BOOL
Dect_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    );
BOOL
Dect_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );
BOOL
Dect_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    );
BOOL
Handsets_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );
BOOL
Handsets_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    );
ULONG
Handsets_Commit
    (
        ANSC_HANDLE                 hInsContext
    );

BOOL
EthernetWAN_MTA_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    );

ULONG
EthernetWAN_MTA_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
EthernetWAN_MTA_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    );

BOOL
EthernetWAN_MTA_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    );

