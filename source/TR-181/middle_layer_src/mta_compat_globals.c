/*
 * Global variable definitions for CCSP compatibility
 * This file defines the global variables that are declared as extern in mta_compat_types.h
 */

#include "mta_compat_types.h"
#include <stdio.h>
#include <string.h>

/* CCSP logging global variables */
int g_iTraceLevel = 6; /* Default to INFO level (CCSP_TRACE_LEVEL_INFO) */
char *pComponentName = "CcspMtaAgent"; /* Component name for logging */

/* MTA-specific trace globals */
FILE* g_mta_logfile = NULL;
int g_mta_trace_level = 6;

/* DML update tracking globals */
ULONG g_currentWriteEntity = 0;
ULONG g_currentBsUpdate = 0;

/* Partner ID retrieval implementation */
int getPartnerId (char *partnerID)
{
	char buffer[64];
	FILE *file;
	char *pos = NULL;

	if ((file = popen ("syscfg get PartnerID", "r")) != NULL)
	{
		pos = fgets (buffer, sizeof(buffer), file);
		pclose (file);
	}

	if ((pos == NULL) && ((file = popen ("/lib/rdk/getpartnerid.sh GetPartnerID", "r")) != NULL))
	{
		pos = fgets (buffer, sizeof(buffer), file);
		pclose (file);
	}

	if (pos)
	{
		size_t len = strlen (pos);

		if ((len > 0) && (pos[len - 1] == '\n'))
		{
			len--;
		}

		memcpy (partnerID, pos, len);
		partnerID[len] = 0;

		return CCSP_SUCCESS;
	}

	CcspTraceInfo(("%s : Error in opening File\n", __FUNCTION__));

	*partnerID = 0;

	return CCSP_FAILURE;
}
