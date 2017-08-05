/* $Id: init_dlusb.c,v 1.20 2009/04/13 07:21:42 geni Exp $
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

static int unused_recs = UNUSED_RECS, unused_recs_appt = UNUSED_RECS,
	   unused_recs_alarm = UNUSED_RECS, unused_recs_note = UNUSED_RECS,
	   unused_recs_chrono = UNUSED_RECS, extra_msg_len = EXTRA_MSG_LEN;

int
main(int argc, char *argv[])
{
	int ret = 1, i, num_apps = 0, user_banner = 0;
	char *dev_file = "/dev/uhid0";
	dldev_t dev;
	appinfo *app = NULL;
	u8 *data, binary = 0;
	u16 len;

	/* for data security */
	/*
	umask(S_IRWXG | S_IRWXO);
	*/

	while((i = getopt(argc, argv, "hd:fu:e:c:b")) != -1){
		switch(i){
		case 'd':
			dev_file = optarg;
			break;
		case 'f':
			user_banner = 1;
			break;
		case 'b':
			binary = 1;
			break;
		case 'u':
			unused_recs = atoi(optarg);
			if(unused_recs < 0)
				unused_recs = 0;
			unused_recs_note = unused_recs;
			unused_recs_appt = unused_recs;
			unused_recs_alarm = unused_recs;
			unused_recs_chrono = unused_recs;
			break;
		case 'n':
			unused_recs_note = atoi(optarg);
			if(unused_recs_note < 0)
				unused_recs_note = 0;
			break;
		case 'a':
			unused_recs_appt = atoi(optarg);
			if(unused_recs_appt < 0)
				unused_recs_appt = 0;
			break;
		case 'l':
			unused_recs_alarm = atoi(optarg);
			if(unused_recs_alarm < 0)
				unused_recs_alarm = 0;
			break;
		case 'c':
			unused_recs_chrono = atoi(optarg);
			if(unused_recs_chrono < 0)
				unused_recs_chrono = 0;
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

#ifdef USB_USBHID
	dev.usb.file = dev_file;
#endif

	BEGIN_OPT()
		APP_OPT("wristapp", -1)
		APP_OPT("contact", APP_CONTACT)
		APP_OPT("note", APP_NOTE)
		APP_OPT("appt", APP_APPT)
		APP_OPT("alarm", APP_ALARM)
		APP_OPT("schedule", APP_SCHEDULE)
		APP_OPT("occasion", APP_OCCASION)
		APP_OPT("chrono", APP_CHRONO)
		APP_OPT("countdn", APP_TIMER)
		APP_OPT("interval", APP_TIMER_INTERVAL)
		APP_OPT("synchro", APP_SYNCHRO)
		APP_OPT("option", APP_OPTION)
	END_OPT()

	if(!num_apps)
		usage();

	/* NUM_APPS: TOD, COMM, ... */
	if(num_apps > NUM_APPS - 2){
		fprintf(stderr, "%d: too many applications (max: %d)\n",
				num_apps, NUM_APPS - 2);
		exit(1);
	}

	fprintf(stderr, "WARNING: INITIALIZE THE WATCH (Y/N)? ");
	if(tolower(getchar()) != 'y')
		exit(0);

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

	if(comm_multi_packet_write(&dev)){
		ERROR("comm_multi_packet_write");
		goto end;
	}
	if(del_all_apps(&dev)){
		ERROR("del_all_apps");
		goto end;
	}

	for(i = 0; i < num_apps; i++){
		if(app[i].file[0])
			fprintf(stderr, "%s\n", app[i].file);

		if(binary){
			int fd;
			struct stat sb;

			switch(app[i].app){
			case APP_CONTACT:
			case APP_NOTE:
			case APP_APPT:
			case APP_ALARM:
			case APP_SCHEDULE:
			case APP_OCCASION:
			case APP_CHRONO:
			case APP_TIMER:
			case APP_TIMER_INTERVAL:
			case APP_OPTION:
				if(stat(app[i].file, &sb)){
					ERROR("%s: stat failed", app[i].file);
					goto end;
				}
				len = sb.st_size;
				if((fd = open(app[i].file, O_RDONLY)) < 0){
					ERROR("%s: open failed", app[i].file);
					goto end;
				}
				data = (u8*)malloc(len);
				if(read(fd, data, len) != len){
					ERROR("%s: read failed", app[i].file);
					goto end;
				}
				close(fd);
				break;
			}
		}

		/* the order of applications == the order of initialization */
		switch(app[i].app){
		/* external applications */
		case -1:
			init_ext_app(&dev, app[i].file, user_banner);
			break;
		case APP_CONTACT:
			if(!binary){
				FILE *fp;
				contact_t db;

				if(!(fp = fopen(app[i].file, "r"))){
					ERROR("%s: open failed", app[i].file);
					goto end;
				}
				init_contact(&db);
				add_contact_file(&db, fp);
				create_contact(&db, &data, &len);
				free_contact(&db);
				fclose(fp);
			}
			init_contact_app(&dev, data, len);
			free(data);
			break;
		case APP_NOTE:
			if(!binary){
				FILE *fp;
				note_t db;

				if(!(fp = fopen(app[i].file, "r"))){
					ERROR("%s: open failed", app[i].file);
					goto end;
				}

				set_note_unused_recs(unused_recs_note);
				set_note_extra_len(extra_msg_len);

				init_note(&db);
				add_note_file(&db, fp);
				create_note(&db, &data, &len);
				free_note(&db);
				fclose(fp);
			}
			init_note_app(&dev, data, len);
			free(data);
			break;
		case APP_APPT:
			set_appt(app[i].app);
			if(!binary){
				FILE *fp;
				appt_t db;

				if(!(fp = fopen(app[i].file, "r"))){
					ERROR("%s: open failed", app[i].file);
					goto end;
				}

				set_appt_unused_recs(unused_recs_appt);
				set_appt_extra_len(extra_msg_len);

				init_appt(&db);
				add_appt_file(&db, fp);
				create_appt(&db, &data, &len);
				free_appt(&db);
				fclose(fp);
			}
			init_appt_app(&dev, data, len);
			free(data);
			break;
		case APP_ALARM:
			set_appt(app[i].app);
			if(!binary){
				FILE *fp;
				appt_t db;

				if(!(fp = fopen(app[i].file, "r"))){
					ERROR("%s: open failed", app[i].file);
					goto end;
				}

				set_appt_unused_recs(unused_recs_alarm);
				set_appt_extra_len(extra_msg_len);

				init_appt(&db);
				add_appt_file(&db, fp);
				create_appt(&db, &data, &len);
				free_appt(&db);
				fclose(fp);
			}
			init_appt_app(&dev, data, len);
			free(data);
			break;
		case APP_SCHEDULE:
			if(!binary){
				FILE *fp;
				schedule_t db;

				if(!(fp = fopen(app[i].file, "r"))){
					ERROR("%s: open failed", app[i].file);
					goto end;
				}
				init_schedule(&db);
				add_schedule_file(&db, fp);
				create_schedule(&db, &data, &len);
				free_schedule(&db);
				fclose(fp);
			}
			init_schedule_app(&dev, data, len);
			free(data);
			break;
		case APP_OCCASION:
			if(!binary){
				FILE *fp;
				occasion_t db;

				if(!(fp = fopen(app[i].file, "r"))){
					ERROR("%s: open failed", app[i].file);
					goto end;
				}
				init_occasion(&db);
				add_occasion_file(&db, fp);
				create_occasion(&db, &data, &len);
				free_occasion(&db);
				fclose(fp);
			}
			init_occasion_app(&dev, data, len);
			free(data);
			break;
		case APP_CHRONO:
			if(!binary){
				FILE *fp;
				chrono_t db;

				if(!(fp = fopen(app[i].file, "r"))){
					ERROR("%s: open failed", app[i].file);
					goto end;
				}

				set_chrono_unused_recs(unused_recs_chrono);

				init_chrono(&db);
				add_chrono_file(&db, fp);
				create_chrono(&db, &data, &len);
				free_chrono(&db);
				fclose(fp);
			}
			init_chrono_app(&dev, data, len);
			free(data);
			break;
		case APP_TIMER:
			if(!binary){
				FILE *fp;
				tymer_t db;

				if(!(fp = fopen(app[i].file, "r"))){
					ERROR("%s: open failed", app[i].file);
					goto end;
				}

				init_timer(&db);
				add_timer_file(&db, fp);
				create_timer(&db, &data, &len);
				free_timer(&db);
				fclose(fp);
			}
			init_timer_app(&dev, data, len);
			free(data);
			break;
		case APP_TIMER | 0x100:
			set_timer(POR_INTERVAL);
			if(!binary){
				FILE *fp;
				tymer_t db;

				if(!(fp = fopen(app[i].file, "r"))){
					ERROR("%s: open failed", app[i].file);
					goto end;
				}

				init_timer(&db);
				add_timer_file(&db, fp);
				create_timer(&db, &data, &len);
				free_timer(&db);
				fclose(fp);
			}
			init_timer_app(&dev, data, len);
			free(data);
			break;
		case APP_SYNCHRO:
			fprintf(stderr, ".SYNCHRO\n");
			init_int_app(&dev, POR_SYNCHRO, NULL, 0);
			break;
		case APP_OPTION:
			fprintf(stderr, ".OPTION\n");
			init_int_app(&dev, POR_OPTION, NULL, 0);
			if(!binary){
				FILE *fp;
				option_t db;

				if(!(fp = fopen(app[i].file, "r"))){
					ERROR("%s: open failed", app[i].file);
					goto end;
				}

				scan_option(&db, fp);
				create_raw_option(&db, data);
				len = sizeof(option_t);
			}
			if(len != sizeof(option_t)){
				ERROR("invalid option data");
				goto end;
			}
			update_raw_option(&dev, data);
			break;
		}
	}

	free(app);

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
"usage: init_dlusb [OPTIONS]\n"
"\n"
"  -h		this help\n"
#ifdef USB_USBHID
"  -d device	device file for Timex Data Link USB watch (default: /dev/uhid0)\n"
#endif
"  -f		use file name for the banner\n"
"  -u unused	number of unused records (overrides previous settings, default: %d)\n"
"  -n unused	number of unused records for Note (default: %d)\n"
"  -a unused	number of unused records for Appt (default: %d)\n"
"  -l unused	number of unused records for Alarm (default: %d)\n"
"  -c unused	number of unused records for Chrono (default: %d)\n"
"  -e extra_len	extra message length (default: %d)\n"
"  -b		binary file\n"
"  wristapp=app	*.app file for an external application\n"
"  contact=file	initialize Contact data\n"
"  note=file	initialize Note data\n"
"  appt=file	initialize Appt data\n"
"  alarm=file	initialize Alarm data\n"
"  schedule=file	initialize Schedule data\n"
"  occasion=file	initialize Occasion data\n"
"  chrono=file	initialize Chrono data\n"
"  countdn=file	initialize Countdn Timer data\n"
"  interval=file	initialize Interval Timer data\n"
"  synchro=	initialize Synchro Timer\n"
"  option=file	initialize Option\n"
"\n"
"libdlusb version: %s\n", unused_recs, unused_recs_note, unused_recs_appt, unused_recs_alarm, extra_msg_len, unused_recs_chrono, VERSION
	);
	exit(1);
}
