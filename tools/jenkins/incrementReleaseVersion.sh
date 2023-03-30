#!/bin/bash

# Exit the script on any failure
set -e

# Change into the script directory
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
pushd ${SCRIPT_DIR} > /dev/null

if [ -z "${GIT_BRANCH}" ]
then
   echo "GIT_BRANCH is not defined. Get the branch from the git command"
   GIT_BRANCH=$( git rev-parse --abbrev-ref HEAD )
fi

if [[ ${GIT_BRANCH} == "origin/master" ]]
then
   GIT_VERSION=`git describe --tags --abbrev=0`

   IFS='.|-' read -ra VERSION_ARRAY <<< "${GIT_VERSION}"
   if [ ${#VERSION_ARRAY[@]} -eq 3 ]
   then
      MAJOR_VERSION=${VERSION_ARRAY[0]}
      MINOR_VERSION=${VERSION_ARRAY[1]}
      RELEASE_VERSION=${VERSION_ARRAY[2]}
   else
      echo "The git version has an invalid format. ${GIT_VERSION}"
      exit -1
   fi

   RELEASE_VERSION=$((RELEASE_VERSION+1))
   NEW_TAG="${MAJOR_VERSION}.${MINOR_VERSION}.${RELEASE_VERSION}"
   echo "Creating tag ${NEW_TAG}"
   git tag -a ${NEW_TAG} -m "Tagging Version ${NEW_TAG}"
   git push origin --tags
else
   echo "New tags are only made from the master branch"
   echo
fi

