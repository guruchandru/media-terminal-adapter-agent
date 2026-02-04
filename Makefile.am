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
SUBDIRS = source

# DML library disabled to avoid cyclic dependencies - all sources now compiled directly into SSP
if !UNIT_TEST_DOCKER_SUPPORT

#lib_LTLIBRARIES = libmta_tr181.la

#libmta_tr181_la_LDFLAGS = -lsysevent -lutapi -lutctx -lulog -lsyscfg
#libmta_tr181_la_SOURCES =
#libmta_tr181_la_DEPENDENCIES = \
#	source/TR-181/middle_layer_src/libCcspMtaAgent_middle_layer_src.la \
#	source/TR-181/integration_src.shared/libCcspMtaAgent_integration_src_shared.la

#libmta_tr181_la_LIBADD = $(libmta_tr181_la_DEPENDENCIES)

endif
