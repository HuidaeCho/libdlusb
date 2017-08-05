/* $Id: trainer.c,v 1.2 2009/07/17 08:15:20 geni Exp $
 *
 * Author: Michael Stone
 * Date: May 11, 2009
 */
#include "dlusb.h"
#include "trainer.h"

static long int get_duration(chrono_split_time_t lap_time);
static chrono_split_time_t get_average_lap_time(int num_laps,
		chrono_split_time_t lap_time);
static chrono_split_time_t get_split_time(chrono_split_t *split);
static chrono_split_time_t get_lap_time(chrono_split_time_t prev_split,
		chrono_split_time_t curr_split);

void
print_chrono_report(chrono_t *db, FILE *fp)
{
	int i, num_workouts;

	num_workouts = get_chrono_num_workouts(db);
	for(i = 1; i <= num_workouts; i++){
		print_chrono_workout_report(db, i, fp);
		if(i < num_workouts)
			fprintf(fp, "\n\n");
	}

	return;
}

int
print_chrono_workout_report(chrono_t *db, u8 workout_id, FILE *fp)
{
	chrono_workout_t *workout;
	chrono_split_t *split;
	chrono_split_time_t split_time, prev_split_time, lap_time, avg_lap_time;
	int i;

	for(workout = db->head, i = 1; workout && i != workout_id;
			workout = workout->next, i++);

	if(!workout)
		/* nothing to report */
		return -1;

	fprintf(fp, "%s\n\n", CHRONO_REPORT_TITLE);
	fprintf(fp, "%s\t%02d/%02d/%04d %s\n", CHRONO_REPORT_DATE,
			debcd(workout->month), debcd(workout->day),
			2000+debcd(workout->year),
			get_chrono_dow_str(workout->dow));
	fprintf(fp, "%s\t%d\n", CHRONO_REPORT_TOTAL_LAPS, workout->num_laps);

	/* totals */
	split_time = get_split_time(workout->tail);
	fprintf(fp, "%s\t%02d:%02d:%02d.%02d\n", CHRONO_REPORT_TOTAL_TIME,
			split_time.hours, split_time.minutes,
			split_time.seconds, split_time.hundredths);

	/* calculate average lap time */
	avg_lap_time = get_average_lap_time(workout->num_laps, split_time);
	fprintf(fp, "%s\t%02d:%02d:%02d.%02d\n", CHRONO_REPORT_AVERAGE_LAP,
			avg_lap_time.hours, avg_lap_time.minutes,
			avg_lap_time.seconds, avg_lap_time.hundredths);

	fprintf(fp, "\n%s\n%s\n\n", CHRONO_REPORT_COLUMN_HEADER,
			CHRONO_REPORT_COLUMN_LINE);

	prev_split_time.hours = prev_split_time.minutes =
		prev_split_time.seconds = prev_split_time.hundredths = 0;
	for(split = workout->head; split; split = split->next){
		fprintf(fp, "%s%3d:", TIMEX_TRAINER_CSV_LAP, split->lap_id);

		/* get current lap split */
		split_time = get_split_time(split);

		/* calculate current lap time from previous and current lap
		 * splits */
		lap_time = get_lap_time(prev_split_time,
				split_time);
		fprintf(fp, "\t%02d:%02d:%02d.%02d",
				lap_time.hours, lap_time.minutes,
				lap_time.seconds, lap_time.hundredths);

		/* print split time */
		fprintf(fp, "\t%02d:%02d:%02d.%02d",
				split_time.hours, split_time.minutes,
				split_time.seconds, split_time.hundredths);

		/* best lap */
		if(split->lap_id == workout->best_lap_id)
			fprintf(fp, "\t%s", CHRONO_REPORT_BEST_LAP);

		/* end of lap */
		fprintf(fp, "\n");

		/* save previous lap split for calculating current lap time */
		prev_split_time = split_time;
	}

	return 0;
}

