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
#include "cosa_x_cisco_com_mta_dml.h"
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

/* Helper: Check if a namespace component is a dynamic table */
static bool is_table_component(const char *component)
{
    if (!component) return false;
    
    return (strcmp(component, "MTALog") == 0 ||
            strcmp(component, "DECTLog") == 0 ||
            strcmp(component, "DSXLog") == 0 ||
            strcmp(component, "LineTable") == 0 ||
            strcmp(component, "ServiceClass") == 0 ||
            strcmp(component, "ServiceFlow") == 0 ||
            strcmp(component, "Handsets") == 0 ||
            strcmp(component, "Calls") == 0);
}

/* Helper: Check if a namespace is a dynamic table */
static bool is_table_namespace(const char *namespace_str)
{
    if (!namespace_str) return false;
    
    /* Check if it ends with a known table name */
    return (strstr(namespace_str, ".MTALog") != NULL ||
            strstr(namespace_str, ".DECTLog") != NULL ||
            strstr(namespace_str, ".DSXLog") != NULL ||
            strstr(namespace_str, ".LineTable") != NULL ||
            strstr(namespace_str, ".ServiceClass") != NULL ||
            strstr(namespace_str, ".ServiceFlow") != NULL ||
            strstr(namespace_str, ".Handsets") != NULL ||
            strstr(namespace_str, ".Calls") != NULL);
}

/* Helper: Insert {i} after all table components in a path
 * Example: "Device.X.LineTable.VQM.Calls" -> "Device.X.LineTable.{i}.VQM.Calls.{i}" */
