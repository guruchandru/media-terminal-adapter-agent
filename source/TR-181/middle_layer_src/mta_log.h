/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
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

#ifndef _MTA_LOG_H_
#define _MTA_LOG_H_

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/*
 * MTA Logging Infrastructure
 * Provides CCSP-compatible logging without common-library dependency
 * Based on advsec_jan16 logging implementation
 */

/* Global log file handle */
extern FILE* g_mta_logfile;

/* Global trace level */
extern int g_mta_trace_level;

/* CCSP Trace Levels */
#ifndef CCSP_TRACE_LEVEL_EMERGENCY
#define CCSP_TRACE_LEVEL_EMERGENCY     0
#define CCSP_TRACE_LEVEL_ALERT         1
#define CCSP_TRACE_LEVEL_CRITICAL      2
#define CCSP_TRACE_LEVEL_ERROR         3
#define CCSP_TRACE_LEVEL_WARNING       4
#define CCSP_TRACE_LEVEL_NOTICE        5
#define CCSP_TRACE_LEVEL_INFO          6
#define CCSP_TRACE_LEVEL_DEBUG         7
#endif

/* Trace level names */
static const char* g_mta_TraceLevelStr[] = {
    "EMERGENCY", "ALERT", "CRITICAL", "ERROR", 
    "WARNING", "NOTICE", "INFO", "DEBUG"
};

/* Initialize logging system */
static inline void mta_trace_init(const char* component_name) {
    const char* log_file = getenv("MTA_LOG_FILE");
    const char* trace_level = getenv("MTA_TRACE_LEVEL");
    
    if (log_file) {
        g_mta_logfile = fopen(log_file, "a");
        if (g_mta_logfile) {
            setvbuf(g_mta_logfile, NULL, _IOLBF, 0); /* Line buffered */
        }
    }
    
    if (trace_level) {
        g_mta_trace_level = atoi(trace_level);
        if (g_mta_trace_level < 0) g_mta_trace_level = 0;
        if (g_mta_trace_level > 7) g_mta_trace_level = 7;
    }
    
    fprintf(stderr, "MTA %s initialized with trace level %d (%s)\n", 
           component_name ? component_name : "Component", 
           g_mta_trace_level,
           g_mta_TraceLevelStr[g_mta_trace_level]);
}

