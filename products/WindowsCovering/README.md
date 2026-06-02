# Window Covering | Dual Relay

## Description

A single window covering (blind / shade / roller) controller that drives a motor through two relay outputs: one for the
"open / up" direction and one for the "close / down" direction. The firmware operates without local buttons or visual
indicators and relies on Matter commands or automations for control.

* **Directional Relay Control**: Relay 1 pulses the "open / up" input and relay 2 pulses the "close / down" input of the
  motor controller. The opposite direction is always released before a new movement starts.

* **Momentary Drive Logic**: Each relay is pulsed for roughly half a second through a simple counter-based busy wait, so the
  active-low coils automatically release after every command. This emulates a momentary wall-switch press.

* **Remote-Only Operation**: No GPIOs are reserved for buttons or LEDs, keeping the hardware footprint minimal.

* **Matter Data Model Specification**:
  * **Device Type**: `Window Covering` (`0x0202`)
  * **Cluster**: `Window Covering` (`0x0102`) with the **Lift (LF)** feature only (`FeatureMap = 0x1`)
  * **Commands**: `UpOrOpen`, `DownOrClose`, `StopMotion`

> **Open-loop, position-less design**: The hardware has no position feedback (no encoder, no travel timing), so only the
> `Lift` feature is advertised *without* position awareness. The device therefore exposes up / down / stop, but **not** a
> calibrated position percentage. This is an honest match for the impulse-based relay hardware.

## How commands reach the application

The low code framework does **not** define a dedicated feature id for window coverings (see `low_code_feature_id_t` in
`low_code.h` — it only covers power, brightness, colour, temperature, etc.). Window Covering commands are therefore decoded
from the **raw Matter details** carried in `data->details.low_level.matter` (`cluster_id`, `attribute_id`, `command_id`).

`feature_update_from_system()` handles, in order:

1. **Path B (primary)** — `cluster_id == 0x0102` (Window Covering):
   * `command_id` `UpOrOpen` / `DownOrClose` / `StopMotion`, or
   * a write to `TargetPositionLiftPercent100ths` (`0x000B`), where a target below the mid point opens and above it closes.
2. **Path A (fallback)** — a boolean `LOW_CODE_FEATURE_ID_POWER` update (`true` = open, `false` = close), in case the system
   image maps the covering onto a power-like feature.

> **Verification note**: The HP-core system image that bridges Matter to the LP core is a pre-built binary and is not part of
> this repository. Whether it forwards the raw Window Covering `command_id` to the LP core could not be proven from source.
> The data model and both decode paths are in place; final behaviour must be confirmed on real hardware.

## Hardware Configuration

The following hardware components are used for this product:

* **Devkit**: [M5Stack Nano C6 Dev Kit](https://shop.m5stack.com/products/m5stack-nanoc6-dev-kit?srsltid=AfmBOooXsbm_fgpDyK1yWqgPOwtjrL3WksxGlhmRKDZFmVj2omLLbWDX)
* **Power Relays**: Two single-channel relays wired to the open / up and close / down inputs of the motor controller

### Pin Assignment

| Peripheral        | GPIO Pin | Function                   |
|-------------------|----------|----------------------------|
| Open relay control| GPIO1    | "Open / up" motor input    |
| Close relay control| GPIO2   | "Close / down" motor input |

> **Note**: GPIO assignments can be customized by modifying the following macros in **app_driver.cpp**:
> `RELAY_OPEN_GPIO_NUM`, `RELAY_CLOSE_GPIO_NUM`

## Understanding Code

### Initialization Sequence

The `app_driver_init()` function performs the following:

* Configures both relay GPIOs as outputs and leaves them released (resting)
* Skips any button or indicator setup, so the firmware depends solely on the relay component

### Core Functions

* **Movement Control**:
  * `app_driver_drive_covering()` takes an `app_covering_action_t` (`APP_COVERING_OPEN`, `APP_COVERING_CLOSE`,
    `APP_COVERING_STOP`). For open / close it releases the opposite relay, pulses the requested relay for ~0.5 s, and stores
    the direction in `last_movement`. For stop it re-pulses the last moving direction (then releases both relays).

> **StopMotion behaviour**: The driver tracks the last commanded direction in `last_movement`. On `StopMotion` it re-pulses
> that direction's relay, emulating the "press during travel = stop" behaviour of **latching / momentary** controllers, and
> then releases both relays. On **continuous-contact** wiring the final release also stops the motor. If no movement is
> active (`last_movement == APP_COVERING_STOP`) the driver only releases the relays.

* **Event Handling**:
  * `app_driver_event_handler()` prints lifecycle events received from the low-code system. Without LEDs, events are surfaced
    for debugging through the serial console.

### Endpoint Mapping

* Endpoint 1 (ID = 1): the single Window Covering, with relay 1 = open and relay 2 = close.

### Extending Functionality

* **Continuous drive instead of impulse**: replace `pulse_relay()` in `app_driver.cpp` with logic that keeps the relay
  energised while the motor should run.
* **Multiple coverings**: add further Window Covering endpoints to `configuration/data_model.zap`, wire two relays per
  covering, and extend `app_driver_drive_covering()` to map each endpoint to its relay pair. Run `Upload Configuration` to
  push the updated data model to the device.
* **Optional local inputs**: if physical control is desired, refer to the `socket` product for an example of registering
  button callbacks.

## Related Documentation

* [Programmer's Model](../../docs/programmer_model.md)
* [Components](../../components/README.md)
* [Drivers](../../drivers/README.md)
* [Products](../README.md)
