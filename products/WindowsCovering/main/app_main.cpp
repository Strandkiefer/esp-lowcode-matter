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

int feature_update_from_system(low_code_feature_data_t *data)
{
    uint16_t endpoint_id = data->details.endpoint_id;
    uint32_t feature_id  = data->details.feature_id;

    if ((endpoint_id == 1 || endpoint_id == 2) &&
        feature_id == LOW_CODE_FEATURE_ID_POWER &&
        data->value.value != NULL) {

        bool on = *(bool *)data->value.value;
        printf("%s: ep=%u power=%d\n", TAG, endpoint_id, on);

        if (on) {
            app_driver_pulse_channel(endpoint_id);
        }
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
