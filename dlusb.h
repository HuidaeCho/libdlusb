/* $Id: dlusb.h,v 1.60 2010/01/05 09:24:02 geni Exp $
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

#ifndef _DLUSB_H_
#define _DLUSB_H_

#define LIB_VERSION "0.0.12"

/*
#define DEBUG
*/
#define EXPERIMENTAL

#include <stdio.h>
#if defined(USB_LIBUSB)
	#include "usb_libusb.h"
	#define USB_LIBRARY "libusb"
#elif defined(USB_LIBUSB_1_0)
	#include "usb_libusb-1.0.h"
	#define USB_LIBRARY "libusb 1.0"
#elif defined(USB_USBHID)
	#include "usb_usbhid.h"
	#define USB_LIBRARY "usbhid"
#endif

#define VERSION LIB_VERSION" ("USB_LIBRARY")"

#define ERROR(fmt, args...) \
	fprintf(stderr, "%s (%d in %s): "fmt"\n", __FUNCTION__, __LINE__, \
			__FILE__, ##args)
#ifdef DEBUG
	#define LINE ERROR("DEBUG");
#else
	#define LINE
#endif

/* TODO: can comm_read_abs_addr() be done just after comm_multi_packet_write()
 * was sent?  I hate duplicated code. */
#define COMM_READ_ABS_ADDR__AFTER__COM_MULTI_PACKET_WRITE

#define zalloc(x) calloc(1, (x))

#define hibyte(x) (((x) >> 8) & 0xff)
#define lobyte(x) ((x) & 0xff)
#define hibits(x) (((x) & 0xf0) >> 4)
#define lobits(x) ((x) & 0x0f)

#define word(hi, lo) (((hi) << 8) | (lo))
#define bcd(x) ((((x) / 10) << 4) | ((x) % 10))
#define bcd2(x) ((bcd((x) / 100) << 8) | bcd((x) % 100))
#define debcd(x) (hibits(x) * 10 + lobits(x))
#define debcd2(x) (debcd(hibyte(x)) * 100 + debcd(lobyte(x)))

#define app_id(type, inst) (((inst) << 8) | (type))

#define seg(x) segch[x]
#define seg16(x) seg16ch[x]
#define ldm(x) ldmch[x]
#define rdm(x) rdmch[(x) & ~(1 << 7)]
#define rdm16(x) rdm16ch[x]

#define segi(x) convert_ch(segch, x)
#define seg16i(x) convert_ch(seg16ch, x)
#define ldmi(x) convert_ch(ldmch, x)
#define rdmi(x) convert_ch(rdmch, x)
#define rdm16i(x) convert_ch(rdm16ch, x)

#define O_WRITE (O_CREAT | O_TRUNC | O_WRONLY)
#define S_IRWUSR (S_IRUSR | S_IWUSR)

/* TODO: do we really need retries? */
#define NUM_RETRIES 5

#define MAX_PACKET_SIZE 255
#define PACKET_SIZE 8
#define PAGE_SIZE 64

#define ROM_SIZE 0xc000
#define RAM_SIZE 0x0800
#define EEPROM_SIZE 0x8000	/* dev.icb.eeprom_size */
#define DB_START 0x0440		/* increases */
#define CODE_START 0x7f00	/* decreases */
#define ICB_SIZE 64
#define SYSMAP_ADDR 0x0028
#define SYSMAP_SIZE 26
#define ACD_SIZE 1
#define ACB_SIZE 14
#define MAX_BANNER_SIZE 27

#define UNUSED_RECS 20		/* the number of unused records */
#define MIN_MSG_LEN 7		/* Is the minimum length of a message 7?
				 * When the length of a message is less than 7
				 * and it doesn't have any extra space, some
				 * garbage characters are appended when editing.
				 */
#define MAX_MSG_LEN 100		/* *_LEN for text, *_SIZE for binary */
#define EXTRA_MSG_LEN 10	/* downloaded data may need extra space for
				 * update.
				 */
#define MIN_CHRONO_RECS 5
#define MAX_CHRONO_RECS 250	/* system limitation */

#define NUM_APPS 16

/* Power-On Reset */
#define POR_TOD 0x00		/* APP_TOD */
#define POR_COMM 0x02		/* APP_COMM */
#define POR_CHRONO 0x04		/* APP_CHRONO */
#define POR_COUNTDN 0x06	/* APP_TIMER: countdn timer */
#define POR_INTERVAL 0x08	/* APP_TIMER: interval timer */
#define POR_ALARM 0x0a		/* APP_ALARM */
#define POR_APPT 0x0c		/* APP_APPT */
#define POR_NOTE 0x0e		/* APP_NOTE */
#define POR_OPTION 0x10		/* APP_OPTION */
#define POR_OCCASION 0x12	/* APP_OCCASION */
#define POR_CONTACT 0x14	/* APP_CONTACT */
#define POR_SCHEDULE 0x16	/* APP_SCHEDULE */
#define POR_SYNCHRO 0x18	/* APP_SYNCHRO */

/* application type */
#define APP_SYS 0x00		/* ?, instance 0 */
#define APP_COMM 0x01		/* POR_COMM, instance 0 */
#define APP_OPTION 0x02		/* POR_OPTION */
#define APP_TOD 0x10		/* POR_TOD, instance 0 */
#define APP_DATE 0x11		/* ? */
#define APP_CHRONO 0x20		/* POR_CHRONO, instance 0 */
#define APP_TIMER 0x21		/* POR_TIMER: countdn timer */
#define APP_TIMER_INTERVAL (0x21|0x100)	/* POR_TIMER: interval timer */
#define APP_SYNCHRO 0x22	/* POR_SYNCHRO */
#define APP_COUNTER 0x23	/* ? */
#define APP_CONTACT 0x40	/* POR_CONTACT */
#define APP_TASK 0x50		/* ? */
#define APP_NOTE 0x60		/* POR_NOTE */
#define APP_SCHEDULE 0x70	/* POR_SCHEDULE */
#define APP_TIDE 0x80		/* ? */
#define APP_DEMO 0x90		/* ? */
#define APP_GAME 0xa0		/* ? */
#define APP_ALARM 0xe0		/* POR_ALARM */
#define APP_APPT 0xe1		/* POR_APPT */
#define APP_OCCASION 0xe2	/* POR_OCCASION */

#define BANNER_TOD "TOD\t"
#define BANNER_COMM "COMM\t"
#define BANNER_SYS "SYS\t"

#define COMM_TRNS_RCV_ERR 0x00
#define COMM_DEV_INFO_REQ 0x01
#define COMM_PROC_COMPLETE 0x02
#define COMM_DEL_ALL_APPS 0x03
#define COMM_BEEP 0x04
#define COMM_IDLE 0x05
#define COMM_CALL_ABS_ADDR 0x06
#define COMM_WRITE_TOD_TZ 0x07
#define COMM_APP_INIT_INT 0x08
#define COMM_APP_INIT_EXT 0x09
#define COMM_MULTI_PACKET_WRITE 0x0a
#define COMM_WRITE_ABS_ADDR 0x0b
#define COMM_READ_ABS_ADDR 0x0c
#define COMM_ACK 0x0d
#define COMM_NACK 0xff


/******************************************************************************/

/* send_read_tucp_packet errors */
enum tucp_error {
	ERR_USB = 1,
	ERR_COMM,

	ERR_COMM_CHECKSUM,
	ERR_COMM_PACKET_SIZE,

	ERR_COMM_DEV_INFO_REQ,
	ERR_COMM_DEV_MISMATCH,
	ERR_COMM_TIMEOUT
};

/* memory types for comm_(write|read)_abs_addr */
enum mem {
	int_mem,	/* RAM */
	ext_mem		/* EEPROM */
};

/* beep status for comm_beep */
enum beep {
	auto_beep,
	ctrl_beep
};

/******************************************************************************/

typedef unsigned char u8;
typedef unsigned short u16;

/* information and configuration block */
typedef struct _icb {		/* 64 bytes packed */
	u8 model_ver[6],	/* '8', '5', '1', ... */
	   serial[3],
	   reserved_9_0:1,
	   crown_ring:1,	/* 0: Crown Set, 1: Ring Set */
	   reserved_9_2:1,
	   mask_flash:1,	/* 0: Mask Part, 1: Flash Part */
	   reserved_9_4:1,
	   reserved_9_5:1,
	   reserved_9_6:1,
	   reserved_9_7:1,
	   contrast:4,
	   battery:4,
	   scroll,
	   passwd[2],
	   max_char,
	   nite_duration;
	u16 eeprom_size;
	u8 zero_18_0:1,
	   zero_18_1:1,
	   zero_18_2:1,
	   zero_18_3:1,
	   zero_18_4:1,
	   ptc_min:1,
	   ptc_hr:1,
	   ptc_day:1;
	u16 ptc_addr;
	u8 bg_cfg,
	   por_ptr[16],
	   usage_tracking,	/* 0x00: usage tracking data is invalid */
	   contrast_msg_scroll,	/* 0x00 to 0x0f */
	   reserved_40,
	   reserved_41,
	   reserved_42,
	   reserved_43,
	   reserved_44,
	   reserved_45,
	   reserved_46,
	   checksum,
	   sync_id[16];
} __attribute__((packed)) icb_t;

/* crown/ring set for icb_t.crown_ring */
enum crown_ring {
	crown_set,
	ring_set
};

/* mask/flash part for icb_t.mask_flash */
enum mask_flash {
	mask_part,
	flash_part
};

/* system map table (internal memory) */
typedef struct _sysmap {	/* 26 bytes packed */
	u16 heap_top_addr,	/* heap top address */
	    heap_top_addr_r,	/* heap top address after reset */
	    heap_bot_addr_r,	/* heap bottom address after reset */
	    overlay_size,	/* overlay size */
	    mode_ls_addr,	/* mode list address */
	    /* NAPPS * 1 bytes */
	    acd_addr,		/* application configuration data address */
	    /* NAPPS * ACB_SIZE bytes */
	    acb_addr,		/* application control block address */
	    opt_addr,		/* option data address */
	    ptc_addr,		/* periodic task control address */
	    melody_addr,	/* melody table address */
	    melody_init_addr,	/* melody init routine address */
	    cust_melody_addr,	/* custom melody address */
	    def_melody_addr;	/* default ROM based melody table address */
} __attribute__((packed)) sysmap_t;

/* application configuration data (internal memory) */
typedef struct _acd {		/* 1 byte packed */
	u8 app_idx:1,		/* restricted. 0: index is avail, 1: reserved */
	   code_loc:1,		/* code location: 0 internal, 1 external mem */
	   db_loc:1,		/* database location: 0 internal, 1 external */
	   code_invalid:1,	/* 1: code is invalid */
	   db_modified:1,	/* 1: database has been modified */
	   db_invalid:1,	/* 1: database is invalid/not present */
	   passwd_req:1,	/* 1: password is required for access */
	   user_banner:1;	/* 1: mode banner is located in EEPROM */
} __attribute__((packed)) acd_t;

/* application control block (internal mamory) */
/*				: when the order of modes changes */
typedef struct _acb {		/* 14 bytes packed */
	u8 app_type,		/* application type */
	   app_inst;		/* application instance: ascending order */
	u16 asd_addr,		/* system data address (internal memory) */
	    add_addr,		/* database data address */
	    state_mgr_addr,	/* mode state manager address: fixed */
	    refresh_addr,	/* refresh handler address: fixed */
	    banner_addr,	/* banner address: fixed */
	    code_addr;		/* code block address
				   in EEPROM */
} __attribute__((packed)) acb_t;

/* datalink device */
typedef struct _dldev {
	usb_t usb;
	/* information and configuration block */
	icb_t icb;
	sysmap_t sysmap;
	struct {
		acd_t acd;
		acb_t acb;
		char banner[MAX_BANNER_SIZE];
		/*
		u16 add_size;
		u8 *add;
		*/
	} app[NUM_APPS];
} dldev_t;


/******************************************************************************/

/* TOD */
typedef struct _tod_tz {	/* 13 bytes packed */
	u8 primary:1,		/* 1: primary, 0: secondary */
	   update_tz_id:1,	/* 1: update TZ id */
	   update_display:1,	/* 1: update display format flags */
	   update_tz_name:1,	/* 1: update TZ name */
	   update_hms:1,	/* 1: update HMS */
	   update_mdy:1,	/* 1: update MDY */
	   reserved_0_6:1,
	   reserved_0_7:1,
	   date_format:2,	/* 0: DMY, 1: YMD, 2: MDY */
	   hr24_format:1,	/* 0: 12hr, 1: 24hr */
	   display_week_no:1,	/* 1: display week number, 0: day of week */
	   tz_in_dst:1,		/* 1: TZ is in DST */
	   euro_format:1,	/* 1: EURO format, 0: US format */
	   tz_observes_dst:1,	/* 1: TZ observes DST */
	   tz_entered_set_state:1,	/* 1: TZ entered set state */
	   tz_name[3],		/* there can be padding spaces */
	   tz_id,		/* used for world time WristApp */
	   second,		/* BCD data from 00 - 59 */
	   minute,		/* BCD data from 00 - 59 */
	   hour,		/* BCD data from 00 - 23 */
	   day,			/* BCD data from 1 to the max days of the mon */
	   month;		/* BCD data from 01 - 12 */
	u16 year;		/* two BCD data 20, 00 - 99 */
} __attribute__((packed)) tod_tz_t;

enum date_format {
	dmy,
	ymd,
	mdy
};

#ifdef _LIBDLUSB_MISC_C_
char *date_format[] = { /* 3 */
	"dmy",
	"ymd",
	"mdy",
};
#else
extern char *date_format[];
#endif

typedef struct _tod {		/* 39 bytes packed */
	tod_tz_t tod[3];
} __attribute__((packed)) tod_t;


/******************************************************************************/

/* Contact */
typedef struct _contact_hdr {	/* 7 bytes packed */
	u16 alloc_size,
	    db_size;
	u8 hdr_size;
	u16 num_recs;		/* end of common header */
} __attribute__((packed)) contact_hdr_t;

typedef struct _contact_rec {	/* 11 bytes packed */
	u16 type;
	u8 phone[7],
	   msg_offset,
	   msg_len,		/* end of packed */
	   *msg;
	struct _contact_rec *prev, *next;
} __attribute__((packed)) contact_rec_t;

typedef struct _contact {
	contact_hdr_t hdr;
	contact_rec_t *head, *tail;
} contact_t;

typedef struct _contact_data {
	char *msg, *type, *area, *number;
} contact_data_t;


/******************************************************************************/

/* Note */
typedef struct _note_hdr {	/* 15 bytes packed */
	u16 alloc_size,
	    db_size;
	u8 hdr_size;
	u16 num_recs,		/* end of common header */
				/* num of used recs = num_recs - unused_recs */
	    unused_recs,
	    head,
	    tail,
	    unused_head;
} __attribute__((packed)) note_hdr_t;

typedef struct _note_rec {	/* 8 bytes packed */
	u16 next_addr,
	    prev_addr;
	u8 msg_len,
	   used:1,		/* 0: unused, 1: used */
	   modified:1,		/* 0: unmodified, 1: modified */
	   reserved_5_2:1,
	   reserved_5_3:1,
	   reserved_5_4:1,
	   reserved_5_5:1,
	   reserved_5_6:1,
	   reserved_5_7:1;
	u16 id;			/* end of packed */
	u8 *msg;
	struct _note_rec *prev, *next;
} __attribute__((packed)) note_rec_t;

/* it looks like we don't need to update ASD */
#if 0
typedef struct _note_asd {	/* 19 bytes packed */
	u8 scroll_stopped:1,	/* 0: scrolling, 1: stopped */
	   unused_displayed:1,	/* 0: unused not displayed, 1: displayed */
	   eol_displayed:1,	/* 0: end-of-list not displayed, 1: displayed */
	   reserved_0_3:1,
	   reserved_0_4:1,
	   reserved_0_5:1,
	   reserved_0_6:1,
	   reserved_0_7:1;
	u16 num_recs,
	    unused_recs,
	    head,
	    tail,
	    unused_head,
	    curr_addr,
	    next_addr,
	    prev_addr;
	u8 curr_size,
	   curr_status;
} __attribute__((packed)) note_asd_t;
#endif

typedef struct _note {
#if 0
	note_asd_t asd;
#endif
	note_hdr_t hdr;
	note_rec_t *head, *tail;
} note_t;


/******************************************************************************/

/* Appointment and Alarm */
typedef struct _appt_hdr {	/* 15 bytes packed */
	u16 alloc_size,
	    db_size;
	u8 hdr_size;
	u16 num_recs,		/* end of common header */
				/* num of used recs = num_recs - unused_recs */
	    unused_recs,
	    head,
	    tail,
	    unused_head;
} __attribute__((packed)) appt_hdr_t;

typedef struct _appt_rec {	/* 16 bytes packed */
	u16 next_addr,
	    prev_addr;
	u8 msg_len,
	   used:1,
	   armed:1,
	   modified:1,
	   deleted:1,
	   appt:1,		/* 0: alarm, 1: appointment */
	   eol_displayed:1,	/* 1: end-of-list displayed */
	   reserved_5_6:1,
	   reserved_5_7:1,
	   freq,		/* 0: 1-day
				   1: daily
				   2: weekdays
				   3: weekends
				   4: weekly sunday
				   5: weekly monday
				   6: weekly tuesday
				   7: weekly wednesday
				   8: weekly thursday
				   9: weekly friday
				   10: weekly saturday
				   11: weekly
				   12: monthly
				   13: yearly

				   appt: 1-day - yearly except weekly sun - sat
				   alarm: daily - weekly sat
				   */
	   prior,		/* 0: 0 mins
				   1: 5 mins
				   2: 10 mins
				   3: 15 mins
				   4: 30 mins
				   5: 60 mins
				   6: 2 hrs
				   7: 3 hrs
				   8: 4 hrs
				   9: 5 hrs
				   10: 6 hrs
				   11: 8 hrs
				   12: 10 hrs
				   13: 12 hrs
				   14: 24 hrs
				   15: 48 hrs
				   */
	   minute,		/* BCD */
	   hour,
	   day,
	   month;
	u16 year,		/* 2 BCDs */
	    id;			/* end of packed */
	u8 *msg;
	struct _appt_rec *prev, *next;
} __attribute__((packed)) appt_rec_t;

/* frequency for appt_rec_t.freq */
enum appt_freq {
	one_day,
	daily,
	weekdays,
	weekends,
	weekly_sun,
	weekly_mon,
	weekly_tue,
	weekly_wed,
	weekly_thu,
	weekly_fri,
	weekly_sat,
	weekly,
	monthly,
	yearly
};

#ifdef _LIBDLUSB_MISC_C_
char *appt_freq[] = { /* 14 */
	"1-day",
	"daily",
	"weekdays",
	"weekends",
	"weekly sunday",
	"weekly monday",
	"weekly tuesday",
	"weekly wednesday",
	"weekly thursday",
	"weekly friday",
	"weekly saturday",
	"weekly",
	"monthly",
	"yearly"
};

/* appt_rec_t.armed strings */
char *appt_armed[] = { /* 2 */
	"unarmed",
	"armed"
};

/* appt_rec_t.prior strings */
char *appt_prior[] = { /* 16 */
	"0 mins",
	"5 mins",
	"10 mins",
	"15 mins",
	"30 mins",
	"60 mins",
	"2 hrs",
	"3 hrs",
	"4 hrs",
	"5 hrs",
	"6 hrs",
	"8 hrs",
	"10 hrs",
	"12 hrs",
	"24 hrs",
	"48 hrs"
};
#else
extern char *appt_freq[], *appt_armed[], *appt_prior[];
#endif

typedef struct _appt {
	appt_hdr_t hdr;
	appt_rec_t *head, *tail;
} appt_t;

typedef struct _appt_data {
	char *msg;
	int year,
	    month,
	    day,
	    hour,
	    minute;
	u8 appt,
	   armed,
	   freq,
	   prior;
} appt_data_t;


/******************************************************************************/

/* Schedule */
typedef struct _schedule_hdr {	/* 27 bytes packed */
	u16 alloc_size,
	    db_size;
	u8 hdr_size;
	u16 num_groups;		/* end of common header */
	u8 group_type;
	u16 msg1_addr,
	    msg2_addr;
	u8 msg_len, /* not including DM_SENTINEL */
	   msg[14];		/* end of packed */
} __attribute__((packed)) schedule_hdr_t;

enum schedule_group_type {
	date = 1,
	dow_time,
	date_time
};

typedef struct _schedule_group_hdr {	/* 8 bytes packed */
	u16 alloc_size,
	    db_size;
	u8 hdr_size; /* 3 + msg_len */
	u16 num_recs; /* limit: 250 */
	u8 msg_len,			/* end of packed */
	   *msg;
} __attribute__((packed)) schedule_group_hdr_t;

typedef struct _schedule_rec {	/* 7 bytes packed */
	u8 minute,
	   hour,
	   day,
	   month,
	   year,
	   dow, /* day of week */
	   msg_len,		/* end of packed */
	   *msg;
	struct _schedule_rec *prev, *next;
} __attribute__((packed)) schedule_rec_t;

typedef struct _schedule_group {
	schedule_group_hdr_t hdr;
	schedule_rec_t *head, *tail;
	struct _schedule_group *prev, *next;
} schedule_group_t;

typedef struct _schedule {
	schedule_hdr_t hdr;
	schedule_group_t *head, *tail;
} schedule_t;

typedef struct _schedule_data {
	char *group, *msg;
	int year,
	    month,
	    day,
	    hour,
	    minute;
	u8 dow;
} schedule_data_t;

/* dow for schedule_rec_t.dow */
enum schedule_dow { /* 7 */
	sun,
	mon,
	tue,
	wed,
	thu,
	fri,
	sat,
};

#ifdef _LIBDLUSB_MISC_C_
char *schedule_dow[] = { /* 7 */
	"sunday",
	"monday",
	"tuesday",
	"wednesday",
	"thursday",
	"friday",
	"saturday",
};

char *schedule_group_type[] = {
	"date",
	"dow_time",
	"date_time"
};
#else
extern char *schedule_dow[], *schedule_group_type[];
#endif

#define get_schedule_group_type_idx(x) ((x) + 1)
#define get_schedule_group_type_str(x) ((x) > 0 && (x) < 4 ? \
		schedule_group_type[(x)-1] : "none")


