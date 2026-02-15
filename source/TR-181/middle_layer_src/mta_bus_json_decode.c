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

#include "mta_bus_json_decode.h"
#include "mta_rbus_handlers.h"
#include "mta_log.h"
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#define MAX_PARAM_NAME_LEN 512
#define UNREFERENCED_PARAMETER(P) (void)(P)
#define MTA_JSON_LOG(fmt, ...) mta_log_write_with_level(CCSP_TRACE_LEVEL_INFO, fmt "\n", ##__VA_ARGS__)
#define MTA_JSON_ERROR(fmt, ...) mta_log_write_with_level(CCSP_TRACE_LEVEL_ERROR, fmt "\n", ##__VA_ARGS__)

typedef struct mta_param_meta_node {
    mta_param_metadata_t meta;
    struct mta_param_meta_node* next;
} mta_param_meta_node_t;

static mta_param_meta_node_t* g_mta_meta_head = NULL;

static mta_namespace_t classify_mta_namespace(const char* full_name)
{
    if (!full_name) return MTA_NAMESPACE_UNKNOWN;
    
    if (strstr(full_name, "X_CISCO_COM_MTA_V6"))
        return MTA_NAMESPACE_X_CISCO_COM_MTA_V6;
    if (strstr(full_name, "X_CISCO_COM_MTA.DSXLog"))
        return MTA_NAMESPACE_X_CISCO_COM_MTA_DSXLOG;
    if (strstr(full_name, "X_CISCO_COM_MTA.LineTable") && strstr(full_name, "VQM.Calls"))
        return MTA_NAMESPACE_X_CISCO_COM_MTA_LINETABLE_VQM_CALLS;
    if (strstr(full_name, "X_CISCO_COM_MTA.LineTable") && strstr(full_name, "VQM"))
        return MTA_NAMESPACE_X_CISCO_COM_MTA_LINETABLE_VQM;
    if (strstr(full_name, "X_CISCO_COM_MTA.LineTable") && strstr(full_name, "CALLP"))
        return MTA_NAMESPACE_X_CISCO_COM_MTA_LINETABLE_CALLP;
    if (strstr(full_name, "X_CISCO_COM_MTA.LineTable"))
        return MTA_NAMESPACE_X_CISCO_COM_MTA_LINETABLE;
    if (strstr(full_name, "X_CISCO_COM_MTA.ServiceClass"))
        return MTA_NAMESPACE_X_CISCO_COM_MTA_SERVICECLASS;
    if (strstr(full_name, "X_CISCO_COM_MTA.ServiceFlow"))
        return MTA_NAMESPACE_X_CISCO_COM_MTA_SERVICEFLOW;
    if (strstr(full_name, "X_CISCO_COM_MTA.Dect.Handsets"))
        return MTA_NAMESPACE_X_CISCO_COM_MTA_DECT_HANDSETS;
    if (strstr(full_name, "X_CISCO_COM_MTA.Dect"))
        return MTA_NAMESPACE_X_CISCO_COM_MTA_DECT;
    if (strstr(full_name, "X_CISCO_COM_MTA.MTALog"))
        return MTA_NAMESPACE_X_CISCO_COM_MTA_MTALOG;
    if (strstr(full_name, "X_CISCO_COM_MTA.DECTLog"))
        return MTA_NAMESPACE_X_CISCO_COM_MTA_DECTLOG;
    if (strstr(full_name, "X_CISCO_COM_MTA.Battery"))
        return MTA_NAMESPACE_X_CISCO_COM_MTA_BATTERY;
    if (strstr(full_name, "X_CISCO_COM_MTA"))
        return MTA_NAMESPACE_X_CISCO_COM_MTA;
    if (strstr(full_name, "VoiceService"))
        return MTA_NAMESPACE_VOICESERVICE;
    if (strstr(full_name, "X_RDKCENTRAL-COM_EthernetWAN_MTA"))
        return MTA_NAMESPACE_X_RDKCENTRAL_COM_ETHERNETWAN_MTA;
    if (strstr(full_name, "X_RDKCENTRAL-COM_MTA"))
        return MTA_NAMESPACE_X_RDKCENTRAL_COM_MTA;
    
    return MTA_NAMESPACE_UNKNOWN;
}

