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

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":1, "PropertyList":{"Name":"","PowerLevel":55,"RampRate":45,"Power":true,}}'

sleep 2

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":1, "PropertyList":{"Name":"zone 1","PowerLevel":155,"RampRate":45,"Power":true}}'

sleep 2

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":1, "PropertyList":{"Name":"zone 1","PowerLevel":55,"RampRate":145,"Power":true}}'

sleep 2

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":1, "PropertyList":{"Name":"zone 1","PowerLevel":50,"RampRate":45,"Power":hello}}'

sleep 2

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":1, "PropertyList":{"Name":"zone 1","PowerLevel":50,"RampRate":45,"Power":true}}'

sleep 20

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":1, "PropertyList":{"Name":"zone 1","PowerLevel":50,"RampRate":45,"Power":true}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":2, "PropertyList":{"Name":"zone 2","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":10, "PropertyList":{"Name":"zone 10","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":11, "PropertyList":{"Name":"zone 11","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":12, "PropertyList":{"Name":"zone 12","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":13, "PropertyList":{"Name":"zone 13","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":14, "PropertyList":{"Name":"zone 14","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":15, "PropertyList":{"Name":"zone 15","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":16, "PropertyList":{"Name":"zone 16","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":17, "PropertyList":{"Name":"zone 17","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":18, "PropertyList":{"Name":"zone 18","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":19, "PropertyList":{"Name":"zone 19","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":20, "PropertyList":{"Name":"zone 20","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":21, "PropertyList":{"Name":"zone 21","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":22, "PropertyList":{"Name":"zone 22","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":23, "PropertyList":{"Name":"zone 23","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":24, "PropertyList":{"Name":"zone 24","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":25, "PropertyList":{"Name":"zone 25","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":26, "PropertyList":{"Name":"zone 26","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":27, "PropertyList":{"Name":"zone 27","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":28, "PropertyList":{"Name":"zone 28","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":29, "PropertyList":{"Name":"zone 29","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":30, "PropertyList":{"Name":"zone 30","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":31, "PropertyList":{"Name":"zone 31","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1
send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":32, "PropertyList":{"Name":"zone 32","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":33, "PropertyList":{"Name":"zone 33","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":34, "PropertyList":{"Name":"zone 34","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":35, "PropertyList":{"Name":"zone 35","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":36, "PropertyList":{"Name":"zone 36","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":37, "PropertyList":{"Name":"zone 37","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":38, "PropertyList":{"Name":"zone 38","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":39, "PropertyList":{"Name":"zone 39","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":40, "PropertyList":{"Name":"zone 40","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":41, "PropertyList":{"Name":"zone 41","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":42, "PropertyList":{"Name":"zone 42","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":43, "PropertyList":{"Name":"zone 43","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":44, "PropertyList":{"Name":"zone 44","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":45, "PropertyList":{"Name":"zone 45","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":46, "PropertyList":{"Name":"zone 46","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":47, "PropertyList":{"Name":"zone 47","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":48, "PropertyList":{"Name":"zone 48","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":49, "PropertyList":{"Name":"zone 49","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":50, "PropertyList":{"Name":"zone 50","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":51, "PropertyList":{"Name":"zone 51","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":52, "PropertyList":{"Name":"zone 52","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":53, "PropertyList":{"Name":"zone 53","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":54, "PropertyList":{"Name":"zone 54","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":55, "PropertyList":{"Name":"zone 55","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":56, "PropertyList":{"Name":"zone 56","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":57, "PropertyList":{"Name":"zone 57","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":58, "PropertyList":{"Name":"zone 58","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":59, "PropertyList":{"Name":"zone 59","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":60, "PropertyList":{"Name":"zone 60","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":61, "PropertyList":{"Name":"zone 61","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":62, "PropertyList":{"Name":"zone 62","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":63, "PropertyList":{"Name":"zone 63","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":64, "PropertyList":{"Name":"zone 64","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":65, "PropertyList":{"Name":"zone 65","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":66, "PropertyList":{"Name":"zone 66","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":67, "PropertyList":{"Name":"zone 67","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":68, "PropertyList":{"Name":"zone 68","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":69, "PropertyList":{"Name":"zone 69","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":70, "PropertyList":{"Name":"zone 70","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":71, "PropertyList":{"Name":"zone 71","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":72, "PropertyList":{"Name":"zone 72","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":73, "PropertyList":{"Name":"zone 73","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":74, "PropertyList":{"Name":"zone 74","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":75, "PropertyList":{"Name":"zone 75","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":76, "PropertyList":{"Name":"zone 76","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":77, "PropertyList":{"Name":"zone 77","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":78, "PropertyList":{"Name":"zone 78","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":79, "PropertyList":{"Name":"zone 79","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":80, "PropertyList":{"Name":"zone 80","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":81, "PropertyList":{"Name":"zone 81","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":82, "PropertyList":{"Name":"zone 82","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":83, "PropertyList":{"Name":"zone 83","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":84, "PropertyList":{"Name":"zone 84","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":85, "PropertyList":{"Name":"zone 85","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":86, "PropertyList":{"Name":"zone 86","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":87, "PropertyList":{"Name":"zone 87","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":88, "PropertyList":{"Name":"zone 88","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":89, "PropertyList":{"Name":"zone 89","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":90, "PropertyList":{"Name":"zone 90","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":91, "PropertyList":{"Name":"zone 91","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":92, "PropertyList":{"Name":"zone 92","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":93, "PropertyList":{"Name":"zone 93","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":94, "PropertyList":{"Name":"zone 94","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":95, "PropertyList":{"Name":"zone 95","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":96, "PropertyList":{"Name":"zone 96","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":97, "PropertyList":{"Name":"zone 97","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":98, "PropertyList":{"Name":"zone 98","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":99, "PropertyList":{"Name":"zone 99","PowerLevel":50,"RampRate":45,"Power":false}}'
sleep 1

send_packet '{ "ID": 2, "Service": "SetZoneProperties", "ZID":100, "PropertyList":{"Name":"zone 100","PowerLevel":50,"RampRate":45,"Power":false}}'

sleep 1
send_packet '{ "ID": 2, "Service": "ReportZoneProperties", "ZID":100}'

sleep 2

send_packet '{ "ID": 2, "Service": "ReportZoneProperties", "ZID":88}'

sleep 2

send_packet '{ "ID": 2, "Service": "ReportZoneProperties", "ZID":1}'

sleep 2


send_packet '{ "ID": 2, "Service": "SetSceneProperties", "SID":0, "PropertyList":{ "Name":"Dinner Scene", "ZoneList":[{"ZID":185,"Lvl":25,"RR":100},{"ZID":1,"Lvl":50,"RR":75},{"ZID":2,"Lvl":50,"RR":75},{"ZID":3,"Lvl":50,"RR":75},{"ZID":4,"Lvl":50,"RR":75},{"ZID":5,"Lvl":50,"RR":75},{"ZID":6,"Lvl":50,"RR":75},{"ZID":7,"Lvl":50,"RR":75},{"ZID":8,"Lvl":50,"RR":75},{"ZID":9,"Lvl":50,"RR":75},{"ZID":10,"Lvl":50,"RR":75},{"ZID":11,"Lvl":50,"RR":75},{"ZID":12,"Lvl":50,"RR":75},{"ZID":13,"Lvl":50,"RR":75},{"ZID":14,"Lvl":50,"RR":75},{"ZID":15,"Lvl":50,"RR":75},{"ZID":16,"Lvl":50,"RR":75},{"ZID":17,"Lvl":50,"RR":75},{"ZID":18,"Lvl":50,"RR":75},{"ZID":19,"Lvl":50,"RR":75},{"ZID":20,"Lvl":50,"RR":75},{"ZID":21,"Lvl":50,"RR":75},{"ZID":22,"Lvl":50,"RR":75},{"ZID":23,"Lvl":50,"RR":75},{"ZID":24,"Lvl":50,"RR":75},{"ZID":25,"Lvl":50,"RR":75},{"ZID":26,"Lvl":50,"RR":75},{"ZID":27,"Lvl":50,"RR":75},{"ZID":28,"Lvl":50,"RR":75},{"ZID":29,"Lvl":50,"RR":75},{"ZID":30,"Lvl":50,"RR":75},{"ZID":31,"Lvl":50,"RR":75},{"ZID":32,"Lvl":50,"RR":75},{"ZID":33,"Lvl":50,"RR":75},{"ZID":34,"Lvl":50,"RR":75},{"ZID":35,"Lvl":50,"RR":75},{"ZID":36,"Lvl":50,"RR":75},{"ZID":37,"Lvl":50,"RR":75},{"ZID":38,"Lvl":50,"RR":75},{"ZID":39,"Lvl":50,"RR":75},{"ZID":40,"Lvl":50,"RR":75},{"ZID":41,"Lvl":50,"RR":75},{"ZID":42,"Lvl":50,"RR":75},{"ZID":43,"Lvl":50,"RR":75},{"ZID":44,"Lvl":50,"RR":75},{"ZID":45,"Lvl":50,"RR":75},{"ZID":46,"Lvl":50,"RR":75},{"ZID":47,"Lvl":50,"RR":75},{"ZID":48,"Lvl":50,"RR":75},{"ZID":49,"Lvl":50,"RR":75},{"ZID":50,"Lvl":50,"RR":75},{"ZID":51,"Lvl":50,"RR":75},{"ZID":52,"Lvl":50,"RR":75},{"ZID":53,"Lvl":50,"RR":75},{"ZID":54,"Lvl":50,"RR":75},{"ZID":55,"Lvl":50,"RR":75},{"ZID":56,"Lvl":50,"RR":75},{"ZID":57,"Lvl":50,"RR":75},{"ZID":58,"Lvl":50,"RR":75},{"ZID":59,"Lvl":50,"RR":75},{"ZID":60,"Lvl":50,"RR":75},{"ZID":61,"Lvl":50,"RR":75},{"ZID":62,"Lvl":50,"RR":75},{"ZID":63,"Lvl":50,"RR":75},{"ZID":64,"Lvl":50,"RR":75},{"ZID":65,"Lvl":50,"RR":75},{"ZID":66,"Lvl":50,"RR":75},{"ZID":67,"Lvl":50,"RR":75},{"ZID":68,"Lvl":50,"RR":75},{"ZID":69,"Lvl":50,"RR":75},{"ZID":70,"Lvl":50,"RR":75},{"ZID":71,"Lvl":50,"RR":75},{"ZID":72,"Lvl":50,"RR":75},{"ZID":73,"Lvl":50,"RR":75},{"ZID":74,"Lvl":50,"RR":75},{"ZID":75,"Lvl":50,"RR":75},{"ZID":76,"Lvl":50,"RR":75},{"ZID":77,"Lvl":50,"RR":75},{"ZID":78,"Lvl":50,"RR":75},{"ZID":79,"Lvl":50,"RR":75},{"ZID":80,"Lvl":50,"RR":75},{"ZID":81,"Lvl":50,"RR":75},{"ZID":82,"Lvl":50,"RR":75},{"ZID":83,"Lvl":50,"RR":75},{"ZID":84,"Lvl":50,"RR":75},{"ZID":85,"Lvl":50,"RR":75},{"ZID":86,"Lvl":50,"RR":75},{"ZID":87,"Lvl":50,"RR":75},{"ZID":88,"Lvl":50,"RR":75},{"ZID":89,"Lvl":50,"RR":75},{"ZID":90,"Lvl":50,"RR":75},{"ZID":91,"Lvl":50,"RR":75},{"ZID":92,"Lvl":50,"RR":75},{"ZID":93,"Lvl":50,"RR":75},{"ZID":94,"Lvl":50,"RR":75},{"ZID":95,"Lvl":50,"RR":75},{"ZID":96,"Lvl":50,"RR":75},{"ZID":97,"Lvl":50,"RR":75},{"ZID":98,"Lvl":50,"RR":75},{"ZID":99,"Lvl":99,"RR":99}], "TriggerTime":554433,"Frequency":1,"TriggerType":0,"Delta":20} }'

sleep 2

send_packet '{ "ID": 2, "Service": "SetSceneProperties", "SID":1, "PropertyList":{ "Name":"Dinner Scene", "ZoneList":[{"ZID":0,"Lvl":25,"RR":100},{"ZID":1,"Lvl":50,"RR":75},{"ZID":2,"Lvl":50,"RR":75},{"ZID":3,"Lvl":50,"RR":75},{"ZID":4,"Lvl":50,"RR":75},{"ZID":5,"Lvl":50,"RR":75},{"ZID":1,"Lvl":50,"RR":75},{"ZID":7,"Lvl":50,"RR":75},{"ZID":8,"Lvl":50,"RR":75},{"ZID":9,"Lvl":50,"RR":75},{"ZID":10,"Lvl":50,"RR":75},{"ZID":11,"Lvl":50,"RR":75},{"ZID":12,"Lvl":50,"RR":75},{"ZID":13,"Lvl":50,"RR":75},{"ZID":14,"Lvl":50,"RR":75},{"ZID":15,"Lvl":50,"RR":75},{"ZID":16,"Lvl":50,"RR":75},{"ZID":17,"Lvl":50,"RR":75},{"ZID":18,"Lvl":50,"RR":75},{"ZID":19,"Lvl":50,"RR":75},{"ZID":20,"Lvl":50,"RR":75},{"ZID":21,"Lvl":50,"RR":75},{"ZID":22,"Lvl":50,"RR":75},{"ZID":23,"Lvl":50,"RR":75},{"ZID":24,"Lvl":50,"RR":75},{"ZID":25,"Lvl":50,"RR":75},{"ZID":26,"Lvl":50,"RR":75},{"ZID":27,"Lvl":50,"RR":75},{"ZID":28,"Lvl":50,"RR":75},{"ZID":29,"Lvl":50,"RR":75},{"ZID":30,"Lvl":50,"RR":75},{"ZID":31,"Lvl":50,"RR":75},{"ZID":32,"Lvl":50,"RR":75},{"ZID":33,"Lvl":50,"RR":75},{"ZID":34,"Lvl":50,"RR":75},{"ZID":35,"Lvl":50,"RR":75},{"ZID":36,"Lvl":50,"RR":75},{"ZID":37,"Lvl":50,"RR":75},{"ZID":38,"Lvl":50,"RR":75},{"ZID":39,"Lvl":50,"RR":75},{"ZID":40,"Lvl":50,"RR":75},{"ZID":41,"Lvl":50,"RR":75},{"ZID":42,"Lvl":50,"RR":75},{"ZID":43,"Lvl":50,"RR":75},{"ZID":44,"Lvl":50,"RR":75},{"ZID":45,"Lvl":50,"RR":75},{"ZID":46,"Lvl":50,"RR":75},{"ZID":47,"Lvl":50,"RR":75},{"ZID":48,"Lvl":50,"RR":75},{"ZID":49,"Lvl":50,"RR":75},{"ZID":50,"Lvl":50,"RR":75},{"ZID":51,"Lvl":50,"RR":75},{"ZID":52,"Lvl":50,"RR":75},{"ZID":53,"Lvl":50,"RR":75},{"ZID":54,"Lvl":50,"RR":75},{"ZID":55,"Lvl":50,"RR":75},{"ZID":56,"Lvl":50,"RR":75},{"ZID":57,"Lvl":50,"RR":75},{"ZID":58,"Lvl":50,"RR":75},{"ZID":59,"Lvl":50,"RR":75},{"ZID":60,"Lvl":50,"RR":75},{"ZID":61,"Lvl":50,"RR":75},{"ZID":62,"Lvl":50,"RR":75},{"ZID":63,"Lvl":50,"RR":75},{"ZID":64,"Lvl":50,"RR":75},{"ZID":65,"Lvl":50,"RR":75},{"ZID":66,"Lvl":50,"RR":75},{"ZID":67,"Lvl":50,"RR":75},{"ZID":68,"Lvl":50,"RR":75},{"ZID":69,"Lvl":50,"RR":75},{"ZID":70,"Lvl":50,"RR":75},{"ZID":71,"Lvl":50,"RR":75},{"ZID":72,"Lvl":50,"RR":75},{"ZID":73,"Lvl":50,"RR":75},{"ZID":74,"Lvl":50,"RR":75},{"ZID":75,"Lvl":50,"RR":75},{"ZID":76,"Lvl":50,"RR":75},{"ZID":77,"Lvl":50,"RR":75},{"ZID":78,"Lvl":50,"RR":75},{"ZID":79,"Lvl":50,"RR":75},{"ZID":80,"Lvl":50,"RR":75},{"ZID":81,"Lvl":50,"RR":75},{"ZID":82,"Lvl":50,"RR":75},{"ZID":83,"Lvl":50,"RR":75},{"ZID":84,"Lvl":50,"RR":75},{"ZID":85,"Lvl":50,"RR":75},{"ZID":86,"Lvl":50,"RR":75},{"ZID":87,"Lvl":50,"RR":75},{"ZID":88,"Lvl":50,"RR":75},{"ZID":89,"Lvl":50,"RR":75},{"ZID":90,"Lvl":50,"RR":75},{"ZID":91,"Lvl":50,"RR":75},{"ZID":92,"Lvl":50,"RR":75},{"ZID":93,"Lvl":50,"RR":75},{"ZID":94,"Lvl":50,"RR":75},{"ZID":95,"Lvl":50,"RR":75},{"ZID":96,"Lvl":50,"RR":75},{"ZID":97,"Lvl":50,"RR":75},{"ZID":98,"Lvl":50,"RR":75},{"ZID":99,"Lvl":99,"RR":99}], "TriggerTime":554433,"Frequency":1,"TriggerType":0,"Delta":20} }'

sleep 2

send_packet '{ "ID": 2, "Service": "SetSceneProperties", "SID":2, "PropertyList":{ "Name":"Dinner Scene", "ZoneList":[{"ZID":0,"Lvl":125,"RR":100},{"ZID":1,"Lvl":50,"RR":75},{"ZID":2,"Lvl":50,"RR":75},{"ZID":3,"Lvl":50,"RR":75},{"ZID":4,"Lvl":50,"RR":75},{"ZID":5,"Lvl":50,"RR":75},{"ZID":6,"Lvl":50,"RR":75},{"ZID":7,"Lvl":50,"RR":75},{"ZID":8,"Lvl":50,"RR":75},{"ZID":9,"Lvl":50,"RR":75},{"ZID":10,"Lvl":50,"RR":75},{"ZID":11,"Lvl":50,"RR":75},{"ZID":12,"Lvl":50,"RR":75},{"ZID":13,"Lvl":50,"RR":75},{"ZID":14,"Lvl":50,"RR":75},{"ZID":15,"Lvl":50,"RR":75},{"ZID":16,"Lvl":50,"RR":75},{"ZID":17,"Lvl":50,"RR":75},{"ZID":18,"Lvl":50,"RR":75},{"ZID":19,"Lvl":50,"RR":75},{"ZID":20,"Lvl":50,"RR":75},{"ZID":21,"Lvl":50,"RR":75},{"ZID":22,"Lvl":50,"RR":75},{"ZID":23,"Lvl":50,"RR":75},{"ZID":24,"Lvl":50,"RR":75},{"ZID":25,"Lvl":50,"RR":75},{"ZID":26,"Lvl":50,"RR":75},{"ZID":27,"Lvl":50,"RR":75},{"ZID":28,"Lvl":50,"RR":75},{"ZID":29,"Lvl":50,"RR":75},{"ZID":30,"Lvl":50,"RR":75},{"ZID":31,"Lvl":50,"RR":75},{"ZID":32,"Lvl":50,"RR":75},{"ZID":33,"Lvl":50,"RR":75},{"ZID":34,"Lvl":50,"RR":75},{"ZID":35,"Lvl":50,"RR":75},{"ZID":36,"Lvl":50,"RR":75},{"ZID":37,"Lvl":50,"RR":75},{"ZID":38,"Lvl":50,"RR":75},{"ZID":39,"Lvl":50,"RR":75},{"ZID":40,"Lvl":50,"RR":75},{"ZID":41,"Lvl":50,"RR":75},{"ZID":42,"Lvl":50,"RR":75},{"ZID":43,"Lvl":50,"RR":75},{"ZID":44,"Lvl":50,"RR":75},{"ZID":45,"Lvl":50,"RR":75},{"ZID":46,"Lvl":50,"RR":75},{"ZID":47,"Lvl":50,"RR":75},{"ZID":48,"Lvl":50,"RR":75},{"ZID":49,"Lvl":50,"RR":75},{"ZID":50,"Lvl":50,"RR":75},{"ZID":51,"Lvl":50,"RR":75},{"ZID":52,"Lvl":50,"RR":75},{"ZID":53,"Lvl":50,"RR":75},{"ZID":54,"Lvl":50,"RR":75},{"ZID":55,"Lvl":50,"RR":75},{"ZID":56,"Lvl":50,"RR":75},{"ZID":57,"Lvl":50,"RR":75},{"ZID":58,"Lvl":50,"RR":75},{"ZID":59,"Lvl":50,"RR":75},{"ZID":60,"Lvl":50,"RR":75},{"ZID":61,"Lvl":50,"RR":75},{"ZID":62,"Lvl":50,"RR":75},{"ZID":63,"Lvl":50,"RR":75},{"ZID":64,"Lvl":50,"RR":75},{"ZID":65,"Lvl":50,"RR":75},{"ZID":66,"Lvl":50,"RR":75},{"ZID":67,"Lvl":50,"RR":75},{"ZID":68,"Lvl":50,"RR":75},{"ZID":69,"Lvl":50,"RR":75},{"ZID":70,"Lvl":50,"RR":75},{"ZID":71,"Lvl":50,"RR":75},{"ZID":72,"Lvl":50,"RR":75},{"ZID":73,"Lvl":50,"RR":75},{"ZID":74,"Lvl":50,"RR":75},{"ZID":75,"Lvl":50,"RR":75},{"ZID":76,"Lvl":50,"RR":75},{"ZID":77,"Lvl":50,"RR":75},{"ZID":78,"Lvl":50,"RR":75},{"ZID":79,"Lvl":50,"RR":75},{"ZID":80,"Lvl":50,"RR":75},{"ZID":81,"Lvl":50,"RR":75},{"ZID":82,"Lvl":50,"RR":75},{"ZID":83,"Lvl":50,"RR":75},{"ZID":84,"Lvl":50,"RR":75},{"ZID":85,"Lvl":50,"RR":75},{"ZID":86,"Lvl":50,"RR":75},{"ZID":87,"Lvl":50,"RR":75},{"ZID":88,"Lvl":50,"RR":75},{"ZID":89,"Lvl":50,"RR":75},{"ZID":90,"Lvl":50,"RR":75},{"ZID":91,"Lvl":50,"RR":75},{"ZID":92,"Lvl":50,"RR":75},{"ZID":93,"Lvl":50,"RR":75},{"ZID":94,"Lvl":50,"RR":75},{"ZID":95,"Lvl":50,"RR":75},{"ZID":96,"Lvl":50,"RR":75},{"ZID":97,"Lvl":50,"RR":75},{"ZID":98,"Lvl":50,"RR":75},{"ZID":99,"Lvl":99,"RR":99}], "TriggerTime":554433,"Frequency":0,"TriggerType":2,"Delta":20} }'

sleep 2

send_packet '{ "ID": 2, "Service": "SetSceneProperties", "SID":3, "PropertyList":{ "Name":"Dinner Scene", "ZoneList":[{"ZID":0,"Lvl":25,"RR":100},{"ZID":1,"Lvl":50,"RR":75},{"ZID":2,"Lvl":50,"RR":75},{"ZID":3,"Lvl":50,"RR":75},{"ZID":4,"Lvl":50,"RR":75},{"ZID":5,"Lvl":50,"RR":175},{"ZID":6,"Lvl":50,"RR":75},{"ZID":7,"Lvl":50,"RR":75},{"ZID":8,"Lvl":50,"RR":75},{"ZID":9,"Lvl":50,"RR":75},{"ZID":10,"Lvl":50,"RR":75},{"ZID":11,"Lvl":50,"RR":75},{"ZID":12,"Lvl":50,"RR":75},{"ZID":13,"Lvl":50,"RR":75},{"ZID":14,"Lvl":50,"RR":75},{"ZID":15,"Lvl":50,"RR":75},{"ZID":16,"Lvl":50,"RR":75},{"ZID":17,"Lvl":50,"RR":75},{"ZID":18,"Lvl":50,"RR":75},{"ZID":19,"Lvl":50,"RR":75},{"ZID":20,"Lvl":50,"RR":75},{"ZID":21,"Lvl":50,"RR":75},{"ZID":22,"Lvl":50,"RR":75},{"ZID":23,"Lvl":50,"RR":75},{"ZID":24,"Lvl":50,"RR":75},{"ZID":25,"Lvl":50,"RR":75},{"ZID":26,"Lvl":50,"RR":75},{"ZID":27,"Lvl":50,"RR":75},{"ZID":28,"Lvl":50,"RR":75},{"ZID":29,"Lvl":50,"RR":75},{"ZID":30,"Lvl":50,"RR":75},{"ZID":31,"Lvl":50,"RR":75},{"ZID":32,"Lvl":50,"RR":75},{"ZID":33,"Lvl":50,"RR":75},{"ZID":34,"Lvl":50,"RR":75},{"ZID":35,"Lvl":50,"RR":75},{"ZID":36,"Lvl":50,"RR":75},{"ZID":37,"Lvl":50,"RR":75},{"ZID":38,"Lvl":50,"RR":75},{"ZID":39,"Lvl":50,"RR":75},{"ZID":40,"Lvl":50,"RR":75},{"ZID":41,"Lvl":50,"RR":75},{"ZID":42,"Lvl":50,"RR":75},{"ZID":43,"Lvl":50,"RR":75},{"ZID":44,"Lvl":50,"RR":75},{"ZID":45,"Lvl":50,"RR":75},{"ZID":46,"Lvl":50,"RR":75},{"ZID":47,"Lvl":50,"RR":75},{"ZID":48,"Lvl":50,"RR":75},{"ZID":49,"Lvl":50,"RR":75},{"ZID":50,"Lvl":50,"RR":75},{"ZID":51,"Lvl":50,"RR":75},{"ZID":52,"Lvl":50,"RR":75},{"ZID":53,"Lvl":50,"RR":75},{"ZID":54,"Lvl":50,"RR":75},{"ZID":55,"Lvl":50,"RR":75},{"ZID":56,"Lvl":50,"RR":75},{"ZID":57,"Lvl":50,"RR":75},{"ZID":58,"Lvl":50,"RR":75},{"ZID":59,"Lvl":50,"RR":75},{"ZID":60,"Lvl":50,"RR":75},{"ZID":61,"Lvl":50,"RR":75},{"ZID":62,"Lvl":50,"RR":75},{"ZID":63,"Lvl":50,"RR":75},{"ZID":64,"Lvl":50,"RR":75},{"ZID":65,"Lvl":50,"RR":75},{"ZID":66,"Lvl":50,"RR":75},{"ZID":67,"Lvl":50,"RR":75},{"ZID":68,"Lvl":50,"RR":75},{"ZID":69,"Lvl":50,"RR":75},{"ZID":70,"Lvl":50,"RR":75},{"ZID":71,"Lvl":50,"RR":75},{"ZID":72,"Lvl":50,"RR":75},{"ZID":73,"Lvl":50,"RR":75},{"ZID":74,"Lvl":50,"RR":75},{"ZID":75,"Lvl":50,"RR":75},{"ZID":76,"Lvl":50,"RR":75},{"ZID":77,"Lvl":50,"RR":75},{"ZID":78,"Lvl":50,"RR":75},{"ZID":79,"Lvl":50,"RR":75},{"ZID":80,"Lvl":50,"RR":75},{"ZID":81,"Lvl":50,"RR":75},{"ZID":82,"Lvl":50,"RR":75},{"ZID":83,"Lvl":50,"RR":75},{"ZID":84,"Lvl":50,"RR":75},{"ZID":85,"Lvl":50,"RR":75},{"ZID":86,"Lvl":50,"RR":75},{"ZID":87,"Lvl":50,"RR":75},{"ZID":88,"Lvl":50,"RR":75},{"ZID":89,"Lvl":50,"RR":75},{"ZID":90,"Lvl":50,"RR":75},{"ZID":91,"Lvl":50,"RR":75},{"ZID":92,"Lvl":50,"RR":75},{"ZID":93,"Lvl":50,"RR":75},{"ZID":94,"Lvl":50,"RR":75},{"ZID":95,"Lvl":50,"RR":75},{"ZID":96,"Lvl":50,"RR":75},{"ZID":97,"Lvl":50,"RR":75},{"ZID":98,"Lvl":50,"RR":75},{"ZID":99,"Lvl":99,"RR":99}], "TriggerTime":554433,"Frequency":1,"TriggerType":1,"Delta":20} }'

sleep 2


send_packet '{ "ID": 2, "Service": "SetSceneProperties", "SID":4, "PropertyList":{ "Name":"Dinner Scene", "ZoneList":[{"ZID":0,"Lvl":25,"RR":100},{"ZID":1,"Lvl":50,"RR":75},{"ZID":2,"Lvl":50,"RR":75},{"ZID":3,"Lvl":50,"RR":75},{"ZID":4,"Lvl":50,"RR":75},{"ZID":5,"Lvl":50,"RR":75},{"ZID":6,"Lvl":50,"RR":75},{"ZID":7,"Lvl":50,"RR":75},{"ZID":8,"Lvl":50,"RR":75},{"ZID":9,"Lvl":50,"RR":75},{"ZID":10,"Lvl":50,"RR":75},{"ZID":11,"Lvl":50,"RR":75},{"ZID":12,"Lvl":50,"RR":75},{"ZID":13,"Lvl":50,"RR":75},{"ZID":14,"Lvl":50,"RR":75},{"ZID":15,"Lvl":50,"RR":75},{"ZID":16,"Lvl":50,"RR":75},{"ZID":17,"Lvl":50,"RR":75},{"ZID":18,"Lvl":50,"RR":75},{"ZID":19,"Lvl":50,"RR":75},{"ZID":20,"Lvl":50,"RR":75},{"ZID":21,"Lvl":50,"RR":75},{"ZID":22,"Lvl":50,"RR":75},{"ZID":23,"Lvl":50,"RR":75},{"ZID":24,"Lvl":50,"RR":75},{"ZID":25,"Lvl":50,"RR":75},{"ZID":26,"Lvl":50,"RR":75},{"ZID":27,"Lvl":50,"RR":75},{"ZID":28,"Lvl":50,"RR":75},{"ZID":29,"Lvl":50,"RR":75},{"ZID":30,"Lvl":50,"RR":75},{"ZID":31,"Lvl":50,"RR":75},{"ZID":32,"Lvl":50,"RR":75},{"ZID":33,"Lvl":50,"RR":75},{"ZID":34,"Lvl":50,"RR":75},{"ZID":35,"Lvl":50,"RR":75},{"ZID":36,"Lvl":50,"RR":75},{"ZID":37,"Lvl":50,"RR":75},{"ZID":38,"Lvl":50,"RR":75},{"ZID":39,"Lvl":50,"RR":75},{"ZID":40,"Lvl":50,"RR":75},{"ZID":41,"Lvl":50,"RR":75},{"ZID":42,"Lvl":50,"RR":75},{"ZID":43,"Lvl":50,"RR":75},{"ZID":44,"Lvl":50,"RR":75},{"ZID":45,"Lvl":50,"RR":75},{"ZID":46,"Lvl":50,"RR":75},{"ZID":47,"Lvl":50,"RR":75},{"ZID":48,"Lvl":50,"RR":75},{"ZID":49,"Lvl":50,"RR":75},{"ZID":50,"Lvl":50,"RR":75},{"ZID":51,"Lvl":50,"RR":75},{"ZID":52,"Lvl":50,"RR":75},{"ZID":53,"Lvl":50,"RR":75},{"ZID":54,"Lvl":50,"RR":75},{"ZID":55,"Lvl":50,"RR":75},{"ZID":56,"Lvl":50,"RR":75},{"ZID":57,"Lvl":50,"RR":75},{"ZID":58,"Lvl":50,"RR":75},{"ZID":59,"Lvl":50,"RR":75},{"ZID":60,"Lvl":50,"RR":75},{"ZID":61,"Lvl":50,"RR":75},{"ZID":62,"Lvl":50,"RR":75},{"ZID":63,"Lvl":50,"RR":75},{"ZID":64,"Lvl":50,"RR":75},{"ZID":65,"Lvl":50,"RR":75},{"ZID":66,"Lvl":50,"RR":75},{"ZID":67,"Lvl":50,"RR":75},{"ZID":68,"Lvl":50,"RR":75},{"ZID":69,"Lvl":50,"RR":75},{"ZID":70,"Lvl":50,"RR":75},{"ZID":71,"Lvl":50,"RR":75},{"ZID":72,"Lvl":50,"RR":75},{"ZID":73,"Lvl":50,"RR":75},{"ZID":74,"Lvl":50,"RR":75},{"ZID":75,"Lvl":50,"RR":75},{"ZID":76,"Lvl":50,"RR":75},{"ZID":77,"Lvl":50,"RR":75},{"ZID":78,"Lvl":50,"RR":75},{"ZID":79,"Lvl":50,"RR":75},{"ZID":80,"Lvl":50,"RR":75},{"ZID":81,"Lvl":50,"RR":75},{"ZID":82,"Lvl":50,"RR":75},{"ZID":83,"Lvl":50,"RR":75},{"ZID":84,"Lvl":50,"RR":75},{"ZID":85,"Lvl":50,"RR":75},{"ZID":86,"Lvl":50,"RR":75},{"ZID":87,"Lvl":50,"RR":75},{"ZID":88,"Lvl":50,"RR":75},{"ZID":89,"Lvl":50,"RR":75},{"ZID":90,"Lvl":50,"RR":75},{"ZID":91,"Lvl":50,"RR":75},{"ZID":92,"Lvl":50,"RR":75},{"ZID":93,"Lvl":50,"RR":75},{"ZID":94,"Lvl":50,"RR":75},{"ZID":95,"Lvl":50,"RR":75},{"ZID":96,"Lvl":50,"RR":75},{"ZID":97,"Lvl":50,"RR":75},{"ZID":98,"Lvl":50,"RR":75},{"ZID":99,"Lvl":99,"RR":99}], "TriggerTime":554433,"Frequency":"sometime","TriggerType":1,"Delta":20} }'

sleep 2


send_packet '{ "ID": 2, "Service": "SetSceneProperties", "SID":5, "PropertyList":{ "Name":"Dinner Scene", "ZoneList":[{"ZID":0,"Lvl":25,"RR":100},{"ZID":1,"Lvl":50,"RR":75},{"ZID":2,"Lvl":50,"RR":75},{"ZID":3,"Lvl":50,"RR":75},{"ZID":4,"Lvl":50,"RR":75},{"ZID":5,"Lvl":50,"RR":75},{"ZID":6,"Lvl":50,"RR":75},{"ZID":7,"Lvl":50,"RR":75},{"ZID":8,"Lvl":50,"RR":75},{"ZID":9,"Lvl":50,"RR":75},{"ZID":10,"Lvl":50,"RR":75},{"ZID":11,"Lvl":50,"RR":75},{"ZID":12,"Lvl":50,"RR":75},{"ZID":13,"Lvl":50,"RR":75},{"ZID":14,"Lvl":50,"RR":75},{"ZID":15,"Lvl":50,"RR":75},{"ZID":16,"Lvl":50,"RR":75},{"ZID":17,"Lvl":50,"RR":75},{"ZID":18,"Lvl":50,"RR":75},{"ZID":19,"Lvl":50,"RR":75},{"ZID":20,"Lvl":50,"RR":75},{"ZID":21,"Lvl":50,"RR":75},{"ZID":22,"Lvl":50,"RR":75},{"ZID":23,"Lvl":50,"RR":75},{"ZID":24,"Lvl":50,"RR":75},{"ZID":25,"Lvl":50,"RR":75},{"ZID":26,"Lvl":50,"RR":75},{"ZID":27,"Lvl":50,"RR":75},{"ZID":28,"Lvl":50,"RR":75},{"ZID":29,"Lvl":50,"RR":75},{"ZID":30,"Lvl":50,"RR":75},{"ZID":31,"Lvl":50,"RR":75},{"ZID":32,"Lvl":50,"RR":75},{"ZID":33,"Lvl":50,"RR":75},{"ZID":34,"Lvl":50,"RR":75},{"ZID":35,"Lvl":50,"RR":75},{"ZID":36,"Lvl":50,"RR":75},{"ZID":37,"Lvl":50,"RR":75},{"ZID":38,"Lvl":50,"RR":75},{"ZID":39,"Lvl":50,"RR":75},{"ZID":40,"Lvl":50,"RR":75},{"ZID":41,"Lvl":50,"RR":75},{"ZID":42,"Lvl":50,"RR":75},{"ZID":43,"Lvl":50,"RR":75},{"ZID":44,"Lvl":50,"RR":75},{"ZID":45,"Lvl":50,"RR":75},{"ZID":46,"Lvl":50,"RR":75},{"ZID":47,"Lvl":50,"RR":75},{"ZID":48,"Lvl":50,"RR":75},{"ZID":49,"Lvl":50,"RR":75},{"ZID":50,"Lvl":50,"RR":75},{"ZID":51,"Lvl":50,"RR":75},{"ZID":52,"Lvl":50,"RR":75},{"ZID":53,"Lvl":50,"RR":75},{"ZID":54,"Lvl":50,"RR":75},{"ZID":55,"Lvl":50,"RR":75},{"ZID":56,"Lvl":50,"RR":75},{"ZID":57,"Lvl":50,"RR":75},{"ZID":58,"Lvl":50,"RR":75},{"ZID":59,"Lvl":50,"RR":75},{"ZID":60,"Lvl":50,"RR":75},{"ZID":61,"Lvl":50,"RR":75},{"ZID":62,"Lvl":50,"RR":75},{"ZID":63,"Lvl":50,"RR":75},{"ZID":64,"Lvl":50,"RR":75},{"ZID":65,"Lvl":50,"RR":75},{"ZID":66,"Lvl":50,"RR":75},{"ZID":67,"Lvl":50,"RR":75},{"ZID":68,"Lvl":50,"RR":75},{"ZID":69,"Lvl":50,"RR":75},{"ZID":70,"Lvl":50,"RR":75},{"ZID":71,"Lvl":50,"RR":75},{"ZID":72,"Lvl":50,"RR":75},{"ZID":73,"Lvl":50,"RR":75},{"ZID":74,"Lvl":50,"RR":75},{"ZID":75,"Lvl":50,"RR":75},{"ZID":76,"Lvl":50,"RR":75},{"ZID":77,"Lvl":50,"RR":75},{"ZID":78,"Lvl":50,"RR":75},{"ZID":79,"Lvl":50,"RR":75},{"ZID":80,"Lvl":50,"RR":75},{"ZID":81,"Lvl":50,"RR":75},{"ZID":82,"Lvl":50,"RR":75},{"ZID":83,"Lvl":50,"RR":75},{"ZID":84,"Lvl":50,"RR":75},{"ZID":85,"Lvl":50,"RR":75},{"ZID":86,"Lvl":50,"RR":75},{"ZID":87,"Lvl":50,"RR":75},{"ZID":88,"Lvl":50,"RR":75},{"ZID":89,"Lvl":50,"RR":75},{"ZID":90,"Lvl":50,"RR":75},{"ZID":91,"Lvl":50,"RR":75},{"ZID":92,"Lvl":50,"RR":75},{"ZID":93,"Lvl":50,"RR":75},{"ZID":94,"Lvl":50,"RR":75},{"ZID":95,"Lvl":50,"RR":75},{"ZID":96,"Lvl":50,"RR":75},{"ZID":97,"Lvl":50,"RR":75},{"ZID":98,"Lvl":50,"RR":75},{"ZID":99,"Lvl":99,"RR":99}], "TriggerTime":554433,"Frequency":0,"TriggerType":"often","Delta":20} }'

sleep 2

send_packet '{ "ID": 2, "Service": "SetSceneProperties", "SID":6, "PropertyList":{ "Name":"Dinner Scene", "ZoneList":[{"ZID":0,"Lvl":25,"RR":100},{"ZID":1,"Lvl":50,"RR":75},{"ZID":2,"Lvl":50,"RR":75},{"ZID":3,"Lvl":50,"RR":75},{"ZID":4,"Lvl":50,"RR":75},{"ZID":5,"Lvl":50,"RR":75},{"ZID":6,"Lvl":50,"RR":75},{"ZID":7,"Lvl":50,"RR":75},{"ZID":8,"Lvl":50,"RR":75},{"ZID":9,"Lvl":50,"RR":75},{"ZID":10,"Lvl":50,"RR":75},{"ZID":11,"Lvl":50,"RR":75},{"ZID":12,"Lvl":50,"RR":75},{"ZID":13,"Lvl":50,"RR":75},{"ZID":14,"Lvl":50,"RR":75},{"ZID":15,"Lvl":50,"RR":75},{"ZID":16,"Lvl":50,"RR":75},{"ZID":17,"Lvl":50,"RR":75},{"ZID":18,"Lvl":50,"RR":75},{"ZID":19,"Lvl":50,"RR":75},{"ZID":20,"Lvl":50,"RR":75},{"ZID":21,"Lvl":50,"RR":75},{"ZID":22,"Lvl":50,"RR":75},{"ZID":23,"Lvl":50,"RR":75},{"ZID":24,"Lvl":50,"RR":75},{"ZID":25,"Lvl":50,"RR":75},{"ZID":26,"Lvl":50,"RR":75},{"ZID":27,"Lvl":50,"RR":75},{"ZID":28,"Lvl":50,"RR":75},{"ZID":29,"Lvl":50,"RR":75},{"ZID":30,"Lvl":50,"RR":75},{"ZID":31,"Lvl":50,"RR":75},{"ZID":32,"Lvl":50,"RR":75},{"ZID":33,"Lvl":50,"RR":75},{"ZID":34,"Lvl":50,"RR":75},{"ZID":35,"Lvl":50,"RR":75},{"ZID":36,"Lvl":50,"RR":75},{"ZID":37,"Lvl":50,"RR":75},{"ZID":38,"Lvl":50,"RR":75},{"ZID":39,"Lvl":50,"RR":75},{"ZID":40,"Lvl":50,"RR":75},{"ZID":41,"Lvl":50,"RR":75},{"ZID":42,"Lvl":50,"RR":75},{"ZID":43,"Lvl":50,"RR":75},{"ZID":44,"Lvl":50,"RR":75},{"ZID":45,"Lvl":50,"RR":75},{"ZID":46,"Lvl":50,"RR":75},{"ZID":47,"Lvl":50,"RR":75},{"ZID":48,"Lvl":50,"RR":75},{"ZID":49,"Lvl":50,"RR":75},{"ZID":50,"Lvl":50,"RR":75},{"ZID":51,"Lvl":50,"RR":75},{"ZID":52,"Lvl":50,"RR":75},{"ZID":53,"Lvl":50,"RR":75},{"ZID":54,"Lvl":50,"RR":75},{"ZID":55,"Lvl":50,"RR":75},{"ZID":56,"Lvl":50,"RR":75},{"ZID":57,"Lvl":50,"RR":75},{"ZID":58,"Lvl":50,"RR":75},{"ZID":59,"Lvl":50,"RR":75},{"ZID":60,"Lvl":50,"RR":75},{"ZID":61,"Lvl":50,"RR":75},{"ZID":62,"Lvl":50,"RR":75},{"ZID":63,"Lvl":50,"RR":75},{"ZID":64,"Lvl":50,"RR":75},{"ZID":65,"Lvl":50,"RR":75},{"ZID":66,"Lvl":50,"RR":75},{"ZID":67,"Lvl":50,"RR":75},{"ZID":68,"Lvl":50,"RR":75},{"ZID":69,"Lvl":50,"RR":75},{"ZID":70,"Lvl":50,"RR":75},{"ZID":71,"Lvl":50,"RR":75},{"ZID":72,"Lvl":50,"RR":75},{"ZID":73,"Lvl":50,"RR":75},{"ZID":74,"Lvl":50,"RR":75},{"ZID":75,"Lvl":50,"RR":75},{"ZID":76,"Lvl":50,"RR":75},{"ZID":77,"Lvl":50,"RR":75},{"ZID":78,"Lvl":50,"RR":75},{"ZID":79,"Lvl":50,"RR":75},{"ZID":80,"Lvl":50,"RR":75},{"ZID":81,"Lvl":50,"RR":75},{"ZID":82,"Lvl":50,"RR":75},{"ZID":83,"Lvl":50,"RR":75},{"ZID":84,"Lvl":50,"RR":75},{"ZID":85,"Lvl":50,"RR":75},{"ZID":86,"Lvl":50,"RR":75},{"ZID":87,"Lvl":50,"RR":75},{"ZID":88,"Lvl":50,"RR":75},{"ZID":89,"Lvl":50,"RR":75},{"ZID":90,"Lvl":50,"RR":75},{"ZID":91,"Lvl":50,"RR":75},{"ZID":92,"Lvl":50,"RR":75},{"ZID":93,"Lvl":50,"RR":75},{"ZID":94,"Lvl":50,"RR":75},{"ZID":95,"Lvl":50,"RR":75},{"ZID":96,"Lvl":50,"RR":75},{"ZID":97,"Lvl":50,"RR":75},{"ZID":98,"Lvl":50,"RR":75},{"ZID":99,"Lvl":99,"RR":99}], "TriggerTime":554433,"Frequency":1,"TriggerType":1,"Delta":-120} }'

sleep 2

send_packet '{ "ID": 2, "Service": "SetSceneProperties", "SID":7, "PropertyList":{ "Name":"Dinner Scene", "ZoneList":[{"ZID":0,"Lvl":25,"RR":100},{"ZID":1,"Lvl":50,"RR":75},{"ZID":2,"Lvl":50,"RR":75},{"ZID":3,"Lvl":50,"RR":75},{"ZID":4,"Lvl":50,"RR":75},{"ZID":5,"Lvl":50,"RR":75},{"ZID":6,"Lvl":50,"RR":75},{"ZID":7,"Lvl":50,"RR":75},{"ZID":8,"Lvl":50,"RR":75},{"ZID":9,"Lvl":50,"RR":75},{"ZID":10,"Lvl":50,"RR":75},{"ZID":11,"Lvl":50,"RR":75},{"ZID":12,"Lvl":50,"RR":75},{"ZID":13,"Lvl":50,"RR":75},{"ZID":14,"Lvl":50,"RR":75},{"ZID":15,"Lvl":50,"RR":75},{"ZID":16,"Lvl":50,"RR":75},{"ZID":17,"Lvl":50,"RR":75},{"ZID":18,"Lvl":50,"RR":75},{"ZID":19,"Lvl":50,"RR":75},{"ZID":20,"Lvl":50,"RR":75},{"ZID":21,"Lvl":50,"RR":75},{"ZID":22,"Lvl":50,"RR":75},{"ZID":23,"Lvl":50,"RR":75},{"ZID":24,"Lvl":50,"RR":75},{"ZID":25,"Lvl":50,"RR":75},{"ZID":26,"Lvl":50,"RR":75},{"ZID":27,"Lvl":50,"RR":75},{"ZID":28,"Lvl":50,"RR":75},{"ZID":29,"Lvl":50,"RR":75},{"ZID":30,"Lvl":50,"RR":75},{"ZID":31,"Lvl":50,"RR":75},{"ZID":32,"Lvl":50,"RR":75},{"ZID":33,"Lvl":50,"RR":75},{"ZID":34,"Lvl":50,"RR":75},{"ZID":35,"Lvl":50,"RR":75},{"ZID":36,"Lvl":50,"RR":75},{"ZID":37,"Lvl":50,"RR":75},{"ZID":38,"Lvl":50,"RR":75},{"ZID":39,"Lvl":50,"RR":75},{"ZID":40,"Lvl":50,"RR":75},{"ZID":41,"Lvl":50,"RR":75},{"ZID":42,"Lvl":50,"RR":75},{"ZID":43,"Lvl":50,"RR":75},{"ZID":44,"Lvl":50,"RR":75},{"ZID":45,"Lvl":50,"RR":75},{"ZID":46,"Lvl":50,"RR":75},{"ZID":47,"Lvl":50,"RR":75},{"ZID":48,"Lvl":50,"RR":75},{"ZID":49,"Lvl":50,"RR":75},{"ZID":50,"Lvl":50,"RR":75},{"ZID":51,"Lvl":50,"RR":75},{"ZID":52,"Lvl":50,"RR":75},{"ZID":53,"Lvl":50,"RR":75},{"ZID":54,"Lvl":50,"RR":75},{"ZID":55,"Lvl":50,"RR":75},{"ZID":56,"Lvl":50,"RR":75},{"ZID":57,"Lvl":50,"RR":75},{"ZID":58,"Lvl":50,"RR":75},{"ZID":59,"Lvl":50,"RR":75},{"ZID":60,"Lvl":50,"RR":75},{"ZID":61,"Lvl":50,"RR":75},{"ZID":62,"Lvl":50,"RR":75},{"ZID":63,"Lvl":50,"RR":75},{"ZID":64,"Lvl":50,"RR":75},{"ZID":65,"Lvl":50,"RR":75},{"ZID":66,"Lvl":50,"RR":75},{"ZID":67,"Lvl":50,"RR":75},{"ZID":68,"Lvl":50,"RR":75},{"ZID":69,"Lvl":50,"RR":75},{"ZID":70,"Lvl":50,"RR":75},{"ZID":71,"Lvl":50,"RR":75},{"ZID":72,"Lvl":50,"RR":75},{"ZID":73,"Lvl":50,"RR":75},{"ZID":74,"Lvl":50,"RR":75},{"ZID":75,"Lvl":50,"RR":75},{"ZID":76,"Lvl":50,"RR":75},{"ZID":77,"Lvl":50,"RR":75},{"ZID":78,"Lvl":50,"RR":75},{"ZID":79,"Lvl":50,"RR":75},{"ZID":80,"Lvl":50,"RR":75},{"ZID":81,"Lvl":50,"RR":75},{"ZID":82,"Lvl":50,"RR":75},{"ZID":83,"Lvl":50,"RR":75},{"ZID":84,"Lvl":50,"RR":75},{"ZID":85,"Lvl":50,"RR":75},{"ZID":86,"Lvl":50,"RR":75},{"ZID":87,"Lvl":50,"RR":75},{"ZID":88,"Lvl":50,"RR":75},{"ZID":89,"Lvl":50,"RR":75},{"ZID":90,"Lvl":50,"RR":75},{"ZID":91,"Lvl":50,"RR":75},{"ZID":92,"Lvl":50,"RR":75},{"ZID":93,"Lvl":50,"RR":75},{"ZID":94,"Lvl":50,"RR":75},{"ZID":95,"Lvl":50,"RR":75},{"ZID":96,"Lvl":50,"RR":75},{"ZID":97,"Lvl":50,"RR":75},{"ZID":98,"Lvl":50,"RR":75},{"ZID":99,"Lvl":99,"RR":99}], "TriggerTime":554433,"Frequency":1,"TriggerType":1,"Delta":20} }'

sleep 2

send_packet '{ "ID": 2, "Service": "ReportSceneProperties", "SID":0}'

sleep 2

send_packet '{ "ID": 2, "Service": "ListScenes"}'

sleep 2


send_packet '{ "ID": 2, "Service": "ListZones"}'

sleep 2


send_packet '{ "ID": 2, "Service": "SystemInfo"}'











