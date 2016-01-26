#!/bin/bash



# set directory for temporary files
dir="."    # suggestions are dir="." or dir="/tmp"


# set up functions to report Usage and Usage with Description
PROGNAME=`type $0 | awk '{print $3}'`  # search for executable on path
PROGDIR=`dirname $PROGNAME`            # extract directory of program
PROGNAME=`basename $PROGNAME`          # base name of program


# function to report error messages
errMsg()
	{
	echo ""
	echo $1
	echo ""
	usage1
	exit 1
	}


# get infile and outfile
infile="$1"
outfile="$2"


# test that infile provided
[ "$infile" = "" ] && errMsg "NO INPUT FILE SPECIFIED"

# test that outfile provided
[ "$outfile" = "" ] && errMsg "NO OUTPUT FILE SPECIFIED"


test=`identify -format '%[exif:orientation]' $infile`
echo $test
if [ $test -eq 1 ]; then
  convert $infile -auto-orient -ordered-dither o8x8,64,64,64 -quality 97 -resize 1920x $outfile
else
  convert $infile -auto-orient -ordered-dither o8x8,64,64,64 -quality 97 -resize x1200 $outfile
fi

exit 0
