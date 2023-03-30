#!/bin/bash

HOST=LCM1.local
PORT=2112

function send_packet {

# echo -e: permits ascii codes, with \000 being '\0'
	echo Sending: $1
	echo Receiving:
	echo -ne "$1\000" | nc -w2 $HOST $PORT
	echo
	echo

	sleep 0.25
}


   send_packet '{ "ID": 2, "Service": "ReportSystemProperties"}'