/******************************************************************************/

/* Occasion */
typedef struct _occasion_hdr {	/* 15 bytes packed */
	u16 alloc_size,
	    db_size;
	u8 hdr_size;
	u16 num_recs,		/* end of common header */
	    num_nonrecur,
	    num_recur,
	    last_nonrecur,
	    last_recur;
} __attribute__((packed)) occasion_hdr_t;

typedef struct _occasion_rec {	/* 6 bytes packed */
	u8 type,
	   day,
	   month;
	u16 year;
	u8 msg_len;		/* end of packed */
	u8 *msg;
	struct _occasion_rec *prev, *next;
} __attribute__((packed)) occasion_rec_t;

typedef struct _occasion {
	occasion_hdr_t hdr;
	occasion_rec_t *head, *tail;
} occasion_t;

typedef struct _occasion_data {
	char *msg;
	int year,
	    month,
	    day;
	u8 type;
} occasion_data_t;

/* type for occasion_rec_t.type */
enum occasion_type {
	none,		/* none norecur: 0, recur: 0 | (1<<4) */
	birthday,	/* birthday: 1 | (1<<5) | (1<<4) == 1 | (3<<4) */
	anniversary,	/* anniversary: 2 | (1<<5) | (1<<4) == 2 | (3<<4) */
	holiday,	/* holiday norecur: 3, recur: 3 | (1<<4) */
	vacation	/* vacation norecur: 4, recur: 4 | (1<<4) */
};
/* 3<<4 == 0x30 */
/*
#define get_occasion_type(x, r) ((x) > 0 && (x) < 3 ? (x)|0x30 : (x)|((r)<<4))
*/
#define get_occasion_type(x) ((x) > 0 && (x) < 3 ? (x) | 0x30 : (x))
#define get_occasion_idx(x) lobits(x)
#define get_occasion_str(x) occasion_type[get_occasion_idx(x)]
#define set_occasion_recurring(x) ((x) | 0x10)
#define is_occasion_recurring(x) ((x) & 0x10)
#define is_occasion_annual(x) ((x) & 0x20) /* is it birthday or anniversary? */

