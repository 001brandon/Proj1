/* udp_server.c */
/* Programmed by Adarsh Sethi */
/* Sept. 19, 2021 */

#include <ctype.h>          /* for toupper */
#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */

#define STRING_SIZE 1024

/* SERV_UDP_PORT is the port number on which the server listens for
   incoming messages from clients. You should change this to a different
   number to prevent conflicts with others in the class. */

#define SERV_UDP_PORT 7892
struct server_Header{
   unsigned short count;
   unsigned short request_ID;
   unsigned short last;
   unsigned short sequence_num;
} Header;

void interpret_client_packet(char *buffer, unsigned short *id, unsigned short *count);
int generate_packet(struct server_Header, int* payload,char *buffer, unsigned short *count,unsigned short request_id, unsigned long *checksum);

int main(void) {

   int sock_server;  /* Socket on which server listens to clients */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   unsigned short server_port;  /* Port number used by server (local port) */

   struct sockaddr_in client_addr;  /* Internet address structure that
                                        stores client address */
   unsigned int client_addr_len;  /* Length of client address structure */

   char sentence[STRING_SIZE];  /* receive message */
   char modifiedSentence[STRING_SIZE]; /* send message */
   unsigned int msg_len;  /* length of message */
   int bytes_sent, bytes_recd; /* number of bytes sent or received */
   unsigned int i;  /* temporary loop variable */
   char packet_received[50]; /* buffer to hold incoming client packets */
   unsigned short request_id; /* request ID of most recent client packet */
   unsigned short count; /* count of most recent client packet */

   char buffer[200]; /* buffer used to store outgoing packets */
   int payload[25]; /* buffer for random ints, gets added to outgoing packet */
   int packet_len; /* specifies length of outgoing packet */

   /* open a socket */

   if ((sock_server = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      perror("Server: can't open datagram socket\n");
      exit(1);
   }

   /* initialize server address information */

   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl (INADDR_ANY);  /* This allows choice of
                                        any host interface, if more than one
                                        are present */
   server_port = SERV_UDP_PORT; /* Server will listen on this port */
   server_addr.sin_port = htons(server_port);

   /* bind the socket to the local server port */

   if (bind(sock_server, (struct sockaddr *) &server_addr,
                                    sizeof (server_addr)) < 0) {
      perror("Server: can't bind to local address\n");
      close(sock_server);
      exit(1);
   }

   /* wait for incoming messages in an indefinite loop */

   printf("Waiting for incoming messages on port %hu\n\n",
                           server_port);

   client_addr_len = sizeof (client_addr);

   for (;;) {

      bytes_recd = recvfrom(sock_server, &packet_received, 4, 0,
                     (struct sockaddr *) &client_addr, &client_addr_len);

      interpret_client_packet(packet_received, &request_id, &count);



      /* prepare the message to send */

      /* variables for packet details, compared to same in client */
      int total_packets = 0;
      int total_bytes = 0;
      int sequence_num = 0;
      int sequence_sum = 0;
      unsigned long checksum = 0;

      /* Generates packets until the requested number of integers is reached */

      while(count!=0){
         sequence_num++;
         sequence_sum += sequence_num;
         Header.sequence_num = sequence_num;
         packet_len=generate_packet(Header, payload,buffer,&count, request_id, &checksum);

         total_packets++;

         /* send message */
         bytes_sent = sendto(sock_server, buffer, packet_len, 0,
                  (struct sockaddr*) &client_addr, client_addr_len);
         total_bytes += bytes_sent;
      }
      printf("Total bytes = %d, Sequence_sum = %hu, Packets = %d, Checksum = %lu\n",total_bytes, sequence_sum, total_packets, checksum);

   }
}
/*
Extract header details from client packet,
perform network-to-host transformations as necessary.
Update count and request_id variables based off content in header.
*/
void interpret_client_packet(char *buffer, unsigned short *id, unsigned short *count){
   unsigned short temp;
   memcpy(&temp,buffer,2); //Extract request id
   *id=ntohs(temp); //write temp in ntohs to id
   memcpy(&temp,buffer+2,2);//extract count
   *count=ntohs(temp);
}


/*
Generate random integer sequence put 25 at a time in payload
send the payload
iterate until temp=0
*/
int generate_packet(struct server_Header Header, int *Payload, char *buffer,unsigned short *count, unsigned short request_id, unsigned long *checksum){
   int packet_len;

   Header.request_ID = request_id;
   /* make the payload based on the requested integers
         passed into the function*/
   if((*count-25)>=0){
      if(*count-25==0){
         Header.last=1;
      }else{
         Header.last = 0;
      }
      Header.count = 25;
      *count-=25;
      for(int i=0;i<25;i++){
         Payload[i]=htonl(rand()%120);
         *checksum += ntohl(Payload[i]);
      }
      packet_len=25*sizeof(int);

   } else{
      Header.last = 1;
      Header.count = *count;
      for(int i=0; i<(*count);i++){
         Payload[i]=htonl(rand()%120);
         *checksum += ntohl(Payload[i]);
      }
   packet_len=(*count)*sizeof(int);
   *count=0;
   }
   /* Copy the necessary information into the buffer to send */
   int mem_temp;
   mem_temp = htons(Header.request_ID);
   memcpy(buffer,&mem_temp,2);
   mem_temp = htons(Header.sequence_num);
   memcpy(buffer+2,&mem_temp,2);
   mem_temp = htons(Header.last);
   memcpy(buffer+4,&mem_temp,2);
   mem_temp = htons(Header.count);
   memcpy(buffer+6,&mem_temp,2);
   memcpy(buffer+8,Payload,packet_len+8);
   return packet_len+8;

}
