HOST=LCM1.local
PORT=2112

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

# Send a command with no building ID. Will Return an error
send_packet '{ "ID": 2, "Service": "TriggerRampCommand"}' '"Status":"Error"'
sleep 0.5

# Send a command with a building ID not an integer. Will return an error
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": "abc"}' '"Status":"Error"'
sleep 0.5

# Send a command with a valid building, but no house ID. Will Return an error
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": 12}' '"Status":"Error"'
sleep 0.5

# Send a command with house ID not an integer. Will return an error
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": 12, "HouseID": "def"}' '"Status":"Error"'
sleep 0.5

# Send a command with valid house, but no group ID. Will Return an error
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": 12, "HouseID": 34}' '"Status":"Error"'
sleep 0.5

# Send a command with valid house, group ID not an integer. Will return an error
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": "def"}' '"Status":"Error"'
sleep 0.5

# Send a command with valid house, valid group, and no power level. 
# Will Return an error
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": 567}' '"Status":"Error"'
sleep 0.5

# Send a command with power level not an integer. Will Return an error
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": 567, "PowerLevel": "ghi"}' '"Status":"Error"'
sleep 0.5

# Send a command with power level < 0. Will Return an error
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": 567, "PowerLevel": -1}' '"Status":"Error"'
sleep 0.5

# Send a command with power level > 100. Will Return an error
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": 567, "PowerLevel": 101}' '"Status":"Error"'
sleep 0.5

# Send a command with valid power level, but no device type
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": 567, "PowerLevel": 0}' '"Status":"Error"'
sleep 0.5

# Send a command with device type not an integer
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": 567, "PowerLevel": 0, "DeviceType": "abc"}' '"Status":"Error"'
sleep 0.5

# Send a command with an invalid device type (< Min)
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": 567, "PowerLevel": 0, "DeviceType": 0}' '"Status":"Error"'
sleep 0.5

# Send a command with an invalid device type (> Max)
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": 567, "PowerLevel": 0, "DeviceType": 68}' '"Status":"Error"'
sleep 0.5

# Send a command with power level == 0, device type = Dimmer. Success
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": 567, "PowerLevel": 0, "DeviceType": 65}' '"Status":"Success"'
sleep 0.5

# Send a command with power level == 100, device type = Switch. Success
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": 567, "PowerLevel": 100, "DeviceType": 66}' '"Status":"Success"'
sleep 0.5

# Send a command with power level == 50, device type = Fan Controller. Success
send_packet '{ "ID": 2, "Service": "TriggerRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": 567, "PowerLevel": 50, "DeviceType": 67}' '"Status":"Success"'