#ifdef _LIBDLUSB_MISC_C_
char *occasion_type[] = { /* 5 */
	"none",
	"birthday",
	"anniversary",
	"holiday",
	"vacation"
};
#else
extern char *occasion_type[];
#endif


/******************************************************************************/

/* Chrono: the protocol documentation 2004-03-12 is incorrect! */
typedef struct _chrono_hdr {	/* 17 bytes packed */
	u16 alloc_size,
	    db_size;
	u8 hdr_size; /* 12 */
	u16 num_recs; /* max laps + 1, end of common header */
	u8 rec_size, /* 5 */
	   display_format, /* 0: lap_split
			      1: split_lap
			      2: time_split
			      3: time_lap */
	   system_flag, /* bit 2 should be 1 when the number of downloaded free
			   records is less than 2 (TODO: ?) */
	   lap_count, /* current lap count: always 1? TODO */
	   unused_recs, /* unused records */
	   max_recs, /* num_recs */
	   used_recs, /* max_recs - unused_recs */
	   last_workout, /* last workout's rec no (start from 0) */
	   stored_rec, /* used_recs */
	   reserved_16; /* undocumented! */
} __attribute__((packed)) chrono_hdr_t;

enum display_format {
	lap_split,
	split_lap,
	time_split,
	time_lap
};

