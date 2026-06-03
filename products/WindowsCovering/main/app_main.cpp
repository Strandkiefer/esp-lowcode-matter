// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>

#include <system.h>
#include <low_code.h>

#include "app_priv.h"

static const char *TAG = "app_main";

static void setup()
{
    low_code_register_callbacks(feature_update_from_system, event_from_system);
    app_driver_init();
}

static void loop()
{
    low_code_get_feature_update_from_system();
    low_code_get_event_from_system();
}

/*
 * Called by the HP-Core whenever a Matter attribute update or command
 * is forwarded to the LP-Core.
 *
 * ENDPOINT ROUTING (FIX):
 *   The HP-Core sets data->details.endpoint_id to the Matter endpoint
 *   that received the command.  We must check this field explicitly to
 *   route EP1 → Relay 1 and EP2 → Relay 2.
 *   Both endpoints share the same On/Off cluster (feature_id ==
 *   LOW_CODE_FEATURE_ID_POWER).  Only the endpoint_id differs.
 *
 * WHY feature_update IS CALLED NOW:
 *   The data model must expose two On/Off Plug-in Unit endpoints
 *   (device type 0x010A), not Window Covering (0x0202).  Only then
 *   does Apple Home send On/Off commands and the HP-Core forward them
 *   as POWER feature updates.  With WC device type the HP-Core receives
 *   UpOrOpen/DownOrClose for cluster 0x0102 and never calls this
 *   callback.
 */
int feature_update_from_system(low_code_feature_data_t *data)
{
    uint16_t endpoint_id = data->details.endpoint_id;
    uint32_t feature_id  = data->details.feature_id;

    /* [CHANGED] Log every call so we can verify the callback fires
     * and confirm which endpoint_id the HP-Core sends. */
    printf("%s: feature_update ep=%u feat=%u\n", TAG, endpoint_id, (unsigned)feature_id);

    /* [CHANGED] Explicit check for EP1 and EP2 with POWER feature. */
    if (feature_id == LOW_CODE_FEATURE_ID_POWER && data->value.value != NULL) {
        bool on = *(bool *)data->value.value;
        printf("%s: ep=%u On/Off=%d\n", TAG, endpoint_id, on);

        if (on) {
            if (endpoint_id == 1 || endpoint_id == 2) {
                app_driver_pulse_channel(endpoint_id);
            } else {
                printf("%s: Ignoring unknown endpoint %u\n", TAG, endpoint_id);
            }
        }
        /* Off: nothing to do – relay is already released after the pulse. */
    }

    return 0;
}

int event_from_system(low_code_event_t *event)
{
    return app_driver_event_handler(event);
}

extern "C" int main()
{
    printf("%s: Starting\n", TAG);
    system_setup();
    setup();

    while (1) {
        system_loop();
        loop();
    }
    return 0;
}
