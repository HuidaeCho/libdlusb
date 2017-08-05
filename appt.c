/* $Id: appt.c,v 1.15 2009/05/12 05:44:00 geni Exp $
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
#include <ctype.h>
#include "dlusb.h"
#include "define.h"

#define is_appt (app_idx == APP_APPT)
#define app_name (is_appt ? "APPT" : "ALARM")
#define app_por (is_appt ? POR_APPT : POR_ALARM)

static int app_idx = APP_APPT, unused_recs = UNUSED_RECS,
	   extra_msg_len = EXTRA_MSG_LEN;

static int cmp_appt(appt_rec_t *a, appt_rec_t *b);
static void sort_appt(appt_t *db);
static void qsort_appt(appt_t *db, appt_rec_t *head, appt_rec_t *tail);
static void update_appt(appt_t *db);

/* TODO: rec->status */

void
set_appt(int idx)
{
	app_idx = (idx == APP_ALARM ? APP_ALARM : APP_APPT);

	return;
}

void
set_appt_unused_recs(int recs)
{
	unused_recs = (recs >= 0 ? recs : 0);

	return;
}

void
set_appt_extra_len(int len)
{
	extra_msg_len = (len < MIN_MSG_LEN ? MIN_MSG_LEN : len);

	return;
}

void
create_appt(appt_t *db, u8 **data, u16 *len)
{
	int i;
	u8 *m;
	u16 offset, prev_addr, next_addr;
	appt_rec_t *rec;

	update_appt(db);

	*len = db->hdr.alloc_size;
	*data = (u8 *)malloc(*len);
	memcpy(*data, &db->hdr, 15);

	offset = db->hdr.head;
	for(rec = db->head; rec; rec = rec->next){
		memcpy(*data+offset, rec, 16);
		memcpy(*data+offset+16, rec->msg, rec->msg_len);
		offset = rec->next_addr;
	}

	/* create unused memory block if there are unused records */
	if(!db->hdr.unused_recs)
		return;

	/* unused blank message */
	m = (u8 *)malloc(31);
	for(i = 0; i < 30; i++)
		m[i] = rdmi(' ');
	m[i] = DM_SENTINEL;

	offset = db->hdr.unused_head;
	prev_addr = 0;
	for(i = 0; i < db->hdr.unused_recs; i++){
		next_addr = offset + 47;
		memset(*data+offset, 0, 16);
		memcpy(*data+offset, &next_addr, 2);	/* next_addr */
		memcpy(*data+offset+2, &prev_addr, 2);	/* prev_addr */
		*(*data+offset+4) = 31;			/* msg_len */
		memcpy(*data+offset+16, m, 31);
		prev_addr = offset;
		offset = next_addr;
	}

	next_addr = 0;
	memcpy(*data+prev_addr, &next_addr, 2);		/* tail's next_addr */

	free(m);

	return;
}

int
add_appt_file(appt_t *db, FILE *fp)
{
	int ret = -1;
	char *line;
	appt_data_t data;

	while((line = read_line(fp))){
		if(read_appt_line(&data, line)){
			free(line);
			continue;
		}
		if(find_appt(db, &data) < 0){
			add_appt(db, &data);
			ret = 0;
		}
		free(line);
	}

	return ret;
}

int
del_appt_file(appt_t *db, FILE *fp)
{
	int ret = -1, i;
	char *line;
	appt_data_t data;

	while((line = read_line(fp))){
		if(read_appt_line(&data, line)){
			free(line);
			continue;
		}
		if((i = find_appt(db, &data)) >= 0 && !del_appt(db, i))
			ret = 0;
		free(line);
	}

	return ret;
}

