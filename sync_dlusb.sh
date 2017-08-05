#!/bin/sh
# $Id: sync_dlusb.sh,v 1.4 2009/05/09 05:04:35 geni Exp $
#
# This script downloads all data from the watch.

# read configuration file
. ~/.dlusbrc

cd $dlusb_dir

backup=`date +"%Y-%m-%d_%H:%M:%S"`

echo Compressing current data...
if [ -d $backup ]
then
	rm -rf $backup
fi
mkdir $backup
cp *.txt $backup
cp -Rpvf wristapp $backup
tar cvfz $backup.tgz $backup
rm -rf $backup

echo Preprocessing...
eval $preprocess

# set time
echo Setting time...
tod_dlusb tz1="$tod_tz1" tz2="$tod_tz2" tz3="$tod_tz3" date="$tod_date" \
	24hr="$tod_24hr" week_no="$tod_week_no"

echo Converting Note to Contact...
note_to_contact.sh > /dev/null

echo Reading internal application data...
for i in $apps
do
	case $i in
	.*)
		i=`echo $i | sed 's/^\.//'`
		;;
	esac
	case $i in
	*.app)
		app=`echo $i | sed 's/\.app$//'`
		db=`awk '/^DB=/{
			sub("\r", "")
			print substr($0, 4)
		}' wristapp/$i`
		if [ "$db" = "" ]
		then
			echo $app: no database
			continue
		fi
		dump_dlusb wristapp=$app=wristapp/$db
		;;
	option=)
		echo option
		option_dlusb > option.txt
		;;
	*=)
		;;
	*)
		echo $i
		print_$i > $i.txt
		;;
	esac
done

echo Postprocessing...
eval $postprocess

echo
echo Sync done!
echo If something went wrong, check the backup file $backup.tgz.
