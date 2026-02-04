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

/**********************************************************************

    module: ssp_messagebus_interface.c

        For MTA Agent module

    description:

        SSP implementation of the RBUS Message Bus Interface
        Service (replacing legacy DBUS).

        *   ssp_MtaMbi_MessageBusEngage
        *   ssp_MtaMbi_EventCallback

**********************************************************************/

/* Compatibility includes to replace common-library */
#include "../TR-181/middle_layer_src/mta_compat_types.h"
#include "../TR-181/middle_layer_src/mta_rbus_handlers.h"  /* RBUS handlers */
#include <rbus/rbus.h>  /* RBUS API */

#include "ssp_global.h"
//#include "safec_lib_common.h"
#include <safec_lib.h>


ANSC_HANDLE                 bus_handle         = NULL;
extern  BOOL                g_bActive;
extern  char                g_Subsystem[32];

/* Legacy DBUS wait function - kept for compatibility but may not be needed with RBUS */
/*BOOLEAN waitConditionReady
(
    void*                           hMBusHandle,
    const char*                     dst_component_id,
    char*                           dbus_path,
    char*                           src_component_id
);*/

#ifdef _ANSC_LINUX
ANSC_STATUS
ssp_PnmMbi_MessageBusEngage
    (
        char * component_id,
        char * config_file,
        char * path
    )
{
    /* Legacy CCSP_Base_Func_CB will be replaced with RBUS method handlers */
    #if 0
    CCSP_Base_Func_CB           cb                 = {0};
    ANSC_STATUS                 returnStatus       = ANSC_STATUS_SUCCESS;
    errno_t rc = -1;
    #endif

    /* Mark unused parameters in RBUS implementation */
    UNREFERENCED_PARAMETER(config_file);
    UNREFERENCED_PARAMETER(path);

    if ( ! component_id )
    {
        CcspTraceError((" !!! ssp_PnmMbi_MessageBusEngage: component_id is NULL !!!\n"));
        return ANSC_STATUS_FAILURE;
    }

    /* Legacy CCSP_Message_Bus_Init will be replaced with rbus_open(). Ansc_AllocateMemory_Callback/Ansc_FreeMemory_Callback not needed in RBUS. */
    #if 0
    /* Connect to message bus */
    returnStatus = 
        CCSP_Message_Bus_Init
            (
                component_id,
                config_file,
                &bus_handle,
                (CCSP_MESSAGE_BUS_MALLOC)Ansc_AllocateMemory_Callback,           /* mallocfc, use default */
                Ansc_FreeMemory_Callback            /* freefc,   use default */
            );

    if ( returnStatus != ANSC_STATUS_SUCCESS )
    {
        CcspTraceError((" !!! PNM Message Bus Init ERROR !!!\n"));

        return returnStatus;
    }

    if ( g_Subsystem[0] != 0 )
    {
        snprintf(PsmName, sizeof(g_Subsystem)+sizeof(CCSP_DBUS_PSM), "%s%s", g_Subsystem, CCSP_DBUS_PSM);
    }
    else
    {
        rc = strcpy_s(PsmName,sizeof(PsmName),CCSP_DBUS_PSM);
        if(rc != EOK)
        {
            ERR_CHK(rc);
            return ANSC_STATUS_FAILURE;
        }
    }

    /* Wait for PSM */
    waitConditionReady(bus_handle, PsmName, CCSP_DBUS_PATH_PSM, component_id);
    #endif

    /* RBUS initialization - replaces legacy DBUS */
    rbusError_t ret = rbus_open((rbusHandle_t*)&bus_handle, component_id);
    if (ret != RBUS_ERROR_SUCCESS) {
        CcspTraceError(("MTA: Failed to open RBUS connection: %d\n", ret));
        return ANSC_STATUS_FAILURE;
    }

    CcspTraceInfo(("MTA: RBUS connection established: %s\n", component_id));

    /* Initialize MTA RBUS handlers with JSON pattern */
    if (mta_rbus_init(component_id) != 0) {
        CcspTraceError(("Failed to initialize MTA RBUS handlers\n"));
        rbus_close((rbusHandle_t)bus_handle);
        return ANSC_STATUS_FAILURE;
    }

    /* Legacy callback structure assignments - all replaced with RBUS method handlers in mta_rbus_handlers.c */
    #if 0
    CCSP_Msg_SleepInMilliSeconds(1000);

    /* Base interface implementation that will be used cross components */
    cb.getParameterValues     = CcspCcMbi_GetParameterValues;
    cb.setParameterValues     = CcspCcMbi_SetParameterValues;
    cb.setCommit              = CcspCcMbi_SetCommit;
    cb.setParameterAttributes = CcspCcMbi_SetParameterAttributes;
    cb.getParameterAttributes = CcspCcMbi_GetParameterAttributes;
    cb.AddTblRow              = CcspCcMbi_AddTblRow;
    cb.DeleteTblRow           = CcspCcMbi_DeleteTblRow;
    cb.getParameterNames      = CcspCcMbi_GetParameterNames;
    cb.currentSessionIDSignal = CcspCcMbi_CurrentSessionIdSignal;

    /* Base interface implementation that will only be used by pnm */
    cb.initialize             = ssp_PnmMbi_Initialize;
    cb.finalize               = ssp_PnmMbi_Finalize;
    cb.freeResources          = ssp_PnmMbi_FreeResources;
    cb.busCheck               = ssp_PnmMbi_Buscheck;
    cb.getHealth              = ssp_PnmMbi_GetHealth;

    CcspBaseIf_SetCallback(bus_handle, &cb);

    /* Register event/signal */
    returnStatus = 
        CcspBaseIf_Register_Event
            (
                bus_handle,
                0,
                "currentSessionIDSignal"
            );

    if ( returnStatus != CCSP_Message_Bus_OK )
    {
        CcspTraceError((" !!! CCSP_Message_Bus_Register_Event: CurrentSessionIDSignal ERROR returnStatus: %lu!!!\n", returnStatus));

        return returnStatus;
    }
    #endif

    CcspTraceInfo(("MTA RBUS initialization complete\n"));
    return ANSC_STATUS_SUCCESS;
}
#endif

