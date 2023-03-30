HOST=LCM1.local
PORT=2112


function send_packet {
   receiveStr=$(echo -ne "$1\000")
   echo "${receiveStr}, $2"
}

# Send a command with no building ID. Will Return an error
send_packet '{ "ID": 2, "Service": "RoomRampCommand"}' '"Status":"Error"'
sleep 0.5

# Send a command with a building ID not an integer. Will return an error
send_packet '{ "ID": 2, "Service": "RoomRampCommand", "BuildingID": "abc"}' '"Status":"Error"'
sleep 0.5

# Send a command with a valid building, but no house ID. Will Return an error
send_packet '{ "ID": 2, "Service": "RoomRampCommand", "BuildingID": 12}' '"Status":"Error"'
sleep 0.5

# Send a command with house ID not an integer. Will return an error
send_packet '{ "ID": 2, "Service": "RoomRampCommand", "BuildingID": 12, "HouseID": "def"}' '"Status":"Error"'
sleep 0.5

# Send a command with valid house, but no group ID. Will Return an error
send_packet '{ "ID": 2, "Service": "RoomRampCommand", "BuildingID": 12, "HouseID": 34}' '"Status":"Error"'
sleep 0.5

# Send a command with valid house, group ID not an integer. Will return an error
send_packet '{ "ID": 2, "Service": "RoomRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": "def"}' '"Status":"Error"'
sleep 0.5

# Send a command with valid house, valid group, and no power level. 
# Will Return an error
send_packet '{ "ID": 2, "Service": "RoomRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": 567}' '"Status":"Error"'
sleep 0.5

# Send a command with power level not an integer. Will Return an error
send_packet '{ "ID": 2, "Service": "RoomRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": 567, "PowerLevel": "ghi"}' '"Status":"Error"'
sleep 0.5

# Send a command with power level < 0. Will Return an error
send_packet '{ "ID": 2, "Service": "RoomRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": 567, "PowerLevel": -1}' '"Status":"Error"'
sleep 0.5

# Send a command with power level > 100. Will Return an error
send_packet '{ "ID": 2, "Service": "RoomRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": 567, "PowerLevel": 101}' '"Status":"Error"'
sleep 0.5

# Send a command with valid power level, but no device type
send_packet '{ "ID": 2, "Service": "RoomRampCommand", "BuildingID": 12, "HouseID": 34, "GroupID": 567, "RoomID": 1,  "PowerLevel": 0}' '"Status":"Error"'
sleep 0.5



