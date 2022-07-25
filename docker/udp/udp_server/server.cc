// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
	
#define PORT	 8080
#define MAXLINE 1024

#include <gflags/gflags.h>
#include <string> 
#include "udp_server.h" 

//Driver Code
int main(int argc, char **argv)
{
	//char hello[] = "Hello from server";
	char buffer[MAXLINE];
	gflags::ParseCommandLineFlags(&argc, &argv, false);
	UDPServer server = UDPServer(8080);
	//server.port = 8080;
	while(true){
		if(server.Recv((uint8_t*) buffer, sizeof(buffer))){
			break;
		}
		// if(server.RecvFrom((uint8_t*) buffer, sizeof(buffer), "192.168.10.11", 8080) > 0){
		// 	break;
		// }
	}
	printf("%s", buffer);
	return 0;

}


	
// // Driver code
// int main() {
// 	int sockfd;
// 	char buffer[MAXLINE];
// 	char *hello = "Hello from server";
// 	struct sockaddr_in servaddr, cliaddr;
		
// 	// Creating socket file descriptor
// 	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
// 		perror("socket creation failed");
// 		exit(EXIT_FAILURE);
// 	}
		
// 	memset(&servaddr, 0, sizeof(servaddr));
// 	memset(&cliaddr, 0, sizeof(cliaddr));
		
// 	// Filling server information
// 	servaddr.sin_family = AF_INET; // IPv4
// 	servaddr.sin_addr.s_addr = INADDR_ANY;
// 	servaddr.sin_port = htons(PORT);
		
// 	// Bind the socket with the server address
// 	if ( bind(sockfd, (const struct sockaddr *)&servaddr,
// 			sizeof(servaddr)) < 0 )
// 	{
// 		perror("bind failed");
// 		exit(EXIT_FAILURE);
// 	}
		
// 	ssize_t n; 
//     socklen_t len;
	
// 	len = sizeof(cliaddr); //len is value/result
	
// 	n = recvfrom(sockfd, (char *)buffer, MAXLINE,
// 				MSG_WAITALL, ( struct sockaddr *) &cliaddr,
// 				&len);
// 	buffer[n] = '\0';
// 	printf("Client : %s\n", buffer);
// 	sendto(sockfd, (const char *)hello, strlen(hello),
// 		MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
// 			len);
// 	printf("Hello message sent.\n");
		
// 	return 0;
// }