/* Enhanced logging function with timestamps */
static inline void mta_log_write_with_level(int level, const char* format, ...) {
    if (level > g_mta_trace_level || level < 0 || level > 7) return;
    
    va_list args;
    time_t rawtime;
    struct tm* timeinfo;
    char timestamp[128];
    
    /* Get current timestamp - matching CCSP format */
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    snprintf(timestamp, sizeof(timestamp), "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d",
            timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    
    /* Print in CCSP format: timestamp-component-level-message */
    fprintf(stderr, "%s-MTA-%s-", timestamp, g_mta_TraceLevelStr[level]);
    
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    /* Also write to log file if available */
    if (g_mta_logfile) {
        fprintf(g_mta_logfile, "%s-MTA-%s-", timestamp, g_mta_TraceLevelStr[level]);
        va_start(args, format);
        vfprintf(g_mta_logfile, format, args);
        va_end(args);
        fflush(g_mta_logfile);
    }
}

/* CCSP-compatible logging function - writes to BOTH stderr AND log file */
static inline void mta_ccsp_log_write(int level, const char* format, ...) {
    extern int g_iTraceLevel;
    extern char *pComponentName;
    
    if (level > g_iTraceLevel || level < 0 || level > 7) return;
    
    va_list args, args_copy;
    time_t rawtime;
    struct tm* timeinfo;
    char timestamp[80];
    
    /* Get current timestamp - matching CCSP format */
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    snprintf(timestamp, sizeof(timestamp), "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d",
            timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    
    /* IMPORTANT: Write to BOTH stderr (preserving existing logs) AND log file */
    
    /* 1. Write to stderr (original behavior - NEVER CHANGE THIS) */
    fprintf(stderr, "%s-%s-%s-", timestamp, pComponentName ? pComponentName : "MTA", g_mta_TraceLevelStr[level]);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    /* 2. ALSO write to log file if available (new feature) */
    if (g_mta_logfile) {
        fprintf(g_mta_logfile, "%s-%s-%s-", timestamp, pComponentName ? pComponentName : "MTA", g_mta_TraceLevelStr[level]);
        va_start(args_copy, format);
        vfprintf(g_mta_logfile, format, args_copy);
        va_end(args_copy);
        fflush(g_mta_logfile);
    }
}

/* CCSP Trace Macros - Exact format matching common-library ccsp_trace.h
 * These macros expect double parentheses: CcspTraceError(("message %s", arg))
 */

/* Helper macro to extract arguments from parentheses */
#define MTA_EXTRACT_ARGS(...) __VA_ARGS__

/* Macro to extract arguments from double parentheses and execute trace */
#define mta_ccsp_trace_exec(level, msg) \
    do { \
        extern int g_iTraceLevel; \
        if ((level) <= g_iTraceLevel) { \
            mta_ccsp_log_write((level), MTA_EXTRACT_ARGS msg); \
        } \
    } while(0)

#ifndef CcspTraceError
#define CcspTraceError(msg) mta_ccsp_trace_exec(CCSP_TRACE_LEVEL_ERROR, msg)
#endif

#ifndef CcspTraceWarning  
#define CcspTraceWarning(msg) mta_ccsp_trace_exec(CCSP_TRACE_LEVEL_WARNING, msg)
#endif

#ifndef CcspTraceInfo
#define CcspTraceInfo(msg) mta_ccsp_trace_exec(CCSP_TRACE_LEVEL_INFO, msg)
#endif

#ifndef CcspTraceDebug
#define CcspTraceDebug(msg) mta_ccsp_trace_exec(CCSP_TRACE_LEVEL_DEBUG, msg)
#endif

#ifndef CcspTraceNotice
#define CcspTraceNotice(msg) mta_ccsp_trace_exec(CCSP_TRACE_LEVEL_NOTICE, msg)
#endif

#ifndef CcspTraceCritical
#define CcspTraceCritical(msg) mta_ccsp_trace_exec(CCSP_TRACE_LEVEL_CRITICAL, msg)
#endif

#ifndef CcspTraceAlert
#define CcspTraceAlert(msg) mta_ccsp_trace_exec(CCSP_TRACE_LEVEL_ALERT, msg)
#endif

#ifndef CcspTraceEmergency
#define CcspTraceEmergency(msg) mta_ccsp_trace_exec(CCSP_TRACE_LEVEL_EMERGENCY, msg)
#endif

/* Additional ANSC trace/debug functions */
#ifndef AnscTraceWarning
#define AnscTraceWarning(msg) CcspTraceWarning(msg)
#endif

#ifndef AnscSetTraceLevel
#define AnscSetTraceLevel(level) do { \
    extern int g_iTraceLevel; \
    extern int g_mta_trace_level; \
    g_mta_trace_level = (level); \
    g_iTraceLevel = (level); \
    if (g_mta_trace_level < 0) g_mta_trace_level = 0; \
    if (g_mta_trace_level > 7) g_mta_trace_level = 7; \
    if (g_iTraceLevel < 0) g_iTraceLevel = 0; \
    if (g_iTraceLevel > 7) g_iTraceLevel = 7; \
} while(0)
#endif

#ifndef AnscGetTraceLevel
#define AnscGetTraceLevel() (g_iTraceLevel)
#endif

/* Cleanup function */
static inline void mta_trace_cleanup(void) {
    if (g_mta_logfile) {
        fflush(g_mta_logfile);
        fclose(g_mta_logfile);
        g_mta_logfile = NULL;
    }
}

#endif /* _MTA_LOG_H_ */