#ifdef _LIBDLUSB_MISC_C_
char *chrono_display_format[] = { /* 4 */
	"lap_split",
	"split_lap",
	"time_split",
	"time_lap",
};
#else
extern char *chrono_display_format[];
#endif

typedef struct _chrono_split {	/* 5 bytes packed */
	u8 hundredths,
	   seconds,
	   minutes,
	   hours,
	   lap_id;		/* end of packed */
	struct _chrono_split *prev, *next;
} __attribute__((packed)) chrono_split_t;

typedef struct _chrono_workout {	/* 5 bytes packed */
	u8 day,
	   month:4,
	   dow:4;
	u8 year,
	   best_lap_id,
	   num_laps;		/* end of packed */
	chrono_split_t *head, *tail;
	struct _chrono_workout *prev, *next;
} __attribute__((packed)) chrono_workout_t;

typedef struct _chrono {
	chrono_hdr_t hdr;
	chrono_workout_t *head, *tail;
} chrono_t;

typedef struct _chrono_data {
	int year,
	    month,
	    day;
	u8 workout_id,
	   lap_id,
	   dow,
	   hours,
	   minutes,
	   seconds,
	   hundredths;
} chrono_data_t;

#define get_chrono_dow_str(x) schedule_dow[x]


/******************************************************************************/

