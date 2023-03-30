#!/bin/sh

#-------------------------------------------------

show_usage()
{
   echo "Usage: $1 [-l logfile] [-d default_y_or_n] \"Yes or no question\"" 
   echo "Example of usage:"
   echo "yorn=\$($1 -l \$_logf -d y \"Does this look ok\")"
   echo "Does this look ok [y]? "
   echo "echo \"yorn=\$yorn\""
   echo "yorn=y"
}

#-------------------------------------------------

   if [ "$1" = "-h" ]
   then
       shift
       show_usage $(basename $0)
       exit 0
   fi

   if [ "$1" = "-l" ]
   then
       shift
       LOGFILE="$1"
       shift
   fi

   if [ "$1" = "-d" ]
   then
       shift
       DEFAULT_ANSWER="$1"
       DEFAULT_ANSWER_STR=" [$1]"
       shift
   fi

   QUESTION="$*"
   yorn_ok=0
   while [ "$yorn_ok" = 0 ]
   do
    echo -n "$QUESTION$DEFAULT_ANSWER_STR? " 1>&2
    if [ ! "$LOGFILE" = "" ]
    then
       echo -n "$QUESTION$DEFAULT_ANSWER_STR? " >> $LOGFILE
    fi
    read yorn
    if [ ! "$LOGFILE" = "" ]
    then
       echo "$yorn" >> $LOGFILE
    fi
    if [ "$yorn" = "" -a ! "" = "$DEFAULT_ANSWER" ]
    then
      yorn=$DEFAULT_ANSWER
    fi
    if [ "$yorn" = "y" -o "$yorn" = "Y" -o "$yorn" = "n" -o "$yorn" = "N" ]
    then
       if [ "$yorn" = "Y" ]
       then
           yorn=y
       else
          if [ "$yorn" = "N" ]
          then
              yorn=n
          fi
       fi
       yorn_ok=1
    else
       echo "Please enter 'y' or 'n'." 1>&2
    fi
   done
   echo "$yorn"
