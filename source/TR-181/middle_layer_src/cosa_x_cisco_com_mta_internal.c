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

// #ifdef CONFIG_TI_PACM
/**************************************************************************

    module: cosa_x_cisco_com_mta_internal.c

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    description:

        This file implementes back-end apis for the COSA Data Model Library

        *  CosaMTACreate
        *  CosaMTAInitialize
        *  CosaMTARemove
    -------------------------------------------------------------------

    environment:

        platform independent

    -------------------------------------------------------------------

    author:

        COSA XML TOOL CODE GENERATOR 1.0

    -------------------------------------------------------------------

    revision:

        01/11/2011    initial revision.

**************************************************************************/

#include "plugin_main_apis.h"
/* Legacy common-library includes commented out for JSON-driven RBUS approach */
/* #include "safec_lib_common.h" */
#include <safec_lib.h>
#include "mta_compat_types.h"  /* Include compatibility layer */
#include "cosa_x_cisco_com_mta_apis.h"
#include "cosa_x_cisco_com_mta_dml.h"
#include "cosa_x_cisco_com_mta_internal.h"
#include "mta_hal.h"
#include <sysevent/sysevent.h>
#include "syscfg/syscfg.h"

#define MAX_BUFF_SIZE 128
#define MAX_IP_PREF_VAL 6
#define MAX_IPV4_HEX_VAL 16
#define MAX_IPV6_HEX_VAL 65

static int sysevent_fd;
static token_t sysevent_token;
/**********************************************************************

    caller:     owner of the object

    prototype:

        ANSC_HANDLE
        CosaMTACreate
            (
            );

    description:

        This function constructs cosa device info object and return handle.

    argument:  

    return:     newly created device info object.

**********************************************************************/
/* Coverity Fix CID 66908 - NonStd_void_param_list */
ANSC_HANDLE
CosaMTACreate
    (
     void
    )
{
    PCOSA_DATAMODEL_MTA          pMyObject    = (PCOSA_DATAMODEL_MTA)NULL;

    /*
     * We create object by first allocating memory for holding the variables and member functions.
     */
    pMyObject = (PCOSA_DATAMODEL_MTA)AnscAllocateMemory(sizeof(COSA_DATAMODEL_MTA));

    if ( !pMyObject )
    {
        return  (ANSC_HANDLE)NULL;
    }

    /*
     * Initialize the common variables and functions for a container object.
     */
    pMyObject->Oid               = COSA_DATAMODEL_MTA_OID;
    pMyObject->Create            = CosaMTACreate;
    pMyObject->Remove            = CosaMTARemove;
    pMyObject->Initialize        = CosaMTAInitialize;

    pMyObject->Initialize   ((ANSC_HANDLE)pMyObject);

    return  (ANSC_HANDLE)pMyObject;
}

ANSC_STATUS ConverStr2Hex(unsigned char buffer[])
{
       /* Coverity Fix :CID 70174 Declare and Never Used */
	int i = 0, len = 0;
	char 	tbuffer [ MAX_IPV6_HEX_VAL ] = { 0 };
        errno_t rc = -1;
			len = strlen((const char*)buffer);
                        rc = strcpy_s(tbuffer,sizeof(tbuffer), (const char*)buffer);
                        if(rc != EOK)
                        {
                            ERR_CHK(rc);
                            return ANSC_STATUS_FAILURE;
                        }
			printf("len = %d\n",len);
			for(i = 0; i<len; i++)
			{
				if((buffer[i] >= 48) && (buffer[i] <= 57))
				{
					buffer[i] = buffer[i]-48;
				}
				else if((buffer[i] >= 65) && (buffer[i] <= 70))
				{
					buffer[i] = buffer[i]-55;
				}
				else if((buffer[i] >= 97) && (buffer[i] <= 102))
				{
					buffer[i] = buffer[i]-87;
				}
				else
				{
					printf("buffer = %s is not correct\n",tbuffer);
					CcspTraceError(("Unsupported format %s %s\n", tbuffer,__FUNCTION__));
					return ANSC_STATUS_FAILURE;
				}
			}
	return ANSC_STATUS_SUCCESS;

}