/* Timer */
typedef struct _timer_hdr {	/* 17 bytes packed */
	u16 alloc_size,
	    db_size;
	u8 hdr_size; /* 3 for countdn timer, 4 for interval timer */
	u16 num_recs; /* max laps + 1, end of common header */
	u8 rec_size, /* 13 */
	   at_end;		/* only for interval timer */
} __attribute__((packed)) timer_hdr_t;

typedef struct _timer_rec {	/* 13 bytes packed */
	u8 at_end:2, /* 0: stop, 1: repeat, 2: chrono */
	   halfway_reminder:1,
	   data_is_0:1,
	   data_less_than_15sec:1,
	   data_less_than_1min:1,
	   reserved_0_6:1,
	   reserved_0_7:1,
	   seconds,
	   minutes,
	   hours,
	   msg[9];		/* end of packed */
	struct _timer_rec *prev, *next;
} __attribute__((packed)) timer_rec_t;

enum timer_at_end {
	stop,
	repeat,
	chrono
};

#ifdef _LIBDLUSB_MISC_C_
char *timer_at_end[] = { /* 3 */
	"stop",
	"repeat",
	"chrono",
};
#else
extern char *timer_at_end[];
#endif

/* use tymer_t because types.h already has timer_t */
typedef struct _tymer {
	timer_hdr_t hdr;
	timer_rec_t *head, *tail;
} tymer_t;

