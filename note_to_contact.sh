#!/bin/sh
# $Id: note_to_contact.sh,v 1.4 2005/10/18 16:58:04 geni Exp $
#
# Simple script to convert note to contact
#
# The format of note is as follows:
#
# <SPACE>name<SPACE><SPACE>space separated phone number(optional type)
#
# For example, if you have the following data in Note (not including double
# quotes):
#
# " HIS, NAME  123 123 1234"	# no type specified: this script will ask!
# " HIS, NAME  123 123 2345H"	# home
# " COMPANY  123 123 3456 CO"	# company
#
# running this script will create a new contact from the note and delete it
# from Note.
#
# The script will be useful since you cannot enter a contact using the watch.

tmp1=/tmp/note_to_contact.$$.1
tmp2=/tmp/note_to_contact.$$.2
trap "rm -f $tmp1 $tmp2" 0 1 2

touch $tmp1 $tmp2
chmod 600 $tmp1 $tmp2
print_note | grep '^ ' > $tmp1

if [ ! -s $tmp1 ]
then
	exit
fi

perl -e '
open FH1, "'$tmp1'";
open FH2, ">'$tmp2'";
while(<FH1>){
	m/^ (.*)  ([0-9 ]+) ([0-9]+ [0-9]+)(?: *([A-Z]*))?$/;
	$name = $1;
	$area = $2;
	$phone = $3;
	$type = $4;
	$area =~ s/ /-/g;
	$phone =~ s/ /-/g;
	printf "Name: %s\nPhone: %s-%s %s\n", $name, $area, $phone, $type;

	while(!($type =~ m/^([hwc]|co)$/i)){
		printf "Type? (Home/Work/Cellular/COmpany) ";
		$type = <STDIN>;
		chomp $type;
	}

	printf "Message? ";
	$msg = <STDIN>;
	chomp $msg;
	$msg =~ s/	/ /g;
	$msg =~ s/^ +//;
	$msg =~ s/ +$//;
	if($msg ne ""){
		$msg = "  $msg";
	}

	$contact = "$name$msg	$type	$area	$phone";
	$contact =~ tr/a-z/A-Z/;
	printf FH2 "%s\n", $contact;
}
close FH2;
close FH1;
'
add_contact < $tmp2
del_note < $tmp1
