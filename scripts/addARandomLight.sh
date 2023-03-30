HOST=LCM1.local
PORT=2112
GROUPID=$((RANDOM%100+1))
function send_packet {

# echo -e: permits ascii codes, with \000 being '\0'
# -w2: wait for no data for two seconds before completing
	#echo Sending: $1
   receiveStr=$(echo -ne "$1\000" | nc -w2 $HOST $PORT)

   if [[ ${receiveStr} == *"$2"* ]]
   then
      echo "Passed: $1"
   else
      echo "Unexpected Result Returned: ${receiveStr}, $2"
   fi
}

# Send a trigger ramp command with a matching house ID to the default zones
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": 1, "HouseID": 136, "GroupID":'"$GROUPID"', "PowerLevel": 100, "DeviceType": 65}' '"Status":"Success"'
sleep 0.5
