/* $Id: comm.c,v 1.14 2005/09/13 02:32:14 geni Exp $
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

#include <stdio.h>
#include <string.h>
#include "dlusb.h"

static void no_data_packet(u8 *packet, u8 cmd);
static void create_packet(u8 *packet, u8 *data, u8 len);

static u8 sbuf[MAX_PACKET_SIZE], rbuf[MAX_PACKET_SIZE];
static int init_app_idx = -1;

int
get_init_app_idx(void)
{
	return init_app_idx;
}

int
get_init_app_inst(void)
{
	return init_app_idx < 0 ? 1 : init_app_idx;
}

int
write_abs_addr(dldev_t *dev, u16 addr, u8 type, u8 *data, u16 len)
{
	u8 l;

	while(len > 0){
		l = (len > 0xff ? 0xff : len);

		if(comm_write_abs_addr(dev, addr, type, data, l)){
			ERROR("comm_write_abs_addr");
			return -1;
		}

		addr += l;
		data += l;
		len -= l;
	}

	return 0;
}

int
read_abs_addr(dldev_t *dev, u16 addr, u8 type, u8 *data, u16 len)
{
	u8 l;

	while(len > 0){
		l = (len > 0xff ? 0xff : len);

		if(comm_read_abs_addr(dev, addr, type, data, l)){
			ERROR("comm_read_abs_addr");
			return -1;
		}

		addr += l;
		data += l;
		len -= l;
	}

	return 0;
}

int
comm_trns_rcv_err(dldev_t *dev, u8 type)
{
	/* TODO: COMM_TRNS_RCV_ERR? */
	/* internal messages sent from the HW/CORE to COMM */
	return 0;

	no_data_packet(sbuf, COMM_TRNS_RCV_ERR);
	sbuf[sbuf[0]] = type;
	if(send_read_tucp_packet(dev, sbuf, rbuf)){
		ERROR("COMM_TRNS_RCV_ERR");
		return -1;
	}

	return 0;
}

int
comm_dev_info_req(dldev_t *dev)
{
	no_data_packet(sbuf, COMM_DEV_INFO_REQ);
	if(send_read_tucp_packet(dev, sbuf, rbuf)){
		ERROR("COMM_DEV_INFO_REQ");
		return -1;
	}

	memcpy(&dev->icb, rbuf+3, ICB_SIZE);

	return 0;
}

int
comm_proc_complete(dldev_t *dev)
{
	no_data_packet(sbuf, COMM_PROC_COMPLETE);
	if(send_read_tucp_packet(dev, sbuf, rbuf)){
		ERROR("COMM_PROC_COMPLETE");
		return -1;
	}

	/* simple ACK: do nothing */

	return 0;
}

int
comm_del_all_apps(dldev_t *dev)
{
#ifdef DEBUG
	ERROR("Do you know what you're doing?");
	return -1;
#endif

	no_data_packet(sbuf, COMM_DEL_ALL_APPS);
	if(send_read_tucp_packet(dev, sbuf, rbuf)){
		ERROR("COMM_DEL_ALL_APPS");
		return -1;
	}

	init_app_idx = -1;

	/* simple ACK: do nothing */

	return 0;
}

int
comm_beep(dldev_t *dev, u8 status)
{
	u8 cmd = COMM_BEEP;

	create_packet(sbuf, NULL, 0);
	create_packet(sbuf, &cmd, 1);
	create_packet(sbuf, &status, 1);
	create_packet(sbuf, NULL, 0);
	if(send_read_tucp_packet(dev, sbuf, rbuf)){
		ERROR("COMM_BEEP");
		return -1;
	}

	/* simple ACK: do nothing */

	return 0;
}

int
comm_idle(dldev_t *dev)
{
	no_data_packet(sbuf, COMM_IDLE);
	if(send_read_tucp_packet(dev, sbuf, rbuf)){
		ERROR("COMM_IDLE");
		return -1;
	}

	/* simple ACK: do nothing */

	return 0;
}

int
comm_call_abs_addr(dldev_t *dev, u16 addr)
{
	/* TODO */
	u8 cmd = COMM_CALL_ABS_ADDR;
	u8 addr_lo = lobyte(addr), addr_hi = hibyte(addr);

	create_packet(sbuf, NULL, 0);
	create_packet(sbuf, &cmd, 1);
	create_packet(sbuf, &addr_lo, 1);
	create_packet(sbuf, &addr_hi, 1);
	create_packet(sbuf, NULL, 0);
	if(send_read_tucp_packet(dev, sbuf, rbuf)){
		ERROR("COMM_CALL_ABS_ADDR");
		return -1;
	}

	/* simple ACK: do nothing */

	return 0;
}

int
comm_write_tod_tz(dldev_t *dev, u8 *data, u8 len)
{
	/* TODO */
	u8 cmd = COMM_WRITE_TOD_TZ;

	create_packet(sbuf, NULL, 0);
	create_packet(sbuf, &cmd, 1);
	create_packet(sbuf, data, len);
	create_packet(sbuf, NULL, 0);
	if(send_read_tucp_packet(dev, sbuf, rbuf)){
		ERROR("COMM_WRITE_TOD_TZ");
		return -1;
	}

	/* simple ACK: do nothing */

	return 0;
}