typedef struct _timer_data {
	char *msg;
	u8 hours,
	   minutes,
	   seconds,
	   at_end,
	   halfway_reminder;
} timer_data_t;


/******************************************************************************/

/* Option */
typedef struct _option {	/* 15 bytes packed */
	u8 nite_mode:1,
	   nite_auto:1,
	   reserved_0_2:1,
	   reserved_0_3:1,
	   chime:1,
	   chime_auto:1,
	   reserved_0_6:1,
	   beep:1,
	   nite_on_minute,
	   nite_on_hour,
	   nite_off_minute,
	   nite_off_hour,
	   chime_on_minute,
	   chime_on_hour,
	   chime_off_minute,
	   chime_off_hour,
	   nite_mode_toggle_duration,
	   last_set_char,
	   passwd[2],
	   reserved_13_0:1,
	   reserved_13_1:1,
	   reserved_13_2:1,
	   reserved_13_3:1,
	   reserved_13_4:1,
	   reserved_13_5:1,
	   timeline_occasion:1,
	   timeline_unknown:1,
	   nite_mode_duration; /* always 8 hours? */
} __attribute__((packed)) option_t;

/* factory setting */
#define NITE_ON_HOUR 20
#define NITE_ON_MINUTE 0
#define NITE_OFF_HOUR 6
#define NITE_OFF_MINUTE 0

#define CHIME_ON_HOUR 7
#define CHIME_ON_MINUTE 0
#define CHIME_OFF_HOUR 21
#define CHIME_OFF_MINUTE 0

#define NITE_MODE_TOGGLE_DURATION 4
#define LAST_SET_CHAR 87


/******************************************************************************/

#ifdef _LIBDLUSB_MISC_C_
/* character maps */
u8 *segch = (u8 *)"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ --+:()$",
   *seg16ch = (u8 *)"0123456789 --+()",
   *ldmch = (u8 *)"0123456789 ALPSTI..:-",
   *rdmch = (u8 *)"0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~$ £¥Ä  çñö   !? °           AP 1<>   ",
   *rdm16ch = (u8 *)"0123456789 -.+*#";
