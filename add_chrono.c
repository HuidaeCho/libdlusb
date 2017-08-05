/* $Id: add_chrono.c,v 1.9 2009/07/13 05:13:18 geni Exp $
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
#include <time.h>
#include <sys/stat.h>
#include "dlusb.h"
#include "misc.h"

static void usage(void);

static char *dev_file = "/dev/uhid0";
static int unused_recs = UNUSED_RECS;

int
main(int argc, char *argv[])
{
	int ret = 1, istty, i, idx;
	char *display = NULL, *workout = NULL, *date = NULL, *split = NULL;
	dldev_t dev;
	chrono_t db;
	u8 *data;
	u16 len;

	/* for data security */
	/*
	umask(S_IRWXG | S_IRWXO);
	*/

	istty = isatty(0);

	while((i = getopt(argc, argv, "hd:u:")) != -1){
		switch(i){
		case 'd':
			dev_file = optarg;
			break;
		case 'u':
			unused_recs = atoi(optarg);
			if(unused_recs < 0)
				unused_recs = 0;
			break;
		case 'h':
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	set_chrono_unused_recs(unused_recs);

#ifdef USB_USBHID
	dev.usb.file = dev_file;
#endif

	BEGIN_OPT()
		OPT("display", display)
		OPT("workout", workout)
		OPT("date", date)
		OPT("split", split)
	END_OPT()

	/* allows the user to change only display format */
	if(istty && !display && ((!workout && !date) || !split))
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

	if((idx = find_app(&dev, "CHRONO")) < 0){
		ERROR("CHRONO application not found");
		goto end;
	}

	if(read_chrono_mem(&dev, &db)){
		ERROR("read_chrono_mem");
		goto end;
	}

	if(!istty)
		add_chrono_file(&db, stdin);

	if(display){
		for(i = 0; i < 4 && strcmp(display, chrono_display_format[i]);
				i++);
		if(i < 4){
			set_chrono_display_format(i);
			update_chrono_display_format(&db);
		}
	}

	if((workout || date) && split){
		char buf[BUFSIZ];
		chrono_data_t rec;

		if(date)
			sprintf(buf, "%s\t0\t%s\t%s", workout, date, split);
		else{
			time_t tim;
			struct tm *tm;

			time(&tim);
			tm = localtime(&tim);

			sprintf(buf, "%s\t0\t%d-%02d-%02d\t%s", workout,
					tm->tm_year + 1900, tm->tm_mon + 1,
					tm->tm_mday, split);
		}
		if(read_chrono_line(&rec, buf))
			fprintf(stderr, "add_chrono: format error!\n");
		else
			add_chrono(&db, &rec);
	}

	create_chrono(&db, &data, &len);
	if(load_add(&dev, idx, data)){
		ERROR("load_add");
		goto end;
	}
	free(data);

	print_chrono(&db, stdout);
	free_chrono(&db);

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
"usage: add_chrono [OPTIONS] [ < data_file ]\n"
"\n"
"  -h		this help\n"
#ifdef USB_USBHID
"  -d device	device file for Timex Data Link USB watch (default: %s)\n"
#endif
"  -u unused	number of unused records (default: %d)\n"
"  display=...	display format (lap_split, split_lap, time_split, time_lap)\n"
"  workout=...	workout id (0 for new workout)\n"
"  date=...	date (YYYY-MM-DD dow, today if no date specified with workout=0)\n"
"  split=...	split record (hh:mm:ss.xx)\n"
"\n"
"libdlusb version: %s\n",
#ifdef USB_USBHID
	dev_file,
#endif
	unused_recs, VERSION
	);
	exit(1);
}
