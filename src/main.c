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

static void start_scan(void);

static struct bt_conn *connections[CONFIG_BT_MAX_CONN];
static uint8_t volatile conn_count;
static bool scan_active = false;
static struct k_work_delayable work;

static bool is_device_connected(const bt_addr_le_t *addr)
{
    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
        if (connections[i]) {
            if (bt_addr_le_cmp(bt_conn_get_dst(connections[i]), addr) == 0) {
                return true;  
            }
        }
    }
    return false;
}

static bool find_device_name(struct bt_data *data, void *user_data)
{
    if (data->type == BT_DATA_NAME_COMPLETE) {
        bt_addr_le_t *addr = user_data;
        int err;
        char name[32] = {0};
        size_t len = MIN(data->data_len, sizeof(name) - 1);
        memcpy(name, data->data, len);
        name[len] = '\0';

        if (is_device_connected(addr)) {
            return false;
        }

        if (strcmp(name, "Jacobs IPhone") == 0 || strcmp(name, "DXC") == 0) {
            
            if (bt_le_scan_stop()) {
		        return false;
	        }
            scan_active = false;
            printk("Matching device found.\n");

            for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) { 
                if (!connections[i]) {
                    printk("Initiating connection to: %s\n", name);
                    err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
			                                BT_LE_CONN_PARAM_DEFAULT, &connections[i]);
                    if (err) {
		                printk("Create conn to %s failed (%d).\n", name, err);
		                start_scan();
	                }
                    break;
                }
            }
            return false;
        }
    }
    else {
        return true;
    }
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
    
	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	/* connect only to devices in close proximity */
	if (rssi < -50) {
		return;
	}
	
    bt_data_parse(ad, find_device_name, (void *)addr);

}

static void start_scan(void)
{
	int err;

	/* This demo doesn't require active scan */
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		printk("Scanning failed to start (err %d).\n", err);
        scan_active = false;
		return;
	}
    scan_active = true;
	printk("Scanning successfully started.\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u).\n", addr, err);
		bt_conn_unref(conn);
		return;
	}

    conn_count++;
    printk("Device Connected. Current connection count: %d\n", conn_count);

    if (conn_count < CONFIG_BT_MAX_CONN) {
        printk("Scheduling scan.\n");
        k_work_reschedule(&work, K_SECONDS(3));
    } else {
        printk("Max device count reached. Not scanning.\n");
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));


    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
        if (connections[i] == conn) {
            bt_conn_unref(connections[i]);
            connections[i] = NULL;
            break;
        }
    }
    conn_count--;
    printk("Device disconnected. Current connection count: %d\n", conn_count);
	if (conn_count < CONFIG_BT_MAX_CONN && !scan_active) {
        printk("Scheduling scan.\n");
        k_work_reschedule(&work, K_SECONDS(3));
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};


static void work_scan(struct k_work *work)
{
    start_scan();
}

int main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d).\n", err);
		return 0;
	}

	printk("Bluetooth Initialized.\n");
    printk("Max Connections: %d\n", CONFIG_BT_MAX_CONN);
    printk("======================\n");
    k_work_init_delayable(&work, work_scan);
    start_scan();

	return 0;
}
