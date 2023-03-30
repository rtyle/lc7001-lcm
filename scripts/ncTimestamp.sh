#!/bin/bash

nc LCM1.local 2112 |

while read -r -d $'\0' message;
do
   NOW=$(date "+%H:%M:%S")
   echo "($NOW): $message"
done