void
add_appt(appt_t *db, appt_data_t *data)
{
	int i;
	appt_rec_t *rec, *r;

	/* use zalloc() to initialize reserved bits */
	rec = (appt_rec_t *)zalloc(sizeof(appt_rec_t));
	i = strlen(data->msg);
	i = (i > MAX_MSG_LEN ? MAX_MSG_LEN : i);
	rec->msg_len = i + 1;
	rec->msg = (u8 *)malloc(rec->msg_len);

	rec->msg[i] = DM_SENTINEL;
	for(i--; i >= 0; i--)
		rec->msg[i] = rdmi(data->msg[i]);

	if(data->minute < 0) data->minute = 0; else
	if(data->minute > 59) data->minute = 59;
	if(data->hour < 0) data->hour = 0; else
	if(data->hour > 23) data->hour = 23;
	if(data->day < 1) data->day = 1; else
	if(data->day > 31) data->day = 31;
	if(data->month < 1) data->month = 1; else
	if(data->month > 12) data->month = 12;
	if(data->year < 2000) data->year = 2000; else
	if(data->year > 2099) data->year = 2099;

	rec->used = 1;
	rec->armed = data->armed;
	/* rec->modified = 1; doesn't work with the Timex PIM */
	rec->modified = 0;
	rec->deleted = 0;
	rec->appt = 1;
	rec->eol_displayed = 0;
	if(is_appt){
		/* appt: 1-day - yearly except weekly sun - sat */
		if(data->freq >= weekly_sun && data->freq <= weekly_sat)
			data->freq = weekly;
	}else{
		/* alarm: daily - weekly sat */
		if(data->freq < daily)
			data->freq = daily;
		else
		if(data->freq > weekly_sat)
			data->freq = weekdays;
	}
	rec->freq = data->freq;
	rec->prior = data->prior;
	rec->minute = bcd(data->minute);
	rec->hour = bcd(data->hour);
	rec->day = bcd(data->day);
	rec->month = bcd(data->month);
	rec->year = bcd2(data->year);

	/* sort */
	for(r = db->head; r && cmp_appt(rec, r) >= 0; r = r->next);
	if(r){
		rec->prev = r->prev;
		if(rec->prev)
			rec->prev->next = rec;
		else
			db->head = rec;
		rec->next = r;
		rec->next->prev = rec;
	}else{
		rec->prev = db->tail;
		if(rec->prev)
			rec->prev->next = rec;
		else
			db->head = rec;
		rec->next = NULL;
		db->tail = rec;
	}

	db->hdr.num_recs++;

	return;
}

int
del_appt(appt_t *db, int idx)
{
	int i;
	appt_rec_t *rec;

	if(idx < 0 || idx >= db->hdr.num_recs - db->hdr.unused_recs)
		return -1;

	for(i = 0, rec = db->head; rec && i != idx; i++, rec = rec->next);
	if(!rec){
		ERROR("DATA CORRUPTED: THIS SHOULD NOT HAPPEN!");
		return -1;
	}

	if(rec == db->head){
		db->head = rec->next;
		if(db->head)
			db->head->prev = NULL;
	}else
		rec->prev->next = rec->next;
	if(rec == db->tail){
		db->tail = rec->prev;
		if(db->tail)
			db->tail->next = NULL;
	}else
		rec->next->prev = rec->prev;

	free(rec->msg);
	free(rec);

	db->hdr.num_recs--;

	return 0;
}

int
find_appt(appt_t *db, appt_data_t *data)
{
	int i, l;
	u8 *m, month, day, hour, minute, appt, armed, freq, prior;
	u16 year;
	appt_rec_t *rec;

	l = strlen(data->msg);
	l = (l > MAX_MSG_LEN ? MAX_MSG_LEN : l);
	m = (u8 *)malloc(l+1);
	for(i = 0; i < l; i++)
		m[i] = rdmi(data->msg[i]);
	m[i] = DM_SENTINEL;

	year = bcd2(data->year);
	month = bcd(data->month);
	day = bcd(data->day);
	hour = bcd(data->hour);
	minute = bcd(data->minute);
	appt = data->appt;
	armed = data->armed;
	freq = data->freq;
	prior = data->prior;

	for(i = 0, rec = db->head; rec; i++, rec = rec->next){
		if(rec->year == year && rec->month == month &&
				rec->day == day && rec->hour == hour &&
				rec->minute == minute && rec->appt == appt &&
				rec->armed == armed && rec->freq == freq &&
				rec->prior == prior &&
				!memcmp(rec->msg, m, l+1))
			break;
	}

	free(m);

	return (rec ? i : -1);
}

