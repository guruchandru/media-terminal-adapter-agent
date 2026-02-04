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

#ifdef MTA_TR104SUPPORT
#include "TR104.h"
#include "cosa_x_cisco_com_mta_apis.h"
#include "mta_hal.h"
#include "syscfg/syscfg.h"
#include "plugin_main_apis.h"
//#include "safec_lib_common.h"
#include <safec_lib.h>
#include "cosa_x_cisco_com_mta_dml.h"
#include "cosa_x_cisco_com_mta_internal.h"
/* Legacy DBUS headers commented out for RBUS approach */
/* #include "ccsp_base_api.h" */
/* #include "messagebus_interface_helper.h" */
char MTA_provision_status[2][20] = {"MTA_PROVISIONED", "MTA_NON_PROVISIONED"};
#define MTA_PROVISIONED "MTA_PROVISIONED"
#define PARAM_BUF_SIZE 128+8192  //DMCLI object name + type + string parameter (8192 bytes from TR104 spec) 

ANSC_STATUS init_TR104_web_config();

int handle_mta_TR104_reset(const char* buffer);

static bool emtaResetNow = false;

int handle_mta_TR104_reset(const char* buffer){

    char *token = NULL;
    char *rest = NULL;
    char *saveptr = NULL;
    char matchstr[] = "ResetNow";

    CcspTraceDebug(("%s: Entering \n", __FUNCTION__));

    if (buffer == NULL) {
         CcspTraceInfo(("%s: buffer is NULL \n", __FUNCTION__));
         return(1);
    }
    
    CcspTraceDebug(("%s: buffer: %s \n", __FUNCTION__, buffer));

    rest = calloc(strlen(buffer)+ 1 , sizeof(char));
    if (rest == NULL) {
         CcspTraceInfo(("%s: Memory allocation failed for rest \n", __FUNCTION__));
         return(1);
    }
 
   
   // Copy the buffer to the allocated memory
    strncpy(rest, buffer,strlen(buffer));
    rest[strlen(buffer)] = '\0'; // terminate rest
  
    token = strtok_r(rest, ".", &saveptr);
    while (token != NULL) 
    {
        CcspTraceDebug(("%s: token=%s saveptr=%s\n", __FUNCTION__, token, saveptr));
        if (strcmp(token, matchstr) == 0) {
            CcspTraceInfo(("%s: MTA TR104 ResetNow is set \n", __FUNCTION__));
            emtaResetNow = true;
            break;
        } 

        token = strtok_r(NULL, ".", &saveptr);
    }

    if (!emtaResetNow) {
        CcspTraceInfo(("%s: ResetNow is not set \n", __FUNCTION__));
    }
	
    free(rest);

    CcspTraceDebug(("%s: Exiting \n",__FUNCTION__));
    return 0;
}

