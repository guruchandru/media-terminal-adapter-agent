##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2015 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
AM_CFLAGS =

ACLOCAL_AMFLAGS = -I m4

# DML library disabled to avoid cyclic dependencies - all sources now compiled directly into SSP
#noinst_LTLIBRARIES = libCcspMtaAgent_middle_layer_src.la

#libCcspMtaAgent_middle_layer_src_la_CFLAGS = $(AM_CFLAGS) $(MTA_CFLAGS)
#libCcspMtaAgent_middle_layer_src_la_CPPFLAGS = -I$(top_srcdir)/source/TR-181/board_sbapi -I$(top_srcdir)/source/TR-181/middle_layer_src -I$(top_srcdir)/source/TR-181/include -I$(top_srcdir)/source/Custom -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/rbus -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/cjson
#libCcspMtaAgent_middle_layer_src_la_SOURCES = cosa_x_cisco_com_mta_internal.c plugin_main.c plugin_main_apis.c cosa_x_cisco_com_mta_dml.c ../../MtaAgentSsp/TR104_webconfig.c mta_rbus_handlers.c mta_bus_json_decode.c mta_compat_globals.c
#libCcspMtaAgent_middle_layer_src_la_LDFLAGS = -lhal_mta -lsafec -lmsgpackc -lrbus -lcjson -ltrower-base64
