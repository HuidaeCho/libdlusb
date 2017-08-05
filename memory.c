/* $Id: memory.c,v 1.38 2009/07/13 05:13:18 geni Exp $
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
#include <fcntl.h>
#include <sys/stat.h>
#include "dlusb.h"

static u8 buf[MAX_PACKET_SIZE];

int
read_sysinfo(dldev_t *dev)
{
	if(read_acd(dev)){
		ERROR("read_acd");
		return -1;
	}

	if(read_acb(dev)){
		ERROR("read_acb");
		return -1;
	}

	return 0;
}

int
read_sysmap(dldev_t *dev)
{
	if(read_abs_addr(dev, SYSMAP_ADDR, int_mem, buf, SYSMAP_SIZE)){
		ERROR("read_abs_addr");
		return -1;
	}

	memcpy(&dev->sysmap, buf, sizeof(dev->sysmap));

#ifdef DEBUG
	ERROR("dev->sysmap.heap_top_addr: 0x%04x", dev->sysmap.heap_top_addr);
	ERROR("dev->sysmap.heap_top_addr_r: 0x%04x", dev->sysmap.heap_top_addr_r);
	ERROR("dev->sysmap.heap_bot_addr_r: 0x%04x", dev->sysmap.heap_bot_addr_r);
	ERROR("dev->sysmap.overlay_size: 0x%04x", dev->sysmap.overlay_size);
	ERROR("dev->sysmap.mode_ls_addr: 0x%04x", dev->sysmap.mode_ls_addr);
	ERROR("dev->sysmap.acd_addr: 0x%04x", dev->sysmap.acd_addr);
	ERROR("dev->sysmap.acb_addr: 0x%04x", dev->sysmap.acb_addr);
	ERROR("dev->sysmap.opt_addr: 0x%04x", dev->sysmap.opt_addr);
	ERROR("dev->sysmap.ptc_addr: 0x%04x", dev->sysmap.ptc_addr);
	ERROR("dev->sysmap.melody_addr: 0x%04x", dev->sysmap.melody_addr);
	ERROR("dev->sysmap.melody_init_addr: 0x%04x", dev->sysmap.melody_init_addr);
	ERROR("dev->sysmap.cust_melody_addr: 0x%04x", dev->sysmap.cust_melody_addr);
	ERROR("dev->sysmap.def_melody_addr: 0x%04x", dev->sysmap.def_melody_addr);
#endif

	return 0;
}

int
read_acd(dldev_t *dev)
{
	int i;

	if(read_abs_addr(dev, dev->sysmap.acd_addr, int_mem, buf, NUM_APPS)){
		ERROR("read_abs_addr");
		return -1;
	}
	for(i = 0; i < NUM_APPS; i++)
		memcpy(&dev->app[i].acd, buf+i, 1);

	return 0;
}

int
read_acb(dldev_t *dev)
{
	int i;

	if(read_abs_addr(dev, dev->sysmap.acb_addr, int_mem, buf,
				NUM_APPS*ACB_SIZE)){
		ERROR("read_abs_addr");
		return -1;
	}

	for(i = 0; i < NUM_APPS; i++){
		memcpy(&dev->app[i].acb, buf+i*ACB_SIZE, ACB_SIZE);
		if(!dev->app[i].acd.app_idx)
			continue;
#ifdef DEBUG
		ERROR("dev->app[%d].acb.app_type: 0x%04x", i, dev->app[i].acb.app_type);
		ERROR("dev->app[%d].acb.app_inst: 0x%04x", i, dev->app[i].acb.app_inst);
		ERROR("dev->app[%d].acb.asd_addr: 0x%04x", i, dev->app[i].acb.asd_addr);
		ERROR("dev->app[%d].acb.add_addr: 0x%04x", i, dev->app[i].acb.add_addr);
		ERROR("dev->app[%d].acb.state_mgr_addr: 0x%04x", i, dev->app[i].acb.state_mgr_addr);
		ERROR("dev->app[%d].acb.refresh_addr: 0x%04x", i, dev->app[i].acb.refresh_addr);
		ERROR("dev->app[%d].acb.banner_addr: 0x%04x", i, dev->app[i].acb.banner_addr);
		ERROR("dev->app[%d].acb.code_addr: 0x%04x", i, dev->app[i].acb.code_addr);
#endif
		if(dev->app[i].acb.app_type == APP_TOD)
			strcpy(dev->app[i].banner, BANNER_TOD);
		else
		if(dev->app[i].acb.app_type == APP_COMM)
			strcpy(dev->app[i].banner, BANNER_COMM);
		else
		if(dev->app[i].acb.app_type == APP_SYS)
			strcpy(dev->app[i].banner, BANNER_SYS);
		else{
			u16 banner_addr;
			u8 mem, buf[MAX_BANNER_SIZE], j, k;

			/* mode banner message database in EEPROM */
			/* A WristApp installer is responsible for storing user
			 * banners in EEPROM.  init_dlusb does not set
			 * user_banner to 1, but will save the file name of
			 * *.app in the corresponding address for reference. */
			/*
			if(dev->app[i].acd.user_banner){
			*/
			if(dev->app[i].acd.code_loc){
				mem = ext_mem;
				banner_addr = 0x40 + (i - 2) * 27;
			}else{
				mem = int_mem;
				banner_addr = dev->app[i].acb.banner_addr;
			}

			if(read_abs_addr(dev, banner_addr, mem, buf,
						MAX_BANNER_SIZE)){
				ERROR("read_abs_addr");
				return -1;
			}

			/* the last character should be LCD_END_BANNER */
			for(j = 0; (buf[j]&0x80) && j < MAX_BANNER_SIZE-1; j++);
			for(k = 0; buf[j]!=0xff && j < MAX_BANNER_SIZE-1; j++){
				if(buf[j] & 0x80)
					buf[j] = 10;
				dev->app[i].banner[k++] = rdm(buf[j]);
			}
			dev->app[i].banner[k] = 0;
		}
#ifdef DEBUG
		ERROR("dev->app[%d].banner: '%s'", i, dev->app[i].banner);
#endif
	}

	return 0;
}

int
find_app(dldev_t *dev, char *banner)
{
	int i;

	for(i = 0; i < NUM_APPS; i++){
		if(dev->app[i].acd.app_idx &&
				!strcmp(dev->app[i].banner, banner))
			break;
	}
	/*
	if(i == NUM_APPS)
		ERROR("%s: no such application", banner);
	*/

	return (i == NUM_APPS ? -1 : i);
}

int
lock_app(dldev_t *dev, char *banner)
{
	int i;

	if(!strcmp(banner, BANNER_TOD) || !strcmp(banner, BANNER_COMM) ||
			!strcmp(banner, BANNER_SYS))
		return -1;

	if((i = find_app(dev, banner)) < 0)
		return -1;

	dev->app[i].acd.passwd_req = 1;

	return 0;
}

int
unlock_app(dldev_t *dev, char *banner)
{
	int i;

	if(!strcmp(banner, BANNER_TOD) || !strcmp(banner, BANNER_COMM) ||
			!strcmp(banner, BANNER_SYS))
		return -1;

	if((i = find_app(dev, banner)) < 0)
		return -1;

	dev->app[i].acd.passwd_req = 0;

	return 0;
}

int
update_acd(dldev_t *dev)
{
	int i;
	u8 buf[NUM_APPS];

	for(i = 0; i < NUM_APPS; i++)
		memcpy(buf+i, &dev->app[i].acd, 1);

	if(write_abs_addr(dev, dev->sysmap.acd_addr, int_mem, buf, NUM_APPS)){
		ERROR("write_abs_addr");
		return -1;
	}

	return 0;
}

int
update_acb(dldev_t *dev)
{
	int i;
	u8 buf[NUM_APPS*ACB_SIZE];

	for(i = 0; i < NUM_APPS; i++)
		memcpy(buf+i*ACB_SIZE, &dev->app[i].acb, ACB_SIZE);

	if(write_abs_addr(dev, dev->sysmap.acb_addr, int_mem, buf,
				NUM_APPS*ACB_SIZE)){
		ERROR("write_abs_addr");
		return -1;
	}

	return 0;
}

int
dump_acd(dldev_t *dev, u8 *data)
{
	if(read_abs_addr(dev, dev->sysmap.acd_addr, int_mem, data, NUM_APPS)){
		ERROR("read_abs_addr");
		return -1;
	}

	return 0;
}

int
dump_acb(dldev_t *dev, u8 *data)
{
	if(read_abs_addr(dev, dev->sysmap.acb_addr, int_mem, data,
				NUM_APPS*ACB_SIZE)){
		ERROR("read_abs_addr");
		return -1;
	}

	return 0;
}

int
dump_add(dldev_t *dev, int idx, u8 **data, u16 *len)
{
	u16 add_addr;
	u8 mem_type;

	if(idx < 0 || idx >= NUM_APPS){
		ERROR("%d: application index out of range", idx);
		return -1;
	}

	add_addr = dev->app[idx].acb.add_addr;
	if(!add_addr){
		ERROR("no add to dump");
		return -1;
	}

	/* TODO: db_loc == int_mem: how to handle it? same as ext_mem? */
	mem_type = dev->app[idx].acd.db_loc;

	if(read_abs_addr(dev, add_addr, mem_type, (u8 *)len, 2)){
		ERROR("read_abs_addr");
		return -1;
	}

	*data = (u8 *)malloc(*len);
	if(read_abs_addr(dev, add_addr, mem_type, *data, *len)){
		ERROR("read_abs_addr");
		return -1;
	}

	return 0;
}