ANSC_STATUS
CosaMTAInitializeEthWanProvDhcpOption
    (
        ANSC_HANDLE                 hThisObject
    )
{
	 MTA_IP_TYPE_TR ip_type;
	 char 	buffer [ MAX_BUFF_SIZE ] = { 0 };
	 int	MtaIPMode = 0,IP_Pref_Mode_Received=0;
	 int i = 0, j =0; int len = 0;
         errno_t rc = -1;
         unsigned char x,y;
	 PMTAMGMT_MTA_PROVISIONING_PARAMS pMtaProv = NULL;
	 pMtaProv = (PMTAMGMT_MTA_PROVISIONING_PARAMS)malloc(sizeof(MTAMGMT_PROVISIONING_PARAMS));
         
        if(pMtaProv == NULL)
        {
		printf("Memory Alloction Failed '%s'\n", __FUNCTION__);
		CcspTraceError(("Memory Alloction Failed '%s'\n", __FUNCTION__));
		return ANSC_STATUS_FAILURE;
	} 

	 PCOSA_DATAMODEL_MTA      pMyObject    = (PCOSA_DATAMODEL_MTA)hThisObject;
         /*Coverity Fix CID 120992  NULL Check */
	 if((pMyObject->pmtaprovinfo = (PCOSA_MTA_ETHWAN_PROV_INFO)AnscAllocateMemory(sizeof(COSA_MTA_ETHWAN_PROV_INFO))) == NULL)
         {
             CcspTraceWarning(("%s:pMyObject->pmtaprovinfo attained NULL\n",__FUNCTION__));
              free(pMtaProv);
             return ANSC_STATUS_FAILURE;
         }    

	 char Ip_Pref [MAX_IP_PREF_VAL] = { 0 }, Ipv4_Primary[MAX_IPV4_HEX_VAL] = {0}, Ipv4_Secondary[MAX_IPV4_HEX_VAL] = {0}, Ipv6_Primary[MAX_IPV6_HEX_VAL] = {0} , Ipv6_Secondary[MAX_IPV6_HEX_VAL] = {0};

         CosaMTAInitializeEthWanProvJournal(pMyObject->pmtaprovinfo);

	sysevent_get(sysevent_fd, sysevent_token, "MTA_IP_PREF", Ip_Pref, sizeof(Ip_Pref));
	sysevent_get(sysevent_fd, sysevent_token, "MTA_DHCPv4_PrimaryAddress", Ipv4_Primary, sizeof(Ipv4_Primary));
	sysevent_get(sysevent_fd, sysevent_token, "MTA_DHCPv4_SecondaryAddress", Ipv4_Secondary, sizeof(Ipv4_Secondary));
	sysevent_get(sysevent_fd, sysevent_token, "MTA_DHCPv6_PrimaryAddress", Ipv6_Primary, sizeof(Ipv6_Primary));
	sysevent_get(sysevent_fd, sysevent_token, "MTA_DHCPv6_SecondaryAddress", Ipv6_Secondary, sizeof(Ipv6_Secondary));

    	if ( Ip_Pref[MAX_IP_PREF_VAL-1] != '\0' )
        	Ip_Pref[MAX_IP_PREF_VAL-1] = '\0' ;

    	if ( Ipv4_Primary[MAX_IPV4_HEX_VAL-1] != '\0' )
        	Ipv4_Primary[MAX_IPV4_HEX_VAL-1] = '\0' ;

    	if ( Ipv4_Secondary[MAX_IPV4_HEX_VAL-1] != '\0' )
        	Ipv4_Secondary[MAX_IPV4_HEX_VAL-1] = '\0' ;

    	if ( Ipv6_Primary[MAX_IPV6_HEX_VAL-1] != '\0' )
        	Ipv6_Primary[MAX_IPV6_HEX_VAL-1] = '\0' ;

    	if ( Ipv6_Secondary[MAX_IPV6_HEX_VAL-1] != '\0' )
        	Ipv6_Secondary[MAX_IPV6_HEX_VAL-1] = '\0' ;

    	CcspTraceInfo(("%s MTA values returned from dhcp server are \n",__FUNCTION__));

    	CcspTraceInfo(("%s MTA_IP_PREF = %s \n",__FUNCTION__,Ip_Pref));
    	CcspTraceInfo(("%s MTA_DHCPv4_PrimaryAddress = %s,MTA_DHCPv4_SecondaryAddress = %s  \n",__FUNCTION__,Ipv4_Primary,Ipv4_Secondary));
    	CcspTraceInfo(("%s MTA_DHCPv6_PrimaryAddress = %s,MTA_DHCPv6_SecondaryAddress = %s  \n",__FUNCTION__,Ipv6_Primary,Ipv6_Secondary));

	if ( ( 0 == strncmp(Ipv4_Primary,"00000000",8) ) || ( 0 == strncmp(Ipv6_Primary,"00000000",8) ) )
	{
	    CcspTraceWarning(("%s Received 0's from dhcp sever ,not initializing MTA \n",__FUNCTION__));
	    /* CID 120995 Resource leak */	
	    free(pMtaProv);
	    return ANSC_STATUS_FAILURE;
	} 
      
	if(Ip_Pref[0] != '\0') {
			sscanf( Ip_Pref, "%d", &IP_Pref_Mode_Received );
			if ( IP_Pref_Mode_Received == 01 )
				MtaIPMode = MTA_IPV4;
			else if ( IP_Pref_Mode_Received == 02 )
				MtaIPMode = MTA_IPV6;
			else if ( ( IP_Pref_Mode_Received == 05 ) || ( IP_Pref_Mode_Received == 06 ) )
				MtaIPMode = MTA_DUAL_STACK;
			else
			{
				CcspTraceWarning(("%s Value received from server is invalid , using default value \n",__FUNCTION__));

				if( 0 == syscfg_get( NULL, "StartupIPMode", buffer, sizeof(buffer)))
				{
				   if(buffer[0] != '\0')
				   {
						sscanf( buffer, "%d", &MtaIPMode );
	     				CcspTraceInfo(("%s Default MtaIPMode is %d \n",__FUNCTION__,MtaIPMode));
				   }
				}
			}

			CcspTraceInfo(("MtaIPMode = %d\n",MtaIPMode));
		}
		else
		{
			// deduce MtaIPMode value from dhcp server  if one of the ip is received , if both are received use default, if not received then use default
			if ( ( (Ipv4_Primary[0] != '\0')  && (Ipv6_Primary[0] != '\0') ) || ( (Ipv4_Primary[0] == '\0')  && (Ipv6_Primary[0] == '\0') ) )
			{

				if( 0 == syscfg_get( NULL, "StartupIPMode", buffer, sizeof(buffer)))
				{
				   if(buffer[0] != '\0')
				   {
						sscanf( buffer, "%d", &MtaIPMode );
	     				CcspTraceInfo(("%s Default MtaIPMode is %d \n",__FUNCTION__,MtaIPMode));
				   }
				}
		       }

			else if ( Ipv4_Primary[0] != '\0')
			{
	     		CcspTraceInfo(("%s Received only Ipv4_Primary from dhcp server , setting IP Preference mode to ipv4 \n",__FUNCTION__));
				MtaIPMode=MTA_IPV4;

			}
			else if ( Ipv6_Primary[0] != '\0')
			{
	     		CcspTraceInfo(("%s Received only Ipv6_Primary from dhcp server , setting IP Preference mode to ipv6  \n",__FUNCTION__));
				MtaIPMode=MTA_IPV6;
			}

		}

		pMtaProv->MtaIPMode = MtaIPMode;
		CosaDmlMTASetStartUpIpMode(pMyObject->pmtaprovinfo,MtaIPMode);
		ip_type = MTA_IPV4_TR;

		rc = memset_s(pMtaProv->DhcpOption122Suboption1, sizeof(pMtaProv->DhcpOption122Suboption1), 0, sizeof(pMtaProv->DhcpOption122Suboption1));
                ERR_CHK(rc);
		if (Ipv4_Primary[0] != '\0' )
			{
                                char* pIpv4Primary = Ipv4_Primary;
                                rc = strcpy_s(buffer, sizeof(buffer), pIpv4Primary);
                                if(rc != EOK)
                                {
                                    ERR_CHK(rc);
                                    /* Coverity Fix CID : 120995 RESOURCE_LEAK */
                                    if (pMtaProv)
                                    {
                                        free(pMtaProv);
                                    }
                                    return ANSC_STATUS_FAILURE;
                                }
			}
			else 
			{
				syscfg_get( NULL, "IPv4PrimaryDhcpServerOptions", buffer, sizeof(buffer));
			}	
				
				   if(buffer[0] != '\0')
				   {
					len = strlen(buffer);
					CosaDmlMTASetPrimaryDhcpServerOptions(pMyObject->pmtaprovinfo, buffer, ip_type);
					if((MtaIPMode ==  MTA_IPV4) || (MtaIPMode == MTA_DUAL_STACK))
				        {       
                                                /* Coverity Fix CID:121023 Incompatible type */
						if(ConverStr2Hex((unsigned char *)buffer) == ANSC_STATUS_SUCCESS)
                                                {
                                                        for(i = 0,j= 0;i<len; i++,j++)
							{
								if(j<MTA_DHCPOPTION122SUBOPTION1_MAX)
								{
                                                                      x = buffer[i]<<4;
                                                                      y = buffer[++i];
									pMtaProv->DhcpOption122Suboption1[j] |= x + y;
								}
								else
									break;
							}
	
							printf("pMtaProv->DhcpOption122Suboption1[0] = %X %d\n",pMtaProv->DhcpOption122Suboption1[0],pMtaProv->DhcpOption122Suboption1[0]);
							printf("pMtaProv->DhcpOption122Suboption1[1] = %X %d\n",pMtaProv->DhcpOption122Suboption1[1],pMtaProv->DhcpOption122Suboption1[1]);
							printf("pMtaProv->DhcpOption122Suboption1[2] = %X %d\n",pMtaProv->DhcpOption122Suboption1[2],pMtaProv->DhcpOption122Suboption1[2]);
							printf("pMtaProv->DhcpOption122Suboption1[3] = %X %d\n",pMtaProv->DhcpOption122Suboption1[3],pMtaProv->DhcpOption122Suboption1[3]);
						}
					}
				   }


			rc = memset_s(pMtaProv->DhcpOption122Suboption2, sizeof(pMtaProv->DhcpOption122Suboption2), 0, sizeof(pMtaProv->DhcpOption122Suboption2));
                        ERR_CHK(rc);
			if (Ipv4_Secondary[0] != '\0' )
			{
                               char* pIpv4Secondary = Ipv4_Secondary;
                               rc = strcpy_s(buffer, sizeof(buffer), pIpv4Secondary);
                               if(rc != EOK)
                               {
                                   ERR_CHK(rc);
                                   /* Coverity Fix CID : 120995 RESOURCE_LEAK */
                                   if (pMtaProv)
                                   {
                                       free(pMtaProv);
                                   }
                                   return ANSC_STATUS_FAILURE;
                               }
                        /* Coverity  fix CID : 340667 Unused Value */
                        //x=0;
                        //y=0;
			}
			else
			{
				syscfg_get( NULL, "IPv4SecondaryDhcpServerOptions", buffer, sizeof(buffer));
			}
				   
				   if(buffer[0] != '\0')
				   {
					len = strlen(buffer);
					CosaDmlMTASetSecondaryDhcpServerOptions(pMyObject->pmtaprovinfo, buffer, ip_type);
					if((MtaIPMode ==  MTA_IPV4) || (MtaIPMode == MTA_DUAL_STACK))
					{       /* Coverity Fix CID:121023 Incompatible type */
						if(ConverStr2Hex((unsigned char *)buffer) == ANSC_STATUS_SUCCESS)
						{
					   		for(i = 0,j= 0;i<len; i++,j++)
							{
								if(j<MTA_DHCPOPTION122SUBOPTION2_MAX)
								{
                                                                        x= buffer[i]<<4;
                                                                        y = buffer[++i];
									pMtaProv->DhcpOption122Suboption2[j] |= x + y;
								}
								else
									break;
							}
							printf("pMtaProv->DhcpOption122Suboption2[0] = %X %d\n",pMtaProv->DhcpOption122Suboption2[0],pMtaProv->DhcpOption122Suboption2[0]);
							printf("pMtaProv->DhcpOption122Suboption2[1] = %X %d\n",pMtaProv->DhcpOption122Suboption2[1],pMtaProv->DhcpOption122Suboption2[1]);
							printf("pMtaProv->DhcpOption122Suboption2[2] = %X %d\n",pMtaProv->DhcpOption122Suboption2[2],pMtaProv->DhcpOption122Suboption2[2]);
							printf("pMtaProv->DhcpOption122Suboption2[3] = %X %d\n",pMtaProv->DhcpOption122Suboption2[3],pMtaProv->DhcpOption122Suboption2[3]);
						}
					}

				   }
	    ip_type = MTA_IPV6_TR;

				rc = memset_s(pMtaProv->DhcpOption2171CccV6DssID1, sizeof(pMtaProv->DhcpOption2171CccV6DssID1), 0, sizeof(pMtaProv->DhcpOption2171CccV6DssID1));
                                ERR_CHK(rc);
            pMtaProv->DhcpOption2171CccV6DssID1Len = 0;
			if ( Ipv6_Primary[0] != '\0' ) 
			{
                                char* pIpv6Primary = Ipv6_Primary;
                                rc = strcpy_s(buffer, sizeof(buffer), pIpv6Primary);
                                if(rc != EOK)
                                {
                                    ERR_CHK(rc);
                                    /* Coverity Fix CID : 120995 RESOURCE_LEAK */
                                    if (pMtaProv)
                                    {
                                       free(pMtaProv);
                                    }
                                    return ANSC_STATUS_FAILURE;
                                }
                                /* Coverity  fix CID : 340667 Unused Value */
                                //x=0;
                                //y=0;
				memset(pMtaProv->DhcpOption2171CccV6DssID1,0,MTA_DHCPOPTION122CCCV6DSSID1_MAX);
			}

			else
			{
				syscfg_get( NULL, "IPv6PrimaryDhcpServerOptions", buffer, sizeof(buffer));
			}
				
				   if(buffer[0] != '\0')
				   {
					len = strlen(buffer);
					CosaDmlMTASetPrimaryDhcpServerOptions(pMyObject->pmtaprovinfo, buffer, ip_type);
					if((MtaIPMode == MTA_IPV6) || (MtaIPMode == MTA_DUAL_STACK))
					{	/* Coverity Fix CID:121023 Incompatible type */
						if(ConverStr2Hex((unsigned char *)buffer) == ANSC_STATUS_SUCCESS)
						{
							printf("Buffer is %s\n",buffer);
								for(i = 0,j= 0;i<len; i++,j++)
										{
								if(j<MTA_DHCPOPTION122CCCV6DSSID1_MAX)
							        {       /*Coverity Fix CID:120994 EVALUTION_ORDER */
                                                                        x = buffer[i]<<4;
                                                                         y = buffer[++i];
									pMtaProv->DhcpOption2171CccV6DssID1[j] |= x + y;
								}
								else
									break;
							}
	
							printf("pMtaProv->DhcpOption2171CccV6DssID1[0] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID1[0],pMtaProv->DhcpOption2171CccV6DssID1[0]);
							printf("pMtaProv->DhcpOption2171CccV6DssID1[1] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID1[1],pMtaProv->DhcpOption2171CccV6DssID1[1]);
							printf("pMtaProv->DhcpOption2171CccV6DssID1[2] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID1[2],pMtaProv->DhcpOption2171CccV6DssID1[2]);
							printf("pMtaProv->DhcpOption2171CccV6DssID1[3] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID1[3],pMtaProv->DhcpOption2171CccV6DssID1[3]);
							pMtaProv->DhcpOption2171CccV6DssID1Len = j;
							printf("pMtaProv->DhcpOption2171CccV6DssID1Len = %d\n",pMtaProv->DhcpOption2171CccV6DssID1Len);
						}
					}

				   }

				rc = memset_s(pMtaProv->DhcpOption2171CccV6DssID2, sizeof(pMtaProv->DhcpOption2171CccV6DssID2), 0, sizeof(pMtaProv->DhcpOption2171CccV6DssID2));
                                ERR_CHK(rc);
            pMtaProv->DhcpOption2171CccV6DssID2Len = 0;
			if ( Ipv6_Secondary[0] != '\0' )   
			{
                                char* pIpv6Secondary = Ipv6_Secondary;
                                rc = strcpy_s(buffer, sizeof(buffer), pIpv6Secondary);
                                if(rc != EOK)
                                {
                                    ERR_CHK(rc);
                                    /* Coverity Fix CID : 120995 RESOURCE_LEAK */
                                    if (pMtaProv)
                                    {
                                       free(pMtaProv);
                                    }
                                    return ANSC_STATUS_FAILURE;
                                }
                                /* Coverity  fix CID : 340667 Unused Value */
                                //x =0;
                                //y =0;
			}
			else
			{
				syscfg_get( NULL, "IPv6SecondaryDhcpServerOptions", buffer, sizeof(buffer));
			}   
				   if(buffer[0] != '\0')
				   {
					len = strlen(buffer);
					CosaDmlMTASetSecondaryDhcpServerOptions(pMyObject->pmtaprovinfo, buffer, ip_type);
					if((MtaIPMode == MTA_IPV6) || (MtaIPMode == MTA_DUAL_STACK))
	                                {	        /* Coverity Fix CID:121023 Incompatible type */
						if(ConverStr2Hex((unsigned char *)buffer) == ANSC_STATUS_SUCCESS)
						{
							printf("Buffer is %s\n",buffer);
					   		for(i = 0,j= 0;i<len; i++,j++)
							{
								if(j<MTA_DHCPOPTION122CCCV6DSSID2_MAX)
								{
                                                                       x =buffer[i]<<4;
                                                                        y = buffer[++i];
									pMtaProv->DhcpOption2171CccV6DssID2[j] |= x + y;
							}
								else
									break;
							}
							printf("pMtaProv->DhcpOption2171CccV6DssID2[0] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID2[0],pMtaProv->DhcpOption2171CccV6DssID2[0]);
							printf("pMtaProv->DhcpOption2171CccV6DssID2[1] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID2[1],pMtaProv->DhcpOption2171CccV6DssID2[1]);
							printf("pMtaProv->DhcpOption2171CccV6DssID2[2] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID2[2],pMtaProv->DhcpOption2171CccV6DssID2[2]);
							printf("pMtaProv->DhcpOption2171CccV6DssID2[3] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID2[3],pMtaProv->DhcpOption2171CccV6DssID2[3]);
							pMtaProv->DhcpOption2171CccV6DssID2Len = j;
							printf("pMtaProv->DhcpOption2171CccV6DssID2Len = %d\n",pMtaProv->DhcpOption2171CccV6DssID2Len);
						}
					}

				   }
    #if defined (ENABLE_ETH_WAN)
		// call hal to start provisioning
		if(mta_hal_start_provisioning(pMtaProv) == RETURN_OK)
		{
			CcspTraceError(("mta_hal_start_provisioning succeded '%s'\n", __FUNCTION__));
            if (syscfg_set(NULL, "MTA_PROVISION","true") != 0)
            {
                CcspTraceWarning(("%s: syscfg_set failed\n", __FUNCTION__));
            }
            else
            {
                CcspTraceInfo(("%s: syscfg MTA_PROVISION successfully set to true\n", __FUNCTION__));
            }
                        /*Coverity Fix  CID:120996 RESOURCE_LEAK */
                        free(pMtaProv);
			return ANSC_STATUS_SUCCESS;
		}
		else
		{
			CcspTraceError(("mta_hal_start_provisioning Failed '%s'\n", __FUNCTION__));
                        /*Coverity Fix  CID:120995 RESOURCE_LEAK */
                        free(pMtaProv);
			return ANSC_STATUS_FAILURE;
		}
     #endif
     /* CID 121017 Structurally dead code fix */
     return ANSC_STATUS_SUCCESS;
}

#define MTA_DHCP_ENABLED 0
#define MTA_DHCP_DISABLED 1
//MAX_TIMEOUT_MTA_DHCP_DISABLED is 1 and MAX_TIMEOUT_MTA_DHCP_ENABLED is 2 when unitTestDockerSupport is enabled
//MAX_TIMEOUT_MTA_DHCP_DISABLED is 300 and MAX_TIMEOUT_MTA_DHCP_ENABLED is 60 when unitTestDockerSupport is not enabled
int checkIfDefMtaDhcpOptionEnabled()
{
	char ipv4Primary[16] , ipv6Primary[16],ip4Sec[16] , ipv6Sec[16] ; 
	memset(ipv4Primary,0,sizeof(ipv4Primary));
	memset(ip4Sec,0,sizeof(ip4Sec));
	memset(ipv6Primary,0,sizeof(ipv6Primary));
	memset(ipv6Sec,0,sizeof(ipv6Sec));

    	syscfg_get( NULL, "IPv4PrimaryDhcpServerOptions", ipv4Primary, sizeof(ipv4Primary));
    	syscfg_get( NULL, "IPv4SecondaryDhcpServerOptions", ip4Sec, sizeof(ip4Sec));
    	syscfg_get( NULL, "IPv6PrimaryDhcpServerOptions", ipv6Primary, sizeof(ipv6Primary));
    	syscfg_get( NULL, "IPv6SecondaryDhcpServerOptions", ipv6Sec, sizeof(ipv6Sec));

    	if ( (strcmp(ipv4Primary,"FFFFFFFF") == 0) || (strcmp(ipv4Primary,"0A0A0A0A") == 0) )
    		return MTA_DHCP_ENABLED;
    	else if ( (strcmp(ip4Sec,"FFFFFFFF") == 0) || (strcmp(ip4Sec,"0A0A0A0A") == 0) )
    		return MTA_DHCP_ENABLED;
    	else if ( (strcmp(ipv6Primary,"FFFFFFFF") == 0) || (strcmp(ipv6Primary,"0A0A0A0A") == 0) )
    		return MTA_DHCP_ENABLED;
    	else if ( (strcmp(ipv6Sec,"FFFFFFFF") == 0) || (strcmp(ipv6Sec,"0A0A0A0A") == 0) )
    		return MTA_DHCP_ENABLED;
    	else
		return MTA_DHCP_DISABLED ;

	return MTA_DHCP_DISABLED ;
}
int getMaxCount()
{
	int maxCount=MAX_TIMEOUT_MTA_DHCP_DISABLED;
	if ( MTA_DHCP_ENABLED == checkIfDefMtaDhcpOptionEnabled() )
	{
		CcspTraceInfo(("%s dhcp_option's enabled in partner defaults,wait for 60sec to receive dhcp options  \n",__FUNCTION__));
		maxCount=MAX_TIMEOUT_MTA_DHCP_ENABLED ;
	}
	return maxCount;
}

void WaitForDhcpOption()
{
	char dhcp_option[10] = {0};
 	int count=0;
 	errno_t rc = -1;
 	int ind = -1;

	int maxCount=getMaxCount() ;
	//  wait for maxCount sec to receive MTA options/Offer , otherwise time out 
 	while ( maxCount >= count )
	{
		rc = memset_s(dhcp_option, sizeof(dhcp_option), 0, sizeof(dhcp_option));
    		ERR_CHK(rc);
		sysevent_get(sysevent_fd, sysevent_token, "dhcp_mta_option", dhcp_option, sizeof(dhcp_option));
    		rc  = strcmp_s((const char*)dhcp_option, sizeof(dhcp_option), "received", &ind);
    		ERR_CHK(rc);
		if ((ind == 0) && (rc == EOK))
		{
			CcspTraceInfo(("%s dhcp_option's received,breaking the loop  \n",__FUNCTION__));
			break;
		}
		sleep(5);
		count+=5;
 	}
 	CcspTraceInfo(("%s Didn't receive dhcp options in %d sec, initializing mta with default values \n",__FUNCTION__,maxCount));
}

/*Coverity Fix CID 121026 Arg Type MisMatch */
void * Mta_Sysevent_thread_Dhcp_Option( void * hThisObject)
{

 PCOSA_DATAMODEL_MTA      pMyObject    = (PCOSA_DATAMODEL_MTA)hThisObject;

 char current_wan_state[10] = {0}, dhcp_option[10] = {0};

 int err;
 char name[25]={0}, val[10]={0};
 errno_t rc = -1;
 int ind = -1;
 async_id_t getnotification_asyncid;
 async_id_t wan_state_asyncid;

 int retValue=0,  namelen=0, vallen =0 ;
 sysevent_set_options(sysevent_fd, sysevent_token, "current_wan_state", TUPLE_FLAG_EVENT);
 retValue=sysevent_setnotification(sysevent_fd, sysevent_token, "current_wan_state",  &wan_state_asyncid);
 CcspTraceWarning(("%s Return value is   %d\n",__FUNCTION__,retValue));

 sysevent_get(sysevent_fd, sysevent_token, "current_wan_state", current_wan_state, sizeof(current_wan_state));
 sysevent_get(sysevent_fd, sysevent_token, "dhcp_mta_option", dhcp_option, sizeof(dhcp_option));

 char value[16] = {'\0'};
 bool tr104ApplySuccess = false;

 char ethWanSyscfg[64]={'\0'};
 bool mtaInEthernetMode = false;
 if( (0 == syscfg_get( NULL, "eth_wan_enabled", ethWanSyscfg, sizeof(ethWanSyscfg))) && ((ethWanSyscfg[0] != '\0') && (strncmp(ethWanSyscfg, "true", strlen("true")) == 0)))
 {
    mtaInEthernetMode = true;
 }

 CcspTraceError(("%s:%d MTA started in %s mode. \n",__FUNCTION__,__LINE__, mtaInEthernetMode?"Ethernet":"DOCSIS"));

 rc = strcmp_s((const char*)current_wan_state, sizeof(current_wan_state), "up", &ind);
 ERR_CHK(rc);
 if((rc == EOK) && (ind == 0))
 {
     if(mtaInEthernetMode)
     {
         rc = strcmp_s((const char*)dhcp_option, sizeof(dhcp_option), "received", &ind);
         ERR_CHK(rc);
         if((rc == EOK) && (ind == 0))
         {
             CcspTraceWarning(("%s current_wan_state up, Initializing MTA \n",__FUNCTION__));
             CosaMTAInitializeEthWanProvDhcpOption(pMyObject);
         }
         else if((rc == EOK) && (ind != 0))
         {
             CcspTraceWarning(("%s current_wan_state up, but dhcp_option's not received.  \n",__FUNCTION__));
             WaitForDhcpOption();
             CosaMTAInitializeEthWanProvDhcpOption(pMyObject);
         }
     }
 }

  do
  {

    namelen = sizeof(name);
    vallen  = sizeof(val);

    rc = memset_s(name, sizeof(name), 0, sizeof(name));
    ERR_CHK(rc);
    rc = memset_s(val, sizeof(val), 0, sizeof(val));
    ERR_CHK(rc);

    err = sysevent_getnotification(sysevent_fd, sysevent_token, name, &namelen, val, &vallen, &getnotification_asyncid);

    if (!err)
    {
        CcspTraceWarning(("%s Recieved notification event  %s, state %s\n",__FUNCTION__,name,val));
        rc = strcmp_s((const char*)name, sizeof(name), "current_wan_state", &ind);
        ERR_CHK(rc);
        if((rc == EOK) && (ind == 0))
        {
            rc = strcmp_s((const char*)val, sizeof(val), "up", &ind);
            ERR_CHK(rc);
            if((rc == EOK) && (ind == 0))
            {
                bool isEthEnabled = false;
                bool tr104Enable = false;
                if( (0 == syscfg_get( NULL, "eth_wan_enabled", ethWanSyscfg, sizeof(ethWanSyscfg))) && ((ethWanSyscfg[0] != '\0') && (strncmp(ethWanSyscfg, "true", strlen("true")) == 0)))
                {
                    isEthEnabled = true;
                }
              
                if((syscfg_get(NULL,"TR104enable", value, sizeof(value)) == 0) && (strcmp(value, "true") == 0))
                {  
    		    tr104Enable = true;
 		}
              
  		CcspTraceWarning((" %s:: tr104Enable: %s \n",__FUNCTION__, value ));
   		memset_s(value,sizeof(value),0,sizeof(value));
        
                if(sysevent_get(sysevent_fd, sysevent_token, "tr104_applied", value, sizeof(value)) == 0)
                {
   		    tr104ApplySuccess = (strcmp(value, "true") == 0);
   		    CcspTraceWarning((" %s:: tr104_applied: %s tr104ApplySuccess=%d \n",__FUNCTION__, value ,tr104ApplySuccess ));
 		} 
                else
                {
   		    CcspTraceError(("%s:%d tr104 not applied :value =%s \n",__FUNCTION__,__LINE__,value));
   		    tr104ApplySuccess = false;
                }
                
                CcspTraceWarning((" %s:: mtaInEthernetMode: %s tr104Enable: %s tr104ApplySuccess: %s \n",__FUNCTION__, mtaInEthernetMode ?"true":"false" ,tr104Enable ?"true":"false" ,tr104ApplySuccess ?"true":"false" ));

                if(mtaInEthernetMode != isEthEnabled)
                {
                    CcspTraceError(("%s:%d MTA is in incorrect WAN state. MTA started in %s, but selected WAN mode %s . MTA agent will be restarted.\n",__FUNCTION__,__LINE__, mtaInEthernetMode?"Ethernet":"DOCSIS", isEthEnabled?"Ethernet":"DOCSIS"));
                    exit(0);
                }
                else if( (mtaInEthernetMode) && (tr104Enable) && (tr104ApplySuccess))
                {
                    CcspTraceWarning(("%s TR104 is Enabled in EthWan Mode, skipping Default config apply. \n",__FUNCTION__));
                }
                else if( ((mtaInEthernetMode) && (!tr104Enable)) ||  ((mtaInEthernetMode) && (tr104Enable) && (!tr104ApplySuccess)) )
                {
                    WaitForDhcpOption();
                    CosaMTAInitializeEthWanProvDhcpOption(pMyObject);
                }
            }
        }
    }
  }while(FOREVER); //FOREVER is 0 when unitTestDockerSupport is enabled, else FOREVER is 1.
}

ANSC_STATUS CosaMTALineTableInitialize
    (
        ANSC_HANDLE                 hThisObject
    )
{
    PCOSA_DATAMODEL_MTA      pMyObject    = (PCOSA_DATAMODEL_MTA)hThisObject;
    PCOSA_MTA_LINETABLE_INFO pLineTable    = (PCOSA_MTA_LINETABLE_INFO)pMyObject->pLineTable;
    ULONG ulCount = CosaDmlMTALineTableGetNumberOfEntries(NULL);

    CcspTraceWarning(("%s %lu\n",__FUNCTION__, ulCount));
    pMyObject->LineTableCount = 0;
    
    if ( ulCount != 0 )
    {
        pLineTable = AnscAllocateMemory(ulCount * sizeof(COSA_MTA_LINETABLE_INFO));
        
        if(pLineTable != NULL)
        {
            ULONG ul=0;
            for (ul=0; ul<ulCount; ul++)
            {
                pLineTable[ul].InstanceNumber = ul + 1;
            }
            pMyObject->pLineTable = pLineTable;
            pMyObject->LineTableCount = ulCount;
        } 
    }
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
CosaMTAInitializeEthWanProv
    (
        ANSC_HANDLE                 hThisObject
    )
{

 MTA_IP_TYPE_TR ip_type;
 char 	buffer [ 128 ] = { 0 };
 int	MtaIPMode = 0;
 int i = 0, j =0; int len = 0;
 errno_t rc = -1;
 unsigned char x,y;
 PMTAMGMT_MTA_PROVISIONING_PARAMS pMtaProv = NULL;
 pMtaProv = (PMTAMGMT_MTA_PROVISIONING_PARAMS)malloc(sizeof(MTAMGMT_PROVISIONING_PARAMS));

 PCOSA_DATAMODEL_MTA      pMyObject    = (PCOSA_DATAMODEL_MTA)hThisObject;
 pMyObject->pmtaprovinfo = (PCOSA_MTA_ETHWAN_PROV_INFO)AnscAllocateMemory(sizeof(COSA_MTA_ETHWAN_PROV_INFO));

/* Coverity Fix : NULL_RETURNS */
if(pMtaProv && pMyObject->pmtaprovinfo)
{
    CosaMTAInitializeEthWanProvJournal(pMyObject->pmtaprovinfo);
	/* CID  173808  Uninitialized scalar variable */
	pMtaProv->MtaIPMode = MTA_IPV4;

	if( 0 == syscfg_get( NULL, "StartupIPMode", buffer, sizeof(buffer)))
	{
	   if(buffer[0] != '\0')
	   {
		sscanf( buffer, "%d", &MtaIPMode );
		printf("MtaIPMode = %d\n",MtaIPMode);
	   }
	}
	/* CID 173808: Uninitialized scalar variable*/
	pMtaProv->MtaIPMode = MtaIPMode;
        CosaDmlMTASetStartUpIpMode(pMyObject->pmtaprovinfo,MtaIPMode);
        ip_type = MTA_IPV4_TR;
		rc = memset_s(buffer, sizeof(buffer), 0, sizeof(buffer));
                ERR_CHK(rc);
		rc = memset_s(pMtaProv->DhcpOption122Suboption1, sizeof(pMtaProv->DhcpOption122Suboption1), 0, sizeof(pMtaProv->DhcpOption122Suboption1));
                ERR_CHK(rc);
		if( 0 == syscfg_get( NULL, "IPv4PrimaryDhcpServerOptions", buffer, sizeof(buffer)))
		{
		   if(buffer[0] != '\0')
		   {
			len = strlen(buffer);
			CosaDmlMTASetPrimaryDhcpServerOptions(pMyObject->pmtaprovinfo, buffer, ip_type);
			if((MtaIPMode ==  MTA_IPV4) || (MtaIPMode == MTA_DUAL_STACK))
			{
				if(ConverStr2Hex((unsigned char*)buffer) == ANSC_STATUS_SUCCESS)
				{

					for(i = 0,j= 0;i<len; i++,j++)
					{
						if(j<MTA_DHCPOPTION122SUBOPTION1_MAX)
						{
                                                        /*Coverity Fix CID:63437 EVALUTION_ORDER */
                                                        x = buffer[i]<<4;
                                                        y = buffer[++i];
							pMtaProv->DhcpOption122Suboption1[j] |= x + y;
						}
						else
							break;
					}
	
					printf("pMtaProv->DhcpOption122Suboption1[0] = %X %d\n",pMtaProv->DhcpOption122Suboption1[0],pMtaProv->DhcpOption122Suboption1[0]);
					printf("pMtaProv->DhcpOption122Suboption1[1] = %X %d\n",pMtaProv->DhcpOption122Suboption1[1],pMtaProv->DhcpOption122Suboption1[1]);
					printf("pMtaProv->DhcpOption122Suboption1[2] = %X %d\n",pMtaProv->DhcpOption122Suboption1[2],pMtaProv->DhcpOption122Suboption1[2]);
					printf("pMtaProv->DhcpOption122Suboption1[3] = %X %d\n",pMtaProv->DhcpOption122Suboption1[3],pMtaProv->DhcpOption122Suboption1[3]);
				}
			}

		   }
		}
		rc = memset_s(buffer,sizeof(buffer), 0, sizeof(buffer));
                ERR_CHK(rc);
		rc = memset_s(pMtaProv->DhcpOption122Suboption2, sizeof(pMtaProv->DhcpOption122Suboption2), 0, sizeof(pMtaProv->DhcpOption122Suboption2));
                ERR_CHK(rc);
		if( 0 == syscfg_get( NULL, "IPv4SecondaryDhcpServerOptions", buffer, sizeof(buffer)))
		{
		   
		   if(buffer[0] != '\0')
		   {
			len = strlen(buffer);
			CosaDmlMTASetSecondaryDhcpServerOptions(pMyObject->pmtaprovinfo, buffer, ip_type);
			if((MtaIPMode ==  MTA_IPV4) || (MtaIPMode == MTA_DUAL_STACK))
			{
				if(ConverStr2Hex((unsigned char*)buffer) == ANSC_STATUS_SUCCESS)
				{
			   		for(i = 0,j= 0;i<len; i++,j++)
					{
						if(j<MTA_DHCPOPTION122SUBOPTION2_MAX)
						{
							unsigned char tmp = buffer[i];
                                                        ++i;
                                                        pMtaProv->DhcpOption122Suboption2[j] |= (unsigned char)((tmp << 4) + (buffer[i]));
						}
						else
							break;
					}
					printf("pMtaProv->DhcpOption122Suboption2[0] = %X %d\n",pMtaProv->DhcpOption122Suboption2[0],pMtaProv->DhcpOption122Suboption2[0]);
					printf("pMtaProv->DhcpOption122Suboption2[1] = %X %d\n",pMtaProv->DhcpOption122Suboption2[1],pMtaProv->DhcpOption122Suboption2[1]);
					printf("pMtaProv->DhcpOption122Suboption2[2] = %X %d\n",pMtaProv->DhcpOption122Suboption2[2],pMtaProv->DhcpOption122Suboption2[2]);
					printf("pMtaProv->DhcpOption122Suboption2[3] = %X %d\n",pMtaProv->DhcpOption122Suboption2[3],pMtaProv->DhcpOption122Suboption2[3]);
				}
			}

		   }
		}
  
        ip_type = MTA_IPV6_TR;
		rc = memset_s(buffer, sizeof(buffer), 0, sizeof(buffer));
                ERR_CHK(rc);
		rc = memset_s(pMtaProv->DhcpOption2171CccV6DssID1, sizeof(pMtaProv->DhcpOption2171CccV6DssID1), 0, sizeof(pMtaProv->DhcpOption2171CccV6DssID1));
                ERR_CHK(rc);
                pMtaProv->DhcpOption2171CccV6DssID1Len = 0;
		if( 0 == syscfg_get( NULL, "IPv6PrimaryDhcpServerOptions", buffer, sizeof(buffer)))
		{
		   if(buffer[0] != '\0')
		   {
			len = strlen(buffer);
			CosaDmlMTASetPrimaryDhcpServerOptions(pMyObject->pmtaprovinfo, buffer, ip_type);
			if((MtaIPMode == MTA_IPV6) || (MtaIPMode == MTA_DUAL_STACK))
			{
				if(ConverStr2Hex((unsigned char*)buffer) == ANSC_STATUS_SUCCESS)
				{

					for(i = 0,j= 0;i<len; i++,j++)
					{
						if(j<MTA_DHCPOPTION122CCCV6DSSID1_MAX)
						{
							unsigned char tmp = buffer[i];
                                                        ++i;
                                                        pMtaProv->DhcpOption2171CccV6DssID1[j] |= (unsigned char)((tmp << 4) + (buffer[i]));
						}
						else
							break;
					}
	
					printf("pMtaProv->DhcpOption2171CccV6DssID1[0] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID1[0],pMtaProv->DhcpOption2171CccV6DssID1[0]);
					printf("pMtaProv->DhcpOption2171CccV6DssID1[1] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID1[1],pMtaProv->DhcpOption2171CccV6DssID1[1]);
					printf("pMtaProv->DhcpOption2171CccV6DssID1[2] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID1[2],pMtaProv->DhcpOption2171CccV6DssID1[2]);
					printf("pMtaProv->DhcpOption2171CccV6DssID1[3] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID1[3],pMtaProv->DhcpOption2171CccV6DssID1[3]);
					pMtaProv->DhcpOption2171CccV6DssID1Len = j;
					printf("pMtaProv->DhcpOption2171CccV6DssID1Len = %d\n",pMtaProv->DhcpOption2171CccV6DssID1Len);
				}

			}
		   }
		}
		rc = memset_s(buffer, sizeof(buffer), 0, sizeof(buffer));
                ERR_CHK(rc);
		rc = memset_s(pMtaProv->DhcpOption2171CccV6DssID2, sizeof(pMtaProv->DhcpOption2171CccV6DssID2), 0, sizeof(pMtaProv->DhcpOption2171CccV6DssID2));
                ERR_CHK(rc);
		pMtaProv->DhcpOption2171CccV6DssID2Len = 0;
		if( 0 == syscfg_get( NULL, "IPv6SecondaryDhcpServerOptions", buffer, sizeof(buffer)))
		{
		   
		   if(buffer[0] != '\0')
		   {
			len = strlen(buffer);
			CosaDmlMTASetSecondaryDhcpServerOptions(pMyObject->pmtaprovinfo, buffer, ip_type);
			if((MtaIPMode == MTA_IPV6) || (MtaIPMode == MTA_DUAL_STACK))
			{
				if(ConverStr2Hex((unsigned char*)buffer) == ANSC_STATUS_SUCCESS)
				{
			   		for(i = 0,j= 0;i<len; i++,j++)
					{
						if(j<MTA_DHCPOPTION122CCCV6DSSID2_MAX)
						{
							unsigned char tmp = buffer[i];
                                                        ++i;
                                                        pMtaProv->DhcpOption2171CccV6DssID2[j] |= (unsigned char)((tmp << 4) + (buffer[i]));
						}
						else
							break;
					}
					printf("pMtaProv->DhcpOption2171CccV6DssID2[0] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID2[0],pMtaProv->DhcpOption2171CccV6DssID2[0]);
					printf("pMtaProv->DhcpOption2171CccV6DssID2[1] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID2[1],pMtaProv->DhcpOption2171CccV6DssID2[1]);
					printf("pMtaProv->DhcpOption2171CccV6DssID2[2] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID2[2],pMtaProv->DhcpOption2171CccV6DssID2[2]);
					printf("pMtaProv->DhcpOption2171CccV6DssID2[3] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID2[3],pMtaProv->DhcpOption2171CccV6DssID2[3]);
					pMtaProv->DhcpOption2171CccV6DssID2Len = j;
					printf("pMtaProv->DhcpOption2171CccV6DssID2Len = %d\n",pMtaProv->DhcpOption2171CccV6DssID2Len);
				}

			}

		   }
		}
    #ifdef ENABLE_ETH_WAN	
		// call hal to start provisioning
		if(mta_hal_start_provisioning(pMtaProv) == RETURN_OK)
		{
            if (syscfg_set(NULL, "MTA_PROVISION","true") != 0)
            {
                CcspTraceWarning(("%s: syscfg_set failed\n", __FUNCTION__));
            }
            else
            {
                CcspTraceInfo(("%s: syscfg MTA_PROVISION successfully set to true\n", __FUNCTION__));
            }
                        /* Coverity Fix CID:74083 RESOURCE_LEAK */
                        free(pMtaProv);
			return ANSC_STATUS_SUCCESS;
		}
		else
		{
			CcspTraceError(("mta_hal_start_provisioning Failed '%s'\n", __FUNCTION__));
                         free(pMtaProv);
			return ANSC_STATUS_FAILURE;
		}
               
     #endif
}
else
	{
		printf("Memory Alloction Failed '%s'\n", __FUNCTION__);
		CcspTraceError(("Memory Alloction Failed '%s'\n", __FUNCTION__));
        if (pMtaProv) 
            free(pMtaProv);
		return ANSC_STATUS_FAILURE;
	}  

/* CID 92066 Structurally dead code fix */
free(pMtaProv);
return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
CosaSetMTAHal
    (
        PCOSA_MTA_ETHWAN_PROV_INFO  pmtaethpro
    )
{
 char 	buffer [ 128 ] = { 0 };
 int	MtaIPMode = 0;
 int i = 0, j =0; int len = 0;
 errno_t rc = -1;
 ANSC_STATUS status = ANSC_STATUS_SUCCESS;
 unsigned char x,y;
 PMTAMGMT_MTA_PROVISIONING_PARAMS pMtaProv = NULL;
 pMtaProv = (PMTAMGMT_MTA_PROVISIONING_PARAMS)malloc(sizeof(MTAMGMT_PROVISIONING_PARAMS));

if(pMtaProv)
{

	MtaIPMode = pmtaethpro->StartupIPMode.ActiveValue;
	pMtaProv->MtaIPMode = MtaIPMode;
	printf("pMtaProv->MtaIPMode = %d\n", pMtaProv->MtaIPMode);
	rc = memset_s(pMtaProv->DhcpOption122Suboption1, sizeof(pMtaProv->DhcpOption122Suboption1), 0, sizeof(pMtaProv->DhcpOption122Suboption1));
        ERR_CHK(rc);
		if( 0 != strlen(pmtaethpro->IPv4PrimaryDhcpServerOptions.ActiveValue))
		{
                   rc = strcpy_s(buffer, sizeof(buffer), pmtaethpro->IPv4PrimaryDhcpServerOptions.ActiveValue);
                   if(rc != EOK)
                   {
                       ERR_CHK(rc);
                       status = ANSC_STATUS_FAILURE;
                       goto EXIT;
                   }
		   if(buffer[0] != '\0')
		   {
			len = strlen(buffer);
			if((MtaIPMode ==  MTA_IPV4) || (MtaIPMode == MTA_DUAL_STACK))
			{
				if(ConverStr2Hex((unsigned char*)buffer) == ANSC_STATUS_SUCCESS)
				{

					for(i = 0,j= 0;i<len; i++,j++)
					{
						if(j<MTA_DHCPOPTION122SUBOPTION1_MAX)
						{
                                                       x= buffer[i]<<4;
                                                        y= buffer[++i];
							pMtaProv->DhcpOption122Suboption1[j] |= x + y;
						}
						else
							break;
					}
	
					printf("pMtaProv->DhcpOption122Suboption1[0] = %X %d\n",pMtaProv->DhcpOption122Suboption1[0],pMtaProv->DhcpOption122Suboption1[0]);
					printf("pMtaProv->DhcpOption122Suboption1[1] = %X %d\n",pMtaProv->DhcpOption122Suboption1[1],pMtaProv->DhcpOption122Suboption1[1]);
					printf("pMtaProv->DhcpOption122Suboption1[2] = %X %d\n",pMtaProv->DhcpOption122Suboption1[2],pMtaProv->DhcpOption122Suboption1[2]);
					printf("pMtaProv->DhcpOption122Suboption1[3] = %X %d\n",pMtaProv->DhcpOption122Suboption1[3],pMtaProv->DhcpOption122Suboption1[3]);
				}
			}

		   }
		}
                rc = memset_s(pMtaProv->DhcpOption122Suboption2, sizeof(pMtaProv->DhcpOption122Suboption2), 0, sizeof(pMtaProv->DhcpOption122Suboption2));
                ERR_CHK(rc);
		if( 0 != strlen(pmtaethpro->IPv4SecondaryDhcpServerOptions.ActiveValue))
		{
                   rc = strcpy_s(buffer, sizeof(buffer), pmtaethpro->IPv4SecondaryDhcpServerOptions.ActiveValue);
                   if(rc != EOK)
	           {
                        ERR_CHK(rc);
                        status = ANSC_STATUS_FAILURE;
                        goto EXIT;
                   }
                x=0;
                y =0;
		   if(buffer[0] != '\0')
		   {
			len = strlen(buffer);
			if((MtaIPMode ==  MTA_IPV4) || (MtaIPMode == MTA_DUAL_STACK))
			{
				if(ConverStr2Hex((unsigned char*)buffer) == ANSC_STATUS_SUCCESS)
				{
			   		for(i = 0,j= 0;i<len; i++,j++)
					{
						if(j<MTA_DHCPOPTION122SUBOPTION2_MAX)

						{     
                                                        x=buffer[i]<<4;
                                                          y =buffer[++i];

							pMtaProv->DhcpOption122Suboption2[j] |= x + y;
						}
						else
							break;
					}
					printf("pMtaProv->DhcpOption122Suboption2[0] = %X %d\n",pMtaProv->DhcpOption122Suboption2[0],pMtaProv->DhcpOption122Suboption2[0]);
					printf("pMtaProv->DhcpOption122Suboption2[1] = %X %d\n",pMtaProv->DhcpOption122Suboption2[1],pMtaProv->DhcpOption122Suboption2[1]);
					printf("pMtaProv->DhcpOption122Suboption2[2] = %X %d\n",pMtaProv->DhcpOption122Suboption2[2],pMtaProv->DhcpOption122Suboption2[2]);
					printf("pMtaProv->DhcpOption122Suboption2[3] = %X %d\n",pMtaProv->DhcpOption122Suboption2[3],pMtaProv->DhcpOption122Suboption2[3]);
				}
			}

		   }
		}
  
		rc = memset_s(pMtaProv->DhcpOption2171CccV6DssID1, sizeof(pMtaProv->DhcpOption2171CccV6DssID1), 0, sizeof(pMtaProv->DhcpOption2171CccV6DssID1));
                ERR_CHK(rc);
		pMtaProv->DhcpOption2171CccV6DssID1Len = 0;
		if(0 != strlen(pmtaethpro->IPv6PrimaryDhcpServerOptions.ActiveValue))
		{
                   rc = strcpy_s(buffer, sizeof(buffer), pmtaethpro->IPv6PrimaryDhcpServerOptions.ActiveValue);
                   if(rc != EOK)
                   {
                        ERR_CHK(rc);
                        status = ANSC_STATUS_FAILURE;
                        goto EXIT;
                   }
                x =0;
                 y=0; 
		   if(buffer[0] != '\0')
		   {
			len = strlen(buffer);
			if((MtaIPMode == MTA_IPV6) || (MtaIPMode == MTA_DUAL_STACK))
			{
				if(ConverStr2Hex((unsigned char*)buffer) == ANSC_STATUS_SUCCESS)
				{

					for(i = 0,j= 0;i<len; i++,j++)
					{
						if(j<MTA_DHCPOPTION122CCCV6DSSID1_MAX)
						{
                                                        x=buffer[i]<<4;
                                                        y = buffer[++i];
							pMtaProv->DhcpOption2171CccV6DssID1[j] |= x + y;
						}
						else
							break;
					}
	
					printf("pMtaProv->DhcpOption2171CccV6DssID1[0] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID1[0],pMtaProv->DhcpOption2171CccV6DssID1[0]);
					printf("pMtaProv->DhcpOption2171CccV6DssID1[1] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID1[1],pMtaProv->DhcpOption2171CccV6DssID1[1]);
					printf("pMtaProv->DhcpOption2171CccV6DssID1[2] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID1[2],pMtaProv->DhcpOption2171CccV6DssID1[2]);
					printf("pMtaProv->DhcpOption2171CccV6DssID1[3] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID1[3],pMtaProv->DhcpOption2171CccV6DssID1[3]);
					pMtaProv->DhcpOption2171CccV6DssID1Len = j;
					printf("pMtaProv->DhcpOption2171CccV6DssID1Len = %d\n",pMtaProv->DhcpOption2171CccV6DssID1Len);
				}
			}
		   }
		}
		rc = memset_s(pMtaProv->DhcpOption2171CccV6DssID2, sizeof(pMtaProv->DhcpOption2171CccV6DssID2), 0, sizeof(pMtaProv->DhcpOption2171CccV6DssID2));
                ERR_CHK(rc);
		pMtaProv->DhcpOption2171CccV6DssID2Len = 0;
		if(0 != strlen(pmtaethpro->IPv6SecondaryDhcpServerOptions.ActiveValue))
		{
                   rc = strcpy_s(buffer, sizeof(buffer), pmtaethpro->IPv6SecondaryDhcpServerOptions.ActiveValue);
                   if(rc != EOK)
                   {
                        ERR_CHK(rc);
                        status = ANSC_STATUS_FAILURE;
                        goto EXIT;
                   }
                x=0;
                y =0;
		   if(buffer[0] != '\0')
		   {
			len = strlen(buffer);
			if((MtaIPMode == MTA_IPV6) || (MtaIPMode == MTA_DUAL_STACK))
			{
				if(ConverStr2Hex((unsigned char*)buffer) == ANSC_STATUS_SUCCESS)
				{
			   		for(i = 0,j= 0;i<len; i++,j++)
					{
						if(j<MTA_DHCPOPTION122CCCV6DSSID2_MAX)
						{
                                                        x= buffer[i]<<4;
                                                        y =buffer[++i];
							pMtaProv->DhcpOption2171CccV6DssID2[j] |= x + y;
                                                }
						else
							break;
					}
					printf("pMtaProv->DhcpOption2171CccV6DssID2[0] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID2[0],pMtaProv->DhcpOption2171CccV6DssID2[0]);
					printf("pMtaProv->DhcpOption2171CccV6DssID2[1] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID2[1],pMtaProv->DhcpOption2171CccV6DssID2[1]);
					printf("pMtaProv->DhcpOption2171CccV6DssID2[2] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID2[2],pMtaProv->DhcpOption2171CccV6DssID2[2]);
					printf("pMtaProv->DhcpOption2171CccV6DssID2[3] = %X %d\n",pMtaProv->DhcpOption2171CccV6DssID2[3],pMtaProv->DhcpOption2171CccV6DssID2[3]);
					pMtaProv->DhcpOption2171CccV6DssID2Len = j;
					printf("pMtaProv->DhcpOption2171CccV6DssID2Len = %d\n",pMtaProv->DhcpOption2171CccV6DssID2Len);
				}
			}

		   }
		}
    #ifdef ENABLE_ETH_WAN	
		// call hal to start provisioning
		if(mta_hal_start_provisioning(pMtaProv) == RETURN_OK)
		{
            if (syscfg_set(NULL, "MTA_PROVISION","true") != 0)
            {
                CcspTraceWarning(("%s: syscfg_set failed\n", __FUNCTION__));
            }
            else
            {
                CcspTraceInfo(("%s: syscfg MTA_PROVISION successfully set to true\n", __FUNCTION__));
            }
			status = ANSC_STATUS_SUCCESS;
                   
		}
		else
		{
			status = ANSC_STATUS_FAILURE;
		}
                free(pMtaProv);
                pMtaProv = NULL;              
#endif
}
else
	{
		printf("Memory Alloction Failed '%s'\n", __FUNCTION__);
		CcspTraceError(("Memory Alloction Failed '%s'\n", __FUNCTION__));
		status = ANSC_STATUS_FAILURE;
	}

EXIT:
      if(pMtaProv)
      {
          free(pMtaProv);
      }
      return status;
}
/*CID 121026 Argument Type mismatch*/
void * Mta_Sysevent_thread(void *  hThisObject)
{

 PCOSA_DATAMODEL_MTA      pMyObject    = (PCOSA_DATAMODEL_MTA)hThisObject;
#if 0
 static int sysevent_fd;
         static token_t sysevent_token;
         sysevent_fd = sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, "WAN State", &sysevent_token);
#endif

#if defined(INTEL_PUMA7)
         //Intel SDK 7.2 Proposed Bug Fix: Prevent CCSP MTA Crash when erouter is in IPv6 mode
         char wan_status[10] = {0};
#else
         char current_wan_state[10] = {0};
#endif

        async_id_t getnotification_asyncid;
          int err;
          char name[25]={0}, val[10]={0};
          int namelen=0, vallen=0;
          errno_t rc = -1;
          int ind = -1;
        char ethWanSyscfg[64]={'\0'};
        bool mtaInEthernetMode = false;
        if( (0 == syscfg_get( NULL, "eth_wan_enabled", ethWanSyscfg, sizeof(ethWanSyscfg))) && ((ethWanSyscfg[0] != '\0') && (strncmp(ethWanSyscfg, "true", strlen("true")) == 0)))
        {
            mtaInEthernetMode = true;
        }

#if defined(INTEL_PUMA7)
         //Intel SDK 7.2 Proposed Bug Fix: Prevent CCSP MTA Crash when erouter is in IPv6 mode
         async_id_t wan_status_asyncid;
         sysevent_set_options(sysevent_fd, sysevent_token, "wan-status", TUPLE_FLAG_EVENT);
         sysevent_setnotification(sysevent_fd, sysevent_token, "wan-status",  &wan_status_asyncid);
 

         sysevent_get(sysevent_fd, sysevent_token, "wan-status", wan_status, sizeof(wan_status));
         rc = strcmp_s((const char*)wan_status, sizeof(wan_status), "started", &ind);
         ERR_CHK(rc);
         if((rc == EOK) && (ind == 0))
         {
             if(mtaInEthernetMode)
             {
                 CcspTraceWarning(("%s wan-status started, Initializing MTA \n",__FUNCTION__));
                 CosaMTAInitializeEthWanProv(pMyObject);
             }
         }
#else


         async_id_t wan_state_asyncid;
         sysevent_set_options(sysevent_fd, sysevent_token, "current_wan_state", TUPLE_FLAG_EVENT);
         sysevent_setnotification(sysevent_fd, sysevent_token, "current_wan_state",  &wan_state_asyncid);

         sysevent_get(sysevent_fd, sysevent_token, "current_wan_state", current_wan_state, sizeof(current_wan_state));
         rc = strcmp_s((const char*)current_wan_state, sizeof(current_wan_state), "up", &ind);
         ERR_CHK(rc);
         if((rc == EOK) && (ind == 0))
         {
             if(mtaInEthernetMode)
             {
                CcspTraceWarning(("%s current_wan_state up, Initializing MTA \n",__FUNCTION__));
      	        CosaMTAInitializeEthWanProv(pMyObject);
             }
         }
#endif

        do
        {

          namelen = sizeof(name);
          vallen  = sizeof(val);
	  rc = memset_s(name, sizeof(name), 0, sizeof(name));
          ERR_CHK(rc);
	  rc = memset_s(val, sizeof(val), 0, sizeof(val));
          ERR_CHK(rc);
          err = sysevent_getnotification(sysevent_fd, sysevent_token, name, &namelen, val, &vallen, &getnotification_asyncid);

                if (!err)
                {
                    bool isEthEnabled = false;
                    if( (0 == syscfg_get( NULL, "eth_wan_enabled", ethWanSyscfg, sizeof(ethWanSyscfg))) && ((ethWanSyscfg[0] != '\0') && (strncmp(ethWanSyscfg, "true", strlen("true")) == 0)))
                    {
                        isEthEnabled = true;
                    }
#if defined(INTEL_PUMA7)
                        //Intel SDK 7.2 Proposed Bug Fix: Prevent CCSP MTA Crash when erouter is in IPv6 mode
                        CcspTraceWarning(("%s Recieved notification event  %s, status %s\n",__FUNCTION__,name,val));
                        rc = strcmp_s((const char*)name, sizeof(name), "wan_status", &ind);
                        ERR_CHK(rc);
                        if((rc == EOK) && ( ind == 0))
                        {
                            rc = strcmp_s((const char*)val, sizeof(val), "started", &ind);
                            ERR_CHK(rc);
                        }
#else
                        CcspTraceWarning(("%s Recieved notification event  %s, state %s\n",__FUNCTION__,name,val));
                        rc = strcmp_s((const char*)name, sizeof(name), "current_wan_state", &ind);
                        ERR_CHK(rc);
                        if((rc == EOK) && ( ind == 0))
                        {
                            rc = strcmp_s((const char*)val, sizeof(val), "up", &ind);
                            ERR_CHK(rc);
                        }
                            
#endif
                        if((rc == EOK) && (ind == 0))
                        {
                            if(mtaInEthernetMode != isEthEnabled)
                            {
                                CcspTraceError(("%s:%d MTA is in incorrect WAN state. MTA started in %s, but selected WAN mode %s . MTA agent will be restarted.\n",__FUNCTION__,__LINE__, mtaInEthernetMode?"Ethernet":"DOCSIS", isEthEnabled?"Ethernet":"DOCSIS"));
                                exit(0);
                            }
                            else if(mtaInEthernetMode)
                            {

                                CcspTraceWarning(("%s Initializing/Reinitializing MTA\n",__FUNCTION__));
                                CosaMTAInitializeEthWanProv(pMyObject);
                            }
                        }
                }      
        }while(FOREVER);



}


/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        CosaMTAInitialize
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate  cosa device info object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS
CosaMTAInitialize
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                  returnStatus = ANSC_STATUS_SUCCESS;
    PCOSA_DATAMODEL_MTA          pMyObject    = (PCOSA_DATAMODEL_MTA)hThisObject;
    /* Initiation all functions */

    /* Initialize middle layer for Device.DeviceInfo.  */
    CosaDmlMTAInit(NULL, (PANSC_HANDLE)pMyObject);

    //Starting thread to monitor Wan mode and wan status
    CcspTraceInfo(("%s %d Starting sysevent thread \n", __FUNCTION__, __LINE__));
#ifdef ENABLE_ETH_WAN
    sysevent_fd = sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, "WAN State", &sysevent_token);
    pthread_t MtaInit;
#if defined (EROUTER_DHCP_OPTION_MTA)
    pthread_create(&MtaInit, NULL, &Mta_Sysevent_thread_Dhcp_Option, (ANSC_HANDLE) hThisObject);
#else
    pthread_create(&MtaInit, NULL, &Mta_Sysevent_thread, (ANSC_HANDLE) hThisObject);
#endif
    //  CosaMTAInitializeEthWanProv(hThisObject);

#endif
    CosaMTALineTableInitialize(hThisObject);
    return returnStatus;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        CosaMTARemove
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate  cosa device info object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS
CosaMTARemove
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PCOSA_DATAMODEL_MTA             pMyObject    = (PCOSA_DATAMODEL_MTA)hThisObject;

    /* Remove necessary resounce */

    /* Remove self */
    AnscFreeMemory((ANSC_HANDLE)pMyObject);

    return returnStatus;
}


//#endif
