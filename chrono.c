/* $Id: chrono.c,v 1.13 2009/07/17 08:30:37 geni Exp $
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

static int unused_recs = UNUSED_RECS;
static u8 display_format = lap_split;

static void update_chrono(chrono_t *db);

void
set_chrono_unused_recs(int recs)
{
	unused_recs = (recs < MIN_CHRONO_RECS ? MIN_CHRONO_RECS :
			(recs > MAX_CHRONO_RECS ? MAX_CHRONO_RECS : recs));

	return;
}

void
set_chrono_display_format(u8 display)
{
	if(display < 4)
		display_format = display;

	return;
}

void
update_chrono_display_format(chrono_t *db)
{
	db->hdr.display_format = display_format;

	return;
}

void
create_chrono(chrono_t *db, u8 **data, u16 *len)
{
	int i;
	chrono_workout_t *workout;
	chrono_split_t *split;

	update_chrono(db);

	*len = db->hdr.alloc_size;
	*data = (u8 *)malloc(*len);
	memcpy(*data, &db->hdr, 17);

	for(workout = db->head, i = 0; workout; workout = workout->next){
		memcpy(*data+17+5*(i++), workout, 5);
		for(split = workout->head; split; split = split->next)
			memcpy(*data+17+5*(i++), split, 5);
	}
	if(i < db->hdr.num_recs)
		memset(*data+17+5*i, 0, (db->hdr.num_recs - i) * 5);

	return;
}

int
add_chrono_file(chrono_t *db, FILE *fp)
{
	int ret = -1;
	char *line;
	chrono_data_t data;

	while((line = read_line(fp))){
		if(read_chrono_line(&data, line)){
			int i;

			for(i = 0; i < 4 &&
				strcmp(line, chrono_display_format[i]); i++);
			if(i < 4){
				set_chrono_display_format(i);
				update_chrono_display_format(db);
			}
			free(line);
			continue;
		}
		if(find_chrono(db, &data) < 0){
			add_chrono(db, &data);
			ret = 0;
		}
		free(line);
	}

	return ret;
}

int
del_chrono_file(chrono_t *db, FILE *fp)
{
	int ret = -1, i;
	char *line;
	chrono_data_t data;

	while((line = read_line(fp))){
		if(read_chrono_line(&data, line)){
			free(line);
			continue;
		}
		if((i = find_chrono(db, &data)) >= 0 && !del_chrono(db, i))
			ret = 0;
		free(line);
	}

	return ret;
}

void
add_chrono(chrono_t *db, chrono_data_t *data)
{
	int i, lap, prev_split, best_lap, best_lap_id;
	chrono_workout_t *workout;
	chrono_split_t *split;

	/* TODO: sort? */

	for(workout = db->head, i = 1; workout && i != data->workout_id;
			workout = workout->next, i++);
	if(!workout){
		workout = (chrono_workout_t *)zalloc(sizeof(chrono_workout_t));

		workout->year = bcd(data->year % 100);
		workout->month = bcd(data->month);
		workout->day = bcd(data->day);
		workout->dow = data->dow;
		workout->best_lap_id = workout->num_laps = 0;

		workout->head = workout->tail = NULL;

		workout->prev = db->tail;
		if(workout->prev)
			workout->prev->next = workout;
		else
			db->head = workout;
		workout->next = NULL;
		db->tail = workout;

		db->hdr.last_workout = db->hdr.used_recs;
		db->hdr.used_recs++;
	}

	split = (chrono_split_t *)malloc(sizeof(chrono_split_t));

	split->lap_id = workout->num_laps + 1;
	split->hours = bcd(data->hours);
	split->minutes = bcd(data->minutes);
	split->seconds = bcd(data->seconds);
	split->hundredths = bcd(data->hundredths);

	split->prev = workout->tail;
	if(split->prev)
		split->prev->next = split;
	else
		workout->head = split;
	split->next = NULL;
	workout->tail = split;

	prev_split = best_lap = 0;
	best_lap_id = 0;
	for(split = workout->head, i = 1; split; split = split->next, i++){
		lap = debcd(split->hours) * 360000 +
			debcd(split->minutes) * 6000 +
			debcd(split->seconds) * 100 +
			debcd(split->hundredths) - prev_split;
		if(!best_lap_id || lap < best_lap){
			best_lap = lap;
			best_lap_id = i;
		}
		prev_split += lap;
	}
	workout->best_lap_id = best_lap_id;

	workout->num_laps++;
	db->hdr.used_recs++;

	return;
}

