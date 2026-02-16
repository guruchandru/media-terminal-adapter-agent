/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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

#ifndef MTA_BUS_JSON_DECODE_H
#define MTA_BUS_JSON_DECODE_H

#include <rbus.h>
#include <stdbool.h>

#define JSON_CONFIG_PATH "/usr/ccsp/mta/mta_dml_config.json"

typedef enum {
    MTA_NAMESPACE_X_CISCO_COM_MTA_V6,
    MTA_NAMESPACE_X_CISCO_COM_MTA,
    MTA_NAMESPACE_X_CISCO_COM_MTA_DSXLOG,
    MTA_NAMESPACE_X_CISCO_COM_MTA_LINETABLE,
    MTA_NAMESPACE_X_CISCO_COM_MTA_LINETABLE_CALLP,
    MTA_NAMESPACE_X_CISCO_COM_MTA_LINETABLE_VQM,
    MTA_NAMESPACE_X_CISCO_COM_MTA_LINETABLE_VQM_CALLS,
    MTA_NAMESPACE_X_CISCO_COM_MTA_SERVICECLASS,
    MTA_NAMESPACE_X_CISCO_COM_MTA_SERVICEFLOW,
    MTA_NAMESPACE_X_CISCO_COM_MTA_DECT,
    MTA_NAMESPACE_X_CISCO_COM_MTA_DECT_HANDSETS,
    MTA_NAMESPACE_X_CISCO_COM_MTA_MTALOG,
    MTA_NAMESPACE_X_CISCO_COM_MTA_DECTLOG,
    MTA_NAMESPACE_X_CISCO_COM_MTA_BATTERY,
    MTA_NAMESPACE_VOICESERVICE,
    MTA_NAMESPACE_X_RDKCENTRAL_COM_MTA,
    MTA_NAMESPACE_X_RDKCENTRAL_COM_ETHERNETWAN_MTA,
    MTA_NAMESPACE_UNKNOWN
} mta_namespace_t;

typedef struct {
    char* full_name;
    char* short_name;
    char* parent_namespace;
    mta_namespace_t namespace_type;
    rbusValueType_t type;
    bool writable;
    bool is_table_param;  /* TRUE if parameter belongs to a table instance */
} mta_param_metadata_t;

int mta_decode_json_config(rbusHandle_t handle, const char *json_file_path);

void mta_free_registered_elements(void);

mta_param_metadata_t* mta_find_param_metadata(const char* full_name);
void mta_free_param_metadata(void);

/* Helper to extract instance number from path like "Device.X.Table.1.Param" */
int mta_extract_instance_number(const char* param_name);

#endif /* MTA_BUS_JSON_DECODE_H */
