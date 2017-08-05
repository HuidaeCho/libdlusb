/* $Id: usb_usbhid.c,v 1.3 2005/09/13 02:32:14 geni Exp $
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
	hid_data_t hd;

	dev->usb.rd = NULL;

	if((dev->usb.fd = open(dev->usb.file, O_RDWR)) < 0){
		ERROR("%s: open failed", dev->usb.file);
		return -1;
	}

	hid_init(NULL);

	if(!(dev->usb.rd = hid_get_report_desc(dev->usb.fd))){
		ERROR("hid_get_report_desc");
		return -1;
	}

	for(hd = hid_start_parse(dev->usb.rd, 1 << hid_output, 0);
			hid_get_item(hd, &dev->usb.shi); ){
		if(dev->usb.shi.kind == hid_output){
			hid_end_parse(hd);
			break;
		}
	}

	for(hd = hid_start_parse(dev->usb.rd, 1 << hid_input, 0);
			hid_get_item(hd, &dev->usb.rhi); ){
		if(dev->usb.rhi.kind == hid_input){
			hid_end_parse(hd);
			break;
		}
	}

	return 0;
}

int
close_dev(dldev_t *dev)
{
	if(!dev->usb.rd || dev->usb.fd < 0){
		ERROR("no device to close");
		return -1;
	}

	hid_dispose_report_desc(dev->usb.rd);
	close(dev->usb.fd);

	return 0;
}

int
send_usb_packet(dldev_t *dev, u8 *packet, u16 len)
{
	int i, ret = -1;
	u8 *report;

	report = (u8 *)malloc(len);

	for(i = 0; i < len; i++)
		hid_set_data(report+i, &dev->usb.shi, packet[i]);

	if(write(dev->usb.fd, report, len) == len)
		ret = 0;
	else
		ERROR("write");

	free(report);

	return ret;
}

int
read_usb_packet(dldev_t *dev, u8 *packet, u16 len)
{
	int i, ret = -1;
	u8 *report;

	report = (u8 *)malloc(len);

	if(read(dev->usb.fd, report, len) == len){
		ret = 0;
		for(i = 0; i < len; i++)
			packet[i] = hid_get_data(report+i, &dev->usb.rhi);
	}else
		ERROR("read");

	free(report);

#if 0
	/* TODO: timeout */
	packet[0] = 0x04;	/* length */
	packet[1] = 0xff;	/* COMM_NACK */
	packet[2] = 0x04;	/* timeout error */
	packet[3] = 0xf9;	/* checksum */
#endif

	return ret;
}
