#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo $DIR

# This script is a helper script to set up the databse.

if [ $#  -lt 2 ]  ; then 
   echo "Usage:  $0 <database file> <schema file>"
else 

   if [ $1 == "-f" ] ; then 
      if [ -e $2 ]; then 
         rm -rf $2
      fi
      dest=$2
      schemaFile=$3
   else
      dest=$1
      schemaFile=$2
   fi

   # make sure target directory exists
   DBDIR=$(dirname $dest)
   mkdir -p $DBDIR 

   if [ -e $dest ] ; then 
      echo "Database file \"$dest\" already exists, aborting." 
      exit -1
   fi 
   echo "Creating database at: $dest"
   sqlite3 $dest < $schemaFile

   echo "Setup Complete"
fi