int
del_chrono(chrono_t *db, int idx)
{
	int i;
	chrono_workout_t *workout;
	chrono_split_t *split;

	for(workout = db->head, i = 0; workout; workout = workout->next){
		for(split = workout->head; split && i != idx;
				split = split->next, i++);
		if(split)
			break;
	}
	if(!workout || !split){
		if(db->head)
			ERROR("DATA CORRUPTED: THIS SHOULD NOT HAPPEN!");
		return -1;
	}

	if(split == workout->head){
		workout->head = split->next;
		if(workout->head)
			workout->head->prev = NULL;
	}else
		split->prev->next = split->next;
	if(split == workout->tail){
		workout->tail = split->prev;
		if(workout->tail)
			workout->tail->next = NULL;
	}else
		split->next->prev = split->prev;

	free(split);

	workout->num_laps--;

	if(workout->num_laps){
		int lap, prev_split, best_lap, best_lap_id;

		for(split = split->next; split; split = split->next)
			split->lap_id--;

		prev_split = best_lap = 0;
		best_lap_id = 0;
		for(split = workout->head, i = 1; split; split = split->next,
				i++){
			lap = debcd(split->hours) * 360000 +
				debcd(split->minutes) * 6000 +
				debcd(split->seconds) * 100 +
				debcd(split->hundredths) - prev_split;
			if(!best_lap_id || lap < best_lap){
				best_lap = lap;
				best_lap_id = i;
			}
			prev_split += lap;
		}
		workout->best_lap_id = best_lap_id;
	}else{
		if(workout == db->head){
			db->head = workout->next;
			if(db->head)
				db->head->prev = NULL;
		}else
			workout->prev->next = workout->next;
		if(workout == db->tail){
			db->tail = workout->prev;
			if(db->tail)
				db->tail->next = NULL;
		}else
			workout->next->prev = workout->prev;

		db->hdr.last_workout -= 1 + workout->num_laps;
		free(workout);

		db->hdr.used_recs--;
	}

	db->hdr.used_recs--;

	return 0;
}

int
find_chrono(chrono_t *db, chrono_data_t *data)
{
	int i, j;
	chrono_workout_t *workout;
	chrono_split_t *split;

	for(workout = db->head, i = 1, j = 0; workout && i != data->workout_id;
			i++, j += workout->num_laps, workout = workout->next);
	if(!workout)
		return -1;

	for(split = workout->head; split && split->lap_id != data->lap_id;
			split = split->next, j++);

	return (split ? j : -1);
}

void
read_chrono(chrono_t *db, u8 *data)
{
	int i, j;
	chrono_workout_t *workout;
	chrono_split_t *split;

	memcpy(&db->hdr, data, 17);

	db->head = db->tail = NULL;
	for(i = 0; i < db->hdr.used_recs; ){
		workout = (chrono_workout_t *)malloc(sizeof(chrono_workout_t));
		memcpy(workout, data+17+5*i, 5);

		workout->head = workout->tail = NULL;
		for(j = 0, i++; j < workout->num_laps; j++, i++){
			split = (chrono_split_t *)
				malloc(sizeof(chrono_split_t));
			memcpy(split, data+17+5*i, 5);

			split->prev = split->next = NULL;
			if(workout->head){
				split->prev = workout->tail;
				workout->tail->next = split;
				workout->tail = split;
			}else
				workout->head = workout->tail = split;
		}

		workout->prev = workout->next = NULL;
		if(db->head){
			workout->prev = db->tail;
			db->tail->next = workout;
			db->tail = workout;
		}else
			db->head = db->tail = workout;
	}

	return;
}

