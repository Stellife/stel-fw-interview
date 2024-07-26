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

// Added this so devices would be remembered
#include <zephyr/settings/settings.h>

static void start_scan(void);

K_WORK_DELAYABLE_DEFINE(scan_work, start_scan);

static struct bt_conn *conn_connecting = NULL;
static uint8_t volatile conn_count = 0;

static bool find_device_name(struct bt_data *data, void *user_data)
{
    bt_addr_le_t *addr = user_data;
    char device_name[31];

    // a return value of "false" will stop data parsing by bt_data_parse

    if (data->type == BT_DATA_NAME_COMPLETE) {
        memcpy(device_name, data->data, data->data_len);
        device_name[data->data_len] = '\0';
        
        if (strcmp(device_name, "DXC") == 0) {

            int err;

            err = bt_le_scan_stop();
            if (err) {
                printk("Stop LE scan failed (err %d)\n", err);
                // I am unsure the of the proper way to handle a
                // bt_le_scan_stop failure but I know a connection should not be 
                // attempted if it fails
                continue;
            }

	        err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, 
                                BT_LE_CONN_PARAM_DEFAULT, &conn_connecting);
            if (err) {
            printk("Create conn failed (%d)\n", err);
            }

            return false;

        } 
    } 
    
    return true;
    
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);
	
    if (conn_connecting) {
        return; 
    }

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
		printk("Scanning failed to start (err %d)\n", err);
        k_work_schedule(&scan_work, K_SECONDS(1));
		return;
	}

	printk("Scanning successfully started\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);

		bt_conn_unref(conn);
		conn_connecting = NULL;

		k_work_schedule(&scan_work, K_SECONDS(1));
		return;
	}

    conn_connecting = NULL;

	conn_count++;

	printk("Connected: %s\n", addr);

    if (conn_count < 5) {
        k_work_schedule(&scan_work, K_SECONDS(1));
    }

#if defined(CONFIG_BT_SMP)
    int sec_err = bt_conn_set_security(conn, BT_SECURITY_L2);

    if (sec_err) {
        printk("Failed to set security (%d).\n", sec_err);
    }
#endif

}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(conn);
	
	conn_count--;

    if (conn_count < 5) {
        k_work_schedule(&scan_work, K_SECONDS(1));
    }
}

#if defined(CONFIG_BT_SMP)
static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d\n", addr, level,
		       err);
	}
}
#endif


static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,

#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif
};

int main(void)
{
	int err;

    err = settings_subsys_init();
    if (err) {
		printk("Settings subsystem init failed (err %d)\n", err);
		return 0;
	}

    err = settings_load();
    if (err) {
		printk("Bluetooth settings failed to load (err %d)\n", err);
		return 0;
	}

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

    bt_conn_cb_register(&conn_callbacks);

	k_work_schedule(&scan_work, K_NO_WAIT);

	return 0;
}
