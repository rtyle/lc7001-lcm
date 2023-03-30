HOST=LCM1.local
PORT=2112

function send_packet {
   receiveStr=$(echo -ne "$1\000" | nc -w2 $HOST $PORT)
}

function get_MAC {
   commandStr='{ "ID": 3, "Service": "SystemInfo"}'
   receiveStr=$(echo -ne "$commandStr\000" | nc -w2 $HOST $PORT)

   outputStr="$(echo "${receiveStr}" | grep -o '"MACAddress":.*?[^\\]"')"
   echo
   echo "Current Value: ${outputStr}"
}

quit=0

while [[ ${quit} -eq 0 ]]
do
   # Print out the current MAC address
   get_MAC

   echo "Do you want to enter a new MAC address?"
   read option

   if [[ ${option} == y* || ${option} == Y* ]]
   then
      echo "Enter the last 3 bytes of the MAC Addres"
      output=""
      MACAddress=()
      for i in {1..3}
      do
         read line
         MACAddress=("${MACAddress[@]}" $line)

         if [[ i -eq 1 ]]
         then
            output="$(printf "%d" 0x${line})"
         else
            output="$(printf "%s,%d" ${output} 0x${line})"
         fi
      done

      echo "Setting MAC Address to 00 26 EC ${MACAddress[@]}"

      #Enter Manufacturing Test Mode
      send_packet '{ "ID": 1, "Service": "ManufacturingTestMode", "State" : true}'
      #Set the MAC Address
      packet=$(echo "{\"ID\":2,\"Service\":\"SetMACAddress\",\"MACAddress\":[${output}]}")
      send_packet ${packet}

      #Exit Manufacturing Test Mode
      send_packet '{ "ID": 2, "Service": "ManufacturingTestMode", "State" : false}'
      #Print out the new MAC address to make sure it worked
      get_MAC

   fi

   echo "Plug in a new LCM and hit enter to continue or q to quit"
   read option

   if [[ ${option} == q* || ${option} == Q* ]]
   then
      quit=1
   elif [[ ${option} == ip ]]
   then
      echo "Enter a new IP Address for this LCM"
      read HOST
   fi
done