int
comm_app_init_int(dldev_t *dev, u8 *data, u8 len)
{
	/* TODO */
	u8 cmd = COMM_APP_INIT_INT;

	create_packet(sbuf, NULL, 0);
	create_packet(sbuf, &cmd, 1);
	create_packet(sbuf, data, len);
	create_packet(sbuf, NULL, 0);
	if(send_read_tucp_packet(dev, sbuf, rbuf)){
		ERROR("COMM_APP_INIT_INT");
		return -1;
	}

	if(init_app_idx < 0)
		init_app_idx = 2;
	else
		init_app_idx++;

	/* simple ACK: do nothing */

	return 0;
}

int
comm_app_init_ext(dldev_t *dev, u8 *data, u8 len)
{
	/* TODO */
	u8 cmd = COMM_APP_INIT_EXT;

	create_packet(sbuf, NULL, 0);
	create_packet(sbuf, &cmd, 1);
	create_packet(sbuf, data, len);
	create_packet(sbuf, NULL, 0);
	if(send_read_tucp_packet(dev, sbuf, rbuf)){
		ERROR("COMM_APP_INIT_EXT");
		return -1;
	}

	if(init_app_idx < 0)
		init_app_idx = 2;
	else
		init_app_idx++;

	/* simple ACK: do nothing */

	return 0;
}

int
comm_multi_packet_write(dldev_t *dev)
{
	/* TODO: what's this for? */
	no_data_packet(sbuf, COMM_MULTI_PACKET_WRITE);
	if(send_read_tucp_packet(dev, sbuf, rbuf)){
		ERROR("COMM_MULTI_PACKET_WRITE");
		return -1;
	}

	/* simple ACK: do nothing */

	return 0;
}

int
comm_write_abs_addr(dldev_t *dev, u16 addr, u8 type, u8 *data, u8 len)
{
	/* TODO */
	u8 cmd = COMM_WRITE_ABS_ADDR;
	u8 addr_lo, addr_hi, l;

	while(len > 0){
		addr_lo = lobyte(addr);
		addr_hi = hibyte(addr);
		l = (len > PAGE_SIZE ? PAGE_SIZE : len);

		create_packet(sbuf, NULL, 0);
		create_packet(sbuf, &cmd, 1);
		create_packet(sbuf, &addr_lo, 1);
		create_packet(sbuf, &addr_hi, 1);
		create_packet(sbuf, &type, 1);
		create_packet(sbuf, data, l);
		create_packet(sbuf, NULL, 0);
		if(send_read_tucp_packet(dev, sbuf, rbuf)){
			ERROR("COMM_WRITE_ABS_ADDR");
			return -1;
		}

		addr += l;
		data += l;
		len -= l;
	}

	/* simple ACK: do nothing */

	return 0;
}

int
comm_read_abs_addr(dldev_t *dev, u16 addr, u8 type, u8 *data, u8 len)
{
	/* TODO */
	u8 cmd = COMM_READ_ABS_ADDR;
	u8 addr_lo, addr_hi, l;

#ifdef EEPROM_FILE
	int fd;

	if(type != ext_mem){
		ERROR("type != ext_mem");
		return -1;
	}
	if(!(fd = open(EEPROM_FILE, O_RDONLY))){
		ERROR("%s: open failed", EEPROM_FILE);
		return -1;
	}
	if(lseek(fd, addr, SEEK_SET) < 0){
		ERROR("%s: seek failed", EEPROM_FILE);
		return -1;
	}
	if(read(fd, data, len) != len){
		ERROR("%s: read failed", EEPROM_FILE);
		return -1;
	}
	if(close(fd)){
		ERROR("%s: close failed", EEPROM_FILE);
		return -1;
	}

	return 0;
#endif
	while(len > 0){
		addr_lo = lobyte(addr);
		addr_hi = hibyte(addr);
		l = (len > PAGE_SIZE ? PAGE_SIZE : len);

		create_packet(sbuf, NULL, 0);
		create_packet(sbuf, &cmd, 1);
		create_packet(sbuf, &addr_lo, 1);
		create_packet(sbuf, &addr_hi, 1);
		create_packet(sbuf, &type, 1);
		create_packet(sbuf, &l, 1);
		create_packet(sbuf, NULL, 0);
		if(send_read_tucp_packet(dev, sbuf, rbuf)){
			ERROR("COMM_READ_ABS_ADDR");
			return -1;
		}

		memcpy(data, rbuf+3, l);

		addr += l;
		data += l;
		len -= l;
	}

	return 0;
}

static void
no_data_packet(u8 *packet, u8 cmd)
{
	create_packet(sbuf, NULL, 0);
	create_packet(sbuf, &cmd, 1);
	create_packet(sbuf, NULL, 0);

	return;
}

static void
create_packet(u8 *packet, u8 *data, u8 len)
{
	static u8 start_or_end = 0, pos;

	if(!data){
		if(!start_or_end){
			start_or_end = 1;
			pos = 1;
		}else{
			u8 i, checksum = 0;

			start_or_end = 0;

			packet[0] = pos + 1;
			for(i = 0; i < pos; i++)
				checksum += packet[i];
			checksum = ~checksum + 1;
			packet[i] = checksum;
		}
		return;
	}

	memcpy(packet+pos, data, len);
	pos += len;

	return;
}