/* dump_add()/read_appt() is usually slower than read_appt_mem() because
 * dump_add() downloads even allocated unused memory.  In contrast, Contact
 * does not have any unused memory. */
void
read_appt(appt_t *db, u8 *data)
{
	int j;
	u16 ptr;
	appt_rec_t *rec;

	memcpy(&db->hdr, data, 15);

	db->head = db->tail = NULL;
	for(ptr = db->hdr.head; ptr; ptr = rec->next_addr){
		rec = (appt_rec_t *)malloc(sizeof(appt_rec_t));
		memcpy(rec, data+ptr, 16);

		rec->msg = (u8 *)malloc(rec->msg_len);
		memcpy(rec->msg, data+ptr+16, rec->msg_len);
		/* TODO: Timex software bug?  Some character values are
		 * unpredictably greater than 0x69 and it should be & ~(1 << 7)
		 * to be compared with dl's index. */
		for(j = 0; j < rec->msg_len && rec->msg[j] != DM_SENTINEL; j++)
			rec->msg[j] &= ~(1 << 7);
		/* recalculate the length of the message */
		rec->msg_len = j + 1;

		rec->prev = rec->next = NULL;
		if(db->head){
			rec->prev = db->tail;
			db->tail->next = rec;
			db->tail = rec;
		}else
			db->head = db->tail = rec;
	}

	return;
}

/* too slow */
int
read_appt_mem(dldev_t *dev, appt_t *db)
{
	int i, j;
	u16 add_addr, ptr;
	appt_rec_t *rec;

	/* find the application */
	if((i = find_app(dev, app_name)) < 0){
		ERROR("%s application not found", app_name);
		return -1;
	}

	/* application database data */
	add_addr = dev->app[i].acb.add_addr;
	/* allocation size */
	if(read_abs_addr(dev, add_addr, ext_mem, (u8 *)&db->hdr, 15)){
		ERROR("read_abs_addr");
		return -1;
	}

	db->head = db->tail = NULL;
	for(ptr = db->hdr.head; ptr; ptr = rec->next_addr){
		rec = (appt_rec_t *)malloc(sizeof(appt_rec_t));
		if(read_abs_addr(dev, add_addr+ptr, ext_mem, (u8 *)rec, 16)){
			ERROR("read_abs_addr");
			return -1;
		}
		rec->msg = (u8 *)malloc(rec->msg_len);
		if(read_abs_addr(dev, add_addr+ptr+16, ext_mem, (u8 *)rec->msg,
					rec->msg_len)){
			ERROR("read_abs_addr");
			return -1;
		}
		/* TODO: Timex software bug?  Some character values are
		 * unpredictably greater than 0x69 and it should be & ~(1 << 7)
		 * to be compared with dl's index. */
		for(j = 0; j < rec->msg_len && rec->msg[j] != DM_SENTINEL; j++)
			rec->msg[j] &= ~(1 << 7);
		/* recalculate the length of the message */
		rec->msg_len = j + 1;

		rec->prev = rec->next = NULL;
		if(db->head){
			rec->prev = db->tail;
			db->tail->next = rec;
			db->tail = rec;
		}else
			db->head = db->tail = rec;
	}

	return 0;
}

void
print_appt(appt_t *db, FILE *fp)
{
	appt_rec_t *rec;

	for(rec = db->head; rec; rec = rec->next){
		print_rdm(rec->msg, rec->msg_len, fp);
		fprintf(fp, "\t%d-%02d-%02d %02d:%02d\t%s\t%s\t%s\n",
			debcd2(rec->year), debcd(rec->month), debcd(rec->day),
			debcd(rec->hour), debcd(rec->minute),
			appt_armed[rec->armed], appt_freq[rec->freq],
			appt_prior[rec->prior]);
	}

	return;
}