int
dump_rom(dldev_t *dev, u8 **data, u16 *len)
{
	*len = ROM_SIZE;
	*data = (u8 *)malloc(*len);

	if(read_abs_addr(dev, 0, int_mem, *data, *len)){
		ERROR("read_abs_addr");
		return -1;
	}

	return 0;
}

int
dump_eeprom(dldev_t *dev, u8 **data, u16 *len)
{
	*len = dev->icb.eeprom_size;
	*data = (u8 *)malloc(*len);

	if(read_abs_addr(dev, 0, ext_mem, *data, *len)){
		ERROR("read_abs_addr");
		return -1;
	}

	return 0;
}

int
load_add(dldev_t *dev, int idx, u8 *data)
{
	int i, num_ext_adds, next_idx;
	u8 *add[NUM_APPS];
	u16 add_size[NUM_APPS], alloc_size, alloc_end, code_start, db_end,
	    total_add_size, avail_db_size, unused_db_size, dist_db_size;

	if(idx < 0 || idx >= NUM_APPS){
		ERROR("%d: application index out of range", idx);
		return -1;
	}
	if(!dev->app[idx].acb.add_addr){
		ERROR("no add to load");
		return -1;
	}

	num_ext_adds = 0;
	next_idx = -1;
	for(i = 0; i < NUM_APPS; i++){
		if(!dev->app[i].acd.app_idx || !dev->app[i].acd.db_loc)
			continue;
		num_ext_adds++;
		if(next_idx == -1 && i > idx)
			next_idx = i;
	}

	/* TODO: no external memory ADD. how to deal with it? */
	if(!num_ext_adds){
		if(!dev->app[idx].acd.db_loc){
			/* TODO: internal memory ADD */
		}else{
			/* ERROR! no external memory ADD found and it's
			 * external ADD? */
			ERROR("ADD CORRUPTED! THIS SHOULD NOT HAPPEN!");
			return -1;
		}
		return -1;
	}

	alloc_size = word(data[1], data[0]);
	alloc_end = dev->app[idx].acb.add_addr + alloc_size;

	code_start = CODE_START;
	for(i = 0; i < NUM_APPS; i++){
		if(!dev->app[i].acd.app_idx || !dev->app[i].acd.code_loc)
			continue;
		if(code_start > dev->app[i].acb.code_addr)
			code_start = dev->app[i].acb.code_addr;
	}

	/* all possible cases:
	 *
	 * 1. only one external ADD (num_ext_adds == 1 -> next_idx == -1)
	 *    1.1. overlapped with code area (alloc_end > code_start)
	 *    1.2. not overlapped (alloc_end <= code_start)
	 *
	 * 2. current ADD is adjacent to the next ADD (next_idx != -1)
	 *    2.1. overlapped with the next ADD (alloc_end >= next_idx.add_addr)
	 *    2.2. not overlapped (alloc_end < next_idx.add_addr)
	 *
	 * 3. current ADD is adjacent to code area (next_idx == -1)
	 *    3.1. overlapped with code area (alloc_end > code_start)
	 *    3.2. not overlapped (alloc_end <= code_start)
	 */

	/* case 1.1 */
	if(num_ext_adds == 1 && alloc_end > code_start){
		/* we don't have enough memory to store data */
		ERROR("not enough memory");
		return -1;
	}

#ifdef COMM_READ_ABS_ADDR__AFTER__COM_MULTI_PACKET_WRITE
	if(comm_multi_packet_write(dev)){
		ERROR("comm_multi_packet_write");
		return -1;
	}
#endif

	/* cases 1.2, 2.2, and 3.2: 3.2 includes 1.2 */
	if((next_idx != -1 && alloc_end < dev->app[next_idx].acb.add_addr) ||
			(next_idx == -1 && alloc_end <= code_start)){
#ifndef COMM_READ_ABS_ADDR__AFTER__COM_MULTI_PACKET_WRITE
		if(comm_multi_packet_write(dev)){
			ERROR("comm_multi_packet_write");
			return -1;
		}
#endif

		if(write_abs_addr(dev, dev->app[idx].acb.add_addr,
				dev->app[idx].acd.db_loc, data, alloc_size)){
			ERROR("write_abs_addr");
			return -1;
		}

		/* dev->app[idx].acd.db_modified = 1; doesn't work with the
		 * Timex PIM */
		dev->app[idx].acd.db_modified = 0;
		if(write_abs_addr(dev, dev->sysmap.acd_addr+idx, int_mem,
					(u8 *)&dev->app[idx].acd, 1)){
			ERROR("write_abs_addr");
			return -1;
		}

		return 0;
	}

	/* otherwise: cases 2.1 and 3.1 */

	total_add_size = 0;
	for(i = 0; i < NUM_APPS; i++){
		if(!dev->app[i].acd.app_idx || !dev->app[i].acd.db_loc)
			continue;
		if(i == idx){
			/* new allocated memory size */
			add_size[i] = alloc_size;
			add[i] = data;
		}else{
			if(read_abs_addr(dev, dev->app[i].acb.add_addr, ext_mem,
						(u8 *)&add_size[i], 2)){
				ERROR("read_abs_addr");
				return -1;
			}
			add[i] = (u8 *)malloc(add_size[i]);
			if(read_abs_addr(dev, dev->app[i].acb.add_addr, ext_mem,
						(u8 *)add[i], add_size[i])){
				ERROR("read_abs_addr");
				return -1;
			}
		}
		total_add_size += add_size[i];
	}
	/* [CODE]{...<add3>...<add2>...<add1>}: avail_db_size = {...} */
	avail_db_size = code_start - DB_START;
	unused_db_size = avail_db_size - total_add_size;
	if(unused_db_size <= 0){
		/* we don't have enough memory to store data */
		ERROR("not enough memory");
		return -1;
	}
	/* make sure that unused_db_size is a multiple of PAGE_SIZE */
	unused_db_size = (unused_db_size / PAGE_SIZE) * PAGE_SIZE;

	/* dist_db_size * num_ext_adds should be less or equal to *
	 * unused_db_size */
	dist_db_size = (unused_db_size / (num_ext_adds*PAGE_SIZE)) * PAGE_SIZE;

	db_end = DB_START;
	for(i = 0; i < NUM_APPS; i++){
		if(!dev->app[i].acd.app_idx || !dev->app[i].acd.db_loc)
			continue;
		dev->app[i].acb.add_addr = db_end;
		/* allocated memory */
		db_end += add_size[i];
		/* distribute unused memory to all ADDs if possible */
		if(dist_db_size)
			db_end += dist_db_size;
		else
		/* if dist_db_size is 0 (not enough memory for all external
		 * ADDs, the rest of the unused memory will be allocated to new
		 * data */
		if(i == idx)
			db_end += unused_db_size;
	}

	/* OK, now move ADD memory according to new add_addrs */
#ifndef COMM_READ_ABS_ADDR__AFTER__COM_MULTI_PACKET_WRITE
	if(comm_multi_packet_write(dev)){
		ERROR("comm_multi_packet_write");
		return -1;
	}
#endif

	for(i = 0; i < NUM_APPS; i++){
		if(!dev->app[i].acd.app_idx || !dev->app[i].acd.db_loc)
			continue;
		if(write_abs_addr(dev, dev->app[i].acb.add_addr, ext_mem,
					add[i], add_size[i])){
			ERROR("write_abs_addr");
			return -1;
		}
		if(i != idx){
			free(add[i]);
			continue;
		}

		/* dev->app[i].acd.db_modified = 1; doesn't work with the Timex
		 * PIM */
		dev->app[i].acd.db_modified = 0;
		if(write_abs_addr(dev, dev->sysmap.acd_addr+i, int_mem,
					(u8 *)&dev->app[i].acd, 1)){
			ERROR("write_abs_addr");
			return -1;
		}
	}

	/* update ACB */
	if(update_acb(dev)){
		ERROR("update_acb");
		return -1;
	}

	return 0;
}

