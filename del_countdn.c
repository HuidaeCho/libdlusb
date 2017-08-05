/* $Id: del_countdn.c,v 1.5 2005/09/25 23:15:38 geni Exp $
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

#define uapp_name (is_countdn ? "COUNTDN TIMER" : "INTERVAL TIMER")
#define lapp_name (is_countdn ? "countdn" : "interval")

static void usage(void);

static char *dev_file = "/dev/uhid0";
static int is_countdn = 1;

int
main(int argc, char *argv[])
{
	int ret = 1, istty, i, idx, delete_all = 0, updated = 0;
	char *msg = NULL;
	dldev_t dev;
	tymer_t db;
	u8 *data;
	u16 len;

	/* for data security */
	/*
	umask(S_IRWXG | S_IRWXO);
	*/

	istty = isatty(0);

	i = strlen(argv[0]) - 1;
	for(; i >= 0 && argv[0][i] != '/'; i--);
	i++;
	if(strstr(argv[0] + i, "interval")){
		set_timer(POR_INTERVAL);
		is_countdn = 0;
	}

	while((i = getopt(argc, argv, "hd:a")) != -1){
		switch(i){
		case 'd':
			dev_file = optarg;
			break;
		case 'a':
			delete_all = 1;
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
		OPT("msg", msg)
	END_OPT()

	if(delete_all){
		fprintf(stderr, "WARNING: DELETE ALL %sS IN THE WATCH (Y/N)? ",
				uapp_name);
		if(tolower(getchar()) != 'y')
			exit(0);
	}else
	if(istty && !msg)
		usage();

	if(open_dev(&dev)){
		ERROR("open_dev");
		goto exit;
	}

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

	if((idx = find_app(&dev, uapp_name)) < 0){
		ERROR("%s application not found", uapp_name);
		goto end;
	}

	if(dump_add(&dev, idx, &data, &len)){
		ERROR("dump_add");
		goto end;
	}
	read_timer(&db, data);
	free(data);

	if(delete_all){
		while(!del_timer(&db, 0))
			updated = 1;
	}else{
		if(!istty && !del_timer_file(&db, stdin))
			updated = 1;

		if(msg){
			timer_data_t rec;

			rec.msg = msg;
			if((i = find_timer(&db, &rec)) >= 0 &&
					!del_timer(&db, i))
				updated = 1;
		}
	}

	if(updated){
		create_timer(&db, &data, &len);
		if(load_add(&dev, idx, data)){
			ERROR("load_add");
			goto end;
		}
		free(data);
	}

	print_timer(&db, stdout);
	free_timer(&db);

/******************************************************************************/
end:
	if(end_session(&dev)){
		ERROR("end_session");
		goto exit;
	}

	ret = 0;
exit:
	close_dev(&dev);

	exit(ret);
}

static void
usage(void)
{
	fprintf(stderr,
"usage: add_%s [OPTIONS] [ < data_file ]\n"
"\n"
"  -h		this help\n"
#ifdef USB_USBHID
"  -d device	device file for Timex Data Link USB watch (default: %s)\n"
#endif
"  -a		delete all countdn timers\n"
"  msg=...	message (max length: 9)\n"
"\n"
"libdlusb version: %s\n", lapp_name,
#ifdef USB_USBHID
	dev_file,
#endif
	VERSION
	);
	exit(1);
}
