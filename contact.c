/* $Id: contact.c,v 1.37 2005/10/06 19:21:22 geni Exp $
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
#include "dlusb.h"

void
create_contact(contact_t *db, u8 **data, u16 *len)
{
	int i;
	u16 offset;
	contact_rec_t *rec;

	*len = db->hdr.alloc_size;
	*data = (u8 *)malloc(*len);
	memcpy(*data, &db->hdr, 7);

	offset = 2 * db->hdr.num_recs;
	for(i = 0, rec = db->head; rec; i++, rec = rec->next){
		memcpy(*data+7+2*i, &offset, 2);
		memcpy(*data+7+offset, rec, 11);
		offset += 11;
		if(!rec->msg_offset){
			memcpy(*data+7+offset, rec->msg, rec->msg_len);
			offset += rec->msg_len;
		}
	}

	return;
}

int
add_contact_file(contact_t *db, FILE *fp)
{
	int ret = -1;
	char *line;
	contact_data_t data;

	while((line = read_line(fp))){
		if(read_contact_line(&data, line)){
			free(line);
			continue;
		}
		if(find_contact(db, &data) < 0){
			add_contact(db, &data);
			ret = 0;
		}
		free(line);
	}

	return ret;
}

int
del_contact_file(contact_t *db, FILE *fp)
{
	int ret = -1, i;
	char *line;
	contact_data_t data;

	while((line = read_line(fp))){
		if(read_contact_line(&data, line)){
			free(line);
			continue;
		}
		if((i = find_contact(db, &data)) >= 0 && !del_contact(db, i))
			ret = 0;
		free(line);
	}

	return ret;
}

void
add_contact(contact_t *db, contact_data_t *data)
{
	int i, l;
	u8 *m;
	contact_rec_t *rec, *r;
	char buf[9];

	/* 2: record offset, 11: record header */
	db->hdr.db_size += 2 + 11;

	rec = (contact_rec_t *)malloc(sizeof(contact_rec_t));

	/* TODO: long phone number extending to the next or previous record */
	rec->msg_offset = 0;
	l = strlen(data->msg);
	l = (l > MAX_MSG_LEN ? MAX_MSG_LEN : l);
	rec->msg_len = l + 1;

	m = (u8 *)malloc(rec->msg_len);
	for(i = 0; i < l; i++)
		m[i] = rdmi(data->msg[i]);
	m[i] = DM_SENTINEL;

	rec->type = word(rdmi(data->type[1]), rdmi(data->type[0]));

	snprintf(buf, 7, "%6s", data->area);
	for(i = 0; i < 3; i++)
		rec->phone[i] = (seg16i(buf[2*i]) << 4) | seg16i(buf[2*i+1]);

	snprintf(buf, 9, "%8s", data->number);
	for(i = 0; i < 4; i++)
		rec->phone[i+3] = (rdm16i(buf[2*i]) << 4) | rdm16i(buf[2*i+1]);

	for(r = db->head; r; r = r->next){
		if(!r->msg_offset){
			/* faster?
			for(i = 0; i < r->msg_len && m[i] == r->msg[i]; i++);
			if(i == r->msg_len)
				break;
			*/
			if(r->msg_len == l && !memcmp(r->msg, m, l))
				break;
		}
	}
	if(r){
		for(i = 1, r = r->next; r && r->msg_offset; i++, r = r->next);
		rec->msg_offset = i;
		rec->msg = NULL;
		free(m);
	}else{
		/* sort */
		int c, l;

		db->hdr.db_size += rec->msg_len;

		rec->msg = m;
		for(r = db->head; r; r = r->next){
			if(!r->msg)
				continue;
			/* DM_SENTINEL is ignored when comparing strings */
			l = (rec->msg_len<r->msg_len?rec->msg_len:r->msg_len)-1;
			c = memcmp(rec->msg, r->msg, l);
			if((!c && rec->msg_len < r->msg_len) || c < 0)
				break;
			if(c > 0)
				continue;
			c = rec->type - r->type;
			if(c < 0)
				break;
			if(c > 0)
				continue;
			c = memcmp(rec->phone, r->phone, 7);
			if(c < 0)
				break;
		}
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
del_contact(contact_t *db, int idx)
{
	int i;
	contact_rec_t *rec, *r;

	if(idx < 0 || idx >= db->hdr.num_recs)
		return -1;

	for(rec = db->head, i = 0; rec && i != idx; rec = rec->next, i++);
	if(!rec){
		ERROR("DATA CORRUPTED: THIS SHOULD NOT HAPPEN!");
		return -1;
	}

	/* 2: record offset, 11: record header */
	db->hdr.db_size -= 2 + 11;

	if(!rec->msg_offset){
		if(rec->next && rec->next->msg_offset){
			rec->next->msg = (u8 *)malloc(rec->msg_len);
			memcpy(rec->next->msg, rec->msg, rec->msg_len);
		}
		free(rec->msg);
		db->hdr.db_size -= rec->msg_len;
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

	for(r = rec->next; r && r->msg_offset; r = r->next)
		r->msg_offset--;

	free(rec);

	db->hdr.num_recs--;
	db->hdr.alloc_size = ((db->hdr.db_size-1) / PAGE_SIZE + 1) * PAGE_SIZE;

	return 0;
}

int
find_contact(contact_t *db, contact_data_t *data)
{
	int i, l;
	u8 *m, *msg = NULL, phone[7];
	u16 type;
	contact_rec_t *rec;
	char buf[9];

	l = strlen(data->msg);
	l = (l > MAX_MSG_LEN ? MAX_MSG_LEN : l);
	m = (u8 *)malloc(l+1);
	for(i = 0; i < l; i++)
		m[i] = rdmi(data->msg[i]);
	m[i] = DM_SENTINEL;

	type = word(rdmi(data->type[1]), rdmi(data->type[0]));

	snprintf(buf, 7, "%6s", data->area);
	for(i = 0; i < 3; i++)
		phone[i] = (seg16i(buf[2*i]) << 4) | seg16i(buf[2*i+1]);

	snprintf(buf, 9, "%8s", data->number);
	for(i = 0; i < 4; i++)
		phone[i+3] = (rdm16i(buf[2*i]) << 4) | rdm16i(buf[2*i+1]);

	for(i = 0, rec = db->head; rec; i++, rec = rec->next){
		if(!rec->msg_offset)
			msg = rec->msg;
		if(rec->type == type && !memcmp(rec->phone, phone, 7) &&
				!memcmp(msg, m, l+1))
			break;
	}

	free(m);

	return (rec ? i : -1);
}

void
read_contact(contact_t *db, u8 *data)
{
	int i, j;
	u16 offset;
	contact_rec_t *rec;

	memcpy(&db->hdr, data, 7);

	db->head = db->tail = NULL;
	for(i = 0; i < db->hdr.num_recs; i++){
		rec = (contact_rec_t *)malloc(sizeof(contact_rec_t));
		memcpy(&offset, data+7+2*i, 2);

		offset += 7;
		memcpy(rec, data+offset, 11);

		if(rec->msg_offset)
			rec->msg = NULL;
		else{
			rec->msg = (u8 *)malloc(rec->msg_len);
			memcpy(rec->msg, data+offset+11, rec->msg_len);
			/* TODO: Timex software bug?  Some character values are
			 * unpredictably greater than 0x69 and it should be &
			 * ~(1 << 7) to be compared with dl's index. */
			for(j = 0; j < rec->msg_len &&
					rec->msg[j] != DM_SENTINEL; j++)
				rec->msg[j] &= ~(1 << 7);
			/* recalculate the length of the message */
			rec->msg_len = j + 1;
		}

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
read_contact_mem(dldev_t *dev, contact_t *db)
{
	int i, j;
	u16 add_addr, offset;
	contact_rec_t *rec;

	/* find the application */
	if((i = find_app(dev, "CONTACT")) < 0){
		ERROR("CONTACT application not found");
		return -1;
	}

	/* application database data */
	add_addr = dev->app[i].acb.add_addr;
	/* allocation size */
	if(read_abs_addr(dev, add_addr, ext_mem, (u8 *)&db->hdr, 7)){
		ERROR("read_abs_addr");
		return -1;
	}

	db->head = db->tail = NULL;
	for(i = 0; i < db->hdr.num_recs; i++){
		rec = (contact_rec_t *)malloc(sizeof(contact_rec_t));
		if(read_abs_addr(dev, add_addr+7+2*i, ext_mem, (u8 *)&offset,
					2)){
			ERROR("read_abs_addr");
			return -1;
		}
		offset += add_addr + 7;
		if(read_abs_addr(dev, offset, ext_mem, (u8 *)rec, 11)){
			ERROR("read_abs_addr");
			return -1;
		}
		if(rec->msg_offset)
			rec->msg = NULL;
		else{
			rec->msg = (u8 *)malloc(rec->msg_len);
			if(read_abs_addr(dev, offset+11, ext_mem, rec->msg,
						rec->msg_len)){
				ERROR("read_abs_addr");
				return -1;
			}
			/* TODO: Timex software bug?  Some character values are
			 * unpredictably greater than 0x69 and it should be &
			 * ~(1 << 7) to be compared with dl's index. */
			for(j = 0; j < rec->msg_len &&
					rec->msg[j] != DM_SENTINEL; j++)
				rec->msg[j] &= ~(1 << 7);
			/* recalculate the length of the message */
			rec->msg_len = j + 1;
		}
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
print_contact(contact_t *db, FILE *fp)
{
	int i;
	contact_rec_t *rec, *r = NULL;

	for(rec = db->head; rec; rec = rec->next){
		if(!rec->msg_offset)
			r = rec;
		print_rdm(r->msg, r->msg_len, fp);
		fprintf(fp, "\t%c%c\t", rdm(lobyte(rec->type)),
			rdm(hibyte(rec->type)));
		for(i = 0; i < 3; i++)
			fprintf(fp, "%c%c", seg16(hibits(rec->phone[i])),
					seg16(lobits(rec->phone[i])));
		fprintf(fp, "\t");
		for(; i < 7; i++)
			fprintf(fp, "%c%c", rdm16(hibits(rec->phone[i])),
					rdm16(lobits(rec->phone[i])));
		fprintf(fp, "\n");
	}

	return;
}

void
init_contact(contact_t *db)
{
	db->hdr.db_size = 7;
	db->hdr.hdr_size = 2;
	db->hdr.num_recs = 0;
	db->hdr.alloc_size = ((db->hdr.db_size-1) / PAGE_SIZE + 1) * PAGE_SIZE;
	db->head = db->tail = NULL;

	return;
}

void
free_contact(contact_t *db)
{
	contact_rec_t *rec, *rec_next;

	for(rec = db->head; rec; rec = rec_next){
		rec_next = rec->next;
		if(rec->msg)
			free(rec->msg);
		free(rec);
	}

	return;
}

int
init_contact_app(dldev_t *dev, u8 *data, u16 len)
{
	return init_int_app(dev, POR_CONTACT, data, len);
}

int
print_all_contacts(dldev_t *dev, char *file)
{
	FILE *fp;
	int i, j;
	u16 num_recs, add_addr, offset;
	contact_rec_t rec;
	u8 msg[101];

	/* find the application */
	if((i = find_app(dev, "CONTACT")) < 0){
		ERROR("CONTACT application not found");
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
		if(read_abs_addr(dev, add_addr+5+2+2*i, ext_mem, (u8 *)&offset,
					2)){
			ERROR("read_abs_addr");
			return -1;
		}
		offset += add_addr + 7;
		if(read_abs_addr(dev, offset, ext_mem, (u8 *)&rec, 11)){
			ERROR("read_abs_addr");
			return -1;
		}
		if(!rec.msg_offset){
			if(read_abs_addr(dev, offset+11, ext_mem, msg,
						rec.msg_len)){
				ERROR("read_abs_addr");
				return -1;
			}
		}

		print_rdm(msg, rec.msg_len, fp);
		fprintf(fp, "\t");
		fprintf(fp, "%c%c\t", rdm(lobyte(rec.type)),
				rdm(hibyte(rec.type)));
		for(j = 0; j < 3; j++)
			fprintf(fp, "%c%c", seg16(hibits(rec.phone[j])),
					seg16(lobits(rec.phone[j])));
		fprintf(fp, "\t");
		for(; j < 7; j++)
			fprintf(fp, "%c%c", rdm16(hibits(rec.phone[j])),
					rdm16(lobits(rec.phone[j])));
		fprintf(fp, "\n");
	}

	fclose(fp);

	return 0;
}

int
read_contact_line(contact_data_t *data, char *line)
{
	int i, j, l;

	l = strlen(line);
	for(i = 0; i < l && line[i] != '\t'; i++);
	if(i == l)
		return -1;
	line[(i > MAX_MSG_LEN ? MAX_MSG_LEN : i)] = 0;
	/*
	data->msg = squeeze_spaces(line);
	*/
	data->msg = line;

	for(j = ++i; i < l && line[i] != '\t'; i++);
	if(i == l)
		return -1;
	line[(i > j+2 ? j+2 : i)] = 0;
	data->type = squeeze_spaces(line + j);

	for(j = ++i; i < l && line[i] != '\t'; i++);
	if(i == l)
		return -1;
	line[(i > j+6 ? j+6 : i)] = 0;
	data->area = squeeze_spaces(line + j);

	j = ++i;
	line[(l > j+8 ? j+8 : l)] = 0;
	data->number = squeeze_spaces(line + j);

	return 0;
}
