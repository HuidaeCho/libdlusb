/* $Id: note.c,v 1.39 2009/05/12 05:44:00 geni Exp $
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
#include "define.h"

static int unused_recs = UNUSED_RECS, extra_msg_len = EXTRA_MSG_LEN;

static int cmp_note(note_rec_t *a, note_rec_t *b);
static void sort_note(note_t *db);
static void qsort_note(note_t *db, note_rec_t *head, note_rec_t *tail);
static void update_note(note_t *db);

/* TODO: rec->status */

void
set_note_unused_recs(int recs)
{
	unused_recs = (recs >= 0 ? recs : 0);

	return;
}

void
set_note_extra_len(int len)
{
	extra_msg_len = (len < MIN_MSG_LEN ? MIN_MSG_LEN : len);

	return;
}

void
create_note(note_t *db, u8 **data, u16 *len)
{
	int i;
	u8 *m;
	u16 offset, prev_addr, next_addr;
	note_rec_t *rec;

	update_note(db);

	*len = db->hdr.alloc_size;
	*data = (u8 *)malloc(*len);
	memcpy(*data, &db->hdr, 15);

	offset = db->hdr.head;
	for(rec = db->head; rec; rec = rec->next){
		memcpy(*data+offset, rec, 8);
		memcpy(*data+offset+8, rec->msg, rec->msg_len);
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
		next_addr = offset + 39;
		memset(*data+offset, 0, 8);
		memcpy(*data+offset, &next_addr, 2);	/* next_addr */
		memcpy(*data+offset+2, &prev_addr, 2);	/* prev_addr */
		*(*data+offset+4) = 31;			/* msg_len */
		memcpy(*data+offset+8, m, 31);
		prev_addr = offset;
		offset = next_addr;
	}

	next_addr = 0;
	memcpy(*data+prev_addr, &next_addr, 2);		/* tail's next_addr */

	free(m);

	return;
}

int
add_note_file(note_t *db, FILE *fp)
{
	int ret = -1, i;
	char *line, *msg;

	while((line = read_line(fp))){
		i = strlen(line);
		line[(i > MAX_MSG_LEN ? MAX_MSG_LEN : i)] = 0;
		/* Unlike Contact, do not squeeze spaces. It's Note! Anything
		 * should be possible. */
		msg = line;

		if(find_note(db, msg) < 0){
			add_note(db, msg);
			ret = 0;
		}

		free(line);
	}

	return ret;
}

int
del_note_file(note_t *db, FILE *fp)
{
	int ret = -1, i;
	char *line, *msg;

	while((line = read_line(fp))){
		i = strlen(line);
		line[(i > MAX_MSG_LEN ? MAX_MSG_LEN : i)] = 0;
		msg = line;

		if((i = find_note(db, msg)) >= 0 && !del_note(db, i))
			ret = 0;

		free(line);
	}

	return ret;
}