/* too slow */
int
read_chrono_mem(dldev_t *dev, chrono_t *db)
{
	int i, j;
	u16 add_addr;
	chrono_workout_t *workout;
	chrono_split_t *split;

	/* find the application */
	if((i = find_app(dev, "CHRONO")) < 0){
		ERROR("CHRONO application not found");
		return -1;
	}

	/* application database data */
	add_addr = dev->app[i].acb.add_addr;
	/* allocation size */
	if(read_abs_addr(dev, add_addr, ext_mem, (u8 *)&db->hdr, 17)){
		ERROR("read_abs_addr");
		return -1;
	}

	db->head = db->tail = NULL;
	for(i = 0; i < db->hdr.used_recs; ){
		workout = (chrono_workout_t *)malloc(sizeof(chrono_workout_t));
		if(read_abs_addr(dev, add_addr+17+5*i, ext_mem, (u8 *)workout,
					5)){
			ERROR("read_abs_addr");
			return -1;
		}

		workout->head = workout->tail = NULL;
		for(j = 0, i++; j < workout->num_laps; j++, i++){
			split = (chrono_split_t *)
				malloc(sizeof(chrono_split_t));
			if(read_abs_addr(dev, add_addr+17+5*i, ext_mem,
						(u8 *)split, 5)){
				ERROR("read_abs_addr");
				return -1;
			}

			split->prev = split->next = NULL;
			if(workout->head){
				split->prev = workout->tail;
				workout->tail->next = split;
				workout->tail = split;
			}else
				workout->head = workout->tail = split;
		}

		workout->prev = workout->next = NULL;
		if(db->head){
			workout->prev = db->tail;
			db->tail->next = workout;
			db->tail = workout;
		}else
			db->head = db->tail = workout;
	}

	return 0;
}

void
print_chrono(chrono_t *db, FILE *fp)
{
	int i;
	chrono_workout_t *workout;
	chrono_split_t *split;

	fprintf(fp, "%s\n", chrono_display_format[db->hdr.display_format]);
	for(workout = db->head, i = 1; workout; workout = workout->next, i++){
		for(split = workout->head; split; split = split->next){
			fprintf(fp, "%d\t%d\t%d-%02d-%02d %s\t"
					"%02d:%02d:%02d.%02d",
					i, split->lap_id,
					2000+debcd(workout->year),
					debcd(workout->month),
					debcd(workout->day),
					get_chrono_dow_str(workout->dow),
					debcd(split->hours),
					debcd(split->minutes),
					debcd(split->seconds),
					debcd(split->hundredths));
			if(split->lap_id == workout->best_lap_id)
				fprintf(fp, "\tbest lap");
			fprintf(fp, "\n");
		}
	}

	return;
}

void
init_chrono(chrono_t *db)
{
	db->hdr.db_size = 17 + unused_recs * 5;
	db->hdr.hdr_size = 12;
	db->hdr.num_recs = unused_recs;
	db->hdr.rec_size = 5;
	db->hdr.display_format = display_format;
	db->hdr.system_flag = 0; /* TODO */
	db->hdr.lap_count = 1;
	db->hdr.unused_recs = db->hdr.num_recs;
	db->hdr.max_recs = db->hdr.num_recs;
	db->hdr.used_recs = 0;
	db->hdr.last_workout = 0;
	db->hdr.stored_rec = db->hdr.used_recs;
	db->hdr.reserved_16 = 0;
	db->hdr.alloc_size = ((db->hdr.db_size-1) / PAGE_SIZE + 1) * PAGE_SIZE;
	db->head = db->tail = NULL;

	return;
}

void
free_chrono(chrono_t *db)
{
	chrono_workout_t *workout, *workout_next;
	chrono_split_t *split, *split_next;

	for(workout = db->head; workout; workout = workout_next){
		workout_next = workout->next;
		for(split = workout->head; split; split = split_next){
			split_next = split->next;
			free(split);
		}
		free(workout);
	}

	return;
}

int
init_chrono_app(dldev_t *dev, u8 *data, u16 len)
{
	return init_int_app(dev, POR_CHRONO, data, len);
}

