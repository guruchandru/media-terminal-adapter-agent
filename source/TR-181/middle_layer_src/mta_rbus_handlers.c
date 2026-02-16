/*
 * MTA - RBUS Handlers (Metadata-Driven)
 * Uses namespace classification and short_name routing 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <syslog.h>
#include <rbus/rbus.h>
#include "mta_rbus_handlers.h"
#include "mta_bus_json_decode.h"
#include "mta_log.h"
#include "cosa_x_cisco_com_mta_dml.h"
#include "cosa_x_cisco_com_mta_internal.h"
#include "plugin_main_apis.h"

/*
 * NOTE: This RBUS implementation calls the existing DML functions 
 * (X_CISCO_COM_MTA_GetParamBoolValue, etc.) which may internally use
 * HAL APIs or direct hardware access. The DML functions do NOT make
 * CCSP message bus calls for parameter access, so there's no conflict
 * between RBUS and the legacy CCSP/dbus infrastructure.
 * 
 * RBUS provides an alternative access path to the same underlying data,
 * allowing both CCSP TR-181 clients (via dbus) and RBUS clients to coexist.
 */

#define STR_EQ(a,b) (strcmp((a), (b)) == 0)
#define MTA_LOG_INFO(fmt, ...) mta_log_write_with_level(CCSP_TRACE_LEVEL_INFO, fmt "\n", ##__VA_ARGS__)
#define MTA_LOG_ERROR(fmt, ...) mta_log_write_with_level(CCSP_TRACE_LEVEL_ERROR, fmt "\n", ##__VA_ARGS__)
#define MTA_LOG_DEBUG(fmt, ...) mta_log_write_with_level(CCSP_TRACE_LEVEL_DEBUG, fmt "\n", ##__VA_ARGS__)

extern PCOSA_BACKEND_MANAGER_OBJECT g_pCosaBEManager;

/* RBUS handle - global for use across files */
rbusHandle_t g_mta_rbus_handle = NULL;

/* ==================== Helper Functions ==================== */

/**
 * Extract instance number from parameter path
 * Examples:
 *   "Device.X_CISCO_COM_MTA.MTALog.1.Time" -> returns 1
 *   "Device.X_CISCO_COM_MTA.LineTable.2.Status" -> returns 2
 *   "Device.X_CISCO_COM_MTA.pktcMtaDevEnabled" -> returns -1 (no instance)
 */
static int extract_instance_number(const char* param_name)
{
    if (!param_name) return -1;
    
    const char* p = param_name;
    const char* last_dot = NULL;
    const char* second_last_dot = NULL;
    
    /* Find last two dots */
    while (*p) {
        if (*p == '.') {
            second_last_dot = last_dot;
            last_dot = p;
        }
        p++;
    }
    
    if (!second_last_dot || !last_dot) return -1;
    
    /* Check if text between second_last and last dot is a number */
    const char* start = second_last_dot + 1;
    const char* end = last_dot;
    
    if (start >= end) return -1;
    
    /* Verify it's all digits */
    for (const char* c = start; c < end; c++) {
        if (*c <'0' || *c > '9') return -1;
    }
    
    /* Convert to number */
    return atoi(start);
}

/**
 * Extract nested instance numbers from parameter path
 * For nested tables like LineTable.{i}.VQM.Calls.{i}, extracts both instance numbers
 * Examples:
 *   "Device.X_CISCO_COM_MTA.LineTable.1.VQM.Calls.2.Codec" -> line_inst=1, calls_inst=2, returns true
 *   "Device.X_CISCO_COM_MTA.LineTable.1.Status" -> line_inst=1, calls_inst=-1, returns true
 *   "Device.X_CISCO_COM_MTA.pktcMtaDevEnabled" -> line_inst=-1, calls_inst=-1, returns false
 */
static bool extract_nested_instances(const char* param_name, int* line_inst, int* calls_inst)
{
    if (!param_name || !line_inst || !calls_inst) return false;
    
    *line_inst = -1;
    *calls_inst = -1;
    
    /* Check if this is a Calls parameter (nested under LineTable) */
    const char* calls_marker = strstr(param_name, ".VQM.Calls.");
    if (!calls_marker) {
        /* Not a nested Calls parameter, try single instance extraction */
        *line_inst = extract_instance_number(param_name);
        return (*line_inst != -1);
    }
    
    /* Extract LineTable instance number */
    const char* linetable_marker = strstr(param_name, ".LineTable.");
    if (!linetable_marker) return false;
    
    const char* line_start = linetable_marker + strlen(".LineTable.");
    const char* line_end = line_start;
    while (*line_end >= '0' && *line_end <= '9') line_end++;
    
    if (line_end > line_start) {
        char line_num_str[16] = {0};
        int len = line_end - line_start;
        if (len > 0 && len < 16) {
            strncpy(line_num_str, line_start, len);
            *line_inst = atoi(line_num_str);
        }
    }
    
    /* Extract Calls instance number */
    const char* calls_start = calls_marker + strlen(".VQM.Calls.");
    const char* calls_end = calls_start;
    while (*calls_end >= '0' && *calls_end <= '9') calls_end++;
    
    if (calls_end > calls_start) {
        char calls_num_str[16] = {0};
        int len = calls_end - calls_start;
        if (len > 0 && len < 16) {
            strncpy(calls_num_str, calls_start, len);
            *calls_inst = atoi(calls_num_str);
        }
    }
    
    return (*line_inst != -1 && *calls_inst != -1);
}

/**
 * Get instance handle for MTALog table
 */
static ANSC_HANDLE get_mtalog_instance(int instance_num)
{
    if (instance_num < 1) return NULL;
    
    ULONG count = MTALog_GetEntryCount(NULL);
    if ((ULONG)instance_num > count) return NULL;
    
    ULONG ins_num = 0;
    return MTALog_GetEntry(NULL, instance_num - 1, &ins_num);
}

/**
 * Get instance handle for DECTLog table
 */
static ANSC_HANDLE get_dectlog_instance(int instance_num)
{
    if (instance_num < 1) return NULL;
    
    ULONG count = DECTLog_GetEntryCount(NULL);
    if ((ULONG)instance_num > count) return NULL;
    
    ULONG ins_num = 0;
    return DECTLog_GetEntry(NULL, instance_num - 1, &ins_num);
}

/**
 * Get instance handle for DSXLog table
 */
static ANSC_HANDLE get_dsxlog_instance(int instance_num)
{
    if (instance_num < 1) return NULL;
    
    ULONG count = DSXLog_GetEntryCount(NULL);
    if ((ULONG)instance_num > count) return NULL;
    
    ULONG ins_num = 0;
    return DSXLog_GetEntry(NULL, instance_num - 1, &ins_num);
}