#else
extern u8 *segch, *seg16ch, *ldmch, *rdmch, *rdm16ch;
#endif
#define DM_SENTINEL 104


/******************************************************************************/

/* contact.c */
void create_contact(contact_t *db, u8 **data, u16 *len);
int add_contact_file(contact_t *db, FILE *fp);
int del_contact_file(contact_t *db, FILE *fp);
void add_contact(contact_t *db, contact_data_t *data);
int del_contact(contact_t *db, int idx);
int find_contact(contact_t *db, contact_data_t *data);
void read_contact(contact_t *db, u8 *data);
int read_contact_mem(dldev_t *dev, contact_t *db);
void print_contact(contact_t *db, FILE *fp);
void init_contact(contact_t *db);
void free_contact(contact_t *db);
int init_contact_app(dldev_t *dev, u8 *data, u16 len);
int print_all_contacts(dldev_t *dev, char *file);
int read_contact_line(contact_data_t *data, char *line);

/* note.c */
void set_note_unused_recs(int recs);
void set_note_extra_len(int len);
void create_note(note_t *db, u8 **data, u16 *len);
int add_note_file(note_t *db, FILE *fp);
int del_note_file(note_t *db, FILE *fp);
void add_note(note_t *db, char *msg);
int del_note(note_t *db, int idx);
int find_note(note_t *db, char *msg);
void read_note(note_t *db, u8 *data);
int read_note_mem(dldev_t *dev, note_t *db);
void print_note(note_t *db, FILE *fp);
void init_note(note_t *db);
void free_note(note_t *db);
int init_note_app(dldev_t *dev, u8 *data, u16 len);
int print_all_notes(dldev_t *dev, char *file);

/* appt.c */
void set_appt(int idx);
void set_appt_unused_recs(int recs);
void set_appt_extra_len(int len);
void create_appt(appt_t *db, u8 **data, u16 *len);
int add_appt_file(appt_t *db, FILE *fp);
int del_appt_file(appt_t *db, FILE *fp);
void add_appt(appt_t *db, appt_data_t *data);
int del_appt(appt_t *db, int idx);
int find_appt(appt_t *db, appt_data_t *data);
void read_appt(appt_t *db, u8 *data);
int read_appt_mem(dldev_t *dev, appt_t *db);
void print_appt(appt_t *db, FILE *fp);
void init_appt(appt_t *db);
void free_appt(appt_t *db);
int init_appt_app(dldev_t *dev, u8 *data, u16 len);
int print_all_appts(dldev_t *dev, char *file);
int read_appt_line(appt_data_t *data, char *line);

/* schedule.c */
void set_schedule_msg(char *msg);
void set_schedule_group_type(u8 type);
void update_schedule_msg(schedule_t *db);
void update_schedule_group_type(schedule_t *db);
void create_schedule(schedule_t *db, u8 **data, u16 *len);
int add_schedule_file(schedule_t *db, FILE *fp);
int del_schedule_file(schedule_t *db, FILE *fp);
void add_schedule(schedule_t *db, schedule_data_t *data);
int del_schedule(schedule_t *db, int idx);
int find_schedule(schedule_t *db, schedule_data_t *data);
void read_schedule(schedule_t *db, u8 *data);
int read_schedule_mem(dldev_t *dev, schedule_t *db);
void print_schedule(schedule_t *db, FILE *fp);
void init_schedule(schedule_t *db);
void free_schedule(schedule_t *db);
int init_schedule_app(dldev_t *dev, u8 *data, u16 len);
int print_all_schedules(dldev_t *dev, char *file);
int read_schedule_line(schedule_data_t *data, char *line);

/* occasion.c */
void create_occasion(occasion_t *db, u8 **data, u16 *len);
int add_occasion_file(occasion_t *db, FILE *fp);
int del_occasion_file(occasion_t *db, FILE *fp);
void add_occasion(occasion_t *db, occasion_data_t *data);
int del_occasion(occasion_t *db, int idx);
int find_occasion(occasion_t *db, occasion_data_t *data);
void read_occasion(occasion_t *db, u8 *data);
int read_occasion_mem(dldev_t *dev, occasion_t *db);
void print_occasion(occasion_t *db, FILE *fp);
void init_occasion(occasion_t *db);
void free_occasion(occasion_t *db);
int init_occasion_app(dldev_t *dev, u8 *data, u16 len);
int print_all_occasions(dldev_t *dev, char *file);
int read_occasion_line(occasion_data_t *data, char *line);

/* chrono.c */
void set_chrono_unused_recs(int recs);
void set_chrono_display_format(u8 display);
void update_chrono_display_format(chrono_t *db);
void create_chrono(chrono_t *db, u8 **data, u16 *len);
int add_chrono_file(chrono_t *db, FILE *fp);
int del_chrono_file(chrono_t *db, FILE *fp);
void add_chrono(chrono_t *db, chrono_data_t *data);
int del_chrono(chrono_t *db, int idx);
int find_chrono(chrono_t *db, chrono_data_t *data);
void read_chrono(chrono_t *db, u8 *data);
int read_chrono_mem(dldev_t *dev, chrono_t *db);
void print_chrono(chrono_t *db, FILE *fp);
void init_chrono(chrono_t *db);
void free_chrono(chrono_t *db);
int init_chrono_app(dldev_t *dev, u8 *data, u16 len);
int print_all_chronos(dldev_t *dev, char *file);
int read_chrono_line(chrono_data_t *data, char *line);

