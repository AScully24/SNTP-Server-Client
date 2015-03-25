/* talker.c - a datagram 'client'
 * need to supply host name/IP and one word message,
 * e.g. talker localhost hello
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> /* for gethostbyname() */

#define PORT 4950 /* server port the client connects to */
#define LISTENPORT 4952 /* server port the client recieves on */
#define SENDPORT 4951 /* server port the client connects to */

int main(int argc, char * argv[]) {

	int sockfd, numbytes, sockfdListen;
	struct hostent *he;
	struct sockaddr_in their_addr, my_addr; /* server address info */

	/*if (argc != 3) {
	 fprintf(stderr, "usage: talker hostname message\n");
	 exit(1);
	 }*/

	char ip[64];
	printf("Input an IP: ");
	scanf("%s", ip);

	/* resolve server host name or IP address */
	if ((he = gethostbyname(ip)) == NULL) {
		perror("Talker gethostbyname");
		exit(1);
	}

	/*if ((he = gethostbyname(argv[1])) == NULL) {
	 perror("Talker gethostbyname");
	 exit(1);
	 }*/

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Talker socket");
		exit(1);
	}

	// To listen for the server, need a different end point
	if ((sockfdListen = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Receiver socket");
		exit(1);
	}

	int addr_len = sizeof(struct sockaddr);

	// My address details
	memset(&my_addr, 0, sizeof(my_addr)); /* zero struct */
	my_addr.sin_family = AF_INET; /* host byte order ... */
	my_addr.sin_port = htons(LISTENPORT); /* ... short, network byte order */
	my_addr.sin_addr.s_addr = INADDR_ANY; /* any of server IP addrs */

	// Server details
	memset(&their_addr, 0, sizeof(their_addr)); /* zero struct */
	their_addr.sin_family = AF_INET; /* host byte order .. */
	their_addr.sin_port = htons(PORT); /* .. short, netwk byte order */
	their_addr.sin_addr = *((struct in_addr *) he->h_addr);

	// Sets up the port for listening
	if (bind(sockfdListen, (struct sockaddr *) &my_addr,
			sizeof(struct sockaddr)) == -1) {
		perror("Listener bind");
		exit(1);
	}

	while (1) {

		char msg[64];
		scanf("%s", msg);

		if ((numbytes = sendto(sockfd, msg, strlen(msg), 0,
				(struct sockaddr *) &their_addr, sizeof(struct sockaddr)))
				== -1) {
			perror("Talker sendto");
			exit(1);
		}

		printf("Sent %d bytes to %s\n", numbytes,
				inet_ntoa(their_addr.sin_addr));

		if ((numbytes = recvfrom(sockfdListen, msg, strlen(msg) - 1, 0,
				(struct sockaddr *) &their_addr, &addr_len)) == -1) {
			perror("Listener recvfrom");
			exit(1);
			continue;
		}
		printf("Recieved form server %d bytes to %s\n", numbytes,
						inet_ntoa(their_addr.sin_addr));
		
	}

	close(sockfd);
	return 0;
}