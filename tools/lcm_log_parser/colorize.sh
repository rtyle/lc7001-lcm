#!/bin/sh

show_usage()
{
   echo "Usage:  $1 [-f path/myfilter_colorize.csv | -f myfilter] filename..." 
   echo "This utility will take any text file and output a colorized .html file."
   echo "Example filter files are in $HBIN/*_colorize.csv, and you can make your own."
}

#---------------------------------------------------------------------------

colorize_filter()
{
   file=$1
   filter=$2
   DEBUG=1

   DEFAULT_COLOR=$(cat $filter | gawk -F "," 'BEGIN {d="";} {if ($2 == "*") d = $1;} END {if (d == "") printf "Grey"; else printf "%s", d; }')

   cat $file | gawk '{printf "%08d: %s\n", NR, $0;}' > $file.nums.tmp

   cat $filter | gawk -F "," 'BEGIN {c=0;}
           {n = split($2,a," "); 
            if (n > 0 && a[1] != "*") 
	    {  c++; printf "%03d|%s|%s\n", c, $1, $2; }
           }' > $filter.nums.tmp

   COUNT=1
   ALL_COLORS=""
   FILTER_COUNT=$(cat $filter.nums.tmp | gawk -F "|" '{n=$1+0;} END {printf "%d", n;}')
   if [ ! "$FILTER_COUNT" = 0 ]
   then
      while [ ! $COUNT = $FILTER_COUNT ]
      do
	      COUNT=$((COUNT + 1))
	      COLOR=$(cat $filter.nums.tmp | gawk -F "|" '{n = $1 + 0; if (n == '$COUNT')
	          { printf "%s", $2; exit; } }')
              ALL_COLORS="$ALL_COLORS $COLOR"
	      STRING=$(cat $filter.nums.tmp | gawk -F "|" '{n = $1 + 0; if (n == '$COUNT')
	          { printf "%s", $3; exit; } }')
              cat $file.nums.tmp | fgrep "$STRING" | fgrep -v "font color" | gawk '{printf "%s <a name=\"line%s\"><font color=\"'$COLOR'\"><B>%s</B></font> </a>\n", $1, substr($1,1,8), substr($0,11); }' > $file.$COLOR.tmp
	      LINES_TO_REMOVE=$(cat $file.$COLOR.tmp | gawk '{printf "%d\n", $1+0;}')
	      if [ "$DEBUG" = "1" ]
	      then
	         if [ "$LINES_TO_REMOVE" = "" ]
   	         then
		    echo "$COUNT: No lines matching \"$STRING\" for $COLOR in $file.nums.tmp" 1>&2
		    echo "fgrep \"$STRING\" $file.nums.tmp" 1>&2
		    fgrep "$STRING" $file.nums.tmp 1>&2
		    echo "fgrep \"$STRING\" $file" 1>&2
		    fgrep "$STRING" $file 1>&2
	         else
	            DBGLINES=$(echo "$LINES_TO_REMOVE" | wc -w)
		    echo "$COUNT: $DBGLINES lines matching \"$STRING\" for $COLOR" 1>&2
	         fi
	      fi
	      for LINENUM in $LINES_TO_REMOVE
	      do
		 cat $file.nums.tmp | gawk '{n=$1+0; if (n != '$LINENUM') print;}' > $file.nums.tmp2
		 mv $file.nums.tmp2 $file.nums.tmp
	      done
      done
   fi
   cat $file.nums.tmp | fgrep -v "font color" | gawk '{if (length(substr($0,11)) > 0) printf "%s <font color=\"'$DEFAULT_COLOR'\"><B>%s</B></font>\n", $1, substr($0,11); }' > $file.$DEFAULT_COLOR.tmp
   for COLOR in $ALL_COLORS
   do
      # STRING=$(cat $filter.nums.tmp | gawk -F "|" '{if ($2 == '$COLOR') { printf "%s", $3; exit; } }')
      STR=$(fgrep "|$COLOR|" $filter.nums.tmp | gawk -F "|" '{printf "%s", $3;}')
      if [ ! "$STR" = "" ]
      then
	 LINES=$(cat $file.$COLOR.tmp | wc -l)
	 if [ ! $LINES = 0 ]
	 then
            if [ "$DEBUG" = "1" ]
            then
               echo "Applying $COLOR header." 1>&2
            fi
            echo "Lines with \"<font color=\"$COLOR\">$STR</font>\":<BR>" 
            cat $file.$COLOR.tmp | gawk '{printf "<a href=\"#line%s\">%s</a>\n", substr($1,1,8), $1+0;}'
            echo "<BR>" 
         else
            echo "No lines with \"<font color=\"$COLOR\">$STR</font>\"<BR>" 
         fi
      fi
   done
   echo "<HR>"
   echo "<PRE>"
   ALL_COLORS="$ALL_COLORS $DEFAULT_COLOR"
   for COLOR in $ALL_COLORS
   do
      cat $file.$COLOR.tmp
   done | sort -u | gawk '{if (NF > 1) printf "%s\n", substr($0,11); }' 
   echo "</PRE>"
   rm $file.*tmp
   rm $filter.*tmp
}

