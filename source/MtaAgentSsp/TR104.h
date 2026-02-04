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
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#ifndef INTEL_PUMA7
#include <LatticeMtaTR104.h>
#endif //INTEL_PUMA7
#include <rbus/rbus.h>
#include "../TR-181/middle_layer_src/mta_compat_types.h"
//#include "ccsp_trace.h"
/*#include "ansc_platform.h"*/
#include "mta_hal.h"

int mta_hal_getTR104parameterValues(char **parameterNamesList, int *parameterListLen, char ***parameterValuesList);
int mta_hal_freeTR104parameterValues(char **parameterValuesList, int  parameterListLen);
int mta_hal_setTR104parameterValues(char **parameterValueList, int *parameterListLen);
int mta_hal_getTR104parameterNames(char ***parameterNamesList, int *parameterListLen);
int TR104_open();
rbusError_t TR104Services_TableHandler(rbusHandle_t handle, rbusProperty_t inProperty, rbusGetHandlerOptions_t* opts);
rbusError_t TR104Services_GetHandler(rbusHandle_t handle, rbusProperty_t inProperty, rbusGetHandlerOptions_t* opts);
rbusError_t TR104Services_SetHandler(rbusHandle_t handle, rbusProperty_t inProperty, rbusSetHandlerOptions_t* opts);

#endif
