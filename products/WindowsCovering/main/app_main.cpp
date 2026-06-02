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
#include <stdint.h>

#include <system.h>
#include <low_code.h>

#include "app_priv.h"

static const char *TAG = "app_main";

/*
 * Matter Window Covering cluster identifiers (Matter 1.x).
 *
 * The low code framework does not expose a dedicated feature id for window
 * coverings (see low_code_feature_id_t), so commands arrive through the raw
 * Matter details carried in data->details.low_level.matter. We match on the
 * cluster id and decode either the command id (UpOrOpen / DownOrClose /
 * StopMotion) or a target lift percentage attribute write. A boolean POWER
 * feature update is also handled as a fallback in case the system image maps
 * the covering onto a power-like feature.
 */
#define WINDOW_COVERING_CLUSTER_ID                  0x0102

#define WC_CMD_UP_OR_OPEN                           0x00
#define WC_CMD_DOWN_OR_CLOSE                        0x01
#define WC_CMD_STOP_MOTION                          0x02
#define WC_CMD_GO_TO_LIFT_VALUE                     0x04
#define WC_CMD_GO_TO_LIFT_PERCENTAGE                0x05

#define WC_ATTR_TARGET_POSITION_LIFT_PERCENT_100THS 0x000B

/* Non-target Window Covering attributes present in our data model. A write to
 * any of these (for example the mandatory, writable Mode attribute) must never
 * be decoded as a command. Attribute 0x0000 (Type) is intentionally omitted so
 * we never clash with a genuine command that may arrive with attribute_id 0. */
#define WC_ATTR_CONFIG_STATUS                       0x0007
#define WC_ATTR_OPERATIONAL_STATUS                  0x000A
#define WC_ATTR_END_PRODUCT_TYPE                    0x000D
#define WC_ATTR_MODE                                0x0017

/* In Matter a lift percentage of 0 means fully open and 100% (10000 in
 * percent-100ths) means fully closed. We use the mid point to pick a
 * direction for this open-loop, position-less controller. */
#define WC_LIFT_PERCENT_100THS_MIDPOINT             5000

static void setup()
{
    /* Register callbacks */
    low_code_register_callbacks(feature_update_from_system, event_from_system);

    /* Initialize driver */
    app_driver_init();
}

static void loop()
{
    /* The corresponding callbacks are called if data is received from system */
    low_code_get_feature_update_from_system();
    low_code_get_event_from_system();
}

static app_covering_action_t action_from_command(uint32_t command_id)
{
    switch (command_id) {
        case WC_CMD_UP_OR_OPEN:
            return APP_COVERING_OPEN;
        case WC_CMD_DOWN_OR_CLOSE:
            return APP_COVERING_CLOSE;
        case WC_CMD_STOP_MOTION:
            return APP_COVERING_STOP;
        default:
            return APP_COVERING_STOP;
    }
}

int feature_update_from_system(low_code_feature_data_t *data)
{
    uint16_t endpoint_id = data->details.endpoint_id;
    uint32_t feature_id = data->details.feature_id;
    uint32_t cluster_id = data->details.low_level.matter.cluster_id;
    uint32_t attribute_id = data->details.low_level.matter.attribute_id;
    uint32_t command_id = data->details.low_level.matter.command_id;

    if (endpoint_id != 1) {
        return 0;
    }

    /* Path B: raw Matter Window Covering cluster (commands and target lift). */
    if (cluster_id == WINDOW_COVERING_CLUSTER_ID) {
        if (attribute_id == WC_ATTR_TARGET_POSITION_LIFT_PERCENT_100THS &&
            data->value.value != NULL) {
            uint16_t target = *(uint16_t *)data->value.value;
            app_covering_action_t action = (target < WC_LIFT_PERCENT_100THS_MIDPOINT)
                                               ? APP_COVERING_OPEN
                                               : APP_COVERING_CLOSE;
            printf("%s: Window covering target lift %u -> %s\n", TAG, target,
                   action == APP_COVERING_OPEN ? "open" : "close");
            return app_driver_drive_covering(endpoint_id, action);
        }

        /* The framework reuses this struct for commands and attribute updates
         * and leaves command_id at 0 (== UpOrOpen) for attribute writes, so
         * ignore known non-target attribute updates before decoding a command
         * to avoid spuriously pulsing the open relay. */
        if (attribute_id == WC_ATTR_CONFIG_STATUS ||
            attribute_id == WC_ATTR_OPERATIONAL_STATUS ||
            attribute_id == WC_ATTR_END_PRODUCT_TYPE ||
            attribute_id == WC_ATTR_MODE) {
            printf("%s: Ignoring Window Covering attribute 0x%04x update\n", TAG,
                   (unsigned)attribute_id);
            return 0;
        }

        app_covering_action_t action = action_from_command(command_id);
        printf("%s: Window covering command %u -> %d\n", TAG, (unsigned)command_id, action);
        return app_driver_drive_covering(endpoint_id, action);
    }

    /* Path A (fallback): boolean power feature. true = open, false = close. */
    if (feature_id == LOW_CODE_FEATURE_ID_POWER && data->value.value != NULL) {
        bool power_value = *(bool *)data->value.value;
        printf("%s: Power fallback %d -> %s\n", TAG, power_value, power_value ? "open" : "close");
        return app_driver_drive_covering(endpoint_id,
                                         power_value ? APP_COVERING_OPEN : APP_COVERING_CLOSE);
    }

    return 0;
}

int event_from_system(low_code_event_t *event)
{
    /* Handle the events from low_code_event_type_t */
    return app_driver_event_handler(event);
}

extern "C" int main()
{
    printf("%s: Starting low code\n", TAG);

    /* Pre-Initializations: This should be called first and should always be present */
    system_setup();
    setup();

    /* Loop */
    while (1) {
        system_loop();
        loop();
    }
    return 0;
}
