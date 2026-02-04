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

#ifndef MTA_RBUS_HANDLERS_H
#define MTA_RBUS_HANDLERS_H

#include <rbus.h>
#include "mta_bus_json_decode.h"

#define RBUS_COMPONENT_NAME_MTA "CcspMtaAgent"

/* RBUS get handler for MTA parameters */
rbusError_t mta_rbus_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);

/* RBUS set handler for MTA parameters */
rbusError_t mta_rbus_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts);

/* RBUS event handler for MTA */
rbusError_t mta_rbus_event_handler(rbusHandle_t handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription);

/* Initialize RBUS for MTA */
int mta_rbus_init(const char *component_name);

/* Terminate RBUS for MTA */
void mta_rbus_terminate(void);

/* RBUS callback registration function - registers get/set callbacks */
void mta_register_callbacks_to_metadata(mta_param_metadata_t *metadata, const char *param_name);

#endif /* MTA_RBUS_HANDLERS_H */
