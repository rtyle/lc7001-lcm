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


send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":15, "PropertyList":{"Name":"4.Rec Room","PowerLevel":40,"RampRate":50}}'
sleep 0.50

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":20, "PropertyList":{"Name":"5.Rec Room","PowerLevel":40,"RampRate":50}}'
sleep 0.50

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":25, "PropertyList":{"Name":"6.Rec Room","PowerLevel":40,"RampRate":50}}'
sleep 0.50

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":30, "PropertyList":{"Name":"7.Rec Room","PowerLevel":40,"RampRate":50}}'
sleep 0.50

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":35, "PropertyList":{"Name":"8.Rec Room","PowerLevel":40,"RampRate":50}}'
sleep 0.50

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":40, "PropertyList":{"Name":"9.Rec Room","PowerLevel":40,"RampRate":50}}'
sleep 0.50

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":45, "PropertyList":{"Name":"10.Rec Room","PowerLevel":40,"RampRate":50}}'
sleep 0.50

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":50, "PropertyList":{"Name":"11.Rec Room","PowerLevel":40,"RampRate":50}}'
sleep 0.50

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":55, "PropertyList":{"Name":"12.Rec Room","PowerLevel":40,"RampRate":50}}'
sleep 0.50

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":60, "PropertyList":{"Name":"13.Rec Room","PowerLevel":40,"RampRate":50}}'
sleep 0.50

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":65, "PropertyList":{"Name":"14.Rec Room","PowerLevel":40,"RampRate":50}}'
sleep 0.50

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":70, "PropertyList":{"Name":"15.Rec Room","PowerLevel":40,"RampRate":50}}'
sleep 0.50

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":75, "PropertyList":{"Name":"16.Rec Room","PowerLevel":40,"RampRate":50}}'



