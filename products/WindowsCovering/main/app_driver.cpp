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

#include <relay_driver.h>
#include <hal/gpio_types.h>
#include "sdkconfig.h"

#include "app_priv.h"


#define RELAY1_GPIO_NUM ((gpio_num_t)1)
#define RELAY2_GPIO_NUM ((gpio_num_t)2)

static const char *TAG = "app_driver";

static bool socket_states[2] = {false, false};

static void report_socket_state(uint16_t endpoint_id)
{
    uint8_t socket_index = endpoint_id - 1;
    low_code_feature_data_t update_data = {};

    update_data.details.endpoint_id = endpoint_id;
    update_data.details.feature_id = LOW_CODE_FEATURE_ID_POWER;
    update_data.value.type = LOW_CODE_VALUE_TYPE_BOOLEAN;
    update_data.value.value_len = sizeof(bool);
    update_data.value.value = reinterpret_cast<uint8_t *>(&socket_states[socket_index]);

    int err = low_code_feature_update_to_system(&update_data);
    if (err != ESP_OK) {
        printf("%s: Failed to report socket %u state (%d)\n", TAG, endpoint_id, err);
    }
}

static void busy_wait_half_second(void)
{
    /*
     * Estimate the number of loop iterations required to reach roughly 500 ms.
     * Each loop expands to a handful of RISC-V instructions, so dividing the
     * CPU frequency by a small constant yields a coarse but repeatable delay
     * without relying on timers that are unavailable to the LP core.
     */
#if defined(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)
    const uint32_t cpu_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
#elif defined(CONFIG_ESP_SYSTEM_DEFAULT_CPU_FREQ_MHZ)
    const uint32_t cpu_freq_mhz = CONFIG_ESP_SYSTEM_DEFAULT_CPU_FREQ_MHZ;
#else
    const uint32_t cpu_freq_mhz = 160;
#endif
    uint32_t iterations = (cpu_freq_mhz * 500000UL) / 3UL;
    if (iterations == 0) {
        iterations = 1;
    }

    uint32_t counter = 0;
    while (counter < iterations) {
        __asm__ __volatile__("nop");
        ++counter;
    }
}


int app_driver_init()
{
    /* Initialize relays */
    relay_driver_init(RELAY1_GPIO_NUM);
    relay_driver_set_power(RELAY1_GPIO_NUM, true);
    relay_driver_init(RELAY2_GPIO_NUM);
    relay_driver_set_power(RELAY2_GPIO_NUM, true);

    printf("%s: App driver initialized\n", TAG);
    return 0;
}

int app_driver_set_socket_state(uint16_t endpoint_id, bool state)
{
    if ((endpoint_id < 1) || (endpoint_id > 2)) {
        printf("%s: Invalid endpoint %u\n", TAG, endpoint_id);
        return -1;
    }

    uint8_t socket_index = endpoint_id - 1;

    printf("%s: Set socket %u state to %d\n", TAG, endpoint_id, state);

    gpio_num_t relay_gpio = (endpoint_id == 1) ? RELAY1_GPIO_NUM : RELAY2_GPIO_NUM;

    if (state) {
        socket_states[socket_index] = true;
        report_socket_state(endpoint_id);

        relay_driver_set_power(relay_gpio, false);
        busy_wait_half_second();
        relay_driver_set_power(relay_gpio, true);

        socket_states[socket_index] = false;
        report_socket_state(endpoint_id);
        printf("%s: Socket %u reset to off after pulse\n", TAG, endpoint_id);
    } else {
        relay_driver_set_power(relay_gpio, true);
        socket_states[socket_index] = false;
        report_socket_state(endpoint_id);
    }


    if (state) {
        relay_driver_set_power(relay_gpio, false);
        busy_wait_half_second();
        relay_driver_set_power(relay_gpio, true);
        socket_states[socket_index] = false;
        printf("%s: Socket %u reset to off after pulse\n", TAG, endpoint_id);
    } else {
        relay_driver_set_power(relay_gpio, true);
        socket_states[socket_index] = false;
    }
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