int
del_all_apps(dldev_t *dev)
{
	if(comm_del_all_apps(dev)){
		ERROR("comm_del_all_apps");
		return -1;
	}
	/* exclude TOD and COMM */
	memset(buf, 0, (NUM_APPS-2)*ACB_SIZE);
	if(write_abs_addr(dev, dev->sysmap.acb_addr+2*ACB_SIZE, int_mem, buf,
				(NUM_APPS-2)*ACB_SIZE)){
		ERROR("write_abs_addr");
		return -1;
	}

	return 0;
}

int
init_int_app(dldev_t *dev, int por, u8 *data, u16 len)
{
	int i;
	u8 buf[5];

	/*
	if(comm_multi_packet_write(dev)){
		ERROR("comm_multi_packet");
		return -1;
	}
	if(comm_del_all_apps(dev)){
		ERROR("comm_del_all_apps");
		return -1;
	}
	*/

	i = 0;
	buf[i++] = lobyte(len);
	buf[i++] = hibyte(len);
	buf[i++] = 0x00;	/* no password (6th bit), no banner (7th bit) */
	buf[i++] = get_init_app_inst();	/* application instance */
	buf[i++] = por;

	if(comm_app_init_int(dev, buf, i)){
		ERROR("comm_app_init_int");
		return -1;
	}

	/* reread the system info to find the application if it was not *
	 * available before */
	if(read_sysinfo(dev)){
		ERROR("read_sysinfo");
		return -1;
	}

	/* get the latest application index */
	if((i = get_init_app_idx()) < 0){
		ERROR("THIS SHOULD NOT HAPPEN!");
		return -1;
	}

	if(write_abs_addr(dev, dev->app[i].acb.add_addr, ext_mem, data, len)){
		ERROR("write_abs_addr");
		return -1;
	}

	return 0;
}