#---------------------------------------------------------------------------

SCRIPTNAME=$(basename $0)

FILTER_FILES=$(ls -1tr $HBIN/*_colorize.csv)
ALL_FILTER_PREFIXES=""

for file in $FILTER_FILES
do
   if [ -f "$file" ]
   then
      bname=$(basename $file _colorize.csv)
      if [ "$ALL_FILTER_PREFIXES" = "" ]
      then
         ALL_FILTER_PREFIXES="$bname"
      else
         ALL_FILTER_PREFIXES="$ALL_FILTER_PREFIXES $bname"
      fi
   else
      echo "$SCRIPTNAME: ERROR -  Cannot open input file \"$fname\"."
   fi
done

FILTER_FILE=""

if [ "$1" = "-f" ]
then
	shift
	FILTER_PREFIX=$1
	shift
	if [ -f "$FILTER_PREFIX" ]
	then
           FILTER_FILE="$FILTER_PREFIX"
           FILTER_PREFIX=$(basename $FILTER_FILE _colorize.csv)
	fi
else
	echo -n "Enter filter prefix [$ALL_FILTER_PREFIXES]: "
	read FILTER_PREFIX
	if [ "$FILTER_PREFIX" = "" ]
	then
	   FILTER_PREFIX=$(echo "$ALL_FILTER_PREFIXES" | gawk '{printf "%s", $NF;}')
	fi
fi

if [ "$FILTER_FILE" = "" ]
then
   for prefix in $ALL_FILTER_PREFIXES
   do
      if [ "$prefix" = "$FILTER_PREFIX" ]
      then
         FILTER_FILE="$HBIN/"$prefix"_colorize.csv"
      fi
  done
fi

if [ ! -f "$FILTER_FILE" ]
then
    echo "$SCRIPTNAME: ERROR -  No filter file \"$FILTER_FILE\"."
    exit 1
fi

for fname in $*
do

   if [ "$fname" = "" -o "$fname" = "-h" ]
   then
      show_usage $(basename $0 .sh)
      exit 1
   fi

   if [ ! -f "$fname" ]
   then
      echo "ERROR: Cannot open input file \"$fname\"."
      show_usage $(basename $0 .sh)
      exit 1
   fi

   TITLE="$fname"
   OUTFILE="$fname.html"

   echo "<HTML>" > $OUTFILE

   echo "<HEAD>" >> $OUTFILE

   # echo "<BODY BACKGROUND=\"/samples/images/backgrnd.gif\">" >> $OUTFILE

   echo "<TITLE>$TITLE</TITLE>" >> $OUTFILE

   echo "</HEAD>" >> $OUTFILE

   echo "<BODY BGCOLOR=\"FFFFFF\">" >> $OUTFILE
   echo "<H1>$TITLE</H1>" >> $OUTFILE

   # echo "<HR>" >> $OUTFILE

   echo "$SCRIPTNAME: colorize_filter $fname $FILTER_FILE"
   colorize_filter $fname $FILTER_FILE >> $OUTFILE

   echo "</BODY>" >> $OUTFILE

   echo "</HTML>" >> $OUTFILE
   ls -ltr $file $OUTFILE
   DIRNAME=$(dirname $OUTFILE)
   if [ "$DIRNAME" = "." ]
   then
      DIRNAME=$(pwd)
   fi
   BNAME=$(basename $OUTFILE)
   WINDIR=$(cygpath -w "$DIRNAME")
   echo "Open in your browser: $WINDIR\\$BNAME"
done

