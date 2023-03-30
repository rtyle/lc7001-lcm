#!/bin/bash

# Exit the script on any failure
set -e

# Set paths to files
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# Export file locations for flash_usage.sh
export elf_file="${SCRIPT_DIR}/../../.build_armel/Embedded/Sources/LCM1-K64.elf"
export defines_h_file="${SCRIPT_DIR}/../../Embedded/Includes/defines.h"

# Size limit adjustment file
SIZE_LIMIT_FILE="${SCRIPT_DIR}/sizeLimit.txt"

# Print flash usage summary
$SCRIPT_DIR/../flash_usage.sh -m

FILE_SIZE=`$SCRIPT_DIR/../flash_usage.sh -p`
FILE_LIMIT=`$SCRIPT_DIR/../flash_usage.sh -c`

# Add adjustment value
FILE_LIMIT=$(($FILE_LIMIT+$(<$SIZE_LIMIT_FILE)))

echo " "

if [ $FILE_SIZE -gt $FILE_LIMIT ]
then
echo "ERROR: S19 is too large! $FILE_SIZE is greater than $FILE_LIMIT"
exit -1
else
echo "S19 image is of size $FILE_SIZE.  This is safely under the current file limit of $FILE_LIMIT."
exit 0
fi
