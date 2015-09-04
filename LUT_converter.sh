#!/bin/bash

# Creates a look up table from a csv files. I'm assuming the csv file 
# has 7 columns, and the first 2 columns are start and end time for 
# the subsequent values.

if (( $# != 1 ))
	then
	echo "usage: ./LUT_converter.sh [filename]"
	exit 1
fi

base_name="${1%.*}"
file_name="LUT_${base_name}.txt"
tmp_name='tmp_${file_name}'

gawk -f 'LUT_converter.awk' $1 > $tmp_name

rows=`wc -l < $tmp_name`

echo "#1" > $file_name
echo "double table($rows,6)" >> $file_name
cat $tmp_name >> $file_name

rm $tmp_name

