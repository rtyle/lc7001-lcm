#!/bin/bash

# Exit the script on any failure
set -e

# Change into the script directory
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
pushd ${SCRIPT_DIR} > /dev/null

# Deploy the Server
echo "Not Deploying the Server application, because the server application is dead for now..."
#./serverDeploy.sh

if [ $? -eq 0 ]
then
   #echo "Server successfully deployed"
   echo
   echo "Checking the S19 file size"
   ./sizeTest.sh

   # Deploy the Firmware
   echo "Deploying the Firmware"
   ./firmwareDeploy.sh

   if [ $? -eq 0 ]
   then
      echo "Firmware successfully deployed"
   else
      echo "Error deploying firmware"
      exit -2
   fi
else
   echo "Error deploying server"
   exit -1
fi

# Pop out of the script directory
popd > /dev/null
