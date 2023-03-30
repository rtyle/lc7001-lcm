Setup Virtual Machine
======================

Virtual Machine Creation
-------------------------
1. Create a new virtual machine using 64bit Ubuntu 
    * At least 2048 MB RAM
    * At least 12 GB Hard Drive
2. Install Linux as usual

Install Required Packages
-------------------------
1. Update apt
    * <code>sudo apt-get update</code>
2. Install virtualbox guest dkms to get correct screen resolution
    * <code>sudo apt-get install virtualbox-guest-dkms</code>
3. Install Git
    * <code>sudo apt-get install git</code>
4. Install cmake
    * <code>sudo apt-get install cmake</code>
5. Install gcc
    * <code>sudo apt-get install gcc-4.8</code>
    * <code>sudo apt-get install g++</code>
    * <code>sudo apt-get install g++-4.8</code>
6. Install avahi
    * <code>sudo apt-get install libavahi-common-dev</code>
    * <code>sudo apt-get install libavahi-client-dev</code>
7. Install boost test framework
    * <code>sudo apt-get install libboost-test-dev</code>
8. Install sqlite3
    * <code>sudo apt-get install sqlite3</code>
9. Install 32 bit library support if it is a 64 bit machine
    * <code>sudo apt-get install lib32z1</code>
    * <code>sudo apt-get install lib32ncurses5</code>
1. Install Ruby
    * <code>sudo apt-get install ruby</code>

Install Packages for KDS
------------------------
1. Install Java
    * <code>sudo apt-get install default-jre</code>
    * <code>sudo apt-get install default-jdk</code>
3. Install gksu
    * <code>sudo apt-get install gksu</code>