int
init_ext_app(dldev_t *dev, char *app, int user_banner)
{
	FILE *fp;
	char *buf, *par_, *code_, *db_;
	u8 *banner, *par, *code, *db;
	u16 par_len, code_len, db_len;
	int i, j, k, fd;
	struct stat sb;

	if(!(fp = fopen(app, "r"))){
		ERROR("%s: open failed", app);
		return -1;
	}

	par_ = code_ = db_ = NULL;
	while((buf = read_line(fp))){
		if(strstr(buf, "PAR=") == buf && buf[4]){
			par_ = (char *)malloc(strlen(buf+4)+1);
			strcpy(par_, buf+4);
		}else
		if(strstr(buf, "CODE=") == buf && buf[5]){
			code_ = (char *)malloc(strlen(buf+5)+1);
			strcpy(code_, buf+5);
		}else
		if(strstr(buf, "DB=") == buf && buf[3]){
			db_ = (char *)malloc(strlen(buf+3)+1);
			strcpy(db_, buf+3);
		}
		free(buf);
	}

	fclose(fp);

	if(!par_){
		ERROR("PAR field empty!");
		return -1;
	}
	if(!code_){
		ERROR("CODE field empty!");
		return -1;
	}

	/* find directory name */
	for(i = strlen(app) - 1; i >= 0 && app[i] != '/'; i--);

	/* read parameter file */
	if(i >= 0){
		buf = (char *)malloc(i + strlen(par_) + 2);
		strcpy(buf, app);
		buf[i+1] = 0;
		strcat(buf, par_);
	}else{
		buf = (char *)malloc(strlen(par_) + 1);
		strcpy(buf, par_);
	}
	free(par_);
	if(stat(buf, &sb)){
		ERROR("%s: stat failed", buf);
		return -1;
	}
	par_len = sb.st_size;
	if((fd = open(buf, O_RDONLY)) < 0){
		ERROR("%s: open failed", buf);
		return -1;
	}
	par = (u8*)malloc(par_len);
	if(read(fd, par, par_len) != par_len){
		ERROR("%s: read failed", buf);
		return -1;
	}
	free(buf);
	close(fd);

	/* read code file */
	if(i >= 0){
		buf = (char *)malloc(i + strlen(code_) + 2);
		strcpy(buf, app);
		buf[i+1] = 0;
		strcat(buf, code_);
	}else{
		buf = (char *)malloc(strlen(code_) + 1);
		strcpy(buf, code_);
	}
	free(code_);
	if(stat(buf, &sb)){
		ERROR("%s: stat failed", buf);
		return -1;
	}
	code_len = sb.st_size;
	if((fd = open(buf, O_RDONLY)) < 0){
		ERROR("%s: open failed", buf);
		return -1;
	}
	code = (u8*)malloc(code_len);
	if(read(fd, code, code_len) != code_len){
		ERROR("%s: read failed", buf);
		return -1;
	}
	free(buf);
	close(fd);

	/* read db file, if any */
	if(db_){
		if(i >= 0){
			buf = (char *)malloc(i + strlen(db_) + 2);
			strcpy(buf, app);
			buf[i+1] = 0;
			strcat(buf, db_);
		}else{
			buf = (char *)malloc(strlen(db_) + 1);
			strcpy(buf, db_);
		}
		free(db_);
		if(stat(buf, &sb)){
			ERROR("%s: stat failed", buf);
			return -1;
		}
		db_len = sb.st_size;
		if((fd = open(buf, O_RDONLY)) < 0){
			ERROR("%s: open failed", buf);
			return -1;
		}
		db = (u8*)malloc(db_len);
		if(read(fd, db, db_len) != db_len){
			ERROR("%s: read failed", buf);
			return -1;
		}
		free(buf);
		close(fd);
	}else{
		db = NULL;
		db_len = 0;
	}

	/* now, app includes only file name */
	app += i + 1;

	par[0x0d] = lobyte(db_len);
	par[0x0e] = hibyte(db_len);
	if(user_banner)
		par[0x0f] |= 0x80;
	else
		par[0x0f] &= ~0x80;
	par[0x11] = get_init_app_inst();

	if(comm_app_init_ext(dev, par, par_len)){
		ERROR("comm_app_init_ext");
		return -1;
	}
	free(par);

	/* reread the system info to find the application if it was not
	 * available before */
	if(read_sysinfo(dev)){
		ERROR("read_sysinfo");
		return -1;
	}

	/* get the latest application index */
	if((i = get_init_app_idx()) < 0){
		ERROR("THIS SHOULD NOT HAPPEN!");
		return -1;
	}

	if(write_abs_addr(dev, dev->app[i].acb.code_addr, ext_mem, code,
				code_len)){
		ERROR("write_abs_addr");
		return -1;
	}
	free(code);

	if(db){
		if(write_abs_addr(dev, dev->app[i].acb.add_addr, ext_mem, db,
					db_len)){
			ERROR("write_abs_addr");
			return -1;
		}
		free(db);
	}

	/* mode banner */
	for(j = strlen(app) - 1; j >= 0 && app[j] != '.'; j--);
	if(j <= 0)
		j = strlen(app);
	j++;
	k = j + 1;
	banner = (u8 *)malloc(k);
	banner[j] = 0xff;
	for(j--; j > 0; j--)
		banner[j] = rdmi(app[j-1]);
	/* the first column */
	banner[j] = 0x82;

	if(write_abs_addr(dev, 0x40 + (i - 2) * 27, ext_mem, banner, k)){
		ERROR("write_abs_addr");
		return -1;
	}
	free(banner);

	return 0;
}

