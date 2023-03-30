#!/bin/bash

# Exit the script on any failure
set -e

# Function to print the usage details
usage()
{
   echo
   echo "Usage: `basename $0` options -t <tag> -n <Release notes> -v <Version> [-r]"
   echo "   -t: Tag version used to create the release from (X.X.X)"
   echo "   -v: New version number for the release being created (X.X.X)"
   echo "   -n: Release notes file to use when deploying the release"
   echo "   -r: Optional command to release to the latest-LCM1 branch. If not present, it will be released to latest-testing"
   echo
}

# Validate the input variables using getopts
declare -a tagArray
declare -a versionArray
releaseNotesFile=""
useTesting=true

if ( ! getopts "t:n:v:r" opt); then
   usage
   exit $E_OPTERROR;
fi

while getopts "t:n:v:r" opt; do
   case $opt in
      t)
         OIFS=$IFS
         IFS='.'
         read -ra tagString <<< "$OPTARG"
         for tag in "${tagString[@]}";
         do
            if [[ ${tag} =~ ^-?[0-9]+$ ]]
            then
               tagArray+=(${tag})
            else
               echo "Tag value \"${tag}\" is not an integer"
               exit 1
            fi
         done
         IFS=$OIFS
         ;;
      n)
         # Set the absolute path of the release notes file
         releaseNotesFile="$(readlink -f ${OPTARG})"
         ;;
      v) 
         OIFS=$IFS
         IFS='.'
         read -ra versionString <<< "$OPTARG"
         for version in "${versionString[@]}";
         do
            if [[ ${version} =~ ^-?[0-9]+$ ]]
            then
               versionArray+=(${version})
            else
               echo "Version value \"${version}\" is not an integer"
               exit 1
            fi
         done
         IFS=$OIFS

         ;;
      r)
         useTesting=false
         ;;
      \?)
         echo "Invalid option -$OPTARG"
         exit 1
         ;;
      :)
         echo "Option -$OPTARG requires an argument"
         exit 1
         ;;
   esac
done

# Change into the script directory so git describe works as expected
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
pushd ${SCRIPT_DIR} > /dev/null

if [[ ${#versionArray[@]} -ne 3 ]]; then
   echo "Version expects 3 parameters X.X.X"
   usage
   exit 1
elif [[ ${#tagArray[@]} -ne 3 ]]; then
   echo "Tag expects 3 parameters X.X.X"
   usage
   exit 1
elif [[ ! -r ${releaseNotesFile} ]]; then
   echo "File \"${releaseNotesFile}\" does not exist or is not readable"
   exit 1
else
   echo "Creating Version ${versionArray[0]}.${versionArray[1]}.${versionArray[2]} from tag ${tagArray[0]}.${tagArray[1]}.${tagArray[2]}"
   echo "Using release notes file $releaseNotesFile"

   if [[ ${useTesting} == true ]]
   then
      echo "Releasing to the testing branch"
   else
      echo "Releasing to the LCM1 branch"
   fi

   # Change to the top level LCM directory
   pushd ../

   # Fetch the latest from git
   echo -e "\ngit fetch"
   git fetch

   # Store the current branch
   currentBranch="$(git rev-parse --abbrev-ref HEAD)"

   # Checkout the tag to build the release from
   tag=${tagArray[0]}.${tagArray[1]}.${tagArray[2]}
   echo -e "\nSwitching from ${currentBranch} to ${tag}"
   echo -e "\ngit checkout ${tag}"
   git checkout ${tag}

   # Clear the working directory in interactive mode
   echo -e "\ngit clean -d -x -i"
   git clean -d -x -i

   # Tag the release with the provided tag
   release=${versionArray[0]}.${versionArray[1]}.${versionArray[2]}
   echo -e "\ngit tag -a ${release} -m \"Creating tag for version ${release}"
   git tag -a ${release} -m "Creating tag for version ${release}"
   git push origin --tags

   # Build the firmware at this release point to get the correct version number
   ./tools/jenkins/build.sh

   # Need to pretend like it is running from the master branch in order
   # to deploy firmware. This check is made in the firmwareDeploy.sh script
   # so jenkins only releases firmware from the master branch
   export GIT_BRANCH=origin/master

   # Deploy this version of firmware to heroku with the supplied release
   # notes and release branch to use
   if [[ ${useTesting} == true ]]
   then
      ./tools/jenkins/firmwareDeploy.sh -n ${releaseNotesFile} -t
   else
      ./tools/jenkins/firmwareDeploy.sh -n ${releaseNotesFile} -r
   fi

   # Unset the git branch variable
   unset GIT_BRANCH

   # Switch back to the original branch
   echo -e "\nSwitching back to ${currentBranch}"
   echo -e "\ngit checkout ${currentBranch}"
   git checkout "${currentBranch}"

   # Switch back to the script directory
   popd
fi

# Pop out of the script directory before exiting
popd > /dev/null