static mta_param_metadata_t* mta_meta_add(const char* full_name, rbusValueType_t type, bool writable)
{
    if(!full_name) return NULL;
    
    mta_param_meta_node_t* n = (mta_param_meta_node_t*)calloc(1, sizeof(*n));
    if(!n) return NULL;
    
    n->meta.full_name = strdup(full_name);
    if(!n->meta.full_name) {
        free(n);
        return NULL;
    }
    
    const char* last_dot = strrchr(full_name, '.');
    n->meta.short_name = last_dot ? strdup(last_dot + 1) : strdup(full_name);
    
    if (last_dot) {
        size_t parent_len = last_dot - full_name;
        n->meta.parent_namespace = strndup(full_name, parent_len);
    } else {
        n->meta.parent_namespace = strdup("");
    }
    
    n->meta.namespace_type = classify_mta_namespace(full_name);
    n->meta.type = type;
    n->meta.writable = writable;
    n->next = g_mta_meta_head;
    g_mta_meta_head = n;

    /* Debug: Log metadata creation */
    fprintf(stderr, "DEBUG: Added metadata for %s (short=%s, ns=%d, type=%d)\n",
            full_name, n->meta.short_name ? n->meta.short_name : "NULL",
            n->meta.namespace_type, type);

    return &n->meta;
}

mta_param_metadata_t* mta_find_param_metadata(const char* full_name)
{
    if(!full_name) return NULL;

    /* Debug: Log lookup attempt */
    static int lookup_count = 0;
    if (lookup_count < 10) {  /* Only log first 10 to avoid spam */
        fprintf(stderr, "DEBUG: Looking up metadata for: %s (head=%p)\n", full_name, (void*)g_mta_meta_head);
        lookup_count++;
    }

    for(mta_param_meta_node_t* n = g_mta_meta_head; n; n = n->next) {
        if(strcmp(n->meta.full_name, full_name) == 0) {
            fprintf(stderr, "DEBUG: Found metadata for %s\n", full_name);
            return &n->meta;
        }
    }

    fprintf(stderr, "DEBUG: Metadata NOT found for %s\n", full_name);
    return NULL;
}

void mta_free_param_metadata(void)
{
    mta_param_meta_node_t* n = g_mta_meta_head;
    while(n) {
        mta_param_meta_node_t* next = n->next;
        free(n->meta.full_name);
        free(n->meta.short_name);
        free(n->meta.parent_namespace);
        free(n);
        n = next;
    }
    g_mta_meta_head = NULL;
}

static char **g_mta_registered_names = NULL;
static size_t g_mta_registered_names_count = 0;

static const char* mta_store_registered_name(const char *param_name)
{
    if (!param_name) return NULL;

    char *heap_copy = strdup(param_name);
    if (!heap_copy) {
        fprintf(stderr, "ERROR: strdup() failed for %s\n", param_name);
        return NULL;
    }

    char **tmp = realloc(g_mta_registered_names,
                         (g_mta_registered_names_count + 1) * sizeof(char*));
    if (!tmp) {
        free(heap_copy);
        fprintf(stderr, "ERROR: realloc() failed for %s\n", param_name);
        return NULL;
    }

    g_mta_registered_names = tmp;
    g_mta_registered_names[g_mta_registered_names_count++] = heap_copy;
    return heap_copy;
}

void mta_free_registered_elements(void)
{
    for (size_t i = 0; i < g_mta_registered_names_count; i++) {
        free(g_mta_registered_names[i]);
    }
    free(g_mta_registered_names);
    g_mta_registered_names = NULL;
    g_mta_registered_names_count = 0;
    fprintf(stderr, "Freed all registered MTA RBUS element names\n");
}

static rbusValueType_t get_rbus_type_from_string(const char *type_str)
{
    if (!type_str) {
        return RBUS_NONE;
    }

    if (strcmp(type_str, "boolean") == 0 || strcmp(type_str, "bool") == 0) {
        return RBUS_BOOLEAN;
    } else if (strcmp(type_str, "uint32") == 0 || strcmp(type_str, "uint32_t") == 0 || strcmp(type_str, "unsigned") == 0) {
        return RBUS_UINT32;
    } else if (strcmp(type_str, "int32") == 0 || strcmp(type_str, "int") == 0 || strcmp(type_str, "integer") == 0) {
        return RBUS_INT32;
    } else if (strcmp(type_str, "string") == 0) {
        return RBUS_STRING;
    }

    return RBUS_NONE;
}

static int register_parameter(rbusHandle_t handle, const char *param_name, rbusValueType_t type, bool writable,
                              mta_param_metadata_t* meta)
{
    (void)type;
    (void)meta;
    
    rbusDataElement_t dataElement;
    memset(&dataElement, 0, sizeof(dataElement));

    dataElement.name = (char *)param_name;
    dataElement.type = RBUS_ELEMENT_TYPE_PROPERTY;
    
    dataElement.cbTable.getHandler = mta_rbus_get_handler;
    dataElement.cbTable.setHandler = writable ? mta_rbus_set_handler : NULL;

    fprintf(stderr, "Registering MTA parameter: %s (writable=%d)\n", param_name, writable);

    rbusError_t rc = rbus_regDataElements(handle, 1, &dataElement);
    if (rc != RBUS_ERROR_SUCCESS) {
        fprintf(stderr, "Failed to register %s: %s\n", param_name, rbusError_ToString(rc));
        return -1;
    }

    return 0;
}

