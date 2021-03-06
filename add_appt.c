/* $Id: add_appt.c,v 1.9 2009/05/12 05:44:00 geni Exp $
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

#define uapp_name (is_appt ? "APPT" : "ALARM")
#define lapp_name (is_appt ? "appt" : "alarm")

static void usage(void);

static char *dev_file = "/dev/uhid0";
static int is_appt = 1, unused_recs = UNUSED_RECS,
	   extra_msg_len = EXTRA_MSG_LEN;

int
main(int argc, char *argv[])
{
	int ret = 1, istty, i, idx;
	char *msg = NULL, *tim = NULL, *armed = NULL, *freq = NULL,
	     *prior = NULL;
	dldev_t dev;
	appt_t db;
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
	if(strstr(argv[0] + i, "alarm")){
		set_appt(APP_ALARM);
		is_appt = 0;
	}

	while((i = getopt(argc, argv, "hd:u:e:")) != -1){
		switch(i){
		case 'd':
			dev_file = optarg;
			break;
		case 'u':
			unused_recs = atoi(optarg);
			if(unused_recs < 0)
				unused_recs = 0;
			break;
		case 'e':
			extra_msg_len = atoi(optarg);
			if(extra_msg_len < 0)
				extra_msg_len = 0;
			break;
		case 'h':
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	set_appt_unused_recs(unused_recs);
	set_appt_extra_len(extra_msg_len);

#ifdef USB_USBHID
	dev.usb.file = dev_file;
#endif

	BEGIN_OPT()
		OPT("msg", msg)
		OPT("time", tim)
		OPT("armed", armed)
		OPT("freq", freq)
		OPT("prior", prior)
	END_OPT()

	if(istty && (!msg || !tim || !freq || !prior))
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

	if(read_appt_mem(&dev, &db)){
		ERROR("read_appt_mem");
		goto end;
	}

	if(!istty)
		add_appt_file(&db, stdin);

	if(msg && tim && freq && prior){
		char buf[BUFSIZ];
		appt_data_t rec;

		sprintf(buf, "%s\t%s\t%s\t%s\t%s", msg, tim,
				appt_armed[(armed && strcmp(armed, "no"))],
				freq, prior);
		if(read_appt_line(&rec, buf))
			fprintf(stderr, "add_%s: format error!\n", lapp_name);
		else
		if(find_appt(&db, &rec) < 0)
			add_appt(&db, &rec);
	}

	create_appt(&db, &data, &len);
	if(load_add(&dev, idx, data)){
		ERROR("load_add");
		goto end;
	}
	free(data);

	print_appt(&db, stdout);
	free_appt(&db);

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
"  -u unused	number of unused records (default: %d)\n"
"  -e extra_len	extra message length (default: %d)\n"
"  msg=...	message (max length: %d)\n"
"  time=...	time (YYYY-MM-DD hh:mm)\n"
"  armed=...	armed (yes, no)\n"
"  freq=...	frequency (%s)\n"
"  prior=...	prior (0 mins, 5 mins, 10 mins, 15 mins, 30 mins, 60 mins, 2 hrs, 3 hrs, 4 hrs, 5 hrs, 6 hrs, 8 hrs, 10 hrs, 12 hrs, 24 hrs, 48 hrs)\n"
"\n"
"libdlusb version: %s\n", lapp_name,
#ifdef USB_USBHID
	dev_file,
#endif
	unused_recs, extra_msg_len, MAX_MSG_LEN,
	(is_appt ? "1-day, daily, weekdays, weekends, weekly, monthly, yearly" :
	 "daily, weekdays, weekends, weekly sunday, weekly monday, weekly tuesday, weekly wednesday, weekly thursday, weekly friday, weekly saturday"),
	VERSION
	);
	exit(1);
}