/* Legacy DBUS callback functions - all disabled for RBUS approach */
#if 0
int
ssp_PnmMbi_Initialize
    (
        void * user_data
    )
{
    UNREFERENCED_PARAMETER(user_data);
    
    printf("In %s()\n", __FUNCTION__);
   
    /* CID 61087  Logically dead code fix */ 
    return ANSC_STATUS_SUCCESS;
}

int
ssp_PnmMbi_Finalize
    (
        void * user_data
    )
{
    UNREFERENCED_PARAMETER(user_data);

    printf("In %s()\n", __FUNCTION__);

    /* CID 60617 Logically dead code */
    return ANSC_STATUS_SUCCESS;
}


int
ssp_PnmMbi_Buscheck
    (
        void * user_data
    )
{
    printf("In %s()\n", __FUNCTION__);
   UNREFERENCED_PARAMETER(user_data);
    return 0;
}


int
ssp_PnmMbi_FreeResources
    (
        int priority,
        void * user_data
    )
{
    UNREFERENCED_PARAMETER(user_data);
    UNREFERENCED_PARAMETER(priority);

    printf("In %s()\n", __FUNCTION__);

    /* CID 55217 Logically dead code */
    return ANSC_STATUS_SUCCESS;
}

int ssp_PnmMbi_GetHealth ( )
{
    /* Health monitoring now handled by RBUS directly */
    return CCSP_COMMON_COMPONENT_HEALTH_Green;
}


/* RBUS waitConditionReady - kept for compatibility but typically not needed */
BOOLEAN waitConditionReady
(
    void*                           hMBusHandle,
    const char*                     dst_component_id,
    char*                           dbus_path,
    char*                           src_component_id
)
{
    UNREFERENCED_PARAMETER(hMBusHandle);
    UNREFERENCED_PARAMETER(dst_component_id);
    UNREFERENCED_PARAMETER(dbus_path);
    UNREFERENCED_PARAMETER(src_component_id);
    
    /* RBUS doesn't require waiting for PSM like DBUS did */
    CcspTraceInfo(("waitConditionReady called - skipped in RBUS implementation\n"));
    return TRUE;
}
#endif /* End of legacy DBUS callbacks */