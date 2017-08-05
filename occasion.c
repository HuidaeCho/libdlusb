/* $Id: occasion.c,v 1.7 2005/10/06 19:21:22 geni Exp $
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

void
create_occasion(occasion_t *db, u8 **data, u16 *len)
{
	int i;
	u16 offset;
	occasion_rec_t *rec;

	*len = db->hdr.alloc_size;
	*data = (u8 *)malloc(*len);
	memcpy(*data, &db->hdr, 15);

	offset = 2 * db->hdr.num_recs;
	for(i = 0, rec = db->head; rec; i++, rec = rec->next){
		memcpy(*data+15+2*i, &offset, 2);
		memcpy(*data+15+offset, rec, 6);
		offset += 6;
		memcpy(*data+15+offset, rec->msg, rec->msg_len);
		offset += rec->msg_len;
	}

	return;
}

int
add_occasion_file(occasion_t *db, FILE *fp)
{
	int ret = -1;
	char *line;
	occasion_data_t data;

	while((line = read_line(fp))){
		if(read_occasion_line(&data, line)){
			free(line);
			continue;
		}
		if(find_occasion(db, &data) < 0){
			add_occasion(db, &data);
			ret = 0;
		}
		free(line);
	}

	return ret;
}

int
del_occasion_file(occasion_t *db, FILE *fp)
{
	int ret = -1, i;
	char *line;
	occasion_data_t data;

	while((line = read_line(fp))){
		if(read_occasion_line(&data, line)){
			free(line);
			continue;
		}
		if((i = find_occasion(db, &data)) >= 0 && !del_occasion(db, i))
			ret = 0;
		free(line);
	}

	return ret;
}

void
add_occasion(occasion_t *db, occasion_data_t *data)
{
	int i, j, c, l;
	u8 buf1[4], buf2[4];
	occasion_rec_t *rec, *r;

	/* 2: record offset, 6: record header */
	db->hdr.db_size += 2 + 6;

	rec = (occasion_rec_t *)malloc(sizeof(occasion_rec_t));

	l = strlen(data->msg);
	l = (l > MAX_MSG_LEN ? MAX_MSG_LEN : l);
	rec->msg_len = l + 1;
	rec->msg = (u8 *)malloc(rec->msg_len);
	for(i = 0; i < l; i++)
		rec->msg[i] = rdmi(data->msg[i]);
	rec->msg[i] = DM_SENTINEL;

	rec->type = data->type;
	rec->year = bcd2(data->year);
	rec->month = bcd(data->month);
	rec->day = bcd(data->day);

	db->hdr.db_size += rec->msg_len;

	r = db->head;
	j = db->hdr.num_recur;
	if(!is_occasion_recurring(rec->type)){
		for(i = 0; r && i < j; r = r->next, i++);
		j = db->hdr.num_nonrecur;
	}

	/* sort */
	buf1[0] = hibyte(rec->year);
	buf1[1] = lobyte(rec->year);
	buf1[2] = rec->month;
	buf1[3] = rec->day;

	for(i = 0; r && i < j; r = r->next, i++){
		buf2[0] = hibyte(r->year);
		buf2[1] = lobyte(r->year);
		buf2[2] = r->month;
		buf2[3] = r->day;
		c = memcmp(buf1, buf2, 4);
		if(c < 0)
			break;
		if(c > 0)
			continue;
		/* DM_SENTINEL is ignored when comparing strings */
		l = (rec->msg_len < r->msg_len ? rec->msg_len : r->msg_len) - 1;
		c = memcmp(rec->msg, r->msg, l);
		if((!c && rec->msg_len < r->msg_len) || c < 0)
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
	if(is_occasion_recurring(rec->type)){
		db->hdr.num_recur++;
		db->hdr.last_recur++;
	}else
		db->hdr.num_nonrecur++;
	db->hdr.last_nonrecur++;
	db->hdr.alloc_size = ((db->hdr.db_size-1) / PAGE_SIZE + 1) * PAGE_SIZE;

	return;
}

int
del_occasion(occasion_t *db, int idx)
{
	int i;
	occasion_rec_t *rec;

	if(idx < 0 || idx >= db->hdr.num_recs)
		return -1;

	for(i = 0, rec = db->head; rec && i != idx; i++, rec = rec->next);
	if(!rec){
		ERROR("DATA CORRUPTED: THIS SHOULD NOT HAPPEN!");
		return -1;
	}

	/* 2: record offset, 6: record header */
	db->hdr.db_size -= 2 + 6;

	free(rec->msg);
	db->hdr.db_size -= rec->msg_len;

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

	if(is_occasion_recurring(rec->type)){
		db->hdr.num_recur--;
		db->hdr.last_recur--;
	}else
		db->hdr.num_nonrecur--;
	db->hdr.last_nonrecur--;

	free(rec);

	db->hdr.num_recs--;
	db->hdr.alloc_size = ((db->hdr.db_size-1) / PAGE_SIZE + 1) * PAGE_SIZE;

	return 0;
}

int
find_occasion(occasion_t *db, occasion_data_t *data)
{
	int i, l;
	u8 *m, month, day, type;
	u16 year;
	occasion_rec_t *rec;

	l = strlen(data->msg);
	l = (l > MAX_MSG_LEN ? MAX_MSG_LEN : l);
	m = (u8 *)malloc(l+1);
	for(i = 0; i < l; i++)
		m[i] = rdmi(data->msg[i]);
	m[i] = DM_SENTINEL;

	type = data->type;
	year = bcd2(data->year);
	month = bcd(data->month);
	day = bcd(data->day);

	for(i = 0, rec = db->head; rec; i++, rec = rec->next){
		if(rec->type == type && rec->year == year &&
				rec->month == month && rec->day == day &&
				!memcmp(rec->msg, m, l+1))
			break;
	}

	free(m);

	return (rec ? i : -1);
}