void
add_note(note_t *db, char *msg)
{
	int i;
	note_rec_t *rec, *r;

	/* use zalloc() to initialize reserved bits */
	rec = (note_rec_t *)zalloc(sizeof(note_rec_t));
	i = strlen(msg);
	i = (i > MAX_MSG_LEN ? MAX_MSG_LEN : i);
	rec->msg_len = i + 1;
	rec->msg = (u8 *)malloc(rec->msg_len);

	rec->msg[i] = DM_SENTINEL;
	for(i--; i >= 0; i--)
		rec->msg[i] = rdmi(msg[i]);

	rec->used = 1;
	/* rec->modified = 1; doesn't work with the Timex PIM */
	rec->modified = 0;

	/* sort */
	for(r = db->head; r && cmp_note(rec, r) >= 0; r = r->next);
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
del_note(note_t *db, int idx)
{
	int i;
	note_rec_t *rec;

	if(idx < 0 || idx >= db->hdr.num_recs - db->hdr.unused_recs)
		return -1;

	for(rec = db->head, i = 0; rec && i != idx; rec = rec->next, i++);
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
find_note(note_t *db, char *msg)
{
	int i, l;
	u8 *m;
	note_rec_t *rec;

	l = strlen(msg);
	l = (l > MAX_MSG_LEN ? MAX_MSG_LEN : l);
	m = (u8 *)malloc(l+1);
	for(i = 0; i < l; i++)
		m[i] = rdmi(msg[i]);
	m[i] = DM_SENTINEL;

	for(i = 0, rec = db->head; rec; i++, rec = rec->next){
		if(!memcmp(rec->msg, m, l+1))
			break;
	}

	free(m);

	return (rec ? i : -1);
}

/* dump_add()/read_note() is usually slower than read_note_mem() because
 * dump_add() downloads even allocated unused memory.  In contrast, Contact
 * does not have any unused memory. */
void
read_note(note_t *db, u8 *data)
{
	int j;
	u16 ptr;
	note_rec_t *rec;

	memcpy(&db->hdr, data, 15);

	db->head = db->tail = NULL;
	for(ptr = db->hdr.head; ptr; ptr = rec->next_addr){
		rec = (note_rec_t *)malloc(sizeof(note_rec_t));
		memcpy(rec, data+ptr, 8);

		rec->msg = (u8 *)malloc(rec->msg_len);
		memcpy(rec->msg, data+ptr+8, rec->msg_len);
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
read_note_mem(dldev_t *dev, note_t *db)
{
	int i, j;
	u16 add_addr, ptr;
	note_rec_t *rec;

	/* find the application */
	if((i = find_app(dev, "NOTE")) < 0){
		ERROR("NOTE application not found");
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
		rec = (note_rec_t *)malloc(sizeof(note_rec_t));
		if(read_abs_addr(dev, add_addr+ptr, ext_mem, (u8 *)rec, 8)){
			ERROR("read_abs_addr");
			return -1;
		}
		rec->msg = (u8 *)malloc(rec->msg_len);
		if(read_abs_addr(dev, add_addr+ptr+8, ext_mem, (u8 *)rec->msg,
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
print_note(note_t *db, FILE *fp)
{
	note_rec_t *rec;

	for(rec = db->head; rec; rec = rec->next){
		print_rdm(rec->msg, rec->msg_len, fp);
		fprintf(fp, "\n");
	}

	return;
}

void
init_note(note_t *db)
{
	db->hdr.db_size = 15 + 39 * db->hdr.unused_recs;
	db->hdr.hdr_size = 10;
	db->hdr.num_recs = db->hdr.unused_recs;
	db->hdr.head = db->hdr.tail = 0;
	db->hdr.unused_head = 15;
	db->hdr.alloc_size = ((db->hdr.db_size-1) / PAGE_SIZE + 1) * PAGE_SIZE;
	db->head = db->tail = NULL;

	return;
}

void
free_note(note_t *db)
{
	note_rec_t *rec, *rec_next;

	for(rec = db->head; rec; rec = rec_next){
		rec_next = rec->next;
		free(rec->msg);
		free(rec);
	}

	return;
}

int
init_note_app(dldev_t *dev, u8 *data, u16 len)
{
	return init_int_app(dev, POR_NOTE, data, len);
}

int
print_all_notes(dldev_t *dev, char *file)
{
	FILE *fp;
	int i;
	u16 add_addr, head, tail, ptr;
	note_rec_t rec;
	u8 msg[101];

	/* find the application */
	if((i = find_app(dev, "NOTE")) < 0){
		ERROR("NOTE application not found");
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
		if(read_abs_addr(dev, add_addr+ptr, ext_mem, (u8 *)&rec, 8)){
			ERROR("read_abs_addr");
			return -1;
		}
		if(read_abs_addr(dev, add_addr+ptr+8, ext_mem, msg,
					rec.msg_len)){
			ERROR("read_abs_addr");
			return -1;
		}

		print_rdm(msg, rec.msg_len, fp);
		fprintf(fp, "\n");
	}

	fclose(fp);

	return 0;
}

static int
cmp_note(note_rec_t *a, note_rec_t *b)
{
	int c, l;

	/* DM_SENTINEL is ignored when comparing strings */
	l = (a->msg_len < b->msg_len ? a->msg_len : b->msg_len) - 1;
	c = memcmp(a->msg, b->msg, l);

	/* a < b: a - b < 0 */
	if(c < 0 || (!c && a->msg_len < b->msg_len))
		return -1;

	/* a > b: a - b > 0 */
	if(c || a->msg_len != b->msg_len)
		return 1;

	return 0;
}

static void
sort_note(note_t *db)
{
	qsort_note(db, db->head, db->tail);

	return;
}

DEFINE_QSORT(note)

static void
update_note(note_t *db)
{
	int i, j, k;
	u16 prev_addr, next_addr;
	note_rec_t *rec;

	sort_note(db);

	db->hdr.num_recs += unused_recs - db->hdr.unused_recs;
	db->hdr.unused_recs = unused_recs;

	next_addr = 15;
	db->hdr.head = (db->head ? next_addr : 0);
	prev_addr = 0;
	for(i = 0, rec = db->head; rec; i++, rec = rec->next){
		rec->id = i;
		rec->prev_addr = prev_addr;
		prev_addr = next_addr;
		/* data entered using the watch has a length of 39 bytes per
		 * record: 8 (header) + 30 (message) + 1 (sentinel). but, when
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
		next_addr += 8 + rec->msg_len;
		rec->next_addr = next_addr;
	}
	if(db->tail)
		db->tail->next_addr = 0;
	db->hdr.tail = prev_addr;
	db->hdr.unused_head = (db->hdr.unused_recs ? next_addr : 0);

	db->hdr.db_size = next_addr + 39 * db->hdr.unused_recs;
	db->hdr.alloc_size = ((db->hdr.db_size-1) / PAGE_SIZE + 1) * PAGE_SIZE;

	return;
}
