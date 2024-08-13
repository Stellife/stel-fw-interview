# Updates Made
- Removed functionality unrelated to interview questions
- Tested project using Adafruit ItsyBitsy, nRF Connect BLE Desktop, & nRF Connect Mobile
- Monitored printk messages using screen command, wrote output to connection_log.txt
## Application Description
This application supports up to two simultaneous BLE connections. It will only connect to devices being advertised as either "DXC" or "Jacobs IPhone". Connections are managed using a struct bt_conn array. Scan scheduling is done using a delayable work item. It can support more than two connections if CONFIG_BT_MAX_CONN is increased, but has not been tested for more than 2.
## Q1 Implementation
- bt_data_parse() is called in device_found()
- the callback function, find_device_name(), checks if the device name is a match, and initiates a connection if so
- is_device_connected() is also called inside of find_device_name() to ensure a connection isn't attempted to an already connected device
## Q2 Implmentation
- struct array "connections" of type bt_conn manages connections
- global variable conn_count keeps track of # of connections
- bt_le_conn_creat adds connections to connections array
- connected() function increments count
- disconnected() function removes connections and decrements count
## Scanning
Scanning is managed by scheduling a work item with a handler that calls start_scan(). If a connection is made and conn_count is less than 2, another scan will be scheduled. If max connection count (2) is reached, a new scan will not be initiated. If a device is disconnected and a scan is not active, another scan will be scheduled. Global variable scan_active is used to check if scan is currently active.
## Testing
I used nRF Connect Mobile to advertise a BLE device named "Jacobs IPhone" and my laptop with a dongle and nRF Connect BLE Desktop to advertise BLE device "DXC". Printk messages are spread throughout main.c. I monitored these by connecting to the ItsyBitsy's serial port and wrote the output to file (see connection_log.txt).
### Files Updated
- main.c
- prj.conf
- build folder
### Files Added
- connection_log.txt