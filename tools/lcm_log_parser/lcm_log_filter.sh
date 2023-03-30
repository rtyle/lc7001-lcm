#!/bin/sh

SCRIPTNAME=$(basename $0)
export DEBUG=0

      gawk -F "{\"ID\":0," 'BEGIN {ts = "00:00:00";} 
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
             ts = strftime("%H:%M:%S", t);
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
                   ts = strftime("%H:%M:%S", t);
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
      }' | gawk -F ":" 'BEGIN { lastMallocDiff = 0; lastMallocDiffTs = ""; lastFreeMem = 0; lastFreeMemTs = ""; maxFreeMem = 0; minFreeMem = 100000000;}
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
	        }'

