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

# Send a set system properties with no property list. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties"}' '"Status":"Error"'

# Send a set system properties with invalid property. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"InvalidPropertyName": 2}}' '"Status":"Error"'

# Send a set system properties with AddALight as not a boolean. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"AddALight": "abc"}}' '"Status":"Error"'

# Send a set system properties with an empty property list. Success
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {}}' '"Status":"Success"'

# Send a set system properties with AddALight true. Success
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"AddALight": true}}' '"Status":"Success"'

# Send a set system properties with AddALight true. Success
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"AddALight": false}}' '"Status":"Success"'

# Send a set system properties with TimeZone as not an integer. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"TimeZone": false}}' '"Status":"Error"'

# Send a set system properties with a valid TimeZone offset. Success
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"TimeZone": -18000}}' '"Status":"Success"'

# Send a set system properties with a daylight saving time as not a boolean. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"DaylightSavingTime": "abc"}}' '"Status":"Error"'

# Send a set system properties with a daylight saving time as true. Success
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"DaylightSavingTime": true}}' '"Status":"Success"'

# Send a set system properties with a daylight saving time as false. Success
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"DaylightSavingTime": false}}' '"Status":"Success"'

# Send a set system properties with a location as not an object. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": false}}' '"Status":"Error"'

# Send a set system properties with a location as an empty object. Success
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {}}}' '"Status":"Success"'

# Send a set system properties with location having an unknown value. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"InvalidProp": {} }}}' '"Status":"Error"'

# Send a set system properties with empty latitude. Success
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Lat":{}}}}' '"Status":"Error"'

# Send with latitude degrees only. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Lat":{"Deg":1}}}}' '"Status":"Error"'

# Send with latitude degrees and minutes. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Lat":{"Deg":1, "Min":2}}}}' '"Status":"Error"'

# Send with latitude degrees, minutes, and seconds. Success
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Lat":{"Deg":1, "Min":2, "Sec":3}}}}' '"Status":"Success"'

# Send latitude with an extra parameter. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Lat":{"Deg":1, "Min":2, "Sec":3, "Extra":4}}}}' '"Status":"Error"'

# Send a set system properties with empty longitude. Success
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Long":{}}}}' '"Status":"Error"'

# Send with longitude degrees only. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Long":{"Deg":1}}}}' '"Status":"Error"'

# Send with longitude degrees and minutes. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Long":{"Deg":1, "Min":2}}}}' '"Status":"Error"'

# Send with longitude degrees, minutes, and seconds. Success
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Long":{"Deg":1, "Min":2, "Sec":3}}}}' '"Status":"Success"'

# Send longitude with an extra parameter. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Long":{"Deg":1, "Min":2, "Sec":3, "Extra":4}}}}' '"Status":"Error"'

# Send latitude with invalid degrees. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Lat":{"Deg":-91, "Min":2, "Sec":3}}}}' '"Status":"Error"'
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Lat":{"Deg":91, "Min":2, "Sec":3}}}}' '"Status":"Error"'

# Send latitude on the border cases. Success
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Lat":{"Deg":-90, "Min":2, "Sec":3}}}}' '"Status":"Success"'
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Lat":{"Deg":90, "Min":2, "Sec":3}}}}' '"Status":"Success"'

# Send longitude with invalid degrees. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Long":{"Deg":-181, "Min":2, "Sec":3}}}}' '"Status":"Error"'
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Long":{"Deg":181, "Min":2, "Sec":3}}}}' '"Status":"Error"'

# Send longitude on the border cases. Success
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Long":{"Deg":-180, "Min":2, "Sec":3}}}}' '"Status":"Success"'
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Long":{"Deg":180, "Min":2, "Sec":3}}}}' '"Status":"Success"'

# Send invalid minutes. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Lat":{"Deg":90, "Min":-1, "Sec":3}}}}' '"Status":"Error"'
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Long":{"Deg":180, "Min":-1, "Sec":3}}}}' '"Status":"Error"'
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Lat":{"Deg":90, "Min":61, "Sec":3}}}}' '"Status":"Error"'
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Long":{"Deg":180, "Min":61, "Sec":3}}}}' '"Status":"Error"'

# Send minutes on the border. Success
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Lat":{"Deg":90, "Min":59, "Sec":3}}}}' '"Status":"Success"'
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Long":{"Deg":180, "Min":59, "Sec":3}}}}' '"Status":"Success"'

# Send invalid seconds. Error
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Lat":{"Deg":90, "Min":59, "Sec":-1}}}}' '"Status":"Error"'
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Long":{"Deg":180, "Min":59, "Sec":-1}}}}' '"Status":"Error"'
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Lat":{"Deg":90, "Min":59, "Sec":61}}}}' '"Status":"Error"'
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Long":{"Deg":180, "Min":59, "Sec":61}}}}' '"Status":"Error"'

# Send seconds on the border. Success
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Lat":{"Deg":90, "Min":59, "Sec":59}}}}' '"Status":"Success"'
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Location": {"Long":{"Deg":180, "Min":59, "Sec":59}}}}' '"Status":"Success"'

# Send with a location info not a string
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"LocationInfo": 15}}' '"Status":"Error"'

# Send with a valid location info string
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"LocationInfo": "17057"}}' '"Status":"Success"'

# Send with a config not a bool
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Configured": 15}}' '"Status":"Error"'

# Send with a valid configured flag
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"Configured": true}}' '"Status":"Success"'

# Send with an effective GMT offset not an int
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"EffectiveTimeZone": true}}' '"Status":"Error"'

# Send with an effective GMT offset as an int. This will still fail because it is a read only property
send_packet '{ "ID": 2, "Service": "SetSystemProperties", "PropertyList": {"EffectiveTimeZone": -18000}}' '"Status":"Error"'
