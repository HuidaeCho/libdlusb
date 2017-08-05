/* $Id: add_countdn.c,v 1.5 2005/09/25 23:15:38 geni Exp $
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
#include "misc.h"

#define uapp_name (is_countdn ? "COUNTDN TIMER" : "INTERVAL TIMER")
#define lapp_name (is_countdn ? "countdn" : "interval")

static void usage(void);

static char *dev_file = "/dev/uhid0";
static int is_countdn = 1;

int
main(int argc, char *argv[])
{
	int ret = 1, istty, i, idx;
	char *msg = NULL, *timer = NULL, *at_end = NULL, *halfway = NULL;
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
		OPT("msg", msg)
		OPT("timer", timer)
		OPT("at_end", at_end)
		OPT("halfway", halfway)
	END_OPT()

	/* allows the user to change only at_end in interval timer */
	if(istty && ((!msg || !timer) && (is_countdn || !at_end)))
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

	if(!istty)
		add_timer_file(&db, stdin);

	if(msg && timer){
		char buf[BUFSIZ];
		timer_data_t rec;

		sprintf(buf, "%s\t%s\t%s%s", msg, timer,
			(at_end ? at_end : "stop"),
			(halfway && strcmp(halfway, "no") ? "\thalfway" : ""));
		if(read_timer_line(&rec, buf))
			fprintf(stderr, "add_%s: format error!\n", lapp_name);
		else
		if(find_timer(&db, &rec) < 0)
			add_timer(&db, &rec);
	}
	if(!is_countdn && at_end){
		int i;

		for(i = 0; i < 3 && strcmp(at_end, timer_at_end[i]); i++);
		if(i < 3){
			set_timer_at_end(i);
			update_timer_at_end(&db);
		}
	}

	create_timer(&db, &data, &len);
	if(load_add(&dev, idx, data)){
		ERROR("load_add");
		goto end;
	}
	free(data);

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
"  msg=...	message (max length: 9)\n"
"  timer=...	time (YYYY-MM-DD hh:mm)\n"
"  at_end=...	action at end (stop, repeat, chrono)\n"
"  halfway=...	halfway reminder (yes, no)\n"
"\n"
"libdlusb version: %s\n", lapp_name,
#ifdef USB_USBHID
	dev_file,
#endif
	VERSION
	);
	exit(1);
}
