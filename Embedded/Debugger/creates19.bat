:; echo "Creating $2"
:; mkdir -p $2
:; echo "Copying $1 to $2/$3"
:; cp $1 ${2}/${3}
:; echo "Creating the s19 file"
:; arm-none-eabi-objcopy -O srec --srec-forceS3 $3
:; exit 0

copy LCM1-K64.elf ..\"Int Flash Debug"\LCM1-K64.s19
pushd ..\"Int Flash Debug"
C:\Freescale\"CW MCU v10.6"\Cross_Tools\arm-none-eabi-gcc-4_7_3\bin\arm-none-eabi-objcopy -O srec --srec-forceS3 LCM1-K64.s19
popd
