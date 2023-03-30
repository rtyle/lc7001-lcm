#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <netdb.h>
#include <string.h>

int main()
{
   int sock;
   struct sockaddr_in servaddr;
   char sendBuffer[100];
   char receiveBuffer[100];
   int bytesReceived = 0;

   sock = socket(AF_INET,SOCK_DGRAM,0);

   memset(&servaddr,0, sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_port=htons(5353);

   memset(sendBuffer, 0, sizeof(sendBuffer));
   sendBuffer[2] = 0x00;
   sendBuffer[12] = 0x04;
   snprintf(&sendBuffer[13], 5, "LCM1");
   sendBuffer[17] = 0x05;
   snprintf(&sendBuffer[18], 6, "local");

   printf("Sending 32 bytes to multicast\n");
   servaddr.sin_addr.s_addr=inet_addr("224.0.0.251");
   sendto(sock,sendBuffer,32,0, (struct sockaddr *)&servaddr, sizeof(servaddr));
   bytesReceived = recvfrom(sock,receiveBuffer,86,0,NULL,NULL);
   printf("Received %d bytes from socket\n", bytesReceived);

   int broadcast = 1;
   setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast);
   printf("Sending 32 bytes to broadcast\n");
   servaddr.sin_addr.s_addr=inet_addr("255.255.255.255");
   sendto(sock,sendBuffer,32,0, (struct sockaddr *)&servaddr, sizeof(servaddr));
   bytesReceived = recvfrom(sock,receiveBuffer,86,0,NULL,NULL);
   printf("Received %d bytes from socket\n", bytesReceived);
}
