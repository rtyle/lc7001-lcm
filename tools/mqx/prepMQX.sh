#!/bin/bash

MQX_TAR_FILE=FSLMQXOS_4_1_1_LINUX_GA.tar.gz
MQX_PATCH_FILE=MQX.patch

if [ ! -f "$MQX_TAR_FILE" ]; then
    echo "$MQX_TAR_FILE does not exist."
    exit 1
fi

if [ ! -f "$MQX_PATCH_FILE" ]; then
    echo "$MQX_PATCH_FILE does not exist."
    exit 1
fi

rm -rf ../../MQX
mkdir ../../MQX
tar -xvzf $MQX_TAR_FILE -C ../../MQX --strip-components=1
cd ../../MQX/mqx/source/io/enet/phy
cp phy_ksz8041.h phy_ksz8081.h
cp phy_ksz8041.c phy_ksz8081.c
sed -i -n '28,$p' phy_ksz8081.*
cd -
cd ../../MQX
patch -p0 < ../tools/mqx/$MQX_PATCH_FILE