#ifdef EXPERIMENTAL
int
read_tod(dldev_t *dev, tod_t *db)
{
	int i, j;
	u16 add_addr;

	if((i = find_app(dev, BANNER_TOD)) < 0){
		ERROR("Time/Date application not found");
		return -1;
	}

	add_addr = dev->app[i].acb.add_addr;
	if(read_abs_addr(dev, add_addr, ext_mem, (u8 *)db, 39)){
		ERROR("read_abs_addr");
		return -1;
	}

	for(i = 0; i < 3; i++){
		for(j = 0; j < 3; j++)
			db->tod[i].tz_name[j] = rdm(db->tod[i].tz_name[j]);

		db->tod[i].second = debcd(db->tod[i].second);
		db->tod[i].minute = debcd(db->tod[i].minute);
		db->tod[i].hour = debcd(db->tod[i].hour);
		db->tod[i].day = debcd(db->tod[i].day);
		db->tod[i].month = debcd(db->tod[i].month);
		db->tod[i].year = debcd2(db->tod[i].year);

		if(db->tod[i].second > 59) db->tod[i].second = 59;
		if(db->tod[i].minute > 59) db->tod[i].minute = 59;
		if(db->tod[i].hour > 23) db->tod[i].hour = 23;
		if(db->tod[i].day < 1) db->tod[i].day = 1; else
		if(db->tod[i].day > 31) db->tod[i].day = 31;
		if(db->tod[i].month < 1) db->tod[i].month = 1; else
		if(db->tod[i].month > 12) db->tod[i].month = 12;
		if(db->tod[i].year < 2000) db->tod[i].year = 2000; else
		if(db->tod[i].year > 2099) db->tod[i].year = 2099;
	}

	return 0;
}
#endif