/* timer.c */
void set_timer(int por);
void set_timer_at_end(u8 end);
void update_timer_at_end(tymer_t *db);
void create_timer(tymer_t *db, u8 **data, u16 *len);
int add_timer_file(tymer_t *db, FILE *fp);
int del_timer_file(tymer_t *db, FILE *fp);
void add_timer(tymer_t *db, timer_data_t *data);
int del_timer(tymer_t *db, int idx);
int find_timer(tymer_t *db, timer_data_t *data);
void read_timer(tymer_t *db, u8 *data);
int read_timer_mem(dldev_t *dev, tymer_t *db);
void print_timer(tymer_t *db, FILE *fp);
void init_timer(tymer_t *db);
void free_timer(tymer_t *db);
int init_timer_app(dldev_t *dev, u8 *data, u16 len);
int print_all_timers(dldev_t *dev, char *file);
int read_timer_line(timer_data_t *data, char *line);

/* memory.c */
int read_sysinfo(dldev_t *dev);
int read_sysmap(dldev_t *dev);
int read_acd(dldev_t *dev);
int read_acb(dldev_t *dev);
int find_app(dldev_t *dev, char *banner);
int lock_app(dldev_t *dev, char *banner);
int unlock_app(dldev_t *dev, char *banner);
int update_acd(dldev_t *dev);
int update_acb(dldev_t *dev);
int dump_acd(dldev_t *dev, u8 *data);
int dump_acb(dldev_t *dev, u8 *data);
int dump_add(dldev_t *dev, int idx, u8 **data, u16 *len);
int dump_rom(dldev_t *dev, u8 **data, u16 *len);
int dump_eeprom(dldev_t *dev, u8 **data, u16 *len);
int load_add(dldev_t *dev, int idx, u8 *data);
int del_all_apps(dldev_t *dev);
int init_int_app(dldev_t *dev, int por, u8 *data, u16 len);
int init_ext_app(dldev_t *dev, char *app, int user_banner);
#ifdef EXPERIMENTAL
int read_tod(dldev_t *dev, tod_t *db);
#endif
int update_tod(dldev_t *dev, tod_t *db);
int read_option(dldev_t *dev, option_t *db);
int update_option(dldev_t *dev, option_t *db);
void print_option(option_t *db, FILE *fp);
void scan_option(option_t *db, FILE *fp);
int read_raw_option(dldev_t *dev, u8 *data);
int update_raw_option(dldev_t *dev, u8 *data);
void scan_raw_option(option_t *db, u8 *data);
void create_raw_option(option_t *db, u8 *data);

/* session.c */
int start_session(dldev_t *dev);
int end_session(dldev_t *dev);
int exit_session(dldev_t *dev);

/* comm.c */
int get_init_app_idx(void);
int get_init_app_inst(void);
int write_abs_addr(dldev_t *dev, u16 addr, u8 type, u8 *data, u16 len);
int read_abs_addr(dldev_t *dev, u16 addr, u8 type, u8 *data, u16 len);
int comm_dev_info_req(dldev_t *dev);
int comm_proc_complete(dldev_t *dev);
int comm_del_all_apps(dldev_t *dev);
int comm_beep(dldev_t *dev, u8 status);
int comm_idle(dldev_t *dev);
int comm_call_abs_addr(dldev_t *dev, u16 addr);
int comm_write_tod_tz(dldev_t *dev, u8 *data, u8 len);
int comm_app_init_int(dldev_t *dev, u8 *data, u8 len);
int comm_app_init_ext(dldev_t *dev, u8 *data, u8 len);
int comm_multi_packet_write(dldev_t *dev);
int comm_write_abs_addr(dldev_t *dev, u16 addr, u8 type, u8 *data, u8 len);
int comm_read_abs_addr(dldev_t *dev, u16 addr, u8 type, u8 *data, u8 len);

/* tucp.c */
int send_read_tucp_packet(dldev_t *dev, u8 *spacket, u8 *rpacket);
int send_idle(dldev_t *dev);
void tucp_progress(int show);

/* usb_*.c */
int open_dev(dldev_t *dev);
int close_dev(dldev_t *dev);
int send_usb_packet(dldev_t *dev, u8 *packet, u16 len);
int read_usb_packet(dldev_t *dev, u8 *packet, u16 len);

/* misc.c */
char *squeeze_spaces(char *buf);
char *read_line(FILE *fp);
int read_file(char *file, u8 **data, u16 *len);
int convert_ch(u8 *fontch, u8 ch);
void print_rdm(u8 *msg, u8 msg_len, FILE *fp);
int str_chr(char *str, char ch);
int str_str(char *big, char *little);
int day_of_week(int y, int m, int d);

#endif
