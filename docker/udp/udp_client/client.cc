// udp client driver program
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<stdlib.h>
#define PORT 8080
#define MAXLINE 1000

#include <gflags/gflags.h>
#include <string> 
#include "udp_client.h" 

// DEFINE_bool(big_menu, true, "Include 'advanced' options in the menu listing");
DEFINE_string(server_ip, "192.168.10.10",
                 "Default IP Value for UDP Server on Docker udp_network bridge");
  
// Driver code
int main(int argc, char **argv)
{
    char message[] = "Hello Server";
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    UDPClient udp_client;
    udp_client.Send(FLAGS_server_ip, 8080, (uint8_t*) message, sizeof(message));
    return 0;
}

// int main(int argc, char** argv)
// {   
// 	gflags::ParseCommandLineFlags(&argc, &argv, false);
//     char buffer[100];
//     char message[] = "Hello Server";
//     char *message_ptr = message;
//     // printf("%s", &message_ptr[0]);
//     int sockfd, n;
//     struct sockaddr_in servaddr;
      
//     // clear servaddr
//     bzero(&servaddr, sizeof(servaddr));
//     servaddr.sin_addr.s_addr = inet_addr(FLAGS_server_ip.c_str());
//     servaddr.sin_port = htons(PORT);
//     servaddr.sin_family = AF_INET;
      
//     // create datagram socket
//     sockfd = socket(AF_INET, SOCK_DGRAM, 0);
      
//     // connect to server
//     if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
//     {
//         printf("\n Error : Connect Failed \n");
//         exit(0);
//     }
  
//     // request to send datagram
//     // no need to specify server address in sendto
//     // connect stores the peers IP and port
//     sendto(sockfd, message, MAXLINE, 0, (struct sockaddr*)NULL, sizeof(servaddr));
      
//     // waiting for response
//     recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)NULL, NULL);
//     puts(buffer);
  
//     // close the descriptor
//     close(sockfd);
// }