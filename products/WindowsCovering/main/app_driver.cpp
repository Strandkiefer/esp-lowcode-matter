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

/*
 * [NOTE] LP-Core GPIO and timing APIs:
 *
 *   This code runs on the ESP32-C6 LP-Core (ULP), NOT on the HP-Core.
 *   Standard ESP-IDF APIs (gpio_set_level, gpio_config_t, vTaskDelay,
 *   esp_timer) are HP-Core functions and are NOT available here.
 *
 *   Correct LP-Core equivalents used in relay_driver.c:
 *     • ulp_lp_core_gpio_set_level()  instead of gpio_set_level()
 *     • ulp_lp_core_delay_us()        instead of vTaskDelay() / esp_timer
 *       (uses LP hardware timer – not a busy-wait)
 *
 *   LP-Core GPIO capability: On ESP32-C6, GPIO0-7 are LP-IO pins and
 *   are accessible from the LP-Core.  GPIO1 and GPIO2 are safe choices.
 *   GPIO9 is the boot-mode strapping pin → do not use.
 */
#include <ulp_lp_core_utils.h>   /* ulp_lp_core_delay_us() */
#include <hal/gpio_types.h>

#include "app_priv.h"

/* [CONFIG] Relay GPIOs.
 *   Endpoint 1 → GPIO1 ("Auf" / open)
 *   Endpoint 2 → GPIO2 ("Zu"  / close)
 *   Both GPIOs are LP-IO capable on ESP32-C6. */
#define RELAY1_GPIO_NUM  ((gpio_num_t)1)
#define RELAY2_GPIO_NUM  ((gpio_num_t)2)

/* [CONFIG] Active-low wiring: coil energises when GPIO is driven LOW.
 *   RELAY_ENERGIZE = false → ulp_lp_core_gpio_set_level(gpio, 0) → LOW
 *   RELAY_RELEASE  = true  → ulp_lp_core_gpio_set_level(gpio, 1) → HIGH */
#define RELAY_ENERGIZE   false
#define RELAY_RELEASE    true

/* [CONFIG] Impulse duration in microseconds (500 ms). */
#define PULSE_US  500000UL

static const char *TAG = "app_driver";

int app_driver_init()
{
    /* [UNCHANGED] Init both relay GPIOs and leave them released (coil OFF). */
    relay_driver_init(RELAY1_GPIO_NUM);
    relay_driver_set_power(RELAY1_GPIO_NUM, RELAY_RELEASE);

    relay_driver_init(RELAY2_GPIO_NUM);
    relay_driver_set_power(RELAY2_GPIO_NUM, RELAY_RELEASE);

    printf("%s: initialized – GPIO%d (EP1/Auf), GPIO%d (EP2/Zu)\n",
           TAG, (int)RELAY1_GPIO_NUM, (int)RELAY2_GPIO_NUM);
    return 0;
}

int app_driver_pulse_channel(uint16_t endpoint_id)
{
    /* [CHANGED] Select relay GPIO based on endpoint_id.
     *   Any endpoint_id other than 1 or 2 is rejected early. */
    gpio_num_t gpio;
    if (endpoint_id == 1) {
        gpio = RELAY1_GPIO_NUM;
    } else if (endpoint_id == 2) {
        gpio = RELAY2_GPIO_NUM;
    } else {
        printf("%s: Unknown endpoint %u – no relay pulsed\n", TAG, endpoint_id);
        return -1;
    }

    printf("%s: EP%u → GPIO%d: energize\n", TAG, endpoint_id, (int)gpio);

    /* [CHANGED] Use ulp_lp_core_delay_us() for the 500 ms hold.
     *   This is the LP-Core hardware-timer delay (not a CPU busy-loop).
     *   vTaskDelay() is not available on the LP-Core. */
    relay_driver_set_power(gpio, RELAY_ENERGIZE);
    ulp_lp_core_delay_us(PULSE_US);
    relay_driver_set_power(gpio, RELAY_RELEASE);

    printf("%s: EP%u → GPIO%d: released\n", TAG, endpoint_id, (int)gpio);

    /* [CHANGED] Report POWER=false back to the HP-Core so the Matter
     *   OnOff attribute is reset to Off after the impulse.
     *   Without this the Matter state would stay stuck at On. */
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
    printf("%s: event %d\n", TAG, event->event_type);
    switch (event->event_type) {
        case LOW_CODE_EVENT_SETUP_MODE_START:       printf("%s: Setup mode start\n",  TAG); break;
        case LOW_CODE_EVENT_SETUP_MODE_END:         printf("%s: Setup mode end\n",    TAG); break;
        case LOW_CODE_EVENT_SETUP_DEVICE_CONNECTED: printf("%s: Device connected\n",  TAG); break;
        case LOW_CODE_EVENT_SETUP_STARTED:          printf("%s: Setup started\n",     TAG); break;
        case LOW_CODE_EVENT_SETUP_SUCCESSFUL:       printf("%s: Setup successful\n",  TAG); break;
        case LOW_CODE_EVENT_SETUP_FAILED:           printf("%s: Setup failed\n",      TAG); break;
        case LOW_CODE_EVENT_NETWORK_CONNECTED:      printf("%s: Network connected\n", TAG); break;
        case LOW_CODE_EVENT_NETWORK_DISCONNECTED:   printf("%s: Network disconnected\n", TAG); break;
        case LOW_CODE_EVENT_OTA_STARTED:            printf("%s: OTA started\n",       TAG); break;
        case LOW_CODE_EVENT_OTA_STOPPED:            printf("%s: OTA stopped\n",       TAG); break;
        case LOW_CODE_EVENT_READY:                  printf("%s: Device ready\n",      TAG); break;
        case LOW_CODE_EVENT_IDENTIFICATION_START:   printf("%s: Identify start\n",    TAG); break;
        case LOW_CODE_EVENT_IDENTIFICATION_STOP:    printf("%s: Identify stop\n",     TAG); break;
        case LOW_CODE_EVENT_TEST_MODE_LOW_CODE:
            printf("%s: Test mode LC subtype=%d\n", TAG, (int)*((int *)(event->event_data)));
            break;
        default: printf("%s: Unhandled event %d\n", TAG, event->event_type); break;
    }
    return 0;
}
