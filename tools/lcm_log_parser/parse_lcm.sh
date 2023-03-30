#!/bin/sh

SCRIPTNAME=$(basename $0)
ALL_FILES=""
OUTFILE2_FILES=""

export COLORIZE="?"

if [ "$1" = "-n" ]
then
   export COLORIZE=0
   shift
fi   

if [ "$1" = "-c" ]
then
   export COLORIZE=1
   shift
fi   

export DEBUG=0
if [ "$1" = "-dbg" ]
then
   export DEBUG=1
   shift
fi   

for file in $*
do
   if [ -f "$file" ]
   then
      echo
      echo "--------------------------------------------"
      echo "Parsing $file:"
      echo "--------------------------------------------"
      cat $file | awk -F "{\"ID\":" '{for (i=1; i<=NF; i++) printf "%d: %s\n", i, $i;}' | fgrep -i "Debug " | awk '{inx = index($0,"Debug"); n = substr($0,inx-5) + 0; if (n != ln + 1) printf "Warning: %2d messages not printed between %06dDebug and %06dDebug\n", (n-ln)-1, ln, n; ln = n;}' > $file.warnings.txt
      WARNINGS=$(cat $file.warnings.txt | wc -l)
      if [ "$WARNINGS" = 0 ]
      then
	 echo
	 rm $file.warnings.txt
	 echo
      else
	 cat $file.warnings.txt
         ALL_FILES="$ALL_FILES $file.warnings.txt"
      fi
      OUTFILE=$file.parsed.txt
      OUTFILE2=$file.txt
      OUTFILE_THIN_CSV=$file.thin.csv

      cat $file | gawk -F "{\"ID\":0," 'BEGIN {ts = "00:00:00";} 
      {for (i=1; i<=NF; i++) 
       { if (index($i,"{\"ZID\":"))
            start = "{\"ZID\":";
         else if (index($i,"{\"SID\":"))
            start = "{\"SID\":";
	 else
         {
           start = "{\"ID\":0,";
           inx = index($i,"CurrentTime\":");
           if (inx) 
           { t = substr($i,inx+13)+0;
             ts = strftime("%Y-%m-%d %H:%M:%S", t);
             inx = index($i,"uSec");
             if (inx) 
             { 
               msec = (substr($i,inx+6) + 0) / 5;
             } 
           } 
           printf "\n%s.%03d %s%s\n", ts, msec, start, $i; 
         } 

	 if (start != "{\"ID\":0,")
         {
	    n = split($i,a,start);
            for (j=1; j<= n; j++)
            {
              if (a[j] != "")
              {
                 if (j == 1)
                   msg = a[j];
                 else
                   msg = sprintf("%s%s", start, a[j]);
                 inx = index(msg,"CurrentTime\":");
                 if (inx) 
                 { 
                   t = substr(msg,inx+13)+0;
                   ts = strftime("%Y-%m-%d %H:%M:%S", t);
                   inx = index(msg,"uSec");
                   if (inx) 
                   { 
                     msec = (substr(msg,inx+6) + 0) / 5;
                   } 
                   else if (ts != last_ts)
                     msec = 0;
                 }
                 printf "\n%s.%03d %s\n", ts, msec, msg; 
		 last_ts = ts;
              } 
           } 
         }
       }
      }' | sed -e "s///g" > $OUTFILE
      # ls -l $OUTFILE
      # 15:42:15.659 {"ID":0,"Service":"BroadcastMemory","FreeMemory:":69932,"FreeMemLowWater:":58876,"Malloc_Count:":31767,"Free_Count:":31600,"StaticRamUsage:":37568, "PeakRamUsage:":224947,"Status":"Success"}
      # 15:42:15.659 ("MemReport":"27109 more mallocs than frees, minMem(58876) <= mem(69932) <= maxMem(80924), -493 since 15:42:15.659"}
      PRE_LOG_LS=$(ls -l $OUTFILE2 2> /dev/null)
      cat $OUTFILE | gawk -F ":" 'BEGIN { lastMallocDiff = 0; lastMallocDiffTs = ""; lastFreeMem = 0; lastFreeMemTs = ""; maxFreeMem = 0; minFreeMem = 100000000;}
                { inx = index($0,"FreeMemory");
		  if (inx)
                  {
                    if ('$DEBUG') printf "DEBUG: FreeMemory inx=%d\n", inx;
	            freeMem = substr($0,inx+13) + 0;
		    if (minFreeMem > freeMem)
		       minFreeMem = freeMem;
		    if (maxFreeMem < freeMem)
		       maxFreeMem = freeMem;
		    ts = sprintf("%s:%s:%s", $1, $2, substr($3,1,6));
	            if (lastFreeMemTs == "")
		       lastFreeMemTs = ts;
	            if (lastMallocDiffTs == "")
		       lastMallocDiffTs = ts;
		    mallocCount = ($11+0);
		    freeCount = ($13+0);
		    mallocDiff = mallocCount - freeCount;
                    if ('$DEBUG') printf "DEBUG: mallocDiff=%d mallocCount=%d freeCount=%d\n", mallocDiff, mallocCount, freeCount;
		    if (mallocDiff > 0)
	            {
		       if (lastMallocDiff != mallocDiff)
		       {
			       printf "%s\n%s (\"MemReport\":\"%d more mallocs than frees, minMem(%d) <= mem(%d) <= maxMem(%d), %d since %s\"}\n", $0, ts, mallocDiff, minFreeMem, freeMem, maxFreeMem, mallocDiff - lastMallocDiff, lastMallocDiffTs; 
		       }  
	               else
		       {
			       printf "%s\n%s (\"MemReport\":\"%d more mallocs than frees, minMem(%d) <= mem(%d) <= maxMem(%d)\"}\n", $0, ts, mallocDiff, minFreeMem, freeMem, maxFreeMem; 
		       }  
		       lastMallocDiff = mallocDiff;
		       lastMallocDiffTs = ts;
		    }
	            else
		    {
	               print;
		    }
	            lastFreeMem = freeMem;
		  }
		  else
	            print;
	        }' > $OUTFILE2
      # ls -l $OUTFILE2
      PRE_CSV_LS=$(ls -l $OUTFILE2.csv 2> /dev/null)

      DIAGNOSTIC_PARAMS=$(cat $OUTFILE2 | fgrep BroadcastDiagnostics | tr -d "\." | tr -d "-" | sed -e "s/\"/ /g" | tr -d "," | sort -u | awk '{for (i=8; i<=NF && $i != "Status"; i += 2) 
      { printf "%s\n", substr($i,1,length($i)-1);} }' | sort -u | awk '{if (NR == 1) printf "%s", $1; else printf ":%s", $1;}')
      
      if [ "$ARTIK_PARAMS" = "" ]
      then
         DIAGNOSTIC_PARAMS="artik_first_time_count:artik_get_user_count:artik_msg_count:artik_ping_fail_count:artik_retry_count:artik_unexpected_adddevice_err_count:artik_unexpected_deldevice_err_count:artik_unexpected_getuser_err_count:qmotion_try_find_count:task_heartbeat_bitmask"
      fi

      cat $OUTFILE2 | fgrep BroadcastDiagnostics | tr -d ":" | tr -d "\." | tr -d "-" | sed -e "s/\"/ /g" | tr -d "," | sort -u | awk 'BEGIN { col = split("'$DIAGNOSTIC_PARAMS'",header,":");
	  # if (1) printf "DEBUG: col=%d header[%s] = %s %s %s %s\n", col, "'$DIAGNOSTIC_PARAMS'", header[1], header[2], header[3], header[4]; 
	  printf "TimeStamp";
          for (i=1; i<=col; i++)
	     printf ",%s", header[i];
	  printf "\n";
        }
	{  # if (1) printf "DEBUG: %s\n", $0; 
           printf "\"%s-%s-%s %s:%s:%s.%s\"", substr($1,1,4), substr($1,5,2), substr($1,7,2), substr($2,1,2), substr($2,3,2), substr($2,5,2), substr($2,7); 
	   for (h=1; h<=col; h++)
	   {
	      hstr = header[h];
              done = 0;
	      for (i=8; done == 0 && i <= (NF-2); i += 2) 
	      {
		  if (hstr == $i)
		  {
	             printf ",%s", $(i+1);
                     done = 1;
		  }
	      }
	      if (done == 0)
		 printf ",0";
           }
	   printf "\n";
	}' > $file.diagnostics.csv

      cat $OUTFILE2 | fgrep MemReport | gawk 'BEGIN {printf "\"Timestamp\",\"Outstanding Mallocs\",\"Min Free Memory\",\"Free Memory\",\"Max Free Memory\"\n";}
      {ts = sprintf("%s %s",$1,$2); 
          inx=index($0,"MemReport"); 
          if (inx) 
	     unfree = substr($0,inx+12)+0;
          inx = index($0,"minMem"); 
	  if (inx) 
	     minMem = substr($0,inx+7)+0; 
	  inx = index($0,"mem("); 
	  if (inx) 
	    mem = substr($0,inx+4)+0;
	  inx = index($0,"maxMem"); 
	  if (inx) maxMem = substr($0,inx+7)+0; 
	  printf "\"%s\",%d,%d,%d,%d\n", ts, unfree, minMem, mem, maxMem; }' > $OUTFILE2.csv
      POST_LOG_LS=$(ls -l $OUTFILE2 2> /dev/null)
      POST_CSV_LS=$(ls -l $OUTFILE2.csv 2> /dev/null)
      if [ "$PRE_CSV_LS" = "$POST_CSV_LS" ]
      then
         if [ ! "$PRE_CSV_LS" = "$POST_CSV_LS" ]
         then
   	    echo "$SCRIPTNAME: Warning - It appears that you may have $OUTFILE2.csv open in another program."
   	    echo -n "Please close the other program and hit return..."
            read junk
            cat $OUTFILE2 | fgrep MemReport | gawk 'BEGIN {printf "\"Timestamp\",\"Outstanding Mallocs\",\"Min Free Memory\",\"Free Memory\",\"Max Free Memory\"\n";}
            {ts = sprintf("%s %s",$1,$2); 
             inx=index($0,"MemReport"); 
             if (inx) 
	        unfree = substr($0,inx+12)+0;
             inx = index($0,"minMem"); 
	     if (inx) 
	        minMem = substr($0,inx+7)+0; 
	     inx = index($0,"mem("); 
	     if (inx) 
	       mem = substr($0,inx+4)+0;
	     inx = index($0,"maxMem"); 
	     if (inx) maxMem = substr($0,inx+7)+0; 
	     printf "\"%s\",%d,%d,%d,%d\n", ts, unfree, minMem, mem, maxMem; }' > $OUTFILE2.csv
             POST_CSV_LS=$(ls -l $OUTFILE2.csv 2> /dev/null)
             if [ "$PRE_CSV_LS" = "$POST_CSV_LS" ]
             then
   	        echo "$SCRIPTNAME: Warning - It still appears that $OUTFILE2.csv is unchanged."
             fi
	 fi
      fi
      # ls -l $OUTFILE2.csv
      cat $OUTFILE2.csv | awk -F "," 'BEGIN {lp = 1;}  {if (NR == 0) print; else {m = $4+0; low = $3; if (m!=lm || low!=llow) { if (!lp) printf "%s\n", last; lp = 1; print } else lp = 0;; lm = m; llow = low; last = $0; } }' > $OUTFILE_THIN_CSV

      if [ "$COLORIZE" = "?" ]
      then
         yorn=$(yornq.sh -d n "colorize.sh -f lcm $OUTFILE2")
      else
         if [ "$COLORIZE" = "1" ]
         then
	    yorn="y"
	 else
	    yorn="n"
         fi
      fi
      ALL_FILES="$ALL_FILES $file $OUTFILE $OUTFILE2 $OUTFILE2.csv $OUTFILE_THIN_CSV $file.diagnostics.csv $file.warnings.txt"
      OUTFILE2_FILES="$OUTFILE2_FILES $OUTFILE2"
      if [ "$yorn" = "y" ]
      then
         ls -ltr $ALL_FILES
         colorize.sh -f lcm $OUTFILE2
         ALL_FILES="$ALL_FILES $OUTFILE2.html"
      fi
      echo
   else
      echo "$SCRIPTNAME: ERROR - $file is not a file." 
   fi
   echo
done
ls -ltr $ALL_FILES 2> /dev/null
if [ "$COLORIZE" = "?" -a ! "$OUTFILE2_FILES" = "" ]
then
   yorn=$(yornq.sh -d y "vi $OUTFILE2_FILES")
   if [ "$yorn" = "y" ]
   then
      vi $OUTFILE2_FILES
   fi
fi

