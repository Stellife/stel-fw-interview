/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/net/buf.h>
#include <string.h>

#define MAX_CONN 3 // define max connections macro

static struct bt_conn *conns[MAX_CONN]; // declare array for >1 connection
static uint8_t conn_count = 0; // initialize connection count
static bool name_matched = false; // initalize flag to check device name

static void start_scan(void);

static bool device_name_found(struct bt_data *data, void *user_data) // string match target device name
{
	char *target_name = (char *)user_data;

	if (data->type == BT_DATA_NAME_COMPLETE) {
		if (strncmp((char *)data->data, target_name, data->data_len) == 0) {
			name_matched = true;
			return false;
		}
	}
	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	const char *target_name = "DXC"; // assumes all devices named DXC, expand to array for other names

	name_matched = false;

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	if (rssi < -50) { // connect only to devices in close proximity
		return;
	}
	
	bt_data_parse(ad, device_name_found, (void *)target_name); // parse advertisement data to filter for device name

	if (!name_matched || conn_count >= MAX_CONN) { // check flag before attempting connection
		return;
	}

	if (bt_le_scan_stop()) {
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &conns[conn_count]);
	if (err) {
		printk("Create conn to %s failed (%d)\n", addr_str, err);
		start_scan();
	} else {
		conn_count++;
	}
}

static void start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found); // assumes no active scan
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) { // error handling of identified failed connection
		printk("Failed to connect to %s (%u)\n", addr, err);
		for (int i = 0; i < conn_count; i++) {
			if (conns[i] == conn) {
				bt_conn_unref(conns[i]);
				conns[i] = NULL;
				break;
			}
		}
		start_scan();
		return;
	}

	printk("Connected: %s\n", addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	for (int i = 0; i < conn_count; i++) {	// identify dropped connection from array and decrement
		if (conns[i] == conn) {
			bt_conn_unref(conns[i]);
			conns[i] = NULL;
			conn_count--;
			break;
		}
	}
	
	start_scan();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

int main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	start_scan();
	return 0;
}
