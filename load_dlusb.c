/* $Id: load_dlusb.c,v 1.1 2005/10/02 09:21:22 geni Exp $
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
#include <ctype.h>
#include <sys/stat.h>
#include "dlusb.h"
#include "misc.h"

static void usage(void);

int
main(int argc, char *argv[])
{
	int ret = 1, i, idx, fd, num_apps = 0;
	char *dev_file = "/dev/uhid0", *banner, *file;
	struct stat sb;
	dldev_t dev;
	appinfo *app = NULL;
	u8 *data;
	u16 len;

	/* for data security */
	/*
	umask(S_IRWXG | S_IRWXO);
	*/

	while((i = getopt(argc, argv, "hd:")) != -1){
		switch(i){
		case 'd':
			dev_file = optarg;
			break;
		case 'h':
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

#ifdef USB_USBHID
	dev.usb.file = dev_file;
#endif

	BEGIN_OPT()
#if 0
		APP_OPT("rom", -3)
		APP_OPT("eeprom", -2)
#endif
		APP_OPT("wristapp", -1)
		APP_OPT("contact", APP_CONTACT)
		APP_OPT("note", APP_NOTE)
		APP_OPT("appt", APP_APPT)
		APP_OPT("alarm", APP_ALARM)
		APP_OPT("schedule", APP_SCHEDULE)
		APP_OPT("occasion", APP_OCCASION)
		APP_OPT("chrono", APP_CHRONO)
		APP_OPT("countdn", APP_TIMER)
		APP_OPT("interval", APP_TIMER | 0x100)
#if 0
		APP_OPT("synchro", APP_SYNCHRO)
#endif
		APP_OPT("option", APP_OPTION)
	END_OPT()

	if(!num_apps)
		usage();

	if(open_dev(&dev)){
		ERROR("open_dev");
		goto exit;
	}

	tucp_progress(1);

	if(start_session(&dev)){
		ERROR("read_app_info");
		goto exit;
	}

/******************************************************************************/
#ifdef DEBUG
	for(i = 0; i < NUM_APPS; i++){
		if(!dev.app[i].acd.app_idx)
			continue;
		printf("%2d: %d%d%d%d%d%d%d%d %02x %02x %04x %04x %04x %04x %04x %04x %s\n", i,
				dev.app[i].acd.app_idx,
				dev.app[i].acd.code_loc,
				dev.app[i].acd.db_loc,
				dev.app[i].acd.code_invalid,
				dev.app[i].acd.db_modified,
				dev.app[i].acd.db_invalid,
				dev.app[i].acd.passwd_req,
				dev.app[i].acd.mode_name,

				dev.app[i].acb.app_type,
				dev.app[i].acb.app_inst,
				dev.app[i].acb.asd_addr,
				dev.app[i].acb.add_addr,
				dev.app[i].acb.state_mgr_addr,
				dev.app[i].acb.refresh_addr,
				dev.app[i].acb.banner_addr,
				dev.app[i].acb.code_addr,
				dev.app[i].banner
		);
	}
#endif
/******************************************************************************/

	for(i = 0; i < num_apps; i++){
		if(app[i].file[0])
			fprintf(stderr, "%s\n", app[i].file);
		else
			continue;

		banner = NULL;
		file = app[i].file;

		switch(app[i].app){
#if 0
		case -3:
			if(load_rom(&dev, &data, &len)){
				ERROR("load_rom");
				goto end;
			}
			break;
		case -2:
			if(load_eeprom(&dev, &data, &len)){
				ERROR("load_eeprom");
				goto end;
			}
			break;
#endif
		case -1:
			banner = file;
			for(; *file && *file != '='; file++);
			if(*file)
				*file++ = 0;
			if(!(*file)){
				ERROR("%s: no file name specified", banner);
				goto end;
			}
			for(; *banner; banner++)
				*banner = toupper(*banner);
			banner = app[i].file;
			break;
		case APP_CONTACT:
			banner = "CONTACT";
			break;
		case APP_NOTE:
			banner = "NOTE";
			break;
		case APP_APPT:
			banner = "APPT";
			break;
		case APP_ALARM:
			banner = "ALARM";
			break;
		case APP_SCHEDULE:
			banner = "SCHEDULE";
			break;
		case APP_OCCASION:
			banner = "OCCASION";
			break;
		case APP_CHRONO:
			banner = "CHRONO";
			break;
		case APP_TIMER:
			banner = "COUNTDN TIMER";
			break;
		case APP_TIMER | 0x100:
			banner = "INTERVAL TIMER";
			break;
#if 0
		case APP_SYNCHRO:
			banner = "SYNCHRO";
			break;
#endif
		case APP_OPTION:
			break;
		}

		if(stat(file, &sb)){
			ERROR("%s: stat failed", file);
			goto end;
		}
		len = sb.st_size;
		if((fd = open(file, O_RDONLY)) < 0){
			ERROR("%s: open failed", file);
			goto end;
		}
		data = (u8*)malloc(len);
		if(read(fd, data, len) != len){
			ERROR("%s: read failed", file);
			goto end;
		}
		close(fd);

		if(banner){
			if((idx = find_app(&dev, banner)) < 0){
				ERROR("%s application not found", banner);
				goto end;
			}
			if(load_add(&dev, idx, data)){
				ERROR("load_add");
				goto end;
			}
		}else{
			len = 15;
			if(write_abs_addr(&dev, dev.sysmap.opt_addr, int_mem,
						data, len)){
				ERROR("write_abs_addr");
				goto end;
			}
		}
		free(data);
	}

/******************************************************************************/
end:
	if(end_session(&dev)){
		ERROR("end_session");
		goto exit;
	}

	tucp_progress(0);

	ret = 0;
exit:
	close_dev(&dev);

	exit(ret);
}

static void
usage(void)
{
	fprintf(stderr,
"usage: load_dlusb [OPTIONS]\n"
"\n"
"  -h		this help\n"
#ifdef USB_USBHID
"  -d device	device file for Timex Data Link USB watch (/dev/uhid0)\n"
#endif
#if 0
"  rom=file	load ROM image\n"
"  eeprom=file	load EEPROM image\n"
#endif
"  wristapp=app=file	load WristApp data\n"
"  contact=file	load Contact data\n"
"  note=file	load Note data\n"
"  appt=file	load Appt data\n"
"  alarm=file	load Alarm data\n"
"  schedule=file	load Schedule data\n"
"  occasion=file	load Occasion data\n"
"  chrono=file	load Chrono data\n"
"  countdn=file	load Countdn Timer data\n"
"  interval=file	load Interval Timer data\n"
"  option=file	load Option\n"
"\n"
"libdlusb version: %s\n", VERSION
	);
	exit(1);
}
