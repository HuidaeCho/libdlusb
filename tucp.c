/* $Id: tucp.c,v 1.10 2005/09/13 02:32:14 geni Exp $
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
#include <string.h>
#include <unistd.h>
#include "dlusb.h"

#define PROGRESS if(show_progress) tucp_progress(-1);

static int check_tucp_packet(u8 *packet);
static int send_tucp_packet(dldev_t *dev, u8 *packet);
static int read_tucp_packet(dldev_t *dev, u8 *packet);
void tucp_progress(int show);

static int slen = 0, rlen = 0, show_progress = 0;

int
send_read_tucp_packet(dldev_t *dev, u8 *spacket, u8 *rpacket)
{
	u8 i;

	if(!spacket)
		return 0;

	for(i = 0; i < NUM_RETRIES; i++){
		/* send the packet */
		if(send_tucp_packet(dev, spacket)){
			ERROR("usb error");
			return ERR_USB;
		}

		/* read the response packet from the device and
		 * check the packet */
		if(read_tucp_packet(dev, rpacket)){
			ERROR("usb error");
			return ERR_USB;
		}

		if(rpacket[1] != COMM_ACK && rpacket[1] != COMM_NACK){
			ERROR("no ack/nack");
			for(i = 0; i < rpacket[0]; i++)
				fprintf(stderr, "%2d: %02x\n", i, rpacket[i]);
			fprintf(stderr, "\n");
			return ERR_COMM;
		}

		/* see if we got a good packet back */
		/* is it an ACK? is it a response to the command we sent out? */
		if(rpacket[1] == COMM_ACK && rpacket[2] == spacket[1] &&
				!check_tucp_packet(rpacket))
			/* process the packet outside this function */
			break;
		/* see if we got a good NACK packet */
		/* check the error type */
		if(rpacket[2] == 0x00 || rpacket[2] == 0x01 ||
				rpacket[2] == 0x04){
			/* retry */
			if(i == NUM_RETRIES-1){
				/* this was the last try;
				 * return with an error */
				switch(rpacket[2]){
				case 0x00:
					ERROR("checksum error");
					return ERR_COMM_CHECKSUM;
				case 0x01:
					ERROR("packet size error");
					return ERR_COMM_PACKET_SIZE;
				case 0x04:
					ERROR("timeout");
					return ERR_COMM_TIMEOUT;
				default:
					ERROR("comm error");
					return ERR_COMM;
				}
			}
			/* TODO: COMM_TRNS_RCV_ERR? */
			/* internal messages sent from the HW/CORE to COMM */
		}else{
			/* non-recoverable error, don't retry */
			switch(rpacket[2]){
			case 0x02:
				ERROR("device info request error");
				return ERR_COMM_DEV_INFO_REQ;
			case 0x03:
				ERROR("device mismatch error");
				return ERR_COMM_DEV_MISMATCH;
			default:
				ERROR("non-recoverable comm error");
				return ERR_COMM;
			}
		}
	}

	return 0;
}

int
send_idle(dldev_t *dev)
{
	/* When the total number of received packets is odd, send idle
	 * Why? I don't know. Anyway, it works! */
#ifdef DEBUG
	fprintf(stderr, "slen: %d (%d), rlen: %d (%d), total: %d (%d)\n",
			slen, slen/8, rlen, rlen/8, slen+rlen, (slen+rlen)/8);
#endif
	if((rlen / 8) & 1){
		if(comm_idle(dev)){
			ERROR("comm_idle");
			return -1;
		}
	}

	return 0;
}

static int
check_tucp_packet(u8 *packet)
{
	u8 i, checksum = 0;

	/* check the length of the packet */
	if(packet[0] < 3){ /* || packet[0] > MAX_PACKET_SIZE) */
		ERROR("too short packet");
		return -1;
	}

	for(i = 0; i < packet[0]-1; i++)
		checksum += packet[i];
	checksum = ~checksum + 1;

	if(checksum != packet[i]){
		ERROR("checksum mismatch");
		return -1;
	}

	return 0;
}

static int
send_tucp_packet(dldev_t *dev, u8 *packet)
{
	u8 i;

	for(i = 0; i < packet[0]; i += PACKET_SIZE){
PROGRESS
		if(send_usb_packet(dev, packet+i, PACKET_SIZE)){
			ERROR("send_usb_packet");
			return -1;
		}
		slen += PACKET_SIZE;
	}

	return 0;
}

static int
read_tucp_packet(dldev_t *dev, u8 *packet)
{
	u8 i, npackets, report[PACKET_SIZE];

PROGRESS
	/* get the first packet from the device */
	if(read_usb_packet(dev, report, PACKET_SIZE)){
		ERROR("read_usb_packet");
		return -1;
	}
	rlen += PACKET_SIZE;

	/* copy the result to the buffer passed from the calling function */
	memcpy(packet, report, PACKET_SIZE);

	/* see how many more packets we need to get */
	npackets = (report[0]-1) / PACKET_SIZE;
	for(i = 0; i < npackets; i++){
PROGRESS
		/* get the next packet from the device */
		if(read_usb_packet(dev, report, PACKET_SIZE)){
			ERROR("read_usb_packet");
			return -1;
		}
		rlen += PACKET_SIZE;
#if 0
		/* TODO: timeout */
		report[0] = 0x04;	/* length */
		report[1] = 0xff;	/* COMM_NACK */
		report[2] = 0x04;	/* timeout error */
		report[3] = 0xf9;	/* checksum */
#endif
		/* copy the result to the buffer passed from the calling
		 * function */
		memcpy(packet+(PACKET_SIZE)*(i+1), report, PACKET_SIZE);
	}

	return 0;
}

void
tucp_progress(int show)
{
	static char *ch = "-\\|/";
	static int i = -1;

	switch(show){
	case 0:
		if(show_progress)
			fprintf(stderr, "\n");
		show_progress = 0;
		break;
	case 1:
		show_progress = 1;
		break;
	default:
		i = (i + 1) % 4;
		fprintf(stderr, "\b%c", ch[i]);
		break;
	}

	return;
}
