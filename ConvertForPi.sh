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
#infile="$1"
#outfile="$2"


# test that infile provided
#[ "$infile" = "" ] && errMsg "NO INPUT FILE SPECIFIED"

# test that outfile provided
#[ "$outfile" = "" ] && errMsg "NO OUTPUT FILE SPECIFIED"

CONV_DIRNAME="conv"
for InputFile in $*
do 
  outfile=$(basename "$InputFile")
  EXT="${outfile##*.}"
#  echo EXT:$EXT
  OUT_WO_EXT="${outfile%.*}"
#  echo OUT_WO_EXT:$OUT_WO_EXT
  DIR=$(dirname "${InputFile}")
  if [ ! -d "${DIR}/${CONV_DIRNAME}" ]; then
    mkdir "${DIR}/${CONV_DIRNAME}"
  fi
  outfile=${DIR}/${CONV_DIRNAME}/conv_${OUT_WO_EXT}.${EXT}
  echo Bearbeite $InputFile nach $outfile
  test=`identify -format '%[exif:orientation]' $InputFile`
  echo Format:$test
  if [ $test -eq 1 ]; then
    convert $InputFile -auto-orient -ordered-dither o8x8,64,64,64 -quality 97 -resize 1920x $outfile
  else
    convert $InputFile -auto-orient -ordered-dither o8x8,64,64,64 -quality 97 -resize x1200 $outfile
  fi
done


exit 0
