/*
 * pssdunlock.cpp
 *
 *  Created on: 2023-06-23 09:15
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libusb-1.0/libusb.h>

#define EP_DATA_IN  0x02
#define EP_DATA_OUT 0x81

#define VID_SAMSUNG   0x04e8

#define PID_T3_NORMAL 0x61f3
#define PID_T3_LOCKED 0x61f4

#define PID_T5_NORMAL 0x61f5
#define PID_T5_LOCKED 0x61f6

uint8_t payload_header[31] = {
    0x55, 0x53, 0x42, 0x43, 0x0a, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x10, 0x85,
    0x0a, 0x26, 0x00, 0xd6, 0x00, 0x01, 0x00, 0xc6,
    0x00, 0x4f, 0x00, 0xc2, 0x00, 0xb0, 0x00
};

uint8_t payload_relink[31] = {
    0x55, 0x53, 0x42, 0x43, 0x0b, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0xe8,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t payload_passwd[512] = {0};

int main(int argc, char **argv)
{
    int transferred = 0;
    uint16_t pid_normal = 0;
    uint16_t pid_locked = 0;
    struct libusb_device_handle *devh = NULL;

    // Check arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <device> <password>\n", argv[0]);
        exit(1);
    }

    // Select device
    if (!strcmp(argv[1], "t3")) {
        pid_normal = PID_T3_NORMAL;
        pid_locked = PID_T3_LOCKED;
    } else if (!strcmp(argv[1], "t5")) {
        pid_normal = PID_T5_NORMAL;
        pid_locked = PID_T5_LOCKED;
    } else {
        fprintf(stderr, "Unknown device: %s\n", argv[1]);
        exit(1);
    }

    // Copy password to payload
    strncpy((char *)payload_passwd, argv[2], sizeof(payload_passwd) - 1);

    // Initialize libusb
    int rc = libusb_init(NULL);
    if (rc < 0) {
        fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(rc));
        exit(1);
    }

    // Find device
    devh = libusb_open_device_with_vid_pid(NULL, VID_SAMSUNG, pid_locked);
    if (!devh) {
        devh = libusb_open_device_with_vid_pid(NULL, VID_SAMSUNG, pid_normal);
        if (!devh) {
            fprintf(stderr, "No %s device found\n", argv[1]);
        } else {
            fprintf(stderr, "Device is unlocked\n", argv[1]);
        }
        goto out;
    }

    // Reset device
    rc = libusb_reset_device(devh);
    if (rc < 0) {
        fprintf(stderr, "Error resetting device: %s\n", libusb_error_name(rc));
        goto out;
    }

    // Detach kernel driver
    rc = libusb_detach_kernel_driver(devh, 0);
    if (rc < 0) {
        fprintf(stderr, "Error detaching kernel driver: %s\n", libusb_error_name(rc));
        goto out;
    }

    // Claim interface
    rc = libusb_claim_interface(devh, 0);
    if (rc < 0) {
        fprintf(stderr, "Error claiming interface: %s\n", libusb_error_name(rc));
        goto out;
    }

    // Send password plyload
    libusb_bulk_transfer(devh, EP_DATA_IN,  payload_header, sizeof(payload_header), &transferred, 5000);
    libusb_bulk_transfer(devh, EP_DATA_IN,  payload_passwd, sizeof(payload_passwd), &transferred, 5000);
    libusb_bulk_transfer(devh, EP_DATA_OUT, payload_passwd, sizeof(payload_passwd), &transferred, 5000);

    if (payload_passwd[9] != 0x00) {
        fprintf(stderr, "Unlock failed\n");
    } else {
        fprintf(stderr, "Unlock successfuly\n");
    }

    // Send relink command
    libusb_bulk_transfer(devh, EP_DATA_IN,  payload_relink, sizeof(payload_relink), &transferred, 5000);
    libusb_bulk_transfer(devh, EP_DATA_OUT, payload_passwd, sizeof(payload_passwd), &transferred, 5000);

    // Release interface
    libusb_release_interface(devh, 0);
out:
    // Close device
    if (devh) {
        libusb_close(devh);
    }

    // Exit libusb
    libusb_exit(NULL);

    return rc;
}
