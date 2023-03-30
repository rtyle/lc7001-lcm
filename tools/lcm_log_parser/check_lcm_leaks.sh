#!/bin/sh

SCRIPTNAME=$(basename $0)
for file in $*
do
   if [ -f "$file" ]
   then
      echo "=========================================================="
      echo "FILE: $file"
      ls -ltr $file
      echo
      fgrep "FreeMemory" $file | sed -e "s/{/ /g" | sed -e "s/}/ /g" | sed -e  "s/,/ /g" | sed -e "s/\"/ /g" | sed -e "s/:/ /g" | gawk \
        'BEGIN {max_f = 0; min_f=10000000; } 
        {f=$7+0;
         m = $9+0;
         fm = $11 + 0;
         if (f > max_f)
         {
            max_f = f;
	    # printf "NEW MAX %s\n", $0;
	 }
         if (f < min_f)
	 {
            min_f = f;
	    # printf "NEW MIN %s\n", $0;
	 }
         d = m - fm;
         printf "%s:%s:%s FreeMemory=%d (max=%d min=%d) notFreedMalloc=%d\n", $1, $2, $3, f, max_f, min_f, d;
         }'
   else
      echo "$SCRIPTNAME: ERROR - file $file does not exist."
   fi
done