void
init_appt(appt_t *db)
{
	db->hdr.db_size = 15 + 47 * db->hdr.unused_recs;
	db->hdr.hdr_size = 10;
	db->hdr.num_recs = db->hdr.unused_recs;
	db->hdr.head = db->hdr.tail = 0;
	db->hdr.unused_head = 15;
	db->hdr.alloc_size = ((db->hdr.db_size-1) / PAGE_SIZE + 1) * PAGE_SIZE;
	db->head = db->tail = NULL;

	return;
}

void
free_appt(appt_t *db)
{
	appt_rec_t *rec, *rec_next;

	for(rec = db->head; rec; rec = rec_next){
		rec_next = rec->next;
		free(rec->msg);
		free(rec);
	}

	return;
}

int
init_appt_app(dldev_t *dev, u8 *data, u16 len)
{
	return init_int_app(dev, app_por, data, len);
}

int
print_all_appts(dldev_t *dev, char *file)
{
	FILE *fp;
	int i;
	u16 add_addr, head, tail, ptr;
	appt_rec_t rec;
	u8 msg[101];

	/* find the application */
	if((i = find_app(dev, app_name)) < 0){
		ERROR("%s application not found", app_name);
		return -1;
	}

	if(!(fp = fopen(file, "w"))){
		ERROR("%s: open failed", file);
		return -1;
	}

	/* application database data */
	add_addr = dev->app[i].acb.add_addr;
	/* list head: ptr to offset of record from ADD start */
	if(read_abs_addr(dev, add_addr+2+2+1+2+2, ext_mem, (u8 *)&head, 2)){
		ERROR("read_abs_addr");
		return -1;
	}
	/* list tail: ptr to offset of record from ADD start */
	if(read_abs_addr(dev, add_addr+11, ext_mem, (u8 *)&tail, 2)){
		ERROR("read_abs_addr");
		return -1;
	}

	for(ptr = head; ptr; ptr = rec.next_addr){
		if(read_abs_addr(dev, add_addr+ptr, ext_mem, (u8 *)&rec, 16)){
			ERROR("read_abs_addr");
			return -1;
		}
		if(read_abs_addr(dev, add_addr+ptr+16, ext_mem, msg,
					rec.msg_len)){
			ERROR("read_abs_addr");
			return -1;
		}

		print_rdm(msg, rec.msg_len, fp);
		fprintf(fp, "\t%d-%02d-%02d %02d:%02d\t%s\t%s\t%s\n",
			debcd2(rec.year), debcd(rec.month), debcd(rec.day),
			debcd(rec.hour), debcd(rec.minute),
			appt_armed[rec.armed], appt_freq[rec.freq],
			appt_prior[rec.prior]);
	}

	fclose(fp);

	return 0;
}

int
read_appt_line(appt_data_t *data, char *line)
{
	int i, j, k, l, n;

	l = strlen(line);
	for(i = 0; i < l && line[i] != '\t'; i++);
	if(i == l)
		return -1;
	line[(i > MAX_MSG_LEN ? MAX_MSG_LEN : i)] = 0;
	/*
	data->msg = squeeze_spaces(line);
	*/
	data->msg = line;

	for(j = ++i; i < l && isdigit(line[i]); i++);
	if(i == l)
		return -1;
	line[(i > j+4 ? j+4 : i)] = 0;
	data->year = atoi(line + j);

	for(j = ++i; i < l && isdigit(line[i]); i++);
	if(i == l)
		return -1;
	line[(i > j+2 ? j+2 : i)] = 0;
	data->month = atoi(line + j);

	for(j = ++i; i < l && isdigit(line[i]); i++);
	if(i == l)
		return -1;
	line[(i > j+2 ? j+2 : i)] = 0;
	data->day = atoi(line + j);

	for(j = ++i; i < l && isdigit(line[i]); i++);
	if(i == l)
		return -1;
	line[(i > j+2 ? j+2 : i)] = 0;
	data->hour = atoi(line + j);

	for(j = ++i; i < l && line[i] != '\t'; i++);
	if(i == l)
		return -1;
	line[(i > j+2 ? j+2 : i)] = 0;
	data->minute = atoi(line + j);

	data->appt = is_appt;

	for(j = ++i; i < l && line[i] != '\t'; i++);
	if(i == l)
		return -1;
	line[i] = 0;
	n = 2;
	for(k = 0; k < n && strcmp(line + j, appt_armed[k]); k++);
	if(k == n)
		return -1;
	data->armed = k;

	for(j = ++i; i < l && line[i] != '\t'; i++);
	if(i == l)
		return -1;
	line[i] = 0;
	n = 14;
	for(k = 0; k < n && strcmp(line + j, appt_freq[k]); k++);
	if(k == n)
		return -1;
	data->freq = k;

	j = ++i;
	n = 16;
	for(k = 0; k < n && strcmp(line + j, appt_prior[k]); k++);
	if(k == n)
		return -1;
	data->prior = k;

	return 0;
}

