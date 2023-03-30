#!/bin/bash

# flash_usage.sh v1.0 - Legrand - 12/30/19

flash_size=1048576
block_count=64
bytes_per_block=($flash_size/$block_count)
embedded_path=../Embedded

color_program="\e[41m\e[37m"
   color_free="\e[47m\e[30m"
     color_nv="\e[42m\e[30m"
    color_ota="\e[44m\e[33m"
 color_normal="\e[40m\e[37m"

if [ "$1" = "-h" ] 
then
echo "Arguments:"
echo "  -h   Print options"
echo "  -p   Only print program size"
echo "  -n   Only print non-volatile storage size"
echo "  -a   Only print space available for program"
echo "  -f   Only print total free space"
echo "  -c   Only print program capacity"
echo "  -m   Suppress memory map"
exit
fi

if [ -z "$elf_file" ]
then
	elf_file="$embedded_path/Int Flash Debug/LCM1-K64.elf"
fi

if [ -z "$defines_h_file" ]
then
	defines_h_file="$embedded_path/includes/defines.h"
fi

if [ ! -f "$elf_file" ]
then
	echo "ELF file $elf_file does not exist, aborting."
	exit 1
fi

if [ ! -f "$defines_h_file" ]
then
	echo "defines.h file $defines_h_file does not exist, aborting."
	exit 1
fi

nv_start=$(($((16#$(cat "$defines_h_file" | grep LCM_DATA_STARTING_BLOCK | awk '{print substr($3,3)}')))))
arm-none-eabi-objcopy -O binary "$elf_file" tmp.bin

nv_size=$(($flash_size-$nv_start))
program_size=$(du -b tmp.bin | awk '{print $1 }')
program_capacity=$(($flash_size/2-$nv_size))
available_for_program=$(($program_capacity-$program_size))
total_free_space=$(($flash_size-2*$program_size-$nv_size))

rm tmp.bin

# Only print program size
if [ "$1" = "-p" ] 
then
echo $program_size
exit
fi

# Only print NV section size
if [ "$1" = "-n" ] 
then
echo $nv_size
exit
fi

# Only print space available for program
if [ "$1" = "-a" ] 
then
echo $available_for_program
exit
fi

# Only print total free space
if [ "$1" = "-f" ] 
then
echo $total_free_space
exit
fi

# Only print program capacity
if [ "$1" = "-c" ]
then
echo $program_capacity
exit
fi

echo ""
echo "       Device capacity: "$flash_size "Bytes"
echo "          Program size: "$program_size "Bytes"
echo "            NV storage: "$nv_size "Bytes"
echo "      Total free space: "$total_free_space "Bytes"
echo "      Program capacity: "$program_capacity "Bytes"
echo " Available for program: "$available_for_program "Bytes"

# Suppress memory map
if [ "$1" = "-m" ] 
then
exit
fi

echo -ne "\n                             M E M O R Y    M A P\n        "
echo -ne $color_program

section=0
bcount=0

for (( x=0; x < $block_count; x++ ))
do
	if (($bcount > $program_size)) && (($section==0))
	then
	section=1
	echo -ne $color_free
	fi

	if (($bcount > $flash_size/2))&& (($section==1))
	then
	section=2
	echo -ne $color_ota
	fi

	if (($bcount > ($flash_size/2)+$program_size   ))&& (($section==2))
	then
	section=3
	echo -ne $color_free
	fi

	if (($bcount > $nv_start))&& (($section==3))
	then
	section=4
	echo -ne $color_nv
	fi

	bcount=$(($bcount + $bytes_per_block))
	echo -n "|"
done

echo -ne $color_normal " "
echo -ne "\n\n Legend: "
echo -ne $color_program "Program space "
echo -ne $color_normal " "
echo -ne $color_free "Free space "
echo -ne $color_normal " "
echo -ne $color_ota "Update storage "
echo -ne $color_normal " "
echo -ne $color_nv "NV storage "
echo -e $color_normal"\n"

exit