static void insert_wildcards_in_path(const char *input_path, char *output_path, size_t output_size)
{
    if (!input_path || !output_path || output_size == 0) return;
    
    char temp_path[MAX_PARAM_NAME_LEN];
    strncpy(temp_path, input_path, sizeof(temp_path) - 1);
    temp_path[sizeof(temp_path) - 1] = '\0';
    
    output_path[0] = '\0';
    char *token = strtok(temp_path, ".");
    bool first = true;
    
    while (token != NULL) {
        if (!first) {
            strncat(output_path, ".", output_size - strlen(output_path) - 1);
        }
        first = false;
        
        strncat(output_path, token, output_size - strlen(output_path) - 1);
        
        /* If this component is a table, insert {i} after it */
        if (is_table_component(token)) {
            strncat(output_path, ".{i}", output_size - strlen(output_path) - 1);
        }
        
        token = strtok(NULL, ".");
    }
}

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

    /* First try exact match */
    for(mta_param_meta_node_t* n = g_mta_meta_head; n; n = n->next) {
        if(strcmp(n->meta.full_name, full_name) == 0) {
            fprintf(stderr, "DEBUG: Found metadata for %s (exact match)\n", full_name);
            return &n->meta;
        }
    }

    /* If no exact match, try wildcard match:
     * Convert "Device.X.MTALog.1.Time" to "Device.X.MTALog.{i}.Time"
     * and search again */
    char wildcard_name[MAX_PARAM_NAME_LEN];
    strncpy(wildcard_name, full_name, sizeof(wildcard_name) - 1);
    wildcard_name[sizeof(wildcard_name) - 1] = '\0';
    
    /* Find instance number pattern and replace with {i} */
    char *p = wildcard_name;
    char *last_dot = NULL;
    char *second_last_dot = NULL;
    
    while (*p) {
        if (*p == '.') {
            second_last_dot = last_dot;
            last_dot = p;
        }
        p++;
    }
    
    /* Check if segment between second_last_dot and last_dot is a number */
    if (second_last_dot && last_dot && second_last_dot < last_dot) {
        char *start = second_last_dot + 1;
        char *end = last_dot;
        bool is_number = true;
        
        for (char *c = start; c < end && is_number; c++) {
            if (*c < '0' || *c > '9') {
                is_number = false;
            }
        }
        
        if (is_number && (end - start) > 0) {
            /* Replace instance number with {i} */
            size_t prefix_len = start - wildcard_name;
            size_t suffix_len = strlen(end);
            
            if (prefix_len + 3 + suffix_len < sizeof(wildcard_name)) {
                strcpy(start, "{i}");
                strcpy(start + 3, end);
                
                /* Now search with wildcard name */
                for(mta_param_meta_node_t* n = g_mta_meta_head; n; n = n->next) {
                    if(strcmp(n->meta.full_name, wildcard_name) == 0) {
                        fprintf(stderr, "DEBUG: Found metadata for %s (wildcard match: %s)\n", 
                                full_name, wildcard_name);
                        return &n->meta;
                    }
                }
            }
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

    /* Insert {i} after all table components in the path
     * Example: LineTable.VQM.Calls -> LineTable.{i}.VQM.Calls.{i} */
    char table_path[MAX_PARAM_NAME_LEN - 128]; /* Reserve space for parameter names */
    insert_wildcards_in_path(parent_path, table_path, sizeof(table_path));
    
    if (strcmp(table_path, parent_path) != 0) {
        fprintf(stderr, "DEBUG: Table path with wildcards: %s (from %s)\n", table_path, parent_path);
    }
    
    /* Check if the immediate parent is a table (for instance registration later) */
    bool is_table = is_table_namespace(parent_path);
    
    /* Register the table element itself if this is a table */
    if (is_table && strcmp(table_path, parent_path) != 0) {
        char table_element_name[MAX_PARAM_NAME_LEN];
        snprintf(table_element_name, sizeof(table_element_name), "%s.", table_path);
        
        /* Allocate stable name for table element */
        const char *stable_table_name = mta_store_registered_name(table_element_name);
        if (stable_table_name) {
            rbusDataElement_t tableElement;
            memset(&tableElement, 0, sizeof(tableElement));
            tableElement.name = (char *)stable_table_name;
            tableElement.type = RBUS_ELEMENT_TYPE_TABLE;
            tableElement.cbTable.getHandler = NULL;
            tableElement.cbTable.setHandler = NULL;
            
            fprintf(stderr, "*** Registering TABLE element: %s (parent_path=%s) ***\n", stable_table_name, parent_path);
            rbusError_t rc = rbus_regDataElements(handle, 1, &tableElement);
            if (rc != RBUS_ERROR_SUCCESS) {
                fprintf(stderr, "ERROR: Failed to register table element %s: %s\n", 
                        stable_table_name, rbusError_ToString(rc));
            } else {
                fprintf(stderr, "SUCCESS: Registered TABLE element %s\n", stable_table_name);
            }
        }
    }

    int array_size = cJSON_GetArraySize(list_of_def);
    for (int i = 0; i < array_size; i++) {
        cJSON *param_obj = cJSON_GetArrayItem(list_of_def, i);
        if (!param_obj) continue;

        cJSON *param = param_obj->child;
        if (!param || !param->string) continue;

        char full_param_name[MAX_PARAM_NAME_LEN];
        snprintf(full_param_name, sizeof(full_param_name), "%s.%s", table_path, param->string);

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

    /* After registering wildcard parameters, register table instances */
    fprintf(stderr, "\n=== TABLE INSTANCE REGISTRATION CHECK ===\n");
    fprintf(stderr, "DEBUG: parent_path='%s', is_table=%d\n", parent_path, is_table);
    
    if (is_table) {
        fprintf(stderr, "DEBUG: This is a TABLE - proceeding with instance registration\n");
        /* Extract table base name */
        char table_name[MAX_PARAM_NAME_LEN - 128];
        strncpy(table_name, parent_path, sizeof(table_name) - 1);
        table_name[sizeof(table_name) - 1] = '\0';
        
        /* Get instance count based on table type */
        ULONG entry_count = 0;
        const char *table_short_name = strrchr(parent_path, '.');
        bool is_nested_table = false;
        
        /* Check if this is a nested table by looking for parent tables in path */
        if (strstr(parent_path, ".LineTable.") && table_short_name) {
            /* This is nested under LineTable (e.g., CALLP, VQM, Calls) */
            is_nested_table = true;
        }
        
        if (table_short_name) {
            table_short_name++; /* Skip the dot */
            
            /* For nested tables, we need to register instances for each parent instance */
            if (is_nested_table) {
                fprintf(stderr, "DEBUG: Nested table detected: %s (will register per parent instance)\n", table_name);
                
                /* Get parent table (LineTable) instance count */
                ULONG parent_count = LineTable_GetEntryCount(NULL);
                
                if (strcmp(table_short_name, "Calls") == 0) {
                    /* For each LineTable instance, register Calls instances */
                    for (ULONG parent_inst = 1; parent_inst <= parent_count; parent_inst++) {
                        /* Build concrete parent path: Device.X_CISCO_COM_MTA.LineTable.1.VQM.Calls */
                        char concrete_table_path[MAX_PARAM_NAME_LEN - 128];
                        snprintf(concrete_table_path, sizeof(concrete_table_path), 
                                "Device.X_CISCO_COM_MTA.LineTable.%lu.VQM.Calls", parent_inst);
                        
                        /* Get entry count for this specific line's calls */
                        ANSC_HANDLE hLineContext = NULL;
                        for (ULONG idx = 0; idx < parent_count; idx++) {
                            ULONG insNum = 0;
                            hLineContext = LineTable_GetEntry(NULL, idx, &insNum);
                            if (insNum == parent_inst) break;
                        }
                        
                        if (hLineContext) {
                            ULONG calls_count = Calls_GetEntryCount(hLineContext);
                            fprintf(stderr, "DEBUG: Adding %lu Calls rows for LineTable.%lu\n", 
                                    calls_count, parent_inst);
                            
                            for (ULONG call_inst = 1; call_inst <= calls_count; call_inst++) {
                                rbusError_t rc = rbusTable_addRow(handle, concrete_table_path, NULL, NULL);
                                if (rc != RBUS_ERROR_SUCCESS) {
                                    fprintf(stderr, "WARNING: Failed to add row %lu for %s: %s\n",
                                            call_inst, concrete_table_path, rbusError_ToString(rc));
                                } else {
                                    fprintf(stderr, "DEBUG: Added row %lu for %s\n", 
                                            call_inst, concrete_table_path);
                                }
                            }
                        }
                    }
                } else {
                    /* CALLP and VQM are singletons under each LineTable, not tables with instances */
                    fprintf(stderr, "DEBUG: Skipping instance registration for singleton %s\n", table_name);
                }
                return 0;
            }
            
            /* Top-level tables */
            fprintf(stderr, "DEBUG: Checking table short name: '%s'\n", table_short_name ? table_short_name : "NULL");
            
            if (strcmp(table_short_name, "MTALog") == 0) {
                entry_count = MTALog_GetEntryCount(NULL);
                fprintf(stderr, "DEBUG: MTALog has %lu entries\n", entry_count);
            } else if (strcmp(table_short_name, "DECTLog") == 0) {
                entry_count = DECTLog_GetEntryCount(NULL);
                fprintf(stderr, "DEBUG: DECTLog has %lu entries\n", entry_count);
            } else if (strcmp(table_short_name, "DSXLog") == 0) {
                entry_count = DSXLog_GetEntryCount(NULL);
                fprintf(stderr, "DEBUG: DSXLog has %lu entries\n", entry_count);
            } else if (strcmp(table_short_name, "LineTable") == 0) {
                entry_count = LineTable_GetEntryCount(NULL);
                fprintf(stderr, "DEBUG: LineTable has %lu entries\n", entry_count);
            } else if (strcmp(table_short_name, "ServiceClass") == 0) {
                entry_count = ServiceClass_GetEntryCount(NULL);
                fprintf(stderr, "DEBUG: ServiceClass has %lu entries\n", entry_count);
            } else if (strcmp(table_short_name, "ServiceFlow") == 0) {
                entry_count = ServiceFlow_GetEntryCount(NULL);
                fprintf(stderr, "DEBUG: ServiceFlow has %lu entries\n", entry_count);
            } else if (strcmp(table_short_name, "Handsets") == 0) {
                entry_count = Handsets_GetEntryCount(NULL);
                fprintf(stderr, "DEBUG: Handsets has %lu entries\n", entry_count);
            } else if (strcmp(table_short_name, "Calls") == 0) {
                entry_count = Calls_GetEntryCount(NULL);
                fprintf(stderr, "DEBUG: Calls has %lu entries\n", entry_count);
            } else {
                fprintf(stderr, "WARNING: Unknown table type '%s'\n", table_short_name);
            }
            
            fprintf(stderr, "=== TABLE INSTANCE REGISTRATION START ===\n");
            fprintf(stderr, "DEBUG: Table='%s', EntryCount=%lu\n", table_name, entry_count);
            
            /* Add table rows using rbusTable_addRow */
            for (ULONG i = 1; i <= entry_count; i++) {
                fprintf(stderr, "DEBUG: Calling rbusTable_addRow(handle, '%s', NULL, NULL)\n", table_name);
                rbusError_t rc = rbusTable_addRow(handle, table_name, NULL, NULL);
                if (rc != RBUS_ERROR_SUCCESS) {
                    fprintf(stderr, "ERROR: Failed to add row %lu for %s: %s (error code=%d)\n",
                            i, table_name, rbusError_ToString(rc), rc);
                } else {
                    fprintf(stderr, "SUCCESS: Added row %lu for table %s\n", i, table_name);
                }
            }
            fprintf(stderr, "=== TABLE INSTANCE REGISTRATION END ===\n");
        }
    } else {
        fprintf(stderr, "DEBUG: NOT a table - skipping instance registration\n");
    }
    fprintf(stderr, "=== TABLE INSTANCE REGISTRATION CHECK END ===\n\n");

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
