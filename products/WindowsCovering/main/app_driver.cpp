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

#include <relay_driver.h>
#include <hal/gpio_types.h>

#include "app_priv.h"

#define RELAY1_GPIO_NUM ((gpio_num_t)2)
#define RELAY2_GPIO_NUM ((gpio_num_t)3)

static const char *TAG = "app_driver";

static bool socket_states[2] = {false, false};

int app_driver_init()
{
    /* Initialize relays */
    relay_driver_init(RELAY1_GPIO_NUM);
    relay_driver_init(RELAY2_GPIO_NUM);

    printf("%s: App driver initialized\n", TAG);
    return 0;
}

int app_driver_set_socket_state(uint16_t endpoint_id, bool state)
{
    uint8_t socket_index = endpoint_id - 1;
    socket_states[socket_index] = state;
    printf("%s: Set socket %d state to %d\n", TAG, endpoint_id, state);

    /* Set appropriate relay */
    gpio_num_t relay_gpio = (endpoint_id == 1) ? RELAY1_GPIO_NUM : RELAY2_GPIO_NUM;
    relay_driver_set_power(relay_gpio, state);

    return 0;
}

int app_driver_event_handler(low_code_event_t *event)
{
    /* Get the events. Approriate indicators should be shown to the user based on the event. */
    printf("%s: Received event: %d\n", TAG, event->event_type);
    /* Handle the events from low_code_event_type_t */
    switch (event->event_type) {
        case LOW_CODE_EVENT_SETUP_MODE_START:
            printf("%s: Setup mode started\n", TAG);
            break;
        case LOW_CODE_EVENT_SETUP_MODE_END:
            printf("%s: Setup mode ended\n", TAG);
            break;
        case LOW_CODE_EVENT_SETUP_DEVICE_CONNECTED:
            printf("%s: Device connected during setup\n", TAG);
            break;
        case LOW_CODE_EVENT_SETUP_STARTED:
            printf("%s: Setup process started\n", TAG);
            break;
        case LOW_CODE_EVENT_SETUP_SUCCESSFUL:
            printf("%s: Setup process successful\n", TAG);
            break;
        case LOW_CODE_EVENT_SETUP_FAILED:
            printf("%s: Setup process failed\n", TAG);
            break;
        case LOW_CODE_EVENT_NETWORK_CONNECTED:
            printf("%s: Network connected\n", TAG);
            break;
        case LOW_CODE_EVENT_NETWORK_DISCONNECTED:
            printf("%s: Network disconnected\n", TAG);
            break;
        case LOW_CODE_EVENT_OTA_STARTED:
            printf("%s: OTA update started\n", TAG);
            break;
        case LOW_CODE_EVENT_OTA_STOPPED:
            printf("%s: OTA update stopped\n", TAG);
            break;
        case LOW_CODE_EVENT_READY:
            printf("%s: Device is ready\n", TAG);
            break;
        case LOW_CODE_EVENT_IDENTIFICATION_START:
            printf("%s: Identification started\n", TAG);
            break;
        case LOW_CODE_EVENT_IDENTIFICATION_STOP:
            printf("%s: Identification stopped\n", TAG);
            break;
        case LOW_CODE_EVENT_TEST_MODE_LOW_CODE:
            printf("%s: Low code test mode is triggered for subtype: %d\n", TAG, (int)*((int*)(event->event_data)));
            break;
        case LOW_CODE_EVENT_TEST_MODE_COMMON:
            printf("%s: common test mode triggered\n", TAG);
            break;
        case LOW_CODE_EVENT_TEST_MODE_BLE:
            printf("%s: ble test mode triggered\n", TAG);
            break;
        case LOW_CODE_EVENT_TEST_MODE_SNIFFER:
            printf("%s: sniffer test mode triggered\n", TAG);
            break;
        default:
            printf("%s: Unhandled event type: %d\n", TAG, event->event_type);
            break;
    }

    return 0;
}
