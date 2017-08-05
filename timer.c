/* $Id: timer.c,v 1.2 2005/09/25 15:51:37 geni Exp $
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

#define is_countdn (app_por == POR_COUNTDN)
#define app_name (is_countdn ? "COUNTDN TIMER" : "INTERVAL TIMER")
#define header_size (is_countdn ? 8 : 9)

static int app_por = POR_COUNTDN;
static u8 at_end = stop;

void
set_timer(int por)
{
	app_por = (por == POR_INTERVAL ? POR_INTERVAL : POR_COUNTDN);

	return;
}

void
set_timer_at_end(u8 end)
{
	at_end = end;

	return;
}

void
update_timer_at_end(tymer_t *db)
{
	db->hdr.at_end = at_end;
}

void
create_timer(tymer_t *db, u8 **data, u16 *len)
{
	int i;
	u16 o;
	timer_rec_t *rec;

	o = header_size;

	*len = db->hdr.alloc_size;
	*data = (u8 *)malloc(*len);
	memcpy(*data, &db->hdr, o);

	for(rec = db->head, i = 0; rec; rec = rec->next, i++)
		memcpy(*data+o+13*i, rec, 13);

	return;
}

int
add_timer_file(tymer_t *db, FILE *fp)
{
	int ret = -1;
	char *line;
	timer_data_t data;

	while((line = read_line(fp))){
		if(read_timer_line(&data, line)){
			if(!is_countdn){
				int i;

				for(i = 0; i < 3 &&
					strcmp(line, timer_at_end[i]); i++);
				if(i < 3){
					set_timer_at_end(i);
					update_timer_at_end(db);
				}
			}
			free(line);
			continue;
		}
		if(find_timer(db, &data) < 0){
			add_timer(db, &data);
			ret = 0;
		}
		free(line);
	}

	return ret;
}

int
del_timer_file(tymer_t *db, FILE *fp)
{
	int ret = -1, i;
	char *line;
	timer_data_t data;

	while((line = read_line(fp))){
		if(read_timer_line(&data, line)){
			free(line);
			continue;
		}
		if((i = find_timer(db, &data)) >= 0 && !del_timer(db, i))
			ret = 0;
		free(line);
	}

	return ret;
}

void
add_timer(tymer_t *db, timer_data_t *data)
{
	int i;
	timer_rec_t *rec, *r;

	/* use zalloc() to initialize reserved bits */
	rec = (timer_rec_t *)zalloc(sizeof(timer_rec_t));

	for(i = 0; i < 9 && data->msg[i]; i++)
		rec->msg[i] = rdmi(data->msg[i]);
	for(; i < 9; i++)
		rec->msg[i] = rdmi(' ');

	rec->hours = bcd(data->hours);
	rec->minutes = bcd(data->minutes);
	rec->seconds = bcd(data->seconds);

	if(is_countdn)
		rec->at_end = data->at_end;
	rec->halfway_reminder = data->halfway_reminder;
	if(!data->hours && !data->minutes && !data->seconds)
		rec->data_is_0 = 1;
	if(!data->hours && !data->minutes && data->seconds < 15)
		rec->data_less_than_15sec = 1;
	if(!data->hours && !data->minutes && data->seconds < 60 )
		rec->data_less_than_1min = 1;

	db->hdr.db_size += 13;

	/* sort */
	for(r = db->head; r; r = r->next){
		if(memcmp(rec->msg, r->msg, 9) < 0)
			break;
	}
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
	db->hdr.alloc_size = ((db->hdr.db_size-1) / PAGE_SIZE + 1) * PAGE_SIZE;

	return;
}

int
del_timer(tymer_t *db, int idx)
{
	int i;
	timer_rec_t *rec;

	if(idx < 0 || idx >= db->hdr.num_recs)
		return -1;

	for(rec = db->head, i = 0; rec && i != idx; rec = rec->next, i++);
	if(!rec){
		ERROR("DATA CORRUPTED: THIS SHOULD NOT HAPPEN!");
		return -1;
	}

	db->hdr.db_size -= 13;

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

	free(rec);

	db->hdr.num_recs--;
	db->hdr.alloc_size = ((db->hdr.db_size-1) / PAGE_SIZE + 1) * PAGE_SIZE;

	return 0;
}

int
find_timer(tymer_t *db, timer_data_t *data)
{
	int i;
	u8 m[9];
	timer_rec_t *rec;

	for(i = 0; i < 9 && data->msg[i]; i++)
		m[i] = rdmi(data->msg[i]);
	for(; i < 9; i++)
		m[i] = rdmi(' ');

	for(rec = db->head, i = 0; rec && memcmp(rec->msg, m, 9);
			rec = rec->next, i++);

	return (rec ? i : -1);
}

