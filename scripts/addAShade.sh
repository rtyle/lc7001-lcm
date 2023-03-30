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

# Send a set system properties with AddAShade true. Success
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"AddAShade": true}}' '"Status":"Success"'

sleep 0.5
