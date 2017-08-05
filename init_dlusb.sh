#!/bin/sh
# $Id: init_dlusb.sh,v 1.10 2009/07/17 07:44:46 geni Exp $
#
# This script initializes the watch with applications and data (up to 14).

# read configuration file
. ~/.dlusbrc

cd $dlusb_dir

# set time
echo Setting time...
tod_dlusb tz1="$tod_tz1" tz2="$tod_tz2" tz3="$tod_tz3" \
	date1="$tod_date1" date2="$tod_date2" date3="$tod_date3" \
	24hr1="$tod_24hr1" 24hr2="$tod_24hr2" 24hr3="$tod_24hr3" \
	week_no1="$tod_week_no1" \
	week_no2="$tod_week_no2" \
	week_no3="$tod_week_no3" \
	dst1="$tod_dst1" dst2="$tod_dst2" dst3="$tod_dst3" \
	euro1="$tod_euro1" euro2="$tod_euro2" euro3="$tod_euro3" \
	observe_dst1="$tod_observe_dst1" \
	observe_dst2="$tod_observe_dst2" \
	observe_dst3="$tod_observe_dst3" \
	adjust="$tod_adjust"

echo Recovering option...
option_dlusb < option.txt

echo Installing applications...
init=
lock=
for i in $apps
do
	case $i in
	.*)
		i=`echo $i | sed 's/^\.//'`
		lock="$lock lock=$i"
		;;
	esac
	case $i in
	*.app)
		if [ "$i" = "ModeBrow.app" ]
		then
			init="-f $init"
		fi
		init="$init wristapp=wristapp/$i"
		;;
	*=)
		init="$init $i"
		;;
	*)
		init="$init $i=$i.txt"
		;;
	esac
done

# initialize the watch and install some applications with data (up to 14)
if [ "$init" != "" ]
then
	init_dlusb $init
fi

# lock applications
if [ "$lock" != "" ]
then
	lock_dlusb $lock
fi
