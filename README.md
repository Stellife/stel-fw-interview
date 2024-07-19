bt_data_parse accepts three parameters: a buffer of the ad data, a callback function, and a pointer where the parsed data of interest can be stored.

Line 52 and 53 of main.c define the character array that will be used to store the parsed data. The variable decays to a pointer to the first element of the array when passed to bt_data_parse.

bt_data_parse calls find_device_name repeated until false is returns. False indicates the device name has been found and parsing can stop.

Device name is printed using printk.

## Q2 - Implementation Summary

Support for multiple devices is effectively managed by utilizing 3 global variables.
```c
static struct bt_conn *conn_connecting = NULL;
static uint8_t volatile conn_count;
static bool volatile is_connecting = false;
```
bt_conn is the major data structure used in Zephyr's Bluetooth stack that keeps track of a connected device's important information. It is passed to bt_conn_le_create, which creates the connection and adds it to a list of connected devices. 
```c
bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &conn_connecting);
```
In the connected function, is_connecting is set back to false, conn_connecting is set back to NULL and conn_count is incremented. Similar logic is used in disconnected callback function.
```c
is_connecting = false;
conn_connecting = NULL;
conn_count++;
```
Main enters an infinite while loop after initiating the first scan. The while loop will check to see if a new scan should be initiated and intermittently yield CPU.
```c
k_sleep(K_MSEC(100));

if (conn_count < CONFIG_BT_MAX_CONN && !is_connecting) {
    start_scan();
}
```
## Other Functionality Implemented
- The settings subsystem was configured to store device pairing information.
- SMP was configured for additional security.
- Privacy was configured for added anonymity.
## prj.conf
- System is configured to support up to 5 simultaneous connections and remember up to 10 devices.
```
CONFIG_BT=y 
CONFIG_BT_CENTRAL=y
CONFIG_BT_MAX_CONN=5
CONFIG_BT_MAX_PAIRED=10 
CONFIG_BT_SMP=y 
CONFIG_BT_PRIVACY=y
CONFIG_BT_SETTINGS=y
CONFIG_SETTINGS=y
CONFIG_BT_HCI=y
```
### Files Changed
main.c and prj.conf