int
update_tod(dldev_t *dev, tod_t *db)
{
	int i, j;
	tod_t tod;

	if((i = find_app(dev, BANNER_TOD)) < 0){
		ERROR("Time/Date application not found");
		return -1;
	}

	memcpy(&tod, db, sizeof(tod_t));

	for(i = 0; i < 3; i++){
		for(j = 0; j < 3 && tod.tod[i].tz_name[j]; j++)
			tod.tod[i].tz_name[j] =
				rdmi(tod.tod[i].tz_name[j]);
		for(; j < 3; j++)
			tod.tod[i].tz_name[j] = rdmi(' ');

		if(tod.tod[i].second > 59) tod.tod[i].second = 59;
		if(tod.tod[i].minute > 59) tod.tod[i].minute = 59;
		if(tod.tod[i].hour > 23) tod.tod[i].hour = 23;
		if(tod.tod[i].day < 1) tod.tod[i].day = 1; else
		if(tod.tod[i].day > 31) tod.tod[i].day = 31;
		if(tod.tod[i].month < 1) tod.tod[i].month = 1; else
		if(tod.tod[i].month > 12) tod.tod[i].month = 12;
		if(tod.tod[i].year < 2000) tod.tod[i].year = 2000; else
		if(tod.tod[i].year > 2099) tod.tod[i].year = 2099;

		tod.tod[i].second = bcd(tod.tod[i].second);
		tod.tod[i].minute = bcd(tod.tod[i].minute);
		tod.tod[i].hour = bcd(tod.tod[i].hour);
		tod.tod[i].day = bcd(tod.tod[i].day);
		tod.tod[i].month = bcd(tod.tod[i].month);
		tod.tod[i].year = bcd2(tod.tod[i].year);
	}

	if(comm_write_tod_tz(dev, (u8 *)&tod, sizeof(tod_t))){
		ERROR("comm_write_tod_tz");
		return -1;
	}

	return 0;
}

int
read_option(dldev_t *dev, option_t *db)
{
	if(read_abs_addr(dev, dev->sysmap.opt_addr, int_mem, (u8 *)db, 15)){
		ERROR("read_abs_addr");
		return -1;
	}

	db->nite_on_hour = debcd(db->nite_on_hour);
	db->nite_on_minute = debcd(db->nite_on_minute);
	db->nite_off_hour = debcd(db->nite_off_hour);
	db->nite_off_minute = debcd(db->nite_off_minute);
	db->chime_on_hour = debcd(db->chime_on_hour);
	db->chime_on_minute = debcd(db->chime_on_minute);
	db->chime_off_hour = debcd(db->chime_off_hour);
	db->chime_off_minute = debcd(db->chime_off_minute);
	db->passwd[0] = rdm(db->passwd[0]);
	db->passwd[1] = rdm(db->passwd[1]);

	return 0;
}

int
update_option(dldev_t *dev, option_t *db)
{
	option_t opt;

	memcpy(&opt, db, 15);

	opt.nite_on_hour = bcd(opt.nite_on_hour);
	opt.nite_on_minute = bcd(opt.nite_on_minute);
	opt.nite_off_hour = bcd(opt.nite_off_hour);
	opt.nite_off_minute = bcd(opt.nite_off_minute);
	opt.chime_on_hour = bcd(opt.chime_on_hour);
	opt.chime_on_minute = bcd(opt.chime_on_minute);
	opt.chime_off_hour = bcd(opt.chime_off_hour);
	opt.chime_off_minute = bcd(opt.chime_off_minute);
	opt.passwd[0] = rdmi(opt.passwd[0]);
	opt.passwd[1] = rdmi(opt.passwd[1]);

	if(write_abs_addr(dev, dev->sysmap.opt_addr, int_mem, (u8 *)&opt, 15)){
		ERROR("write_abs_addr");
		return -1;
	}

	return 0;
}

void
print_option(option_t *db, FILE *fp)
{
	fprintf(fp, "nite_mode: %d\n", db->nite_mode);
	fprintf(fp, "nite_auto: %d\n", db->nite_auto);
	fprintf(fp, "chime: %d\n", db->chime);
	fprintf(fp, "chime_auto: %d\n", db->chime_auto);
	fprintf(fp, "beep: %d\n", db->beep);
	fprintf(fp, "nite_on_time: %02d:%02d\n", db->nite_on_hour,
			db->nite_on_minute);
	fprintf(fp, "nite_off_time: %02d:%02d\n", db->nite_off_hour,
			db->nite_off_minute);
	fprintf(fp, "chime_on_time: %02d:%02d\n", db->chime_on_hour,
			db->chime_on_minute);
	fprintf(fp, "chime_off_time: %02d:%02d\n", db->chime_off_hour,
			db->chime_off_minute);
	fprintf(fp, "nite_mode_toggle_duration: %d\n",
			db->nite_mode_toggle_duration);
	fprintf(fp, "last_set_char: %d\n", db->last_set_char);
	fprintf(fp, "passwd: %c%c\n", db->passwd[0], db->passwd[1]);
	fprintf(fp, "timeline_occasion: %d\n", db->timeline_occasion);
	fprintf(fp, "timeline_unknown: %d\n", db->timeline_unknown);
	fprintf(fp, "nite_mode_duration: %d\n", db->nite_mode_duration);

	return;
}

