/* $Id: add_schedule.c,v 1.8 2009/07/13 05:13:18 geni Exp $
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

static void usage(void);

static char *dev_file = "/dev/uhid0";

int
main(int argc, char *argv[])
{
	int ret = 1, istty, i, idx, updated = 0;
	char *desc = NULL, *type = NULL, *group = NULL, *msg = NULL, *tim = NULL;
	dldev_t dev;
	schedule_t db;
	u8 *data;
	u16 len;

	/* for data security */
	/*
	umask(S_IRWXG | S_IRWXO);
	*/

	istty = isatty(0);

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
		OPT("desc", desc)
		OPT("type", type)
		OPT("group", group)
		OPT("msg", msg)
		OPT("time", tim)
	END_OPT()

	/* allows the user to change only desc */
	if(istty && !desc && (!group || !msg || !tim))
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

	if((idx = find_app(&dev, "SCHEDULE")) < 0){
		ERROR("SCHEDULE application not found");
		goto end;
	}

	if(dump_add(&dev, idx, &data, &len)){
		ERROR("dump_add");
		goto end;
	}

	read_schedule(&db, data);
	free(data);

	if(!istty && !add_schedule_file(&db, stdin))
		updated = 1;

	if(desc){
		set_schedule_msg(desc);
		update_schedule_msg(&db);
		updated = 1;
	}

	/* do we need it? */
	if(type){
		int i;

		for(i = 0; i < 3 && strcmp(type, schedule_group_type[i]); i++);
		if(i < 3){
			set_schedule_group_type(get_schedule_group_type_idx(i));
			update_schedule_group_type(&db);
		}
	}

	if(group && msg && tim){
		char buf[BUFSIZ];
		schedule_data_t rec;

		sprintf(buf, "%s\t%s\t%s", group, msg, tim);
		if(read_schedule_line(&rec, buf))
			fprintf(stderr, "add_schedule: format error!\n");
		else
		if(find_schedule(&db, &rec) < 0)
			add_schedule(&db, &rec);
		updated = 1;
	}

	if(updated){
		create_schedule(&db, &data, &len);
		if(load_add(&dev, idx, data)){
			ERROR("load_add");
			goto end;
		}
		free(data);
	}

	print_schedule(&db, stdout);
	free_schedule(&db);

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
"usage: add_schedule [OPTIONS] [ < data_file ]\n"
"\n"
"  -h		this help\n"
#ifdef USB_USBHID
"  -d device	device file for Timex Data Link USB watch (default: %s)\n"
#endif
"  desc=...	member description (max length: 14)\n"
"  type=...	schedule type (date, dow_time, date_time)\n"
"  group=...	group (max length: %d)\n"
"  msg=...	message (max length: %d)\n"
"  time=...	time (YYYY-MM-DD for date, *day hh:mm for dow_time, YYYY-MM-DD hh:mm for date_time)\n"
"\n"
"libdlusb version: %s\n",
#ifdef USB_USBHID
	dev_file,
#endif
	MAX_MSG_LEN, MAX_MSG_LEN, VERSION
	);
	exit(1);
}
