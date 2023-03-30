
HOST=LCM1.local
PORT=2112

function send_packet {

# echo -e: permits ascii codes, with \000 being '\0'
# -w2: wait for no data for two seconds before completing
	echo Sending: $1
	echo Receiving:
	echo -ne "$1\000" | nc -w9 $HOST $PORT
	echo
	echo

	sleep 0.75
}

send_packet '{ "ID": 2, "Service": "ListZones"}'
