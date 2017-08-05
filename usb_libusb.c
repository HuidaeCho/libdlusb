/* $Id: usb_libusb.c,v 1.10 2010/01/05 09:24:02 geni Exp $
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

int
open_dev(dldev_t *dev)
{
	struct usb_bus *busses, *bus;
	struct usb_device *udev;

	dev->usb.handle = NULL;

	usb_init();
	usb_find_busses();
	usb_find_devices();
	busses = usb_get_busses();

	for(bus = busses; bus; bus = bus->next){
		for(udev = bus->devices; udev; udev = udev->next){
			if(udev->descriptor.idVendor == VENDOR_ID &&
				udev->descriptor.idProduct == PRODUCT_ID)
				goto open;
		}
	}

	if(!bus){
		ERROR("no device found");
		return -1;
	}

open:
	if(!(dev->usb.handle = usb_open(udev))){
		ERROR("usb_open");
		return -1;
	}

	dev->usb.interface =
		udev->config->interface->altsetting->bInterfaceNumber;
	dev->usb.endpoint =
		udev->config->interface->altsetting->endpoint->bEndpointAddress;

#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
	usb_detach_kernel_driver_np(dev->usb.handle, dev->usb.interface);
#endif
	if(usb_claim_interface(dev->usb.handle, dev->usb.interface)){
		ERROR("usb_claim_interface");
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
	if(usb_release_interface(dev->usb.handle, dev->usb.interface)){
		ERROR("usb_release_interface");
		return -1;
	}
	if(usb_close(dev->usb.handle)){
		ERROR("usb_close");
		return -1;
	}

	return 0;
}

int
send_usb_packet(dldev_t *dev, u8 *packet, u16 len)
{
	if(usb_control_msg(dev->usb.handle,
			USB_ENDPOINT_OUT | USB_TYPE_CLASS |
			USB_RECIP_INTERFACE, USB_REQ_SET_CONFIGURATION, 0, 0,
			(char *)packet, len, USB_TIMEOUT) != len){
		ERROR("usb_control_msg");
		return -1;
	}

	return 0;
}

int
read_usb_packet(dldev_t *dev, u8 *packet, u16 len)
{
	if(usb_interrupt_read(dev->usb.handle, dev->usb.endpoint,
			(char *)packet, len, USB_TIMEOUT) != len){
		ERROR("usb_interrupt_read");
		return -1;
	}

	return 0;
}