/**
 * Get instance handle for LineTable  
 */
static ANSC_HANDLE get_linetable_instance(int instance_num)
{
    if (instance_num < 1) return NULL;
    
    ULONG count = LineTable_GetEntryCount(NULL);
    if ((ULONG)instance_num > count) return NULL;
    
    ULONG ins_num = 0;
    return LineTable_GetEntry(NULL, instance_num - 1, &ins_num);
}

/**
 * Get instance handle for Handsets table
 */
static ANSC_HANDLE get_handsets_instance(int instance_num)
{
    if (instance_num < 1) return NULL;
    
    ULONG count = Handsets_GetEntryCount(NULL);
    if ((ULONG)instance_num > count) return NULL;
    
    ULONG ins_num = 0;
    return Handsets_GetEntry(NULL, instance_num - 1, &ins_num);
}

/**
 * Get instance handle for ServiceClass table
 */
static ANSC_HANDLE get_serviceclass_instance(int instance_num)
{
    if (instance_num < 1) return NULL;
    
    ULONG count = ServiceClass_GetEntryCount(NULL);
    if ((ULONG)instance_num > count) return NULL;
    
    ULONG ins_num = 0;
    return ServiceClass_GetEntry(NULL, instance_num - 1, &ins_num);
}

/**
 * Get instance handle for ServiceFlow table
 */
static ANSC_HANDLE get_serviceflow_instance(int instance_num)
{
    if (instance_num < 1) return NULL;
    
    ULONG count = ServiceFlow_GetEntryCount(NULL);
    if ((ULONG)instance_num > count) return NULL;
    
    ULONG ins_num = 0;
    return ServiceFlow_GetEntry(NULL, instance_num - 1, &ins_num);
}

/**
 * Get instance handle for Calls table (VQM.Calls)
 * Note: Calls is a nested table under LineTable, so it requires parent context
 */
static ANSC_HANDLE get_calls_instance(ANSC_HANDLE hLineContext, int instance_num)
{
    if (!hLineContext || instance_num < 1) return NULL;
    
    PCOSA_MTA_LINETABLE_INFO pLineInfo = (PCOSA_MTA_LINETABLE_INFO)hLineContext;
    
    /* Get count from parent LineTable context */
    ULONG count = pLineInfo->CallsNumber;
    if ((ULONG)instance_num > count) return NULL;
    
    /* Get entry from parent context's Calls array */
    ULONG ins_num = 0;
    return Calls_GetEntry(hLineContext, instance_num - 1, &ins_num);
}

/* ==================== Namespace GET Dispatchers ==================== */

/**
 * GET handler for Device.X_CISCO_COM_MTA.* parameters
 */
