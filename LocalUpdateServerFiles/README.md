# Local Firmware Update Server Instructions

This directory contains instructions and files necessary to run a LC7001 firmware update server in a local Linux machine.

# Install Dependencies
1. Install Linux dependencies.

```
sudo apt update
sudo apt install ruby ruby-dev expect
```

2. Install Ruby gems that are required for running a web server to host the LC7001 firmware updates locally.

```
sudo gem install thin
sudo gem install sinatra
```


# Make Custom LC7001 Firmware Build

1. Edit Version/CMakeLists.txt: replace @FIRMWARE_VERSION@ and @FIRMWARE_DATE@ with your own values, for example "1.0.0-1-abcde" and "2023-03-31".
2. Edit Embedded/Includes/sysinfo.h and change SYSTEM_INFO_FIRMWARE_BRANCH to "NONE" (otherwise when your build is running it will re-download the official version).
3. Follow the instructions in BuildUsingCmake.md to produce an LCM.s19 file.  You can rename it if you like.


# Install Files in the Local Update Server

1. Create a directory on your Linux machine that will hold your personal LC7001 firmware (.s19) files.
2. Copy the config.ru and updater.rb files into your new directory.
3. Copy your personal LC7001 firmware (.s19) file(s) into the same directory.
4. Edit updater.rb and add details for your personal firmware file(s) within the "dict" data structure.  An example element is below.  More than one element can be included in updater.rb, separated by commas.  NOTE: version must be in the exact format shown below, otherwise the firmware update will fail.

```
  "1-0-0-1-abcde-LCM1" => {
    "filename"      => "1-0-0-1-abcde-LCM1.s19",
    "version"       => "1.0.0-1-abcde",
    "date"          => "2023-03-31",
    "branch"        => "dev",
    "notes"         => "TEST NOTES",
    "lines"         => [],
    "signatureLines"=> [],
  }
```


# Run Local Update Server on Linux

1. Run the thin web server with your configuration.  NOTE: the server must be restarted when updates are made to update.rb.

```
sudo thin -p 80 -R config.ru start
```

# Update the LC7001 from the Local Server

1. Be sure the mobile app is NOT running on the local network during the update process.

2. Trigger the LC7001 to download & install an update from the local server.  In the above examaple, `<firmware name>` would be `1-0-0-1-abcde-LCM1`.  Replace `<firmware name>` with whatever name you gave in your updater.rb file.  If your LC7001 has a local password set, include the `<LC7001 local password>`, otherwise leave it off.

```
./downloadFirmware.sh LCM1.local 2112 <update server IP address> <firmware name> <LC7001 local password>
./installFirmware.sh LCM1.local 2112 <LC7001 local password>
```

