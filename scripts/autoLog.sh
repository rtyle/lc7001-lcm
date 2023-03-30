#!/usr/bin/expect

set hostname [lindex $argv 0];
set port [lindex $argv 1];
set password [lindex $argv 2];
set timeout 10
set challenge_greeting "Hello V1 "
set setkey_greeting "SETKEY"

# Loop telnet forever
set forever 1;
while { $forever == 1 } {

  send_user "\n*** CONNECTING TO $hostname\n\n"

  set retries 0;
  spawn nc -vv $hostname $port
  expect {
    timeout
    {
      incr retries 1;
      if { $retries > 3 } {
        send_user "\n*** TIMEOUT\n\n";
        set retries 0;
        send "\x03";
      } else {
        send_user "\n*** TIMEOUT - RETRY\n\n";
        exp_continue
      }
    }

    # Make a newline after every time we see "CurrentTime"
    -re ".*CurrentTime.*"
    {
      send_user "\n";
      exp_continue
    }

    -re ".$setkey_greeting."
    {
      # There is no key set - set it to MD5 hash of the provided password
      set empty_key [exec echo -n "" | md5]
      set new_key [exec echo -n $password | md5]

      # Use the following series of shell commands connected with pipes to turn the keys and empty password into a command to set the real password
      # 1. echo - to send the keys into xxd
      # 2. xxd - to convert ASCII to binary
      # 3. openssl - to encrypt the binary keys using the MD5 hash of the empty password as key to AES 128 ECB
      # 4. xxd - to convert binary to ASCII

      set encrypted_empty_key [exec echo $empty_key | xxd -r -p | openssl aes-128-ecb -md md5 -pass pass: -nosalt -nopad | xxd -p]
      set encrypted_new_key [exec echo $new_key | xxd -r -p | openssl aes-128-ecb -md md5 -pass pass: -nosalt -nopad | xxd -p]

      set combined_encrypted_keys $encrypted_empty_key$encrypted_new_key

      set set_keys_command "{\"ID\":0,\"PropertyList\":{\"Keys\":\"$combined_encrypted_keys\"},\"Service\":\"SetSystemProperties\"}"

      send_user "\n******>"
      send_user "$set_keys_command";
      send_user "<****** set_keys_command\n"

      # Send the set keys command followed by null and end of transmission
      send "$set_keys_command";
      send "\x00";
      send "\x04";

      exp_continue
    }

    -re "$challenge_greeting................................"
    {
      # Parse the challenge phrase
      set challenge_phrase $expect_out(0,string);
      set challenge_phrase [regsub -all {[^ .,[:alnum:]]} $challenge_phrase ""]
      set challenge_phrase [regsub -all $challenge_greeting $challenge_phrase ""]

      send_user "\n******>"
      send_user "$challenge_phrase";
      send_user "<****** challenge_phrase\n"

      #send_user "******>"
      #send_user "$password";
      #send_user "<****** password\n"

      # Use the following series of shell commands connected with pipes to turn the challenge phrase and password into a response
      # 1. echo - to send the challenge phrase into xxd
      # 2. xxd - to convert ASCII to binary
      # 3. openssl - to encrypt the binary challenge phrase using the MD5 hash of the password as key to AES 128 ECB
      # 4. xxd - to convert binary to ASCII


      # Catch the warning that newer versions of open ssl generate
      catch {exec echo $challenge_phrase | xxd -r -p | openssl aes-128-ecb -md md5 -pass pass:$password -nosalt -nopad | xxd -p} response_phrase code

      # Truncate the response phrase before any warning message
      set response_phrase [string range $response_phrase 0 31]

      send_user "******>"
      send_user "$response_phrase";
      send_user "<****** response_phrase\n\n"

      # Send the response followed by end of transmission
      send "$response_phrase";
      send "\x04";

      exp_continue
    }
  }

  interact
}