int
export_chrono_csv(chrono_t *db)
{
	char file[PATH_MAX];
	FILE *fp;
	chrono_workout_t *workout;
	chrono_split_t *split;
	chrono_split_time_t split_time, prev_split_time, lap_time;
	int i;

	for(workout = db->head, i = 0; workout; workout = workout->next, i++){
		sprintf(file, "%04d%02d%02d-%d.csv",
				2000+debcd(workout->year),
				debcd(workout->month), debcd(workout->day), i);
		if(!(fp = fopen(file, "w"))){
			ERROR("%s: open failed", file);
			return -1;
		}

		fprintf(fp, "%s\n%s\n%s\n",
				TIMEX_TRAINER_CSV_HEADER,
				TIMEX_TRAINER_CSV_SESSION,
				TIMEX_TRAINER_CSV_VERSION);
		fprintf(fp, "%s%02d/%02d/%04d 12:00.00 PM\n",
				TIMEX_TRAINER_CSV_SESSION_DATE,
				debcd(workout->month),
				debcd(workout->day),
				2000+debcd(workout->year));
		fprintf(fp, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
				TIMEX_TRAINER_CSV_SPORT_NAME,
				TIMEX_TRAINER_CSV_SPORT_ID,
				TIMEX_TRAINER_CSV_DISTANCE_UNIT,
				TIMEX_TRAINER_CSV_AVG_SPEED_UNIT,
				TIMEX_TRAINER_CSV_MAX_SPEED_UNIT,
				TIMEX_TRAINER_CSV_AVG_PACE_UNIT,
				TIMEX_TRAINER_CSV_MAX_PACE_UNIT,
				TIMEX_TRAINER_CSV_DISTANCE);

		/* duration is the last lap split time in seconds, rounding
		 * hundredths */
		fprintf(fp, "%s%ld\n", TIMEX_TRAINER_CSV_DURATION,
				get_duration(get_split_time(workout->tail)));

		fprintf(fp, "%s%s", TIMEX_TRAINER_CSV_DESCRIPTION,
				TIMEX_TRAINER_CSV_LAP_SEPARATOR);
		prev_split_time.hours = prev_split_time.minutes =
			prev_split_time.seconds =
			prev_split_time.hundredths = 0;
		for(split = workout->head; split; split = split->next){
			fprintf(fp, "%s%3d: ", TIMEX_TRAINER_CSV_LAP,
					split->lap_id);

			/* get current lap split */
			split_time = get_split_time(split);

			/* calculate current lap time from previous and current
			 * lap splits */
			lap_time = get_lap_time(prev_split_time, split_time);
			fprintf(fp, "%s%02d:%02d:%02d.%02d ",
					TIMEX_TRAINER_CSV_LAP_TIME,
					lap_time.hours,
					lap_time.minutes,
					lap_time.seconds,
					lap_time.hundredths);
			fprintf(fp, "%s%02d:%02d:%02d.%02d%s",
					TIMEX_TRAINER_CSV_SPLIT_TIME,
					split_time.hours,
					split_time.minutes,
					split_time.seconds,
					split_time.hundredths,
					TIMEX_TRAINER_CSV_LAP_SEPARATOR);

			/* save previous lap split for calculating current lap
			 * time */
			prev_split_time = split_time;
		}
		fprintf(fp, "\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s",
				TIMEX_TRAINER_CSV_SESSION_TYPE,
				TIMEX_TRAINER_CSV_MAX_HR,
				TIMEX_TRAINER_CSV_MAX_SPEED,
				TIMEX_TRAINER_CSV_AVG_HR,
				TIMEX_TRAINER_CSV_AVG_SPEED,
				TIMEX_TRAINER_CSV_FROM_DATA_RECORDER,
				TIMEX_TRAINER_CSV_TIME_ZONE1,
				TIMEX_TRAINER_CSV_TIME_ZONE2,
				TIMEX_TRAINER_CSV_TIME_ZONE3,
				TIMEX_TRAINER_CSV_TIME_ZONE4,
				TIMEX_TRAINER_CSV_TIME_ZONE5);

		fclose(fp);
	}

	return 0;
}

int
get_chrono_num_workouts(chrono_t *db)
{
	chrono_workout_t *workout;
	int i;

	for(workout = db->head, i = 0; workout; workout = workout->next, i++);

	return i;
}

static long int
get_duration(chrono_split_time_t lap_time)
{
	long int duration;

	/* return duration in seconds, rounding hundredths */
	duration = lap_time.hours * 3600
		+ lap_time.minutes * 60
		+ lap_time.seconds
		+ ((lap_time.hundredths >= 50) ? 1 : 0);

	return duration;
}

static chrono_split_time_t
get_average_lap_time(int num_laps, chrono_split_time_t lap_time)
{
	long int avg_lap;
	chrono_split_time_t avg_lap_time;

	if(num_laps < 1){
		ERROR("get_average_lap_time: there must be at least 1 lap");
		avg_lap_time.hours = avg_lap_time.minutes =
			avg_lap_time.seconds = avg_lap_time.hundredths = 0;
		return avg_lap_time;
	}

	/* calculate average lap time in hundredths */
	avg_lap = ((lap_time.hours * 360000)
			+ (lap_time.minutes * 6000)
			+ (lap_time.seconds * 100)
			+ lap_time.hundredths)
		/ num_laps;
	avg_lap_time.hours = avg_lap / 360000;
	avg_lap_time.minutes = (avg_lap / 6000) - (avg_lap_time.hours * 60);
	avg_lap_time.seconds = (avg_lap / 100) - (avg_lap_time.hours * 3600) -
		(avg_lap_time.minutes * 60);
	avg_lap_time.hundredths = avg_lap - (avg_lap_time.hours * 360000) -
		(avg_lap_time.minutes * 6000) - (avg_lap_time.seconds * 100);

	return avg_lap_time;
}

static chrono_split_time_t
get_split_time(chrono_split_t *split)
{
	chrono_split_time_t split_time;

	if(NULL == split){
		ERROR("get_split_time: NULL split");
		split_time.hours = split_time.minutes = split_time.seconds =
			split_time.hundredths = 0;
	}else{
		split_time.hours = debcd(split->hours);
		split_time.minutes = debcd(split->minutes);
		split_time.seconds = debcd(split->seconds);
		split_time.hundredths = debcd(split->hundredths);
	}

	return split_time;
}

static chrono_split_time_t
get_lap_time(chrono_split_time_t prev_split, chrono_split_time_t curr_split)
{
	chrono_split_time_t lap_time;

	/* calculate current lap time from previous and current lap splits */
	lap_time.hundredths = curr_split.hundredths - prev_split.hundredths;
	if(lap_time.hundredths < 0){
		++prev_split.seconds;
		lap_time.hundredths += 100;
	}
	lap_time.seconds = curr_split.seconds - prev_split.seconds;
	if(lap_time.seconds < 0){
		++prev_split.minutes;
		lap_time.seconds += 60;
	}
	lap_time.minutes = curr_split.minutes - prev_split.minutes;
	if(lap_time.minutes < 0){
		++prev_split.hours;
		lap_time.minutes += 60;
	}
	lap_time.hours = curr_split.hours - prev_split.hours;
	if(lap_time.hours < 0)
		ERROR("DATA CORRUPTED: THIS SHOULD NOT HAPPEN!");

	return lap_time;
}
