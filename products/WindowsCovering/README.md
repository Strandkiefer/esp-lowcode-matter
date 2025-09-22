# Windows Covering | Dual Relay

## Description

A dual-channel window covering controller that drives motor directions exclusively through relay outputs. The firmware operates
without local buttons or visual indicators and relies on Matter commands or automations for control.

* **Dual Relay Control**: Independently switches two relay channels that can be wired to "open" and "close" inputs of a motor
  controller.
* **Momentary Drive Logic**: Each relay is pulsed for roughly half a second through a simple counter-based busy wait so the
  active-low coils automatically release after every command.
* **Remote-Only Operation**: No GPIOs are reserved for buttons or LEDs, keeping the hardware footprint minimal.
* **Matter Data Model Specification**:
  * **Device Type** : `Window Covering`

## Hardware Configuration

The following hardware components are used for this product:

* **Devkit**: [M5Stack Nano C6 Dev Kit](https://shop.m5stack.com/products/m5stack-nanoc6-dev-kit?srsltid=AfmBOooXsbm_fgpDyK1yWqgPOwtjrL3WksxGlhmRKDZFmVj2omLLbWDX)
* **Power Relays**: Two single-channel relays wired to the actuator or motor controller inputs

### Pin Assignment

| Peripheral      | GPIO Pin | Function                    |
|-----------------|----------|-----------------------------|
| Relay 1 Control | GPIO1    | Primary direction / channel |
| Relay 2 Control | GPIO2    | Secondary direction / channel |
| (Not used)      | —        | —                           |


> **Note**: GPIO assignments can be customized by modifying the following macros in **app_driver.cpp**:
> `RELAY1_GPIO_NUM`, `RELAY2_GPIO_NUM`

## Understanding Code

### Initialization Sequence

The `app_driver_init()` function performs the following:

* Configures both relay GPIOs as outputs
* Skips any button or indicator setup, so the firmware depends solely on the relay component

### Core Functions

* **Power Control**:
  * `app_driver_set_socket_state` validates the requested endpoint, pulses the appropriate relay for ~0.5 seconds, and then
    resets the stored state to reflect the resting (off) coil level.
* **Event Handling**:
  * `app_driver_event_handler` prints lifecycle events received from the low-code system. Without LEDs, events are surfaced for
    debugging through the serial console.

### Multi-Endpoint Implementation

* Endpoint mapping:
  * Endpoint1 (ID = 1): Controls the relay connected to `RELAY1_GPIO_NUM`
  * Endpoint2 (ID = 2): Controls the relay connected to `RELAY2_GPIO_NUM`
* State tracking:
  * `socket_states[]` captures the requested state momentarily so the driver can acknowledge commands before restoring the
    resting `false` value after each pulse.

### Extending Functionality

To add more relay channels to the system, implement the following changes:

* **Matter Data Model Extension**:
  * Add the required number of Window Covering endpoints to the Matter cluster configuration.
  * Run `Upload Configuration` to push the updated data model to the device.
* **Optional Local Inputs**:
  * If physical control is desired, refer to the `socket` product for an example of registering button callbacks that invoke
    `app_driver_set_socket_state`.

## Related Documentation

* [Programmer's Model](../../docs/programmer_model.md)
* [Components](../../components/README.md)
* [Drivers](../../drivers/README.md)
* [Products](../README.md)
