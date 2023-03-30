#!/bin/bash

# Exit the script on any failure
set -e

# Parse any command line options
updateFile=latest-development.s19
shortUpdateFile=dev.s19
while getopts "n:tr" opt; do
   case ${opt} in
      n)
         releaseNotesFile="$(readlink -f ${OPTARG})"
         if [[ -r ${releaseNotesFile} ]]
         then
            echo "Using ${releaseNotesFile} for release notes file"
            RELEASE_NOTES_STR="$(cat ${releaseNotesFile})"
         else
            echo "${releaseNotesFile} not readable"
         fi
         ;;
      r)
         updateFile=latest-LCM1.s19
         ;;
      t)
         updateFile=latest-testing.s19
         ;;
      \?)
         exit 1
         ;;
   esac
done

# Change into the script directory so git describe works as expected
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
pushd ${SCRIPT_DIR} > /dev/null

FIRMWARE_PATH="${SCRIPT_DIR}/../../.build_armel/Embedded/Sources/LCM.s19"
RELEASE_DIR="${SCRIPT_DIR}/../../Releases"
HEROKU_GIT="${SCRIPT_DIR}/../heroku/herokuGit.sh"

if [ ! -e "${FIRMWARE_PATH}" ]
then
   echo "Firmware build was not successful"
   if [ -z "${GIT_BRANCH}" ]
   then
      echo "GIT_BRANCH is not defined. Get the branch from the git command"
      GIT_BRANCH=$( git rev-parse --abbrev-ref HEAD )
   fi

   # Set build description for failure
   GIT_BRANCH=${GIT_BRANCH#"origin/"}
   echo "[LEGRAND_VERSION] ${GIT_BRANCH} failed!"

   exit -1
else
   if [ -z "${GIT_BRANCH}" ]
   then
      echo "GIT_BRANCH is not defined. Get the branch from the git command"
      GIT_BRANCH=$( git rev-parse --abbrev-ref HEAD )
   fi

   echo "Running from branch ${GIT_BRANCH}"

   if [[ "${GIT_BRANCH}" == "origin/master" || "${GIT_BRANCH}" =~ "origin/feature" ]]
   then
      # Remove the origin/ because it makes the branch too long
      GIT_BRANCH=${GIT_BRANCH#"origin/"}
      # Use updateFile Name above.
      : ;
   elif [[ "${GIT_BRANCH}" =~ build ]]
   then
      GIT_BRANCH=${GIT_BRANCH#"origin/"}
      updateFile="latest-${GIT_BRANCH}.s19"
      shortUpdateFile="${GIT_BRANCH:0:2}.s19"
   else
      echo "Branch ${GIT_BRANCH} does not get deployed"

      #Set build description for builds that don't get uploaded
      GIT_BRANCH=${GIT_BRANCH#"origin/"}
      echo "[LEGRAND_VERSION] ${GIT_BRANCH}"
      exit 0;
   fi

   echo "Firmware Path = ${FIRMWARE_PATH}"
   echo "Release Directory = ${RELEASE_DIR}"

   echo "Checking out the Releases repository"
   ${HEROKU_GIT} clone https://git.heroku.com/legrand-lcm.git ${RELEASE_DIR}

   GIT_VERSION=`git describe --tags --long --abbrev=4`
   RELEASE_DATE_STR=$( git show `git describe --long --tags` -s --format=%ci )

   if [[ ! -n "${RELEASE_NOTES_STR}" ]]
   then
      echo "Getting release notes from commit"
      RELEASE_NOTES_STR=$( git show `git describe --long --tags` -s --format=%B )
   fi

   # Replace double quotes with \"
   RELEASE_NOTES_STR=$( echo "${RELEASE_NOTES_STR}" | sed "s/\"/\\\\\"/g")

   # Replace single quotes with \'
   RELEASE_NOTES_STR=$( echo "${RELEASE_NOTES_STR}" | sed "s/'/\\\'/g" )

   # Replace path delimiters with \/
   RELEASE_NOTES_STR=$( echo "${RELEASE_NOTES_STR}" | sed "s/\//\\\\\//g" )
      BRANCH=$GIT_BRANCH

   IFS=' ' read -ra DATE_ARRAY <<< "${RELEASE_DATE_STR}"
   if [ ${#DATE_ARRAY[@]} -gt 1 ]
   then
      RELEASE_DATE=${DATE_ARRAY[0]}
   fi

   IFS='.|-' read -ra VERSION_ARRAY <<< "${GIT_VERSION}"
   if [ ${#VERSION_ARRAY[@]} -eq 5 ]
   then
      MAJOR_VERSION=${VERSION_ARRAY[0]}
      MINOR_VERSION=${VERSION_ARRAY[1]}
      RELEASE_VERSION=${VERSION_ARRAY[2]}
      NUM_COMMITS=${VERSION_ARRAY[3]}
      COMMIT_HASH=${VERSION_ARRAY[4]}
   else
      echo "The git version has an invalid format. ${GIT_VERSION}"
      exit -1
   fi

   RELEASE="${MAJOR_VERSION}-${MINOR_VERSION}-${RELEASE_VERSION}-${NUM_COMMITS}-${COMMIT_HASH}-LCM1.s19"
   echo "Releasing File ${RELEASE} with update date ${RELEASE_DATE}"

   # Convert the s19 file using the converter tool
   pushd ${RELEASE_DIR} > /dev/null
   echo "Converting the s19 file"
   ruby ${SCRIPT_DIR}/../s19-converter/converter.rb ${FIRMWARE_PATH} > ${RELEASE}
   
   echo "Releasing to ${updateFile}"
   if [ ! -e "${updateFile}" ]
   then
      echo "${updateFile} did not yet exist, creating it now"
      touch ${updateFile}
   fi

   ln -sf ${RELEASE} ${updateFile}
   
   echo "Also Releasing to ${shortUpdateFile}"
   if [ ! -e "${shortUpdateFile}" ]
   then
      echo "${shortUpdateFile} did not yet exist, creating it now"
      touch ${shortUpdateFile}
   fi

   ln -sf ${RELEASE} ${shortUpdateFile}

   # Create the release notes file
   echo "Creating the release notes"
   RELEASE_NOTES=`mktemp releaseNotes.XXX`
   echo "  \"${MAJOR_VERSION}-${MINOR_VERSION}-${RELEASE_VERSION}-${NUM_COMMITS}-${COMMIT_HASH}-LCM1\" => {" > ${RELEASE_NOTES}
   echo "    \"filename\"      => \"${RELEASE}\"," >> ${RELEASE_NOTES}
   echo "    \"version\"       => \"${MAJOR_VERSION}.${MINOR_VERSION}.${RELEASE_VERSION}-${NUM_COMMITS}-${COMMIT_HASH}\"," >> ${RELEASE_NOTES}
   echo "    \"date\"          => \"${RELEASE_DATE}\"," >> ${RELEASE_NOTES}
   echo "    \"branch\"        => \"${BRANCH}\"," >> ${RELEASE_NOTES}
   echo "    "\"notes\""         "=\> \""${RELEASE_NOTES_STR}"\", >> ${RELEASE_NOTES}
   echo "    \"lines\"         => []," >> ${RELEASE_NOTES}
   echo "    \"signatureLines\"=> []," >> ${RELEASE_NOTES}
   echo "  }," >> ${RELEASE_NOTES}
   echo "" >> ${RELEASE_NOTES}
   echo "Release notes created:"
   cat "${RELEASE_NOTES}"

   # Update the ruby script to include the latest release notes
   sed -i "/Generated Release/r "${RELEASE_NOTES}"" updater.rb
   rm -rf ${RELEASE_NOTES}

   # Update heroku with the latest versions
   echo "Adding changes for commit"
   git add .
   git status
   echo "Checking in changes"
   git commit -m "Commit release ${RELEASE}"
   echo "Pushing updates to heroku"
   ${HEROKU_GIT} push origin master


   # Set Build description and release notes link for builds that do get uploaded
   RELEASE_NAME="${MAJOR_VERSION}-${MINOR_VERSION}-${RELEASE_VERSION}-${NUM_COMMITS}-${COMMIT_HASH}-LCM1"
   RELEASE_NOTES_LINK=https://legrand-lcm.herokuapp.com/${RELEASE_NAME}/info
   echo "[LEGRAND_VERSION] <a href=\"${RELEASE_NOTES_LINK}\">$GIT_BRANCH</a>"
   echo GITHUB_STATUS_URL=$RELEASE_NOTES_LINK > ../variables.properties

   # Pop out of the releases directory
   popd > /dev/null

fi

# Pop out of the script directory before exiting
popd > /dev/null