int
print_all_chronos(dldev_t *dev, char *file)
{
	FILE *fp;
	int i, j, k;
	u16 add_addr;
	chrono_split_t split;
	chrono_workout_t workout;
	u8 display, used_recs;

	/* find the application */
	if((i = find_app(dev, "CHRONO")) < 0){
		ERROR("CHRONO application not found");
		return -1;
	}

	if(!(fp = fopen(file, "w"))){
		ERROR("%s: open failed", file);
		return -1;
	}

	/* application database data */
	add_addr = dev->app[i].acb.add_addr;
	if(read_abs_addr(dev, add_addr+8, ext_mem, (u8 *)&display, 1)){
		ERROR("read_abs_addr");
		return -1;
	}
	fprintf(fp, "%s\n", chrono_display_format[display]);

	/* number of records */
	if(read_abs_addr(dev, add_addr+13, ext_mem, (u8 *)&used_recs, 1)){
		ERROR("read_abs_addr");
		return -1;
	}

	for(i = 0, j = 1; i < used_recs; j++){
		if(read_abs_addr(dev, add_addr+17+5*i, ext_mem, (u8 *)&workout,
					5)){
			ERROR("read_abs_addr");
			return -1;
		}
		for(k = 0, i++; k < workout.num_laps; k++, i++){
			if(read_abs_addr(dev, add_addr+17+5*i, ext_mem,
						(u8 *)&split, 5)){
				ERROR("read_abs_addr");
				return -1;
			}
			fprintf(fp, "%d\t%d\t%d-%02d-%02d %s\t"
					"%02d:%02d:%02d.%02d",
					j, split.lap_id,
					2000+debcd(workout.year),
					debcd(workout.month),
					debcd(workout.day),
					get_chrono_dow_str(workout.dow),
					debcd(split.hours),
					debcd(split.minutes),
					debcd(split.seconds),
					debcd(split.hundredths));
			if(split.lap_id == workout.best_lap_id)
				fprintf(fp, "\tbest lap");
			fprintf(fp, "\n");
		}
	}

	fclose(fp);

	return 0;
}

int
read_chrono_line(chrono_data_t *data, char *line)
{
	int i, j, l;
	char t;

	l = strlen(line);
	for(i = 0; i < l && line[i] != '\t'; i++);
	if(i == l)
		return -1;
	line[i] = 0;
	data->workout_id = atoi(line);

	for(j = ++i; i < l && line[i] != '\t'; i++);
	if(i == l)
		return -1;
	line[i] = 0;
	data->lap_id = atoi(line + j);

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
	t = line[i];
	line[(i > j+2 ? j+2 : i)] = 0;
	data->day = atoi(line + j);

	if(t != '\t'){
		for(j = ++i; i < l && line[i] != '\t'; i++);
		if(i == l)
			return -1;
		line[i] = 0;
	}
	data->dow = day_of_week(data->year, data->month, data->day);

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

	for(j = ++i; i < l && isdigit(line[i]); i++);
	if(i == l)
		return -1;
	line[(i > j+2 ? j+2 : i)] = 0;
	data->seconds = atoi(line + j);

	j = ++i;
	line[(l > j+2 ? j+2 : l)] = 0;
	data->hundredths = atoi(line + j);

	return 0;
}

static void
update_chrono(chrono_t *db)
{
	unused_recs = (db->hdr.used_recs + unused_recs > MAX_CHRONO_RECS ?
			MAX_CHRONO_RECS - db->hdr.used_recs : unused_recs);
	db->hdr.num_recs = db->hdr.used_recs + unused_recs;
	db->hdr.unused_recs = unused_recs;
	db->hdr.max_recs = db->hdr.num_recs;
	db->hdr.last_workout = (db->tail ?
			db->hdr.used_recs - db->tail->num_laps - 1 : 0);
	db->hdr.stored_rec = db->hdr.used_recs;

	db->hdr.db_size = 17 + db->hdr.num_recs * 5;
	db->hdr.alloc_size = ((db->hdr.db_size-1) / PAGE_SIZE + 1) * PAGE_SIZE;

	return;
}
