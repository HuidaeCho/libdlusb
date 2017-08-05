/* $Id: tod_dlusb.c,v 1.9 2009/07/13 05:13:18 geni Exp $
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

#define isset(x) (x && x[0])
/* setcmp(x, y) == 1 only if x != NULL and x != y */
#define setcmp(x, y) (x && strcmp(x, y))

static void usage(void);

static char *dev_file = "/dev/uhid0";

int
main(int argc, char *argv[])
{
	int ret = 1, i;
	char *primary = NULL, *tz[3], *date[3], *hr24[3], *week_no[3], *dst[3],
	     *euro[3], *observe_dst[3], *adjust = NULL;
	u8 primary_tz = 1, date_fmt[3];
	time_t dsec[3], adjust_sec = 0;
	time_t tim, tim2;
	struct tm *tm;
	dldev_t dev;
	tod_t db;

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

	memset(tz, 0, sizeof(tz));
	memset(date, 0, sizeof(date));
	memset(hr24, 0, sizeof(hr24));
	memset(week_no, 0, sizeof(week_no));
	memset(dst, 0, sizeof(dst));
	memset(euro, 0, sizeof(euro));
	memset(observe_dst, 0, sizeof(observe_dst));

	BEGIN_OPT()
		OPT("primary", primary);
		OPT("tz1", tz[0])
		OPT("tz2", tz[1])
		OPT("tz3", tz[2])
		OPT("date1", date[0])
		OPT("date2", date[1])
		OPT("date3", date[2])
		OPT("24hr1", hr24[0])
		OPT("24hr2", hr24[1])
		OPT("24hr3", hr24[2])
		OPT("week_no1", week_no[0])
		OPT("week_no2", week_no[1])
		OPT("week_no3", week_no[2])
		OPT("dst1", dst[0])
		OPT("dst2", dst[1])
		OPT("dst3", dst[2])
		OPT("euro1", euro[0])
		OPT("euro2", euro[1])
		OPT("euro3", euro[2])
		OPT("observe_dst1", observe_dst[0])
		OPT("observe_dst2", observe_dst[1])
		OPT("observe_dst3", observe_dst[2])
		/* non-standard options */
		OPT("adjust", adjust)
	END_OPT()

	if(isset(primary)){
		primary_tz = primary[0] - '0';
		if(primary_tz < 1 || primary_tz > 3)
			primary_tz = 1;
	}

	for(i = 0; i < 3; i++){
		dsec[i] = 0;
		if(isset(tz[i])){
			char buf[10];

			strncpy(buf, tz[i], 9);
			buf[9] = 0;
			if(strlen(buf) != 9 ||
					(buf[3] != '+' && buf[3] != '-')){
				fprintf(stderr, "%s: format error!\n", tz[i]);
				usage();
			}
			buf[6] = 0;
			dsec[i] = (buf[3] == '-' ? -1 : 1) *
				(atoi(buf+4) * 3600 + atoi(buf+7) * 60);
		}

		if(isset(date[i])){
			int j;

			for(j = 0; j < 3 && strcmp(date[i], date_format[j]);
					j++);
			if(j == 3){
				fprintf(stderr, "%s: format error!\n", date[i]);
				usage();
			}
			date_fmt[i] = j;
		}else
			date_fmt[i] = mdy;
	}

	if(isset(adjust)){
		char buf[10];

		strncpy(buf, adjust, 9);
		buf[9] = 0;
		if(strlen(buf) != 9 || (buf[0] != '+' && buf[0] != '-')){
			fprintf(stderr, "%s: format error!\n", adjust);
			usage();
		}
		buf[3] = buf[6] = 0;
		adjust_sec = (buf[0] == '-' ? -1 : 1) *
			(atoi(buf+1) * 3600 + atoi(buf+4) * 60 + atoi(buf+7));
	}

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

	time(&tim);

#ifdef EXPERIMENTAL
	if(read_tod(&dev, &db)){
		ERROR("read_tod");
		goto exit;
	}
#else
	memset(&db, 0, sizeof(tod_t));
#endif

	for(i = 0; i < 3; i++){
		db.tod[i].primary = (primary_tz == i+1);	/* user */

		db.tod[i].update_tz_id = 1; /* -> tz_id */
		db.tod[i].update_display = 1; /* -> everything else? */
		db.tod[i].update_tz_name = 1; /* -> tz_name */
		db.tod[i].update_hms = 1; /* -> hour/minute/second */
		db.tod[i].update_mdy = 1; /* -> year/month/day */

		/* update_display */
		db.tod[i].date_format = date_fmt[i]; /* user */
		db.tod[i].hr24_format = setcmp(hr24[i], "no"); /* user */
		db.tod[i].display_week_no = setcmp(week_no[i], "no");
			/* user */
		db.tod[i].tz_in_dst = setcmp(dst[i], "no"); /* user */
		db.tod[i].euro_format = setcmp(euro[i], "no"); /* user */
		db.tod[i].tz_observes_dst = setcmp(observe_dst[i], "no"); /* user */

		/* set if update_mdy = 1 */
		db.tod[i].tz_entered_set_state = 1;

		/* update_tz_name */
		if(isset(tz[i]))
			strncpy((char *)db.tod[i].tz_name, tz[i], 3); /* user */
		else
			strncpy((char *)db.tod[i].tz_name, "UTC", 3); /* user */

		/* update_tz_id */
		db.tod[i].tz_id = 0; /* used for WorldTime WristApp */

		tim2 = tim + dsec[i] + adjust_sec;
		tm = gmtime(&tim2);

		/* update_hms */
		db.tod[i].second = tm->tm_sec; /* user */
		db.tod[i].minute = tm->tm_min; /* user */
		db.tod[i].hour = tm->tm_hour; /* user */

		/* update_mdy */
		db.tod[i].day = tm->tm_mday; /* user */
		db.tod[i].month = tm->tm_mon + 1; /* user */
		db.tod[i].year = tm->tm_year + 1900; /* user */
	}

	if(update_tod(&dev, &db)){
		ERROR("update_tod");
		goto end;
	}

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
"usage: tod_dlusb [OPTIONS]\n"
"\n"
"  -h		this help\n"
#ifdef USB_USBHID
"  -d device	device file for Timex Data Link USB watch (default: %s)\n"
#endif
"  primary=...	primary time zone (1, 2, 3, default: 1)\n"
"  tz1=...	time zone 1\n"
"		format: {3-character time zone name}{{+ or -}HH:MM relative to UTC}\n"
"		example: CST or GMT+6 => CST+06:00\n"
"  tz2=...	time zone 2\n"
"  tz3=...	time zone 3\n"
"  date1=...	date format (dmy, ymd, mdy)\n"
"  date2=...	date format (dmy, ymd, mdy)\n"
"  date3=...	date format (dmy, ymd, mdy)\n"
"  24hr1=...	24 hour format (yes, no)\n"
"  24hr2=...	24 hour format (yes, no)\n"
"  24hr3=...	24 hour format (yes, no)\n"
"  week_no1=...	display week number (yes, no)\n"
"  week_no2=...	display week number (yes, no)\n"
"  week_no3=...	display week number (yes, no)\n"
"  dst1=...	dst (yes, no)\n"
"  dst2=...	dst (yes, no)\n"
"  dst3=...	dst (yes, no)\n"
"  euro1=...	euro format (yes, no)\n"
"  euro2=...	euro format (yes, no)\n"
"  euro3=...	euro format (yes, no)\n"
"  observe_dst1=...	observe dst (yes, no)\n"
"  observe_dst2=...	observe dst (yes, no)\n"
"  observe_dst3=...	observe dst (yes, no)\n"
"  adjust=...	adjust time, format: {+ or -}HH:MM:SS, example: +00:05:00\n"
"\n"
"libdlusb version: %s\n",
#ifdef USB_USBHID
	dev_file,
#endif
	VERSION
	);
	exit(1);
}