static int
cmp_appt(appt_rec_t *a, appt_rec_t *b)
{
	int c, l;
	u8 buf1[6], buf2[6];

	buf1[0] = hibyte(a->year);
	buf1[1] = lobyte(a->year);
	buf1[2] = a->month;
	buf1[3] = a->day;
	buf1[4] = a->hour;
	buf1[5] = a->minute;

	buf2[0] = hibyte(b->year);
	buf2[1] = lobyte(b->year);
	buf2[2] = b->month;
	buf2[3] = b->day;
	buf2[4] = b->hour;
	buf2[5] = b->minute;

	c = memcmp(buf1, buf2, 6);
	if(c < 0)
		return -1;

	if(c > 0)
		return 1;

	/* DM_SENTINEL is ignored when comparing strings */
	l = (a->msg_len < b->msg_len ? a->msg_len : b->msg_len) - 1;
	c = memcmp(a->msg, b->msg, l);
	if(c < 0 || (!c && a->msg_len < b->msg_len))
		return -1;

	return 0;
}

static void
sort_appt(appt_t *db)
{
	qsort_appt(db, db->head, db->tail);

	return;
}

DEFINE_QSORT(appt)

static void
update_appt(appt_t *db)
{
	int i, j, k;
	u16 prev_addr, next_addr;
	appt_rec_t *rec;

	sort_appt(db);

	db->hdr.num_recs += unused_recs - db->hdr.unused_recs;
	db->hdr.unused_recs = unused_recs;

	next_addr = 15;
	db->hdr.head = (db->head ? next_addr : 0);
	prev_addr = 0;
	for(i = 0, rec = db->head; rec; i++, rec = rec->next){
		rec->id = i;
		rec->prev_addr = prev_addr;
		prev_addr = next_addr;
		/* data entered using the watch has a length of 47 bytes per
		 * record: 16 (header) + 30 (message) + 1 (sentinel). but, when
		 * it's downloaded to the watch by the PC, the length of the
		 * data need to be corrected. */
		for(j = 0; j < rec->msg_len && rec->msg[j] != DM_SENTINEL; j++);
		k = j + extra_msg_len;
		rec->msg_len = (k > MAX_MSG_LEN ? MAX_MSG_LEN : k) + 1;
		if(rec->msg){
			rec->msg = (u8 *)realloc(rec->msg, rec->msg_len);
			for(; j < rec->msg_len; j++)
				rec->msg[j] = DM_SENTINEL;
		}
		next_addr += 16 + rec->msg_len;
		rec->next_addr = next_addr;
	}
	if(db->tail)
		db->tail->next_addr = 0;
	db->hdr.tail = prev_addr;
	db->hdr.unused_head = (db->hdr.unused_recs ? next_addr : 0);

	db->hdr.db_size = next_addr + 47 * db->hdr.unused_recs;
	db->hdr.alloc_size = ((db->hdr.db_size-1) / PAGE_SIZE + 1) * PAGE_SIZE;

	return;
}
