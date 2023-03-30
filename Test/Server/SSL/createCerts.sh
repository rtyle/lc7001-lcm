#!/bin/bash

# Create the server certificate and key
openssl req -x509 -nodes -newkey rsa:2048 -days 3650 -subj /C=US/ST=Pennsylvania/L=Middletown/O=Legrand/CN=legrand.us -keyout serverKey.pem -out serverCert.pem

# Create the expired certificate and key
openssl req -x509 -nodes -newkey rsa:2048 -days -1 -subj /C=US/ST=Pennsylvania/L=Middletown/O=Legrand/CN=legrand.us -keyout expiredKey.pem -out expiredCert.pem

# Create the client certificate and key
openssl req -x509 -nodes -newkey rsa:2048 -days 3650 -subj /C=US/ST=Pennsylvania/L=Middletown/O=Legrand/CN=legrand.us -keyout clientKey.pem -out clientCert.pem

# Create the DER versions of the certificates and keys
openssl x509 -outform der -in serverCert.pem -out serverCert.der
openssl x509 -outform der -in clientCert.pem -out clientCert.der
openssl x509 -outform der -in expiredCert.pem -out expiredCert.der

openssl rsa -in serverKey.pem -inform PEM -out serverKey.der -outform DER
openssl rsa -in clientKey.pem -inform PEM -out clientKey.der -outform DER
openssl rsa -in expiredKey.pem -inform PEM -out expiredKey.der -outform DER