static int process_list_of_def(rbusHandle_t handle, cJSON *list_of_def, const char *parent_path, cJSON *definitions)
{
    UNREFERENCED_PARAMETER(definitions);  /* Parameter not used in current implementation */
    
    if (!cJSON_IsArray(list_of_def)) {
        fprintf(stderr, "List_Of_Def is not an array for %s\n", parent_path);
        return -1;
    }

    int array_size = cJSON_GetArraySize(list_of_def);
    for (int i = 0; i < array_size; i++) {
        cJSON *param_obj = cJSON_GetArrayItem(list_of_def, i);
        if (!param_obj) continue;

        cJSON *param = param_obj->child;
        if (!param || !param->string) continue;

        char full_param_name[MAX_PARAM_NAME_LEN];
        snprintf(full_param_name, sizeof(full_param_name), "%s.%s", parent_path, param->string);

        cJSON *type_obj = cJSON_GetObjectItem(param, "type");
        cJSON *writable_obj = cJSON_GetObjectItem(param, "writable");

        if (!type_obj || !cJSON_IsString(type_obj)) {
            fprintf(stderr, "Missing or invalid type for %s\n", full_param_name);
            continue;
        }

        rbusValueType_t rbus_type = get_rbus_type_from_string(type_obj->valuestring);
        bool writable = writable_obj && cJSON_IsTrue(writable_obj);

        const char *stable_name = mta_store_registered_name(full_param_name);
        if (!stable_name) {
            fprintf(stderr, "ERROR: Failed to allocate stable name for %s\n", full_param_name);
            continue;
        }

        mta_param_metadata_t* meta = mta_meta_add(stable_name, rbus_type, writable);
        if(!meta) {
            fprintf(stderr, "ERROR: Failed to allocate metadata for %s\n", stable_name);
            continue;
        }

        if (register_parameter(handle, stable_name, rbus_type, writable, meta) != 0) {
            fprintf(stderr, "WARNING: Failed to register %s\n", full_param_name);
        }
    }

    return 0;
}

static void process_object_recursive(rbusHandle_t handle, cJSON *obj, const char *path, cJSON *definitions)
{
    if (!obj || !cJSON_IsObject(obj)) {
        return;
    }

    cJSON *child = obj->child;
    while (child) {
        if (strcmp(child->string, "List_Of_Def") == 0) {
            process_list_of_def(handle, child, path, definitions);
        } else if (cJSON_IsObject(child)) {
            char new_path[MAX_PARAM_NAME_LEN];
            snprintf(new_path, sizeof(new_path), "%s.%s", path, child->string);
            process_object_recursive(handle, child, new_path, definitions);
        }
        child = child->next;
    }
}

int mta_decode_json_config(rbusHandle_t handle, const char *json_file_path)
{
    FILE *file = fopen(json_file_path, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open JSON config file: %s\n", json_file_path);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *json_data = (char *)malloc(file_size + 1);
    if (!json_data) {
        fprintf(stderr, "Error: Memory allocation failed for JSON data\n");
        fclose(file);
        return -1;
    }

    fread(json_data, 1, file_size, file);
    json_data[file_size] = '\0';
    fclose(file);

    cJSON *root = cJSON_Parse(json_data);
    free(json_data);

    if (!root) {
        const char *error_ptr = cJSON_GetErrorPtr();
        fprintf(stderr, "Error: Failed to parse JSON: %s\n", error_ptr ? error_ptr : "unknown error");
        return -1;
    }

    cJSON *definitions = cJSON_GetObjectItem(root, "definitions");
    cJSON *device_obj = cJSON_GetObjectItem(root, "Device");
    if (!device_obj || !cJSON_IsObject(device_obj)) {
        fprintf(stderr, "Error: 'Device' object not found in JSON\n");
        cJSON_Delete(root);
        return -1;
    }

    fprintf(stderr, "Processing MTA JSON config: %s\n", json_file_path);
    process_object_recursive(handle, device_obj, "Device", definitions);

    /* Debug: Count metadata entries */
    int meta_count = 0;
    for(mta_param_meta_node_t* n = g_mta_meta_head; n; n = n->next) {
        meta_count++;
    }
    fprintf(stderr, "DEBUG: Total metadata entries created: %d (g_mta_meta_head=%p)\n", meta_count, (void*)g_mta_meta_head);

    cJSON_Delete(root);
    fprintf(stderr, "MTA JSON config processing complete\n");
    return 0;
}