void
scan_option(option_t *db, FILE *fp)
{
	int i, j;
	char *line;

	while((line = read_line(fp))){
		if(sscanf(line, "nite_mode: %d", &i))
			db->nite_mode = (i != 0);
		else
		if(sscanf(line, "nite_auto: %d", &i))
			db->nite_auto = (i != 0);
		else
		if(sscanf(line, "chime: %d", &i))
			db->chime = (i != 0);
		else
		if(sscanf(line, "chime_auto: %d", &i))
			db->chime_auto = (i != 0);
		else
		if(sscanf(line, "beep: %d", &i))
			db->beep = (i != 0);
		else
		if(sscanf(line, "nite_on_time: %d:%d", &i, &j) == 2){
			db->nite_on_hour = (i >= 0 && i < 24 ? i :
					NITE_ON_HOUR);
			db->nite_on_minute = (j >= 0 && j < 60 ? j :
					NITE_ON_MINUTE);
		}else
		if(sscanf(line, "nite_off_time: %d:%d", &i, &j) == 2){
			db->nite_off_hour = (i >= 0 && i < 24 ? i :
					NITE_OFF_HOUR);
			db->nite_off_minute = (j >= 0 && j < 60 ? j :
					NITE_OFF_MINUTE);
		}else
		if(sscanf(line, "chime_on_time: %d:%d", &i, &j) == 2){
			db->chime_on_hour = (i >= 0 && i < 24 ? i :
					CHIME_ON_HOUR);
			db->chime_on_minute = (j >= 0 && j < 60 ? j :
					CHIME_ON_MINUTE);
		}else
		if(sscanf(line, "chime_off_time: %d:%d", &i, &j) == 2){
			db->chime_off_hour = (i >= 0 && i < 24 ? i :
					CHIME_OFF_HOUR);
			db->chime_off_minute = (j >= 0 && j < 60 ? j :
					CHIME_OFF_MINUTE);
		}else
		if(sscanf(line, "nite_mode_toggle_duration: %d", &i))
			db->nite_mode_toggle_duration = (u8)i;
		else
		if(sscanf(line, "last_set_char: %d", &i))
			db->last_set_char = (u8)i;
		else
		if(sscanf(line, "passwd: %c%c", (char *)&i, (char *)&j) == 2){
			db->passwd[0] = (u8)i;
			db->passwd[1] = (u8)j;
		}else
		if(sscanf(line, "timeline_occasion: %d", &i))
			db->timeline_occasion = (i != 0);
		else
		if(sscanf(line, "timeline_unknown: %d", &i))
			db->timeline_unknown = (i != 0);
		else
		if(sscanf(line, "nite_mode_duration: %d", &i))
			db->nite_mode_duration = (u8)i;
	}

	return;
}

int
read_raw_option(dldev_t *dev, u8 *data)
{
	if(read_abs_addr(dev, dev->sysmap.opt_addr, int_mem, data, 15)){
		ERROR("read_abs_addr");
		return -1;
	}

	return 0;
}

int
update_raw_option(dldev_t *dev, u8 *data)
{
	if(write_abs_addr(dev, dev->sysmap.opt_addr, int_mem, data, 15)){
		ERROR("write_abs_addr");
		return -1;
	}

	return 0;
}

void
scan_raw_option(option_t *db, u8 *data)
{
	memcpy(&db, data, 15);

	db->nite_on_hour = debcd(db->nite_on_hour);
	db->nite_on_minute = debcd(db->nite_on_minute);
	db->nite_off_hour = debcd(db->nite_off_hour);
	db->nite_off_minute = debcd(db->nite_off_minute);
	db->chime_on_hour = debcd(db->chime_on_hour);
	db->chime_on_minute = debcd(db->chime_on_minute);
	db->chime_off_hour = debcd(db->chime_off_hour);
	db->chime_off_minute = debcd(db->chime_off_minute);
	db->passwd[0] = rdm(db->passwd[0]);
	db->passwd[1] = rdm(db->passwd[1]);

	return;
}

void
create_raw_option(option_t *db, u8 *data)
{
	option_t opt;

	memcpy(&opt, db, 15);

	opt.nite_on_hour = bcd(opt.nite_on_hour);
	opt.nite_on_minute = bcd(opt.nite_on_minute);
	opt.nite_off_hour = bcd(opt.nite_off_hour);
	opt.nite_off_minute = bcd(opt.nite_off_minute);
	opt.chime_on_hour = bcd(opt.chime_on_hour);
	opt.chime_on_minute = bcd(opt.chime_on_minute);
	opt.chime_off_hour = bcd(opt.chime_off_hour);
	opt.chime_off_minute = bcd(opt.chime_off_minute);
	opt.passwd[0] = rdmi(opt.passwd[0]);
	opt.passwd[1] = rdmi(opt.passwd[1]);

	memcpy(data, &opt, 15);
}
