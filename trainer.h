/* $Id: trainer.h,v 1.1 2009/07/13 05:16:42 geni Exp $
 *
 * Author: Michael Stone
 * Date: May 11, 2009
 */

#ifndef _TRAINER_H_
#define _TRAINER_H_

#include <stdio.h>
#include "dlusb.h"

/* Timex Trainer */
#define TIMEX_TRAINER_CSV_HEADER "[Timex Trainer Data File]"
#define TIMEX_TRAINER_CSV_SESSION "[session data]"
#define TIMEX_TRAINER_CSV_VERSION "Version=1.1_mm/dd/yy"
#define TIMEX_TRAINER_CSV_SESSION_DATE "Sessiondate="
#define TIMEX_TRAINER_CSV_SPORT_NAME "sportName="
#define TIMEX_TRAINER_CSV_SPORT_ID "sportID="
#define TIMEX_TRAINER_CSV_DISTANCE_UNIT "distanceunit=mi"
#define TIMEX_TRAINER_CSV_AVG_SPEED_UNIT "avgspeedunit=0"
#define TIMEX_TRAINER_CSV_MAX_SPEED_UNIT "maxspeedunit=0"
#define TIMEX_TRAINER_CSV_AVG_PACE_UNIT "avgpaceunits="
#define TIMEX_TRAINER_CSV_MAX_PACE_UNIT "maxpaceunits="
#define TIMEX_TRAINER_CSV_DISTANCE "distance="
#define TIMEX_TRAINER_CSV_DURATION "duration="
#define TIMEX_TRAINER_CSV_DESCRIPTION "description=Timex Data Link USB Workout."
#define TIMEX_TRAINER_CSV_SESSION_TYPE "sessionType="
#define TIMEX_TRAINER_CSV_MAX_HR "maxHR="
#define TIMEX_TRAINER_CSV_MAX_SPEED "maxSpeed="
#define TIMEX_TRAINER_CSV_AVG_HR "avgHR="
#define TIMEX_TRAINER_CSV_AVG_SPEED "avgSpeed="
#define TIMEX_TRAINER_CSV_FROM_DATA_RECORDER "fromDataRecorder=False"
#define TIMEX_TRAINER_CSV_TIME_ZONE1 "TimeZone1=0"
#define TIMEX_TRAINER_CSV_TIME_ZONE2 "TimeZone2=0"
#define TIMEX_TRAINER_CSV_TIME_ZONE3 "TimeZone3=0"
#define TIMEX_TRAINER_CSV_TIME_ZONE4 "TimeZone4=0"
#define TIMEX_TRAINER_CSV_TIME_ZONE5 "TimeZone5=0"
#define TIMEX_TRAINER_CSV_LAP_SEPARATOR "^^"
#define TIMEX_TRAINER_CSV_LAP "Lap "
#define TIMEX_TRAINER_CSV_LAP_TIME "Lap Time: "
#define TIMEX_TRAINER_CSV_SPLIT_TIME "SplitTime: "

#define CHRONO_REPORT_TITLE "Timex Data Link Chrono Workout"
#define CHRONO_REPORT_DATE "Date:"
#define CHRONO_REPORT_TOTAL_LAPS "Total Laps:"
#define CHRONO_REPORT_TOTAL_TIME "Total Time:"
#define CHRONO_REPORT_AVERAGE_LAP "Average Lap:"
#define CHRONO_REPORT_BEST_LAP "(best lap)"
#define CHRONO_REPORT_COLUMN_HEADER "Lap\tLap Time\tSplit Time"
#define CHRONO_REPORT_COLUMN_LINE "===\t========\t=========="

typedef struct _chrono_split_time {
	int hours,
	    minutes,
	    seconds,
	    hundredths;
} chrono_split_time_t;

void print_chrono_report(chrono_t *db, FILE *fp);
int print_chrono_workout_report(chrono_t *db, u8 workout_id, FILE *fp);
int export_chrono_csv(chrono_t *db);
int get_chrono_num_workouts(chrono_t *db);

#endif