void
read_occasion(occasion_t *db, u8 *data)
{
	int i, j;
	u16 offset;
	occasion_rec_t *rec;

	memcpy(&db->hdr, data, 15);

	db->head = db->tail = NULL;
	for(i = 0; i < db->hdr.num_recs; i++){
		rec = (occasion_rec_t *)malloc(sizeof(occasion_rec_t));
		memcpy(&offset, data+15+2*i, 2);

		offset += 15;
		memcpy(rec, data+offset, 6);

		rec->msg = (u8 *)malloc(rec->msg_len);
		memcpy(rec->msg, data+offset+6, rec->msg_len);
		/* TODO: Timex software bug?  Some character values are
		 * unpredictably greater than 0x69 and it should be &
		 * ~(1 << 7) to be compared with dl's index. */
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
read_occasion_mem(dldev_t *dev, occasion_t *db)
{
	int i, j;
	u16 add_addr, offset;
	occasion_rec_t *rec;

	/* find the application */
	if((i = find_app(dev, "OCCASION")) < 0){
		ERROR("OCCASION application not found");
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
	for(i = 0; i < db->hdr.num_recs; i++){
		rec = (occasion_rec_t *)malloc(sizeof(occasion_rec_t));
		if(read_abs_addr(dev, add_addr+15+2*i, ext_mem, (u8 *)&offset,
					2)){
			ERROR("read_abs_addr");
			return -1;
		}
		offset += add_addr + 15;
		if(read_abs_addr(dev, offset, ext_mem, (u8 *)rec, 6)){
			ERROR("read_abs_addr");
			return -1;
		}
		rec->msg = (u8 *)malloc(rec->msg_len);
		if(read_abs_addr(dev, offset+6, ext_mem, rec->msg,
					rec->msg_len)){
			ERROR("read_abs_addr");
			return -1;
		}
		/* TODO: Timex software bug?  Some character values are
		 * unpredictably greater than 0x69 and it should be &
		 * ~(1 << 7) to be compared with dl's index. */
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
print_occasion(occasion_t *db, FILE *fp)
{
	occasion_rec_t *rec;

	for(rec = db->head; rec; rec = rec->next){
		print_rdm(rec->msg, rec->msg_len, fp);
		fprintf(fp, "\t%d-%02d-%02d\t%s", debcd2(rec->year),
				debcd(rec->month), debcd(rec->day),
				get_occasion_str(rec->type));
		if(is_occasion_recurring(rec->type) &&
				!is_occasion_annual(rec->type))
			fprintf(fp, "\trecurring");
		fprintf(fp, "\n");
	}

	return;
}

void
init_occasion(occasion_t *db)
{
	db->hdr.db_size = 15;
	db->hdr.hdr_size = 10;
	db->hdr.num_recs = db->hdr.num_nonrecur = db->hdr.num_recur = 0;
	db->hdr.last_nonrecur = db->hdr.last_recur = -1;
	db->hdr.alloc_size = ((db->hdr.db_size-1) / PAGE_SIZE + 1) * PAGE_SIZE;
	db->head = db->tail = NULL;

	return;
}

void
free_occasion(occasion_t *db)
{
	occasion_rec_t *rec, *rec_next;

	for(rec = db->head; rec; rec = rec_next){
		rec_next = rec->next;
		free(rec->msg);
		free(rec);
	}

	return;
}

int
init_occasion_app(dldev_t *dev, u8 *data, u16 len)
{
	return init_int_app(dev, POR_OCCASION, data, len);
}

int
print_all_occasions(dldev_t *dev, char *file)
{
	FILE *fp;
	int i;
	u16 num_recs, add_addr, offset;
	occasion_rec_t rec;
	u8 msg[101];

	/* find the application */
	if((i = find_app(dev, "OCCASION")) < 0){
		ERROR("OCCASION application not found");
		return -1;
	}

	if(!(fp = fopen(file, "w"))){
		ERROR("%s: open failed", file);
		return -1;
	}

	/* application database data */
	add_addr = dev->app[i].acb.add_addr;
	/* number of records */
	if(read_abs_addr(dev, add_addr+2+2+1, ext_mem, (u8 *)&num_recs, 2)){
		ERROR("read_abs_addr");
		return -1;
	}

	for(i = 0; i < num_recs; i++){
		if(read_abs_addr(dev, add_addr+5+10+2*i, ext_mem, (u8 *)&offset,
					2)){
			ERROR("read_abs_addr");
			return -1;
		}
		offset += add_addr + 15;
		if(read_abs_addr(dev, offset, ext_mem, (u8 *)&rec, 6)){
			ERROR("read_abs_addr");
			return -1;
		}
		if(read_abs_addr(dev, offset+6, ext_mem, msg, rec.msg_len)){
			ERROR("read_abs_addr");
			return -1;
		}

		print_rdm(msg, rec.msg_len, fp);
		fprintf(fp, "\t%d-%02d-%02d\t%s", debcd2(rec.year),
				debcd(rec.month), debcd(rec.day),
				get_occasion_str(rec.type));
		if(is_occasion_recurring(rec.type) &&
				!is_occasion_annual(rec.type))
			fprintf(fp, "\trecurring");
		fprintf(fp, "\n");
	}

	fclose(fp);

	return 0;
}

int
read_occasion_line(occasion_data_t *data, char *line)
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

	for(j = ++i; i < l && line[i] != '\t'; i++);
	line[i] = 0;
	n = 5;
	for(k = 0; k < n && strcmp(line + j, occasion_type[k]); k++);
	if(k == n)
		return -1;
	data->type = get_occasion_type(k);
	if(i == l || is_occasion_annual(data->type))
		return 0;

	j = ++i;
	if(!strcmp(line + j, "recurring"))
		data->type = set_occasion_recurring(data->type);

	return 0;
}