/* This function is to return ParamValue for all the elements in this partial path Query */
rbusError_t TR104Services_TableHandler(rbusHandle_t handle, rbusProperty_t inProperty, rbusGetHandlerOptions_t* opts)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(opts);
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    int retParamSize = 1;
    const char *pParamNameList = NULL;
    char **pParamValueList = NULL;

    MTAMGMT_MTA_PROVISION_STATUS provisionStatus = 0;
    if(mta_hal_getMtaProvisioningStatus(&provisionStatus) != 0)
    {
        CcspTraceError(("%s mta_hal_getMtaProvisioningStatus fail\n",__FUNCTION__));
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    if( strcmp(MTA_provision_status[provisionStatus],MTA_PROVISIONED) )
    {
        CcspTraceInfo(("%s provisionStatus = %s \n",__FUNCTION__,MTA_provision_status[provisionStatus]));
        return RBUS_ERROR_ACCESS_NOT_ALLOWED;
    }
    pParamNameList = rbusProperty_GetName(inProperty);

    retParamSize = 1;
    CcspTraceInfo(("calling HAL get from %s with %s\n",__FUNCTION__,pParamNameList));
    /* Get Data From HAL and return in as rbusProperty_t */
    if(mta_hal_getTR104parameterValues((char**)&pParamNameList, &retParamSize, &pParamValueList) == 0 )
    {
        int loopCnt;
        rbusProperty_t getProp = NULL;
        rbusProperty_t lastProp = inProperty;
        for (loopCnt = 0; loopCnt < retParamSize; loopCnt++)
        {
            CcspTraceDebug(("%s: mta_hal_getTR104parameterValues returned %s\n",__FUNCTION__,pParamValueList[loopCnt]));
            rbusValue_t value = NULL;
            /* Tokenize the returns value list and set the tlv and pass it to inProperty */
            char *pPropName = NULL;
            char *pStr = strdup (pParamValueList[loopCnt]);
            pPropName = strtok(pStr, ",");
            if (pPropName)
            {
                char* pPropType = NULL;
                char* pPropValue = NULL;

                /* Get Type */
                pPropType = strtok(NULL, ",");
                if (pPropType == NULL)
                {
                    CcspTraceError(("ccspMtaAgentTR104Hal: The pre-defined formatted string is not present\n"));
                    /* Free the memory */
                    free (pStr);
                    continue;
                }

                /* Get Value */
                pPropValue = strtok(NULL, "");
                if(!pPropValue)
                {
                    pPropValue = "";
                }
                CcspTraceDebug(("%s: pPropName=%s\t pPropType=%s\t pPropValue=%s\n",__FUNCTION__,pPropName,pPropType,pPropValue));
                rbusValue_Init(&value);
                /* Parse the incoming data */
                if(strcasecmp (pPropType, "boolean") == 0)
                {
                    rbusValue_SetFromString(value, RBUS_BOOLEAN, pPropValue);
                }
                else if(strcasecmp (pPropType, "dateTime") == 0)
                {
                    if(0 != strncmp(pPropValue,"0-",2))
                    {
                        rbusValue_SetFromString(value, RBUS_DATETIME, pPropValue);
                    }
                    else
                    {
                        rbusValue_SetFromString(value, RBUS_DATETIME, "0000-");
                    }
                }
                else if(strcasecmp (pPropType, "int") == 0)
                {
                    rbusValue_SetFromString(value, RBUS_INT32, pPropValue);
                }
                else if(strcasecmp (pPropType, "unsignedint") == 0)
                {
                    rbusValue_SetFromString(value, RBUS_UINT32, pPropValue);
                }
                else if(strcasecmp (pPropType, "long") == 0)
                {
                    rbusValue_SetFromString(value, RBUS_INT64, pPropValue);
                }
                else if(strcasecmp (pPropType, "unsignedLong") == 0)
                {
                    rbusValue_SetFromString(value, RBUS_UINT64, pPropValue);
                }
                else if(strcasecmp (pPropType, "string") == 0)
                {
                    rbusValue_SetFromString(value, RBUS_STRING, pPropValue);
                }
                else
                {
                    CcspTraceWarning(("ccspMtaAgentTR104Hal: Invalid format to send across..\n"));
                    rbusValue_Release(value);
                    /* Free the memory */
                    free (pStr);
                    continue;
                }

                rbusProperty_Init(&getProp, pPropName, NULL);
                rbusProperty_SetValue(getProp, value);

                rbusProperty_PushBack(lastProp, getProp);
                rbusProperty_Release(getProp);
                rbusValue_Release(value);
                lastProp = getProp;
            }
            /* Free the memory */
            free (pStr);
        }
        /* Free the memory */
        CcspTraceDebug(("%s: calling mta_hal_freeTR104parameterValues for %s values and rc = %d\n",__FUNCTION__,pParamNameList,rc));
        mta_hal_freeTR104parameterValues(pParamValueList, retParamSize);
    }
    else
    {
        CcspTraceError(("ccspMtaAgentTR104Hal: Failed to communicate to hal.. \n"));
        rc = RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }

    return rc;
}
rbusError_t TR104Services_GetHandler(rbusHandle_t handle, rbusProperty_t inProperty, rbusGetHandlerOptions_t* opts)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(opts);
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    int retParamSize = 1;
    const char *pParamNameList = NULL;
    char **pParamValueList = NULL;

    MTAMGMT_MTA_PROVISION_STATUS provisionStatus = 0;
    if(mta_hal_getMtaProvisioningStatus(&provisionStatus) != 0)
    {
        CcspTraceError(("%s mta_hal_getMtaProvisioningStatus fail\n",__FUNCTION__));
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    if( strcmp(MTA_provision_status[provisionStatus],MTA_PROVISIONED) )
    {
        CcspTraceInfo(("%s provisionStatus = %s \n",__FUNCTION__,MTA_provision_status[provisionStatus]));
        return RBUS_ERROR_ACCESS_NOT_ALLOWED;
    }
    rbusValue_t value;
    rbusValue_Init(&value);

    pParamNameList = rbusProperty_GetName(inProperty);

    retParamSize = 1;
    CcspTraceInfo(("calling HAL get from %s with %s\n",__FUNCTION__,pParamNameList)); 
    
    /* Get Data From HAL and return in as rbusProperty_t */
    if(mta_hal_getTR104parameterValues((char**)&pParamNameList, &retParamSize, &pParamValueList) == 0 )
    {
        /* Tokenize the returns value list and set the tlv and pass it to inProperty */
        CcspTraceDebug(("%s: mta_hal_getTR104parameterValues returned %s from %s\n",__FUNCTION__,pParamValueList[0],__FUNCTION__));
        char *pTmp = NULL;
        char *pStr = strdup (pParamValueList[0]);
        pTmp = strtok(pStr, ",");
        if (pTmp)
        {
            char* pType = strtok(NULL, ",");
            char* pValue = strtok(NULL, "");
            if(!pValue)
            {
                pValue = "";
            }
            if (pType == NULL)
            {
                CcspTraceError(("ccspMtaAgentTR104Hal: The pre-defined formatted string is not present\n"));
                rc = RBUS_ERROR_INVALID_INPUT; 
            }
            CcspTraceDebug(("%s: pTmp=%s\t pType=%s\t pValue=%s\n",__FUNCTION__,pTmp,pType,pValue));
            if (rc == RBUS_ERROR_SUCCESS)
            {
                /* Parse the Value  */
                /* Get the Type */
                if(strcasecmp (pType, "boolean") == 0)
                {
                    rbusValue_SetFromString(value, RBUS_BOOLEAN, pValue);
                }
                else if(strcasecmp (pType, "dateTime") == 0)
                {
                    if(0 != strncmp(pValue,"0-",2))
                    {
                        rbusValue_SetFromString(value, RBUS_DATETIME, pValue);
                    }
                    else
                    {
                        rbusValue_SetFromString(value, RBUS_DATETIME, "0000-");
                    }
                }
                else if(strcasecmp (pType, "int") == 0)
                {
                    rbusValue_SetFromString(value, RBUS_INT32, pValue);
                }
                else if(strcasecmp (pType, "unsignedint") == 0)
                {
                    rbusValue_SetFromString(value, RBUS_UINT32, pValue);
                }
                else if(strcasecmp (pType, "long") == 0)
                {
                    rbusValue_SetFromString(value, RBUS_INT64, pValue);
                }
                else if(strcasecmp (pType, "unsignedLong") == 0)
                {
                    rbusValue_SetFromString(value, RBUS_UINT64, pValue);
                }
                else if(strcasecmp (pType, "string") == 0)
                {
                    rbusValue_SetFromString(value, RBUS_STRING, pValue);
                }
                else
                {
                    CcspTraceError(("ccspMtaAgentTR104Hal: Invalid format to send across..\n"));
                    rc = RBUS_ERROR_INVALID_INPUT;
                }

                if (rc == RBUS_ERROR_SUCCESS)
                    rbusProperty_SetValue(inProperty, value);

                rbusValue_Release(value);
            }
        }
        /* Free the memory */
        free (pStr);
        CcspTraceDebug(("%s: calling mta_hal_freeTR104parameterValues for %s values and rc = %d\n",__FUNCTION__,pParamNameList,rc));
        mta_hal_freeTR104parameterValues(pParamValueList, retParamSize);
    }
    else
    {
        CcspTraceError(("ccspMtaAgentTR104Hal: Failed to communicate to hal.. \n"));
        rc = RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }

    return rc;
}
rbusError_t TR104Services_SetHandler(rbusHandle_t handle, rbusProperty_t inProperty, rbusSetHandlerOptions_t* opts)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(opts);
    int count = 1;

    MTAMGMT_MTA_PROVISION_STATUS provisionStatus = 0;
    if(mta_hal_getMtaProvisioningStatus(&provisionStatus) != 0)
    {
        CcspTraceError(("%s mta_hal_getMtaProvisioningStatus fail\n",__FUNCTION__));
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    if( strcmp(MTA_provision_status[provisionStatus],MTA_PROVISIONED) )
    {
        CcspTraceInfo(("%s provisionStatus = %s \n",__FUNCTION__,MTA_provision_status[provisionStatus]));
        return RBUS_ERROR_ACCESS_NOT_ALLOWED;
    }
    const char * name = rbusProperty_GetName(inProperty);
    rbusValue_t value = rbusProperty_GetValue(inProperty);
    rbusValueType_t type = rbusValue_GetType(value);

    char *aParamDetail = NULL;
    handle_mta_TR104_reset (name);
    CcspTraceInfo(("calling HAL set\n"));
    /* Arrive at length and do malloc; for now 512; */
    aParamDetail = calloc (1, PARAM_BUF_SIZE);
    if( aParamDetail != NULL )
    {
        char* pStrValue = rbusValue_ToString(value, NULL, 0);
        /* Make a single string */
        if (type == RBUS_BOOLEAN)
        {
            if (rbusValue_GetBoolean(value) == true)
                snprintf(aParamDetail, PARAM_BUF_SIZE-1, "%s,%s,%s", name, "boolean", "true");
            else
                snprintf(aParamDetail, PARAM_BUF_SIZE-1, "%s,%s,%s", name, "boolean", "false");
        }
        else if (type == RBUS_INT32)
        {
            snprintf(aParamDetail, PARAM_BUF_SIZE-1, "%s,%s,%s", name, "int", pStrValue);
        }
        else if (type == RBUS_UINT32)
        {
            snprintf(aParamDetail, PARAM_BUF_SIZE-1, "%s,%s,%s", name, "unsignedInt", pStrValue);
        }
        else if (type == RBUS_INT64)
        {
            snprintf(aParamDetail, PARAM_BUF_SIZE-1, "%s,%s,%s", name, "long", pStrValue);
        }
        else if (type == RBUS_UINT64)
        {
            snprintf(aParamDetail, PARAM_BUF_SIZE-1, "%s,%s,%s", name, "unsignedLong", pStrValue);
        }
        else if (type == RBUS_STRING)
        {
            snprintf(aParamDetail, PARAM_BUF_SIZE-1, "%s,%s,%s", name, "string", pStrValue);
        }
        else if (type == RBUS_DATETIME)
        {
            snprintf(aParamDetail, PARAM_BUF_SIZE-1, "%s,%s,%s", name, "dateTime", pStrValue);
        }

        /* Free the pStrValue */
        if (pStrValue)
            free (pStrValue);

        if (0 != mta_hal_setTR104parameterValues(&aParamDetail, &count))
        {
            CcspTraceError(("ccspMtaAgentTR104Hal: Set Failed\n"));
            free(aParamDetail);
            return RBUS_ERROR_INVALID_INPUT;
        }
        free(aParamDetail);
        CcspTraceInfo(("ccspMtaAgentTR104Hal: Set Succsess\n"));
        
        if (emtaResetNow){
         CcspTraceInfo(("%s Calling in emtaResetNow \n",__FUNCTION__)); 
         CosaDmlMtaProvisioningStatusGet();
        }
        return RBUS_ERROR_SUCCESS;
    }
    return RBUS_ERROR_OUT_OF_RESOURCES;
}
int TR104_open()
{
    int i = 0;
    int halRet = 0;
    char **pParamList = NULL;
    int paramCount = 0;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    rbusHandle_t rbusHandle = NULL;
    rbusDataElement_t* dataElements = NULL;
    CcspTraceInfo(("calling mta_hal_getTR104parameterNames\n"));
    /* Get the list of properties that are supported by MTA Hal */
    halRet = mta_hal_getTR104parameterNames(&pParamList, &paramCount);
    if (halRet != 0)
    {
        CcspTraceError(("ccspMtaAgentTR104Hal: MTA Hal Returned Failure\n"));
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    CcspTraceInfo(("calling rbus_open for registering with rbus\n"));
    rc = rbus_open(&rbusHandle, "ccspMtaAgentTR104Hal");
    if(rc != RBUS_ERROR_SUCCESS)
    {
        CcspTraceError(("ccspMtaAgentTR104Hal: rbus_open failed: %d\n", rc));
        return rc;
    }

    dataElements = (rbusDataElement_t*)calloc(paramCount, sizeof(rbusDataElement_t));
    if( dataElements != NULL )
    {      
        while (i < paramCount)
        {

            if (strcmp(pParamList[i] + strlen(pParamList[i])-5,".{i}.") == 0 )
            {
                dataElements[i].name = pParamList[i];
                dataElements[i].type = RBUS_ELEMENT_TYPE_TABLE;
                dataElements[i].cbTable.getHandler = TR104Services_TableHandler;
            }
            else
            {
                dataElements[i].name = pParamList[i];
                dataElements[i].type = RBUS_ELEMENT_TYPE_PROPERTY;
                dataElements[i].cbTable.getHandler = (rbusGetHandler_t)TR104Services_GetHandler;
                dataElements[i].cbTable.setHandler = (rbusSetHandler_t)TR104Services_SetHandler;
            }
            i++;
        }


        rc = rbus_regDataElements(rbusHandle, paramCount, dataElements);
        if(rc != RBUS_ERROR_SUCCESS)
        {
            CcspTraceError(("ccspMtaAgentTR104Hal: rbusPropertyProvider_Register failed: %d\n", rc));
            free(dataElements);
            rbus_close(rbusHandle);
            return rc;
        }
        CcspTraceInfo(("registered successfully\n"));
        free(dataElements);
        init_TR104_web_config();
        return RBUS_ERROR_SUCCESS;
    }

    return RBUS_ERROR_OUT_OF_RESOURCES;
}
#endif
