#!/bin/bash

# Change into the script directory so we know where we are running from
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
pushd ${SCRIPT_DIR} > /dev/null

# Change to the main repository directory so the KEY_PATH will work properly
pushd ../../ > /dev/null

if [ -z "${GIT_BRANCH}" ]
then
   echo "GIT_BRANCH is not defined. Get the branch from the git command"
   GIT_BRANCH=$( git rev-parse --abbrev-ref HEAD )
   echo "Running from branch ${GIT_BRANCH}"
fi

# The environment variable KEY_PATH is set up by cloning the repository 
# github.com/nuvo-legrand/nuvoconfidential.git and searching for the 
# shared key in that repository. This is done on the jenkins server prior to
# this script being executed
if [ -z "$KEY_PATH" ]
then
   echo "KEY_PATH does not exist"
   KEY_PATH_DIR="${SCRIPT_DIR}/../../nuvoconfidential"
   echo "KEY_PATH_DIR = ${KEY_PATH_DIR}"
   rm -rf ${KEY_PATH_DIR}
   git clone git@github.com:nuvo-legrand/nuvoconfidential.git ${KEY_PATH_DIR}
   KEY_PATH=${KEY_PATH_DIR}/servers/legrand-rflc/legrand-rflc.pem
   echo "KEY_PATH = ${KEY_PATH}"
fi

SERVER_PATH="${SCRIPT_DIR}/../../.build_amd/Server/LCMServer/LCMServer"
if [ ! -e "${SERVER_PATH}" ]
then
   echo "Server build was not successful"
   exit -1
else
   chmod 400 $KEY_PATH

   if [ $? -ne 0 ]
   then
      "Chmod failed. Exiting"
      exit -1
   fi

   SERVER_IP=54.152.106.172

   # Get the branch name to determine where to put it on the server
   echo "GIT Branch = ${GIT_BRANCH}"
   echo "BUILD_NUMBER = ${BUILD_NUMBER}"
   if [[ ${GIT_BRANCH} == "origin/master" ]]
   then
      echo "Running from the master branch"
      DIRECTORY="master"
      EXECUTABLE_NAME="LCMServer-master"
      PORT=2112
   elif [[ ${GIT_BRANCH} == *"mpe"* ]]
   then
      echo "Running from an mpe branch"
      DIRECTORY="mpe"
      EXECUTABLE_NAME="LCMServer-mpe"
      PORT=2113
   elif [[ ${GIT_BRANCH} == *"mtm"* ]]
   then
      echo "Running from an mtm branch"
      DIRECTORY="mtm"
      EXECUTABLE_NAME="LCMServer-mtm"
      PORT=2114
   elif [[ ${GIT_BRANCH} == *"sal"* ]]
   then
      echo "Running from a sal branch"
      DIRECTORY="sal"
      EXECUTABLE_NAME="LCMServer-sal"
      PORT=2115
   else
      echo "Running from an unsupported branch"
   fi

   if [ ! -z "${DIRECTORY}" ]
   then
      # Connect to the server and kill any instances of LCMServer that are running
      echo "Killing any instances of ${EXECUTABLE_NAME}"
      ssh -T -i $KEY_PATH -o StrictHostKeyChecking=no ubuntu@${SERVER_IP} "killall ${EXECUTABLE_NAME}"

      # Upload the newly created application
      echo "Uploading the newly created LCMServer application"
      scp -i $KEY_PATH ${SERVER_PATH} ubuntu@54.152.106.172:/home/ubuntu/LCMServerReleases/${DIRECTORY}/${EXECUTABLE_NAME}

      if [ $? -ne 0 ]
      then
         "Upload failed. Exiting."
         exit -1
      fi

      # Start the server instance
      echo "Starting the server instance in a screen session"
      # Determine if a screen session exists
      exists=$(ssh -T -i $KEY_PATH -o StrictHostKeyChecking=no ubuntu@${SERVER_IP} $'screen -list')
      if [[ ${exists} == *"${EXECUTABLE_NAME}"* ]]
      then
         echo "Screen Session exists"
      else
         echo "Screen session does not exist. Creating a new one"
         ssh -T -i $KEY_PATH -o StrictHostKeyChecking=no ubuntu@${SERVER_IP} $"screen -mdS ${EXECUTABLE_NAME}"

         if [ $? -ne 0 ]
         then
            "Upload failed. Exiting."
            exit -1
         fi

      fi

      ssh -T -i $KEY_PATH -o StrictHostKeyChecking=no ubuntu@${SERVER_IP} $"screen -S ${EXECUTABLE_NAME} -X stuff '/home/ubuntu/LCMServerReleases/${DIRECTORY}/${EXECUTABLE_NAME} -p ${PORT} -z /home/ubuntu/LCMServerReleases/${DIRECTORY}/Zones.txt -s /home/ubuntu/LCMServerReleases/${DIRECTORY}/Scenes.txt\n'"

      if [ $? -ne 0 ]
      then
         "Server Start Failed. Exiting."
         exit -1
      fi

   fi
fi

# Pop out of the script directory
popd > /dev/null
