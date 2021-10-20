/* udp_client.c */ 
/* Programmed by Adarsh Sethi */
/* Sept. 19, 2021 */

#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset, memcpy, and strlen */
#include <netdb.h>          /* for struct hostent and gethostbyname */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */

#define STRING_SIZE 1024
struct client_Packet{
   unsigned short count;
   unsigned short request_ID;
} Packet;

void htonPacket(struct client_Packet,char *buffer);
void interpret_server_packet(char *packet_received, int bytes_recd, int *last, unsigned short *sequence_sum, unsigned long *checksum);

int main(void) {

   int sock_client;  /* Socket used by client */ 

   struct sockaddr_in client_addr;  /* Internet address structure that
                                        stores client address */
   unsigned short client_port;  /* Port number used by client (local port) */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   struct hostent * server_hp;      /* Structure to store server's IP
                                        address */
   char server_hostname[STRING_SIZE]; /* Server's hostname */
   unsigned short server_port;  /* Port number used by server (remote port) */

   char sentence[STRING_SIZE];  /* send message */
   char packet_received[STRING_SIZE]; /* receive message */
   unsigned int msg_len;  /* length of message */
   int bytes_sent, bytes_recd; /* number of bytes sent or received */

   int done = 0; /* while loop var */
   int temp_count;  //holds temporary value for count
   char buffer[50];
   /*unsigned short count; //count for packet header
   unsigned short request_ID=1; //request ID for header*/
  
   /* open a socket */

   if ((sock_client = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      perror("Client: can't open datagram socket\n");
      exit(1);
   }

   /* Note: there is no need to initialize local client address information
            unless you want to specify a specific local port.
            The local address initialization and binding is done automatically
            when the sendto function is called later, if the socket has not
            already been bound. 
            The code below illustrates how to initialize and bind to a
            specific local port, if that is desired. */

   /* initialize client address information */

   client_port = 0;   /* This allows choice of any available local port */

   /* Uncomment the lines below if you want to specify a particular 
             local port: */
   /*
   printf("Enter port number for client: ");
   scanf("%hu", &client_port);
   */

   /* clear client address structure and initialize with client address */
   memset(&client_addr, 0, sizeof(client_addr));
   client_addr.sin_family = AF_INET;
   client_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* This allows choice of
                                        any host interface, if more than one 
                                        are present */
   client_addr.sin_port = htons(client_port);

   /* bind the socket to the local client port */

   if (bind(sock_client, (struct sockaddr *) &client_addr,
                                    sizeof (client_addr)) < 0) {
      perror("Client: can't bind to local address\n");
      close(sock_client);
      exit(1);
   }

   /* end of local address initialization and binding */

   /* initialize server address information */

   printf("Enter hostname of server: ");
   scanf("%s", server_hostname);

   
   if ((server_hp = gethostbyname(server_hostname)) == NULL) {
      perror("Client: invalid server hostname\n");
      close(sock_client);
      exit(1);
   }
   printf("Enter port number for server: ");
   scanf("%hu", &server_port);

   /* Clear server address structure and initialize with server address */
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   memcpy((char *)&server_addr.sin_addr, server_hp->h_addr,
                                    server_hp->h_length);
   server_addr.sin_port = htons(server_port);

   /* user interface */

   Packet.request_ID=1;
   while(!done) {

      printf("Enter number of requested integers:\n");
      scanf("%d", &temp_count);

      while (temp_count < 1 || temp_count > 65535) {
         printf("Enter number between 1 and 65,535:\n");
         scanf("%d", &temp_count); //temp request is here so overflow does not occur
      }
      Packet.count = temp_count;

      /* send message */
      htonPacket(Packet,buffer);

      bytes_sent = sendto(sock_client, &buffer, 4, 0,
               (struct sockaddr *) &server_addr, sizeof (server_addr));

           /* get response from server */


      int last = 0;
      unsigned short sequence_sum = 0;
      int total_packets = 0;
      int total_bytes = 0;
      unsigned long checksum = 0;
      printf("Waiting for response from server...\n");
      /* Start recieving data from the server and interpret the packets */

      while (!last) {
         total_packets++;
         unsigned short request_id;
         
         bytes_recd = recvfrom(sock_client, packet_received, STRING_SIZE, 0,
                     (struct sockaddr *) 0, (int *) 0);
         unsigned short temp;
         memcpy(&temp,packet_received,2);
         request_id = ntohs(temp);

         /* If packets are the same interpret the packet */
         if(request_id==Packet.request_ID){
            total_bytes += bytes_recd;
            interpret_server_packet(packet_received, bytes_recd, &last, &sequence_sum, &checksum); /* gets values from one packet */
         } else {
            last = 1;
         }
         
      }

      printf("Total bytes = %d, Sequence_sum = %hu, Packets = %d, Checksum = %lu\n",total_bytes, sequence_sum, total_packets, checksum);
      
      /* Allow for another request to be made */
      char loop_response[10];
      printf("Send another request? Enter y/n\n");
      scanf("%s", loop_response);

      if (strcmp(loop_response, "n") == 0) {
         done = 1;
      } else if(strcmp(loop_response, "y")==0){
         done=0;
         Packet.request_ID++;
      } else{
         printf("Invalid Input\n");
         done=1;
      }
   }
   /* close the socket */

   close (sock_client);
}
/*    Interpret_server_packet takes the data packet
         sent by the server and breaks it into readable
         data to be used by the client such as bytes recieved
         whether or not it is the last packet, sequence num,
         and checksum
*/
void interpret_server_packet(char *packet_received, int bytes_recd, int *last, unsigned short *sequence_sum, unsigned long *checksum){
   unsigned short temp;
   unsigned long temp_int;
   memcpy(&temp,packet_received+4,2); //Get last value
   *last = ntohs(temp);
   memcpy(&temp,packet_received+2,2); //Get Sequence Sum
   *sequence_sum += ntohs(temp);
   memcpy(&temp,packet_received+6,2); //Get Count
   temp=ntohs(temp);
   for(int i=0;i<temp;i++){
      memcpy(&temp_int,packet_received+8+4*i,4);
      *checksum += ntohl(temp_int);
   }
}

/*       htonPacket takes a packet struct and a buffer
            and formats the header to be sent to the 
            server for the client request of integers
            and the request ID
*/
void htonPacket(struct client_Packet Packet,char *buffer){
   unsigned short temp;
   temp=htons(Packet.request_ID);
   memcpy(buffer,&temp,2);  //Copy in request ID
   temp=htons(Packet.count); 
   memcpy(buffer+2,&temp,2);//Copy in count
}
