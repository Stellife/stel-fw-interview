# Stel Interview

# Overview
The central project sample was updated to filter devices by name and allow multiple device connections.

### Q1: Filter Device by Name in `device_found` Function
Within main.c, bt_data_parse parses the advertisement data coming in from nearby devices. device_name_found is used as its callback function, which verifies that the complete target device name, DXC, matches the advertisement data. If a match is found, the name_matched flag is set to true, which prompts a connection attempt.

### Q2: Support for Multiple Device Connections
prj.conf is modified to allow multiple device connections by setting CONFIG_BT_MAX_CONN=3 and CONFIG_BT_MAX_PAIRED=5. In main.c, MAX_CONN is defined as a macro to set a limit for these connections. An array of pointers conns[MAX_CONN] stores these connection objects while conn_count tracks active connections. When a connection is successful or drops, the connected and disconnected functions respectively increment and decrement the conn_count by looping through the array of connections of interest and setting to NULL.

> **Note:** The nRF52 Adafruit Feather board was used to build this project, which cmakelists.txt accounts for by setting the board to nrf52_adafruit_feather. The files for this particular build were excluded for this submission. Additional comments were made within main.c to elaborate on lines of code.
