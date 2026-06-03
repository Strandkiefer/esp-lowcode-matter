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
#include <ulp_lp_core_utils.h>
#include <hal/gpio_types.h>

#include "app_priv.h"

/* Endpoint 1 → GPIO1 ("Auf"), Endpoint 2 → GPIO2 ("Zu").
 * Relays are active-low: GPIO low = energised. */
#define RELAY1_GPIO_NUM  ((gpio_num_t)1)
#define RELAY2_GPIO_NUM  ((gpio_num_t)2)
#define RELAY_ON         false
#define RELAY_OFF        true

#define PULSE_DURATION_US 500000UL   /* 500 ms */

static const char *TAG = "app_driver";

int app_driver_init()
{
    relay_driver_init(RELAY1_GPIO_NUM);
    relay_driver_set_power(RELAY1_GPIO_NUM, RELAY_OFF);

    relay_driver_init(RELAY2_GPIO_NUM);
    relay_driver_set_power(RELAY2_GPIO_NUM, RELAY_OFF);

    printf("%s: App driver initialized\n", TAG);
    return 0;
}

int app_driver_pulse_channel(uint16_t endpoint_id)
{
    gpio_num_t gpio = (endpoint_id == 1) ? RELAY1_GPIO_NUM : RELAY2_GPIO_NUM;

    printf("%s: Pulse channel %u (GPIO%d)\n", TAG, endpoint_id, (int)gpio);

    relay_driver_set_power(gpio, RELAY_ON);
    ulp_lp_core_delay_us(PULSE_DURATION_US);
    relay_driver_set_power(gpio, RELAY_OFF);

    /* Report state back to Off so the Matter attribute stays consistent. */
    bool off = false;
    low_code_feature_data_t update = {
        .details = {
            .endpoint_id = endpoint_id,
            .feature_id  = LOW_CODE_FEATURE_ID_POWER,
        },
        .value = {
            .type      = LOW_CODE_VALUE_TYPE_BOOLEAN,
            .value_len = sizeof(bool),
            .value     = (uint8_t *)&off,
        },
    };
    low_code_feature_update_to_system(&update);

    return 0;
}

int app_driver_event_handler(low_code_event_t *event)
{
    printf("%s: Received event: %d\n", TAG, event->event_type);
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
            printf("%s: Low code test mode subtype: %d\n", TAG, (int)*((int *)(event->event_data)));
            break;
        case LOW_CODE_EVENT_TEST_MODE_COMMON:
            printf("%s: Common test mode triggered\n", TAG);
            break;
        case LOW_CODE_EVENT_TEST_MODE_BLE:
            printf("%s: BLE test mode triggered\n", TAG);
            break;
        case LOW_CODE_EVENT_TEST_MODE_SNIFFER:
            printf("%s: Sniffer test mode triggered\n", TAG);
            break;
        default:
            printf("%s: Unhandled event type: %d\n", TAG, event->event_type);
            break;
    }
    return 0;
}
