HOST=LCM1.local
PORT=2112
USER_ID="19c2e02bdfb144809fda5d38f8a6c43e"

function send_packet {

   echo "Sending -> $1"

   receiveStr=$(echo -ne "$1\000" | nc -w2 $HOST $PORT)

   echo "Response -> ${receiveStr}"
}

send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"SamsungBearerId": "'${USER_ID}'"}}'
