/* $Id: usb_libusb-1.0.c,v 1.1 2010/01/05 09:24:02 geni Exp $
 *
 * Copyright (c) 2005 Huidae Cho
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "dlusb.h"

static libusb_device **devices;
static struct libusb_config_descriptor *config_desc;

int
open_dev(dldev_t *dev)
{
	libusb_device *device;
	int i;

	dev->usb.handle = NULL;

	if(libusb_init(NULL)){
		ERROR("libusb_init");
		return -1;
	}

	if(libusb_get_device_list(NULL, &devices) < 0){
		ERROR("libusb_get_device_list");
		return -1;
	}

	for(i = 0; (device = devices[i]) != NULL; i++){
		struct libusb_device_descriptor device_desc;

		if(!libusb_get_device_descriptor(device, &device_desc)){
			if(device_desc.idVendor == VENDOR_ID &&
				device_desc.idProduct == PRODUCT_ID)
				break;
		}
	}

	if(!device){
		ERROR("no device found");
		return -1;
	}

	if(libusb_get_config_descriptor(device, 0, &config_desc)){
		ERROR("libusb_get_config_descriptor");
		return -1;
	}

	dev->usb.interface =
		config_desc->interface->altsetting->bInterfaceNumber;
	dev->usb.endpoint =
		config_desc->interface->altsetting->endpoint->bEndpointAddress;

	if(libusb_open(device, &dev->usb.handle)){
		ERROR("libusb_open");
		return -1;
	}

	if(libusb_kernel_driver_active(dev->usb.handle, dev->usb.interface)){
		if(libusb_detach_kernel_driver(dev->usb.handle,
					dev->usb.interface)){
			ERROR("libusb_detach_kernel_driver");
			return -1;
		}
	}

	if(libusb_claim_interface(dev->usb.handle, dev->usb.interface)){
		ERROR("libusb_claim_interface");
		return -1;
	}

	return 0;
}

int
close_dev(dldev_t *dev)
{
	if(!dev->usb.handle){
		ERROR("no device to close");
		return -1;
	}
	if(libusb_release_interface(dev->usb.handle, dev->usb.interface)){
		ERROR("libusb_release_interface");
		return -1;
	}
	libusb_close(dev->usb.handle);
	libusb_free_config_descriptor(config_desc);
	libusb_free_device_list(devices, 1);
	libusb_exit(NULL);

	return 0;
}

int
send_usb_packet(dldev_t *dev, u8 *packet, u16 len)
{
	if(libusb_control_transfer(dev->usb.handle, LIBUSB_RECIPIENT_INTERFACE
				| LIBUSB_REQUEST_TYPE_CLASS |
				LIBUSB_ENDPOINT_OUT ,
				LIBUSB_REQUEST_SET_CONFIGURATION, 0, 0, packet,
				len, USB_TIMEOUT) != len){
		ERROR("libusb_control_transfer");
		return -1;
	}

	return 0;
}

int
read_usb_packet(dldev_t *dev, u8 *packet, u16 len)
{
	int transferred;

	if(libusb_interrupt_transfer(dev->usb.handle, dev->usb.endpoint,
				packet, len, &transferred, USB_TIMEOUT) ||
			len != transferred){
		ERROR("libusb_interrupt_transfer");
		return -1;
	}

	return 0;
}
