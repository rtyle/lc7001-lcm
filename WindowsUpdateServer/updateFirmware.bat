@ECHO OFF
SET address=LCM1.local
SET port=2112

set count=0
for %%x in (%*) do Set /A count+=1

if %count% NEQ 0 (
   SET branch=%1
) else (
   set branch=1-2-17-5-gfe2ef-LCM1
)

echo Getting Local Address
for /f "skip=1 delims={}, " %%A in ('wmic nicconfig get ipaddress') do for /f "tokens=1" %%B in ("%%~A") do set "IP=%%~B"
echo Local IP Address for Update Server is %IP%

ping -n 1 %address% >nul: 2>nul:
if %errorlevel% == 0 (
   echo Able to ping the LC7001. Using MDNS Name
) else (
   echo Could not ping the LC7001
   echo Manually enter the IP Address of the LC7001
   set /p address= 
)

SET data='{\"ID\":2, \"Service\":\"FirmwareUpdate\", \"Host\":\"%IP%\", \"Branch\":\"%branch%\"}'
echo Sending command: %data% to %address%
PowerShell.exe -ExecutionPolicy Bypass -Command "& 'powerShell\sendJSON.ps1' -data %data% -address %address% -port %port%"
PAUSE
