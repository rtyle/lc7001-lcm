HOST=LCM1.local
PORT=2112
USER_ID="f643be3c7c0342738e49f1a74c0eee85"
REFRESH_ID="32e049c75c634d7b84ad12ffd1c0fca1"

function send_packet {

   echo "Sending -> $1"

   receiveStr=$(echo -ne "$1\000" | nc -w2 $HOST $PORT)

   echo "Response -> ${receiveStr}"
}

send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"SamsungBearerId": "'${USER_ID}'", "SamsungRefreshId": "'${REFRESH_ID}'"}}'
