# Stel Interview Question

## Instruction for Setup

For detailed instructions on setting up the environment, please refer to the [Nordic Semiconductor documentation](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/installation/install_ncs.html).

## Project Sample

> **Note:** If you want to compile the project, feel free to use any nRF52 boards. You don't need to load the firmware to the device.

You can find the project sample at the following path: `zephyr/samples/bluetooth/central`.

## Questions

### Q1: Filter Device by Name in `device_found` Function

In the `device_found` function, filter the connecting device based on its name. Example: Device with name `DXC`. Please update the `device_found` function using the `bt_data_parse` API to filter by `type: BT_DATA_NAME_COMPLETE`.

### Q2: Support for Multiple Device Connections

The current application only supports a single device connection. Please propose an idea for adding support for multiple device connections and add changes to the current central app.

> **Note:** This is only an interview project. You don't really need to make it work properly. We are able to understand your ideas :)

## Submission

Please create a branch based on your name and submit the central app you changed.