static rbusError_t get_x_cisco_com_mta(const char *short_name, rbusValue_t *data)
{
    if (!data || !*data) {
        MTA_LOG_ERROR("get_x_cisco_com_mta: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    if (STR_EQ(short_name, "pktcMtaDevEnabled")) {
        BOOL val = FALSE;
        if (X_CISCO_COM_MTA_GetParamBoolValue(NULL, "pktcMtaDevEnabled", &val)) {
            rbusValue_SetBoolean(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "DSXLogEnable")) {
        BOOL val = FALSE;
        if (X_CISCO_COM_MTA_GetParamBoolValue(NULL, "DSXLogEnable", &val)) {
            rbusValue_SetBoolean(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "ClearDSXLog")) {
        BOOL val = FALSE;
        if (X_CISCO_COM_MTA_GetParamBoolValue(NULL, "ClearDSXLog", &val)) {
            rbusValue_SetBoolean(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "CallSignallingLogEnable")) {
        BOOL val = FALSE;
        if (X_CISCO_COM_MTA_GetParamBoolValue(NULL, "CallSignallingLogEnable", &val)) {
            rbusValue_SetBoolean(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "ClearCallSignallingLog")) {
        BOOL val = FALSE;
        if (X_CISCO_COM_MTA_GetParamBoolValue(NULL, "ClearCallSignallingLog", &val)) {
            rbusValue_SetBoolean(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "EnableDECTLog")) {
        BOOL val = FALSE;
        if (X_CISCO_COM_MTA_GetParamBoolValue(NULL, "EnableDECTLog", &val)) {
            rbusValue_SetBoolean(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "EnableMTALog")) {
        BOOL val = FALSE;
        if (X_CISCO_COM_MTA_GetParamBoolValue(NULL, "EnableMTALog", &val)) {
            rbusValue_SetBoolean(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "pktcSigDefCallSigTos") ||
             STR_EQ(short_name, "pktcSigDefMediaStreamTos") ||
             STR_EQ(short_name, "pktcMtaDevRealmOrgName") ||
             STR_EQ(short_name, "pktcMtaDevCmsKerbRealmName") ||
             STR_EQ(short_name, "pktcMtaDevCmsIpsecCtrl") ||
             STR_EQ(short_name, "pktcMtaDevCmsSolicitedKeyTimeout") ||
             STR_EQ(short_name, "pktcMtaDevRealmPkinitGracePeriod") ||
             STR_EQ(short_name, "LeaseTimeRemaining") ||
             STR_EQ(short_name, "LineTableNumberOfEntries") ||
             STR_EQ(short_name, "ServiceFlowNumberOfEntries") ||
             STR_EQ(short_name, "DSXLogNumberOfEntries") ||
             STR_EQ(short_name, "MTAResetCount") ||
             STR_EQ(short_name, "LineResetCount") ||
             STR_EQ(short_name, "IPAddress") ||
             STR_EQ(short_name, "SubnetMask") ||
             STR_EQ(short_name, "Gateway") ||
             STR_EQ(short_name, "PrimaryDNS") ||
             STR_EQ(short_name, "SecondaryDNS") ||
             STR_EQ(short_name, "PrimaryDHCPServer") ||
             STR_EQ(short_name, "SecondaryDHCPServer") ||
             STR_EQ(short_name, "ClearLineStats")){
        ULONG val = 0;
        if (X_CISCO_COM_MTA_GetParamUlongValue(NULL, (char*)short_name, &val)) {
            rbusValue_SetUInt32(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "BootFileName") ||
             STR_EQ(short_name, "FQDN") ||
             STR_EQ(short_name, "RebindTimeRemaining") ||
             STR_EQ(short_name, "RenewTimeRemaining") ||
             STR_EQ(short_name, "DHCPOption3") ||
             STR_EQ(short_name, "DHCPOption6") ||
             STR_EQ(short_name, "DHCPOption7") ||
             STR_EQ(short_name, "DHCPOption8") ||
             STR_EQ(short_name, "PCVersion") ||
             STR_EQ(short_name, "MACAddress")) {
        char val[512] = {0};
        ULONG val_len = sizeof(val);
        if (X_CISCO_COM_MTA_GetParamStringValue(NULL, (char*)short_name, val, &val_len) == 0) {
            rbusValue_SetString(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * GET handler for Device.X_CISCO_COM_MTA_V6.* parameters
 */
static rbusError_t get_x_cisco_com_mta_v6(const char *short_name, rbusValue_t *data)
{
    if (!data || !*data) {
        MTA_LOG_ERROR("get_x_cisco_com_mta_v6: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    if (STR_EQ(short_name, "PrimaryDHCPv6Server") ||
        STR_EQ(short_name, "SecondaryDHCPv6Server") ||
        STR_EQ(short_name, "IPV6Address") ||
        STR_EQ(short_name, "Prefix") ||
        STR_EQ(short_name, "BootFileName") ||
        STR_EQ(short_name, "FQDN") ||
        STR_EQ(short_name, "Gateway") ||
        STR_EQ(short_name, "RebindTimeRemaining") ||
        STR_EQ(short_name, "RenewTimeRemaining") ||
        STR_EQ(short_name, "PrimaryDNS") ||
        STR_EQ(short_name, "SecondaryDNS") ||
        STR_EQ(short_name, "DHCPOption3") ||
        STR_EQ(short_name, "DHCPOption6") ||
        STR_EQ(short_name, "DHCPOption7") ||
        STR_EQ(short_name, "DHCPOption8") ||
        STR_EQ(short_name, "PCVersion") ||
        STR_EQ(short_name, "MACAddress")) {
        char val[512] = {0};
        ULONG val_len = sizeof(val);
        if (X_CISCO_COM_MTA_V6_GetParamStringValue(NULL, (char*)short_name, val, &val_len) == 0) {
            rbusValue_SetString(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "LeaseTimeRemaining")) {
        ULONG val = 0;
        if (X_CISCO_COM_MTA_V6_GetParamUlongValue(NULL, (char*)short_name, &val)) {
            rbusValue_SetUInt32(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * GET handler for Device.X_CISCO_COM_MTA.LineTable.{i}.* parameters
 */
static rbusError_t get_line_table(const char *short_name, rbusValue_t *data, int instance_num)
{
    if (!data || !*data) {
        MTA_LOG_ERROR("get_line_table: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    MTA_LOG_DEBUG("get_line_table: short_name='%s', instance_num=%d", short_name, instance_num);
    
    if (instance_num <= 0) {
        MTA_LOG_ERROR("get_line_table: Invalid instance number %d (must be >= 1)", instance_num);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    
    ANSC_HANDLE hInsContext = get_linetable_instance(instance_num);
    if (!hInsContext) {
        MTA_LOG_ERROR("get_line_table: Failed to get instance handle for instance %d", instance_num);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    MTA_LOG_DEBUG("get_line_table: Got instance handle %p for instance %d", hInsContext, instance_num);
    
    if (STR_EQ(short_name, "TriggerDiagnostics")) {
        BOOL val = FALSE;
        if (LineTable_GetParamBoolValue(hInsContext, "TriggerDiagnostics", &val)) {
            rbusValue_SetBoolean(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "LineNumber") ||
        STR_EQ(short_name, "Status") ||
        STR_EQ(short_name, "CAPort") ||
        STR_EQ(short_name, "MWD") ||
        STR_EQ(short_name, "OverCurrentFault")) {
        ULONG val = 0;
        if (LineTable_GetParamUlongValue(hInsContext, (char*)short_name, &val)) {
            rbusValue_SetUInt32(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "HazardousPotential") ||
        STR_EQ(short_name, "ForeignEMF") ||
        STR_EQ(short_name, "ResistiveFaults") ||
        STR_EQ(short_name, "ReceiverOffHook") ||
        STR_EQ(short_name, "RingerEquivalency") ||
        STR_EQ(short_name, "CAName")) {
        char val[256] = {0};
        ULONG val_len = sizeof(val);
        if (LineTable_GetParamStringValue(hInsContext, (char*)short_name, val, &val_len) == 0) {
            rbusValue_SetString(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * GET handler for Device.X_CISCO_COM_MTA.LineTable.{i}.CALLP.* parameters
 */
static rbusError_t get_line_table_callp(const char *short_name, rbusValue_t *data, int instance_num)
{
    if (!data || !*data) {
        MTA_LOG_ERROR("get_line_table_callp: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    ANSC_HANDLE hInsContext = get_linetable_instance(instance_num);
    if (!hInsContext && instance_num > 0) {
        MTA_LOG_ERROR("get_line_table_callp: Invalid instance %d", instance_num);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    
    if (STR_EQ(short_name, "LCState") ||
        STR_EQ(short_name, "CallPState") ||
        STR_EQ(short_name, "LoopCurrent")) {
        char val[256] = {0};
        ULONG val_len = sizeof(val);
        if (CALLP_GetParamStringValue(hInsContext, (char*)short_name, val, &val_len) == 0) {
            rbusValue_SetString(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * GET handler for Device.X_CISCO_COM_MTA.LineTable.{i}.VQM.* parameters
 */
static rbusError_t get_line_table_vqm(const char *short_name, rbusValue_t *data, int instance_num)
{
    if (!data || !*data) {
        MTA_LOG_ERROR("get_line_table_vqm: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    ANSC_HANDLE hInsContext = get_linetable_instance(instance_num);
    if (!hInsContext && instance_num > 0) {
        MTA_LOG_ERROR("get_line_table_vqm: Invalid instance %d", instance_num);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    
    if (STR_EQ(short_name, "ResetStats")) {
        BOOL val = FALSE;
        if (VQM_GetParamBoolValue(hInsContext, (char*)short_name, &val)) {
            rbusValue_SetBoolean(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/** 
 * GET handler for Device.X_CISCO_COM_MTA.LineTable.{i}.VQM.Calls.{i}.* parameters
 * Note: This is a nested table handler - requires TWO instance numbers
 */
static rbusError_t get_line_table_vqm_calls(const char *short_name, rbusValue_t *data, int line_instance, int calls_instance)
{
    if (!data || !*data) {
        MTA_LOG_ERROR("get_line_table_vqm_calls: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    /* First get the LineTable instance */
    ANSC_HANDLE hLineContext = get_linetable_instance(line_instance);
    if (!hLineContext) {
        MTA_LOG_ERROR("get_line_table_vqm_calls: Invalid LineTable instance %d", line_instance);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    
    /* Then get the Calls instance from that LineTable */
    ANSC_HANDLE hInsContext = get_calls_instance(hLineContext, calls_instance);
    if (!hInsContext) {
        MTA_LOG_ERROR("get_line_table_vqm_calls: Invalid Calls instance %d in LineTable %d", calls_instance, line_instance);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    if (STR_EQ(short_name, "JitterBufferAdaptive") ||
        STR_EQ(short_name, "Originator") ||
        STR_EQ(short_name, "JitterBufferDelay") ||
        STR_EQ(short_name, "RemoteJitterBufferAdaptive")) {
        BOOL val = FALSE;
        if (Calls_GetParamBoolValue(hInsContext, (char*)short_name, &val)) {
            rbusValue_SetBoolean(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "RemoteIPAddress") ||
        STR_EQ(short_name, "CallDuration")) {
        ULONG val = 0;
        if (Calls_GetParamUlongValue(hInsContext, (char*)short_name, &val)) {
            rbusValue_SetUInt32(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "Codec") ||
        STR_EQ(short_name, "RemoteCodec") ||
        STR_EQ(short_name, "CallEndTime") ||
        STR_EQ(short_name, "CallStartTime") ||
        STR_EQ(short_name, "CWErrorRate") ||
        STR_EQ(short_name, "PktLossConcealment") ||
        STR_EQ(short_name, "RemotePktLossConcealment") ||
        STR_EQ(short_name, "CWErrors") ||
        STR_EQ(short_name, "SNR") ||
        STR_EQ(short_name, "MicroReflections") ||
        STR_EQ(short_name, "DownstreamPower") ||
        STR_EQ(short_name, "UpstreamPower") ||
        STR_EQ(short_name, "EQIAverage") ||
        STR_EQ(short_name, "EQIMinimum") ||
        STR_EQ(short_name, "EQIMaximum") ||
        STR_EQ(short_name, "EQIInstantaneous") ||
        STR_EQ(short_name, "MOS-LQ") ||
        STR_EQ(short_name, "MOS-CQ") ||
        STR_EQ(short_name, "EchoReturnLoss") ||
        STR_EQ(short_name, "SignalLevel") ||
        STR_EQ(short_name, "NoiseLevel") ||
        STR_EQ(short_name, "LossRate") ||
        STR_EQ(short_name, "DiscardRate") ||
        STR_EQ(short_name, "BurstDensity") ||
        STR_EQ(short_name, "GapDensity") ||
        STR_EQ(short_name, "BurstDuration") ||
        STR_EQ(short_name, "GapDuration") ||
        STR_EQ(short_name, "RoundTripDelay") ||
        STR_EQ(short_name, "Gmin") ||
        STR_EQ(short_name, "RFactor") ||
        STR_EQ(short_name, "ExternalRFactor") ||
        STR_EQ(short_name, "JitterBufRate") ||
        STR_EQ(short_name, "JBNominalDelay") ||
        STR_EQ(short_name, "JBMaxDelay") ||
        STR_EQ(short_name, "JBAbsMaxDelay") ||
        STR_EQ(short_name, "TxPackets") ||
        STR_EQ(short_name, "TxOctets") ||
        STR_EQ(short_name, "RxPackets") ||
        STR_EQ(short_name, "RxOctets") ||
        STR_EQ(short_name, "PacketLoss") ||
        STR_EQ(short_name, "IntervalJitter") ||
        STR_EQ(short_name, "RemoteIntervalJitter") ||
        STR_EQ(short_name, "RemoteMOS-LQ") ||
        STR_EQ(short_name, "RemoteMOS-CQ") ||
        STR_EQ(short_name, "RemoteEchoReturnLoss") ||
        STR_EQ(short_name, "RemoteSignalLevel") ||
        STR_EQ(short_name, "RemoteNoiseLevel") ||
        STR_EQ(short_name, "RemoteLossRate") ||
        STR_EQ(short_name, "RemoteDiscardRate") ||
        STR_EQ(short_name, "RemoteBurstDensity") ||
        STR_EQ(short_name, "RemoteGapDensity") ||
        STR_EQ(short_name, "RemoteBurstDuration") ||
        STR_EQ(short_name, "RemoteGapDuration") ||
        STR_EQ(short_name, "RemoteRoundTripDelay") ||
        STR_EQ(short_name, "RemoteGmin") ||
        STR_EQ(short_name, "RemoteRFactor") ||
        STR_EQ(short_name, "RemoteExternalRFactor") ||
        STR_EQ(short_name, "RemoteJitterBufRate") ||
        STR_EQ(short_name, "RemoteJBNominalDelay") ||
        STR_EQ(short_name, "RemoteJBMaxDelay") ||
        STR_EQ(short_name, "RemoteJBAbsMaxDelay")) {
        char val[256] = {0};
        ULONG val_len = sizeof(val);
        if (Calls_GetParamStringValue(hInsContext, (char*)short_name, val, &val_len) == 0) {
            rbusValue_SetString(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }

    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * GET handler for Device.X_CISCO_COM_MTA.ServiceClass.{i}.* parameters
 */
static rbusError_t get_service_class(const char *short_name, rbusValue_t *data, int instance_num)
{
    if (!data || !*data) {
        MTA_LOG_ERROR("get_service_class: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    ANSC_HANDLE hInsContext = get_serviceclass_instance(instance_num);
    if (!hInsContext && instance_num > 0) {
        MTA_LOG_ERROR("get_service_class: Invalid instance %d", instance_num);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    
    if (STR_EQ(short_name, "ServiceClassName")) {
        char val[256] = {0};
        ULONG val_len = sizeof(val);
        if (ServiceClass_GetParamStringValue(hInsContext, (char*)short_name, val, &val_len) == 0) {
            rbusValue_SetString(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * GET handler for Device.X_CISCO_COM_MTA.ServiceFlow.{i}.* parameters
 */
static rbusError_t get_service_flow(const char *short_name, rbusValue_t *data, int instance_num)
{
    if (!data || !*data) {
        MTA_LOG_ERROR("get_service_flow: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    MTA_LOG_DEBUG("get_service_flow: short_name='%s', instance_num=%d", short_name, instance_num);
    
    if (instance_num <= 0) {
        MTA_LOG_ERROR("get_service_flow: Invalid instance number %d (must be >= 1)", instance_num);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    
    ANSC_HANDLE hInsContext = get_serviceflow_instance(instance_num);
    if (!hInsContext) {
        MTA_LOG_ERROR("get_service_flow: Failed to get instance handle for instance %d", instance_num);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    MTA_LOG_DEBUG("get_service_flow: Got instance handle %p for instance %d", hInsContext, instance_num);

    if (STR_EQ(short_name, "DefaultFlow")) {
        BOOL val = FALSE;
        if (ServiceFlow_GetParamBoolValue(hInsContext, "DefaultFlow", &val)) {
            rbusValue_SetBoolean(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "TrafficType") ||
        STR_EQ(short_name, "Direction") ||
        STR_EQ(short_name, "ServiceClassName")) {
        char val[256] = {0};
        ULONG val_len = sizeof(val);
        if (ServiceFlow_GetParamStringValue(hInsContext, (char*)short_name, val, &val_len) == 0) {
            rbusValue_SetString(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "SFID") ||
        STR_EQ(short_name, "ScheduleType") ||
        STR_EQ(short_name, "NomGrantInterval") ||
        STR_EQ(short_name, "UnsolicitGrantSize") ||
        STR_EQ(short_name, "TolGrantJitter") ||
        STR_EQ(short_name, "NomPollInterval") ||
        STR_EQ(short_name, "MinReservedPkt") ||
        STR_EQ(short_name, "MaxTrafficRate") ||
        STR_EQ(short_name, "MinReservedRate") ||
        STR_EQ(short_name, "MaxTrafficBurst") ||
        STR_EQ(short_name, "NumberOfPackets")) {
        ULONG val = 0;
        if (ServiceFlow_GetParamUlongValue(hInsContext, (char*)short_name, &val)) {
            rbusValue_SetUInt32(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * GET handler for Device.X_CISCO_COM_MTA.Dect.* parameters
 */
static rbusError_t get_dect(const char *short_name, rbusValue_t *data)
{
    if (!data || !*data) {
        MTA_LOG_ERROR("get_dect: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }

    if (STR_EQ(short_name, "Enable") ||
        STR_EQ(short_name, "RegistrationMode")){
        BOOL val = FALSE;
        if (Dect_GetParamBoolValue(NULL, "Enable", &val)) {
            rbusValue_SetBoolean(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "RegisterDectHandset") ||
        STR_EQ(short_name, "DeregisterDectHandset")) {
        ULONG val = 0;
        if (Dect_GetParamUlongValue(NULL, (char*)short_name, &val)) {
            rbusValue_SetUInt32(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "HardwareVersion") ||
        STR_EQ(short_name, "SoftwareVersion") ||
        STR_EQ(short_name, "RFPI") ||
        STR_EQ(short_name, "PIN")) {
        char val[256] = {0};
        ULONG val_len = sizeof(val);
        if (Dect_GetParamStringValue(NULL, (char*)short_name, val, &val_len) == 0) {
            rbusValue_SetString(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }

    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * GET handler for Device.X_CISCO_COM_MTA.Dect.Handsets.{i}.* parameters
 */
static rbusError_t get_dect_handsets(const char *short_name, rbusValue_t *data, int instance_num)
{
    if (!data || !*data) {
        MTA_LOG_ERROR("get_dect_handsets: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    ANSC_HANDLE hInsContext = get_handsets_instance(instance_num);
    if (!hInsContext && instance_num > 0) {
        MTA_LOG_ERROR("get_dect_handsets: Invalid instance %d", instance_num);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    if (STR_EQ(short_name, "LastActiveTime") ||
        STR_EQ(short_name, "HandsetName") ||
        STR_EQ(short_name, "HandsetFirmware") ||
        STR_EQ(short_name, "OperatingTN") ||
        STR_EQ(short_name, "SupportedTN")) {
        char val[256] = {0};
        ULONG val_len = sizeof(val);
        if (Handsets_GetParamStringValue(hInsContext, (char*)short_name, val, &val_len) == 0) {
            rbusValue_SetString(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "Status")) {
        BOOL val = FALSE;
        if (Handsets_GetParamBoolValue(hInsContext, (char*)short_name, &val)) {
            rbusValue_SetBoolean(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }

    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * GET handler for Device.X_CISCO_COM_MTA.DSXLog.{i}.* parameters
 */
static rbusError_t get_dsxlog(const char *short_name, rbusValue_t *data, int instance_num)
{    if (!data || !*data) {
        MTA_LOG_ERROR("get_dsxlog: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    ANSC_HANDLE hInsContext = get_dsxlog_instance(instance_num);
    if (!hInsContext && instance_num > 0) {
        MTA_LOG_ERROR("get_dsxlog: Invalid instance %d", instance_num);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    if (STR_EQ(short_name, "Time") ||
        STR_EQ(short_name, "Description")) {
        char val[256] = {0};
        ULONG val_len = sizeof(val);
        if (DSXLog_GetParamStringValue(hInsContext, (char*)short_name, val, &val_len) == 0) {
            rbusValue_SetString(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "ID") ||
        STR_EQ(short_name, "Level")) {
        ULONG val = 0;
        if (DSXLog_GetParamUlongValue(hInsContext, (char*)short_name, &val)) {
            rbusValue_SetUInt32(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * GET handler for Device.X_CISCO_COM_MTA.MTALog.{i}.* parameters
 */
static rbusError_t get_mtalog(const char *short_name, rbusValue_t *data, int instance_num)
{
    if (!data || !*data) {
        MTA_LOG_ERROR("get_mtalog: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    ANSC_HANDLE hInsContext = get_mtalog_instance(instance_num);
    if (!hInsContext && instance_num > 0) {
        MTA_LOG_ERROR("get_mtalog: Invalid instance %d", instance_num);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    if (STR_EQ(short_name, "Time") ||
        STR_EQ(short_name, "Description") ||
        STR_EQ(short_name, "EventLevel")) {
        char val[256] = {0};
        ULONG val_len = sizeof(val);
        if (MTALog_GetParamStringValue(hInsContext, (char*)short_name, val, &val_len) == 0) {
            rbusValue_SetString(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "Index") ||
        STR_EQ(short_name, "EventID")) {
        ULONG val = 0;
        if (MTALog_GetParamUlongValue(hInsContext, (char*)short_name, &val)) {
            rbusValue_SetUInt32(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * GET handler for Device.X_CISCO_COM_MTA.DECTLog.{i}.* parameters
 */
static rbusError_t get_dectlog(const char *short_name, rbusValue_t *data, int instance_num)
{
    if (!data || !*data) {
        MTA_LOG_ERROR("get_dectlog: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    ANSC_HANDLE hInsContext = get_dectlog_instance(instance_num);
    if (!hInsContext && instance_num > 0) {
        MTA_LOG_ERROR("get_dectlog: Invalid instance %d", instance_num);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    if (STR_EQ(short_name, "Time") ||
        STR_EQ(short_name, "Description")) {
        char val[256] = {0};
        ULONG val_len = sizeof(val);
        if (DECTLog_GetParamStringValue(hInsContext, (char*)short_name, val, &val_len) == 0) {
            rbusValue_SetString(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "Index") ||
        STR_EQ(short_name, "EventID") ||
        STR_EQ(short_name, "EventLevel")) {
        ULONG val = 0;
        if (DECTLog_GetParamUlongValue(hInsContext, (char*)short_name, &val)) {
            rbusValue_SetUInt32(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * GET handler for Device.X_CISCO_COM_MTA.Battery.* parameters
 */
static rbusError_t get_battery(const char *short_name, rbusValue_t *data)
{
    if (!data || !*data) {
        MTA_LOG_ERROR("get_battery: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }

    if (STR_EQ(short_name, "PowerStatus") ||
        STR_EQ(short_name, "Condition") ||
        STR_EQ(short_name, "Status") ||
        STR_EQ(short_name, "Life") ||
        STR_EQ(short_name, "ModelNumber") ||
        STR_EQ(short_name, "SerialNumber") ||
        STR_EQ(short_name, "PartNumber") ||
        STR_EQ(short_name, "ChargerFirmwareRevision")){
        char val[256] = {0};
        ULONG val_len = sizeof(val);
        if (Battery_GetParamStringValue(NULL, (char*)short_name, val, &val_len) == 0) {
            rbusValue_SetString(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "TotalCapacity") ||
        STR_EQ(short_name, "ActualCapacity") ||
        STR_EQ(short_name, "RemainingCharge") ||
        STR_EQ(short_name, "RemainingTime") ||
        STR_EQ(short_name, "NumberofCycles")) {
        ULONG val = 0;
        if (Battery_GetParamUlongValue(NULL, (char*)short_name, &val)) {
            rbusValue_SetUInt32(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "Installed")) {
        BOOL val = FALSE;
        if (Battery_GetParamBoolValue(NULL, (char*)short_name, &val)) {
            rbusValue_SetBoolean(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * GET handler for Device.VoiceService.* parameters
 */
static rbusError_t get_voiceservice(const char *short_name, rbusValue_t *data)
{
    if (!data || !*data) {
        MTA_LOG_ERROR("get_voiceservice: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    if (STR_EQ(short_name, "Data")) {
        char val[2048] = {0};
        ULONG val_len = sizeof(val);
        if (VoiceService_GetParamStringValue(NULL, "Data", val, &val_len) == 0) {
            rbusValue_SetString(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * GET handler for Device.X_RDKCENTRAL-COM_MTA.* parameters
 */
static rbusError_t get_x_rdkcentral_com_mta(const char *short_name, rbusValue_t *data)
{
    if (!data || !*data) {
        MTA_LOG_ERROR("get_x_rdkcentral_com_mta: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    if (STR_EQ(short_name, "Ipv4DhcpStatus") ||
        STR_EQ(short_name, "Ipv6DhcpStatus") ||
        STR_EQ(short_name, "ConfigFileStatus")){
        ULONG val = 0;
        if (X_RDKCENTRAL_COM_MTA_GetParamUlongValue(NULL, (char*)short_name, &val)) {
            rbusValue_SetUInt32(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "LineRegisterStatus")) {
        char val[256] = {0};
        ULONG val_len = sizeof(val);
        if (X_RDKCENTRAL_COM_MTA_GetParamStringValue(NULL, (char*)short_name, val, &val_len) == 0) {
            rbusValue_SetString(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "pktcMtaDevResetNow")) {
        BOOL val = FALSE;
        if (X_RDKCENTRAL_COM_MTA_GetParamBoolValue(NULL, "pktcMtaDevResetNow", &val)) {
            rbusValue_SetBoolean(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * GET handler for Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.* parameters
 */
static rbusError_t get_x_rdkcentral_com_ethernetwan_mta(const char *short_name, rbusValue_t *data)
{
    if (!data || !*data) {
        MTA_LOG_ERROR("get_x_rdkcentral_com_ethernetwan_mta: NULL data pointer");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    if (STR_EQ(short_name, "StartupIPMode")) {
        int val = 0;
        if (EthernetWAN_MTA_GetParamIntValue(NULL, (char*)short_name, &val)) {
            rbusValue_SetInt32(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "IPv4PrimaryDhcpServerOptions") ||
        STR_EQ(short_name, "IPv4SecondaryDhcpServerOptions") ||
        STR_EQ(short_name, "IPv6PrimaryDhcpServerOptions") ||
        STR_EQ(short_name, "IPv6SecondaryDhcpServerOptions")) {
        char val[256] = {0};
        ULONG val_len = sizeof(val);
        if (EthernetWAN_MTA_GetParamStringValue(NULL, (char*)short_name, val, &val_len) == 0) {
            rbusValue_SetString(*data, val);
            return RBUS_ERROR_SUCCESS;
        }
    }
    
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/* ==================== Namespace SET Dispatchers ==================== */

/**
 * SET handler for Device.X_CISCO_COM_MTA.* parameters
 */
static rbusError_t set_x_cisco_com_mta(const char *short_name, rbusValue_t value)
{
    if (STR_EQ(short_name, "pktcMtaDevEnabled") ||
        STR_EQ(short_name, "ClearDSXLog") ||
        STR_EQ(short_name, "DSXLogEnable") ||
        STR_EQ(short_name, "CallSignallingLogEnable") ||
        STR_EQ(short_name, "ClearCallSignallingLog") ||
        STR_EQ(short_name, "EnableDECTLog") ||
        STR_EQ(short_name, "EnableMTALog")) {
        BOOL val = rbusValue_GetBoolean(value);
        if (X_CISCO_COM_MTA_SetParamBoolValue(NULL, (char*)short_name, val)) {
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "pktcSigDefCallSigTos") ||
        STR_EQ(short_name, "pktcSigDefMediaStreamTos") ||
        STR_EQ(short_name, "ClearLineStats")) {
        ULONG val = rbusValue_GetUInt32(value);
        if (X_CISCO_COM_MTA_SetParamUlongValue(NULL, (char*)short_name, val)) {
            return RBUS_ERROR_SUCCESS;
        }
    }

    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * SET handler for Device.X_CISCO_COM_MTA.LineTable.{i}.* parameters
 */
static rbusError_t set_line_table(const char *short_name, rbusValue_t value)
{    if (STR_EQ(short_name, "TriggerDiagnostics")) {
        BOOL val = rbusValue_GetBoolean(value);
        if (LineTable_SetParamBoolValue(NULL, "TriggerDiagnostics", val)) {
            return RBUS_ERROR_SUCCESS;
        }
    }
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * SET handler for Device.X_CISCO_COM_MTA.LineTable.{i}.VQM.* parameters
 */
static rbusError_t set_line_table_vqm(const char *short_name, rbusValue_t value)
{    if (STR_EQ(short_name, "ResetStats")) {
        BOOL val = rbusValue_GetBoolean(value);
        if (VQM_SetParamBoolValue(NULL, "ResetStats", val)) {
            return RBUS_ERROR_SUCCESS;
        }
    }
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * SET handler for Device.X_CISCO_COM_MTA.Dect.* parameters
 */
static rbusError_t set_dect(const char *short_name, rbusValue_t value)
{    if (STR_EQ(short_name, "Enable") ||
        STR_EQ(short_name, "RegistrationMode")){
        BOOL val = rbusValue_GetBoolean(value);
        if (Dect_SetParamBoolValue(NULL, "Enable", val)) {
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "RegisterDectHandset") ||
        STR_EQ(short_name, "DeregisterDectHandset")) {
        ULONG val = rbusValue_GetUInt32(value);
        if (Dect_SetParamUlongValue(NULL, (char*)short_name, val)) {
            return RBUS_ERROR_SUCCESS;
        }
    }
    else if (STR_EQ(short_name, "PIN")) {
        char val[256] = {0};
        rbusValue_GetString(value, NULL);
        if (Dect_SetParamStringValue(NULL, "PIN", val)) {
            return RBUS_ERROR_SUCCESS;
        }
    }
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * SET handler for Device.X_RDKCENTRAL-COM_MTA.Dect.Handsets.{i}.* parameters
 */
static rbusError_t set_dect_handsets(const char *short_name, rbusValue_t value)
{
    if (STR_EQ(short_name, "OperatingTN")) {
        char val[256] = {0};
        rbusValue_GetString(value, NULL);
        if (Handsets_SetParamStringValue(NULL, "OperatingTN", val)) {
            return RBUS_ERROR_SUCCESS;
        }
    }
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * SET handler for Device.X_RDKCENTRAL-COM_MTA.* parameters
 */
static rbusError_t set_x_rdkcentral_com_mta(const char *short_name, rbusValue_t value)
{    if (STR_EQ(short_name, "pktcMtaDevResetNow")) {
        BOOL val = rbusValue_GetBoolean(value);
        if (X_RDKCENTRAL_COM_MTA_SetParamBoolValue(NULL, "pktcMtaDevResetNow", val)) {
            return RBUS_ERROR_SUCCESS;
        }
    }
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * SET handler for Device.VoiceService.* parameters
 */
static rbusError_t set_voiceservice(const char *short_name, rbusValue_t value)
{
    if (STR_EQ(short_name, "Data")) {
        char val[2048] = {0};
        rbusValue_GetString(value, NULL);
        if (VoiceService_SetParamStringValue(NULL, "Data", val)) {
            return RBUS_ERROR_SUCCESS;
        }
    }
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/**
 * SET handler for Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.* parameters
 */
static rbusError_t set_x_rdkcentral_com_ethernetwan_mta(const char *short_name, rbusValue_t value)
{    if (STR_EQ(short_name, "StartupIPMode")) {
        int val = rbusValue_GetInt32(value);
        if (EthernetWAN_MTA_SetParamIntValue(NULL, (char*)short_name, val)) {
            return RBUS_ERROR_SUCCESS;
        }
    }
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

/* ==================== Main RBUS GET Handler ==================== */

rbusError_t mta_rbus_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    
    const char* param_name = rbusProperty_GetName(property);
    if (!param_name) {
        MTA_LOG_ERROR("mta_rbus_get_handler: NULL parameter name");
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    MTA_LOG_INFO("=== GET HANDLER START ===");
    MTA_LOG_INFO("GET request for: %s", param_name);
    
    /* Check if we received a wildcard path instead of concrete instance */
    if (strstr(param_name, "{i}")) {
        MTA_LOG_ERROR("CRITICAL: Received wildcard path with {i} instead of instance number: %s", param_name);
        MTA_LOG_ERROR("This means table instances were not registered properly with RBUS");
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    
    mta_param_metadata_t* metadata = mta_find_param_metadata(param_name);
    if (!metadata) {
        MTA_LOG_ERROR("mta_rbus_get_handler: Parameter not found in metadata: %s", param_name);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    
    rbusValue_t value = rbusValue_Init(NULL);
    if (!value) {
        MTA_LOG_ERROR("mta_rbus_get_handler: Failed to initialize rbus value");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    /* Extract instance number(s) for table parameters */
    int instance_num = extract_instance_number(param_name);
    int line_instance = -1;
    int calls_instance = -1;
    
    MTA_LOG_DEBUG("Namespace type: %d", metadata->namespace_type);
    MTA_LOG_DEBUG("Extracted instance_num: %d from path: %s", instance_num, param_name);
    
    /* For nested tables, extract both instance numbers */
    if (metadata->namespace_type == MTA_NAMESPACE_X_CISCO_COM_MTA_LINETABLE_VQM_CALLS) {
        if (!extract_nested_instances(param_name, &line_instance, &calls_instance)) {
            MTA_LOG_ERROR("mta_rbus_get_handler: Failed to extract nested instances from %s", param_name);
            rbusValue_Release(value);
            return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
        }
        MTA_LOG_INFO("Nested table query: LineTable.%d.VQM.Calls.%d", line_instance, calls_instance);
    }
    
    rbusError_t rc = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    
    switch (metadata->namespace_type) {
        case MTA_NAMESPACE_X_CISCO_COM_MTA:
            rc = get_x_cisco_com_mta(metadata->short_name, &value);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_V6:
            rc = get_x_cisco_com_mta_v6(metadata->short_name, &value);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_LINETABLE:
            rc = get_line_table(metadata->short_name, &value, instance_num);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_LINETABLE_CALLP:
            rc = get_line_table_callp(metadata->short_name, &value, instance_num);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_LINETABLE_VQM:
            rc = get_line_table_vqm(metadata->short_name, &value, instance_num);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_LINETABLE_VQM_CALLS:
            rc = get_line_table_vqm_calls(metadata->short_name, &value, line_instance, calls_instance);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_SERVICECLASS:
            rc = get_service_class(metadata->short_name, &value, instance_num);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_SERVICEFLOW:
            rc = get_service_flow(metadata->short_name, &value, instance_num);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_DECT:
            rc = get_dect(metadata->short_name, &value);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_DECT_HANDSETS:
            rc = get_dect_handsets(metadata->short_name, &value, instance_num);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_DSXLOG:
            rc = get_dsxlog(metadata->short_name, &value, instance_num);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_MTALOG:
            rc = get_mtalog(metadata->short_name, &value, instance_num);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_DECTLOG:
            rc = get_dectlog(metadata->short_name, &value, instance_num);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_BATTERY:
            rc = get_battery(metadata->short_name, &value);
            break;
        case MTA_NAMESPACE_VOICESERVICE:
            rc = get_voiceservice(metadata->short_name, &value);
            break;
        case MTA_NAMESPACE_X_RDKCENTRAL_COM_MTA:
            rc = get_x_rdkcentral_com_mta(metadata->short_name, &value);
            break;
        case MTA_NAMESPACE_X_RDKCENTRAL_COM_ETHERNETWAN_MTA:
            rc = get_x_rdkcentral_com_ethernetwan_mta(metadata->short_name, &value);
            break;
        default:
            MTA_LOG_ERROR("mta_rbus_get_handler: Unsupported namespace for %s", param_name);
            rc = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
            break;
    }
    
    if (rc == RBUS_ERROR_SUCCESS) {
        rbusProperty_SetValue(property, value);
        MTA_LOG_INFO("GET successful for: %s", param_name);
    } else {
        MTA_LOG_ERROR("GET failed for: %s (error: %d)", param_name, rc);
    }
    
    rbusValue_Release(value);
    return rc;
}

/* ==================== Main RBUS SET Handler ==================== */

rbusError_t mta_rbus_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    
    const char* param_name = rbusProperty_GetName(property);
    if (!param_name) {
        MTA_LOG_ERROR("mta_rbus_set_handler: NULL parameter name");
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    MTA_LOG_INFO("SET request for: %s", param_name);
    
    mta_param_metadata_t* metadata = mta_find_param_metadata(param_name);
    if (!metadata) {
        MTA_LOG_ERROR("mta_rbus_set_handler: Parameter not found in metadata: %s", param_name);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    
    if (!metadata->writable) {
        MTA_LOG_ERROR("mta_rbus_set_handler: Parameter is read-only: %s", param_name);
        return RBUS_ERROR_ACCESS_NOT_ALLOWED;
    }
    
    rbusValue_t value = rbusProperty_GetValue(property);
    if (!value) {
        MTA_LOG_ERROR("mta_rbus_set_handler: NULL value for %s", param_name);
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    rbusError_t rc = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    
    switch (metadata->namespace_type) {
        case MTA_NAMESPACE_X_CISCO_COM_MTA:
            rc = set_x_cisco_com_mta(metadata->short_name, value);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_LINETABLE:
            rc = set_line_table(metadata->short_name, value);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_LINETABLE_VQM:
            rc = set_line_table_vqm(metadata->short_name, value);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_DECT:
            rc = set_dect(metadata->short_name, value);
            break;
        case MTA_NAMESPACE_X_CISCO_COM_MTA_DECT_HANDSETS:
            rc = set_dect_handsets(metadata->short_name, value);
            break;
        case MTA_NAMESPACE_VOICESERVICE:
            rc = set_voiceservice(metadata->short_name, value);
            break;
        case MTA_NAMESPACE_X_RDKCENTRAL_COM_MTA:
            rc = set_x_rdkcentral_com_mta(metadata->short_name, value);
            break;
        case MTA_NAMESPACE_X_RDKCENTRAL_COM_ETHERNETWAN_MTA:
            rc = set_x_rdkcentral_com_ethernetwan_mta(metadata->short_name, value);
            break;
        default:
            MTA_LOG_ERROR("mta_rbus_set_handler: Unsupported namespace for SET on %s", param_name);
            rc = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
            break;
    }
    
    if (rc == RBUS_ERROR_SUCCESS) {
        MTA_LOG_INFO("SET successful for: %s", param_name);
    } else {
        MTA_LOG_ERROR("SET failed for: %s (error: %d)", param_name, rc);
    }
    
    return rc;
}

/* ==================== RBUS Event Handler ==================== */

rbusError_t mta_rbus_event_handler(rbusHandle_t handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription)
{
    (void)handle;
    (void)event;
    (void)subscription;
    
    MTA_LOG_INFO("mta_rbus_event_handler called");
    return RBUS_ERROR_SUCCESS;
}

/* ==================== RBUS Initialization and Registration ==================== */

void mta_register_callbacks_to_metadata(mta_param_metadata_t *metadata, const char *param_name)
{
    if (!metadata) {
        MTA_LOG_ERROR("mta_register_callbacks_to_metadata: NULL metadata");
        return;
    }
    
    MTA_LOG_INFO("Registered callbacks for parameter: %s", param_name);
}

int mta_rbus_init(const char *component_name)
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    
    if (!component_name) {
        MTA_LOG_ERROR("mta_rbus_init: NULL component name");
        return -1;
    }
    
    MTA_LOG_INFO("Initializing RBUS for component: %s", component_name);
    
    rc = rbus_open(&g_mta_rbus_handle, component_name);
    if (rc != RBUS_ERROR_SUCCESS) {
        MTA_LOG_ERROR("mta_rbus_init: rbus_open failed with error %d", rc);
        g_mta_rbus_handle = NULL;
        return -1;
    }
    
    /* Create and Intialize DML data structures (replaces COSA_Init)*/
    if (!g_pCosaBEManager)
    {
        MTA_LOG_INFO("Creating MTA data model...\n");
        g_pCosaBEManager = CosaBackEndManagerCreate();
        if (!g_pCosaBEManager ||!g_pCosaBEManager->Initialize)
        {
            MTA_LOG_ERROR("Failed to create MTA data model");
            rbus_close(g_mta_rbus_handle);
            g_mta_rbus_handle = NULL;
            return -1;
        }
         MTA_LOG_INFO("MTA data model created successfully");
         g_pCosaBEManager->Initialize   ((ANSC_HANDLE)g_pCosaBEManager);
    }
    
    /* Decode and register elements from JSON config */
    if (mta_decode_json_config(g_mta_rbus_handle, JSON_CONFIG_PATH) != 0) {
        MTA_LOG_ERROR("mta_rbus_init: Failed to decode JSON config");
        rbus_close(g_mta_rbus_handle);
        g_mta_rbus_handle = NULL;
        return -1;
    }
    
    MTA_LOG_INFO("MTA RBUS initialization complete");
    return 0;
}

void mta_rbus_terminate(void)
{
    if (g_mta_rbus_handle) {
        MTA_LOG_INFO("Terminating RBUS");
        mta_free_registered_elements();
        mta_free_param_metadata();
        rbus_close(g_mta_rbus_handle);
        g_mta_rbus_handle = NULL;
        MTA_LOG_INFO("RBUS terminated");
    }
}