void
read_timer(tymer_t *db, u8 *data)
{
	int i, j;
	u16 o;
	timer_rec_t *rec;

	o = header_size;
	memcpy(&db->hdr, data, o);

	db->head = db->tail = NULL;
	for(i = 0; i < db->hdr.num_recs; i++){
		rec = (timer_rec_t *)malloc(sizeof(timer_rec_t));
		memcpy(rec, data+o+13*i, 13);
		/* TODO: Timex software bug?  Some character values are
		 * unpredictably greater than 0x69 and it should be & ~(1 << 7)
		 * to be compared with dl's index. */
		for(j = 0; j < 9; j++)
			rec->msg[j] &= ~(1 << 7);

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
read_timer_mem(dldev_t *dev, tymer_t *db)
{
	int i, j;
	u16 add_addr, o;
	timer_rec_t *rec;

	/* find the application */
	if((i = find_app(dev, app_name)) < 0){
		ERROR("%s application not found", app_name);
		return -1;
	}

	/* application database data */
	add_addr = dev->app[i].acb.add_addr;
	/* allocation size */
	o = header_size;
	if(read_abs_addr(dev, add_addr, ext_mem, (u8 *)&db->hdr, o)){
		ERROR("read_abs_addr");
		return -1;
	}

	db->head = db->tail = NULL;
	for(i = 0; i < db->hdr.num_recs; i++){
		rec = (timer_rec_t *)malloc(sizeof(timer_rec_t));
		if(read_abs_addr(dev, add_addr+o+13*i, ext_mem, (u8 *)rec, 13)){
			ERROR("read_abs_addr");
			return -1;
		}
		/* TODO: Timex software bug?  Some character values are
		 * unpredictably greater than 0x69 and it should be & ~(1 << 7)
		 * to be compared with dl's index. */
		for(j = 0; j < 9; j++)
			rec->msg[j] &= ~(1 << 7);

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
print_timer(tymer_t *db, FILE *fp)
{
	timer_rec_t *rec;

	if(!is_countdn)
		fprintf(fp, "%s\n", timer_at_end[db->hdr.at_end]);
	for(rec = db->head; rec; rec = rec->next){
		print_rdm(rec->msg, 9, fp);
		fprintf(fp, "\t%02d:%02d:%02d", debcd(rec->hours),
				debcd(rec->minutes), debcd(rec->seconds));
		if(is_countdn)
			fprintf(fp, "\t%s", timer_at_end[rec->at_end]);
		if(rec->halfway_reminder)
			fprintf(fp, "\thalfway reminder");
		fprintf(fp, "\n");
	}

	return;
}

void
init_timer(tymer_t *db)
{
	db->hdr.db_size = header_size;
	db->hdr.hdr_size = 3 + !is_countdn;
	db->hdr.num_recs = 0;
	db->hdr.rec_size = 13;
	if(!is_countdn)
		db->hdr.at_end = stop;
	db->hdr.alloc_size = ((db->hdr.db_size-1) / PAGE_SIZE + 1) * PAGE_SIZE;
	db->head = db->tail = NULL;

	return;
}

void
free_timer(tymer_t *db)
{
	timer_rec_t *rec, *rec_next;

	for(rec = db->head; rec; rec = rec_next){
		rec_next = rec->next;
		free(rec);
	}

	return;
}

int
init_timer_app(dldev_t *dev, u8 *data, u16 len)
{
	return init_int_app(dev, app_por, data, len);
}

int
print_all_timers(dldev_t *dev, char *file)
{
	FILE *fp;
	int i;
	u16 add_addr, o;
	timer_rec_t rec;
	u8 at_end, num_recs;

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
	/* number of records */
	if(read_abs_addr(dev, add_addr+5, ext_mem, (u8 *)&num_recs, 1)){
		ERROR("read_abs_addr");
		return -1;
	}

	if(!is_countdn){
		if(read_abs_addr(dev, add_addr+8, ext_mem, (u8 *)&at_end, 1)){
			ERROR("read_abs_addr");
			return -1;
		}
		fprintf(fp, "%s\n", timer_at_end[at_end]);
	}

	o = header_size;
	for(i = 0; i < num_recs; i++){
		if(read_abs_addr(dev, add_addr+o+13*i, ext_mem, (u8 *)&rec,
					13)){
			ERROR("read_abs_addr");
			return -1;
		}
		print_rdm(rec.msg, 9, fp);
		fprintf(fp, "\t%02d:%02d:%02d", debcd(rec.hours),
				debcd(rec.minutes), debcd(rec.seconds));
		if(is_countdn)
			fprintf(fp, "\t%s", timer_at_end[rec.at_end]);
		if(rec.halfway_reminder)
			fprintf(fp, "\thalfway reminder");
		fprintf(fp, "\n");
	}

	fclose(fp);

	return 0;
}

int
read_timer_line(timer_data_t *data, char *line)
{
	int i, j, k, l, n;

	l = strlen(line);
	for(i = 0; i < l && line[i] != '\t'; i++);
	if(i == l)
		return -1;
	line[(i > 9 ? 9 : i)] = 0;
	data->msg = line;

	for(j = ++i; i < l && isdigit(line[i]); i++);
	if(i == l)
		return -1;
	line[(i > j+2 ? j+2 : i)] = 0;
	data->hours = atoi(line + j);

	for(j = ++i; i < l && isdigit(line[i]); i++);
	if(i == l)
		return -1;
	line[(i > j+2 ? j+2 : i)] = 0;
	data->minutes = atoi(line + j);

	for(j = ++i; i < l && line[i] != '\t'; i++);
	if(i == l && is_countdn)
		return -1;
	line[(i > j+2 ? j+2 : i)] = 0;
	data->seconds = atoi(line + j);

	if(is_countdn){
		for(j = ++i; i < l && line[i] != '\t'; i++);
		line[i] = 0;
		n = 3;
		for(k = 0; k < n && strcmp(line + j, timer_at_end[k]); k++);
		if(k == n)
			return -1;
		data->at_end = k;
		data->halfway_reminder = 0;
		if(i == l)
			return 0;
	}

	j = ++i;
	if(!strcmp(line + j, "halfway reminder"))
		data->halfway_reminder = 1;

	return 0;
}
