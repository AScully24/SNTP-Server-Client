/* Server - Waits for a client to send a message, then sends back an updated time. */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h> /* gettimeofday */
#include <arpa/inet.h> /* inet_addr */
#include "ntp_time_conversion.h" /* Custom made header file for converting types.*/

#define CLIENT_MODE	3

void setServerMessage(struct sntpMsgFormat*,struct timeval);

int main(int argc, char * argv[]) {

	int sockfd;
	int numbytes = 0;
	struct ip_mreq mreq;
	struct sockaddr_in group_addr; /* Group address to listen on */
	struct sntpMsgFormat recvBuffer, msg;
	socklen_t addr_len = (socklen_t) sizeof(struct sockaddr);
	struct hostent *he;
	char serverIP[] = "239.0.0.1"; //"time.nist.gov";
	initialiseMsgFormat(&recvBuffer);
	int LISTEN_PORT = 1234; /* The port the server listens on. */

	/*
	 * Argument handler */
	int serverArg = 0,manycast = 0;
	char *argIP;
	int i;
	for (i = 1; i < argc; i++){   /* Skip argv[0]*/

		if (strcmp(argv[i], "-m") == 0){/* Enters manycast mode. */
			manycast = 1;
			printf("Entering Manycast Mode\n");

		} else if (strcmp(argv[i], "-p") == 0){/* Allows the user to input a specific port. */
			i++;
			int newPort = 0;
			int x = 0;

			while(argv[i][x] != '\0'){ /*Convert  char array to number. */
				newPort *=10;
				newPort +=argv[i][x] - '0';
				x++;
			}
			LISTEN_PORT = newPort;

		} else if (strcmp(argv[i], "-ip") == 0){/* Allows the user to input an ip address to connect to. */
			i++;
			serverArg = 1;
			argIP = malloc(sizeof(argv[i]));
			strcpy(argIP,argv[i]);
		}
	}

	/*
	 * Sets up the IP for listening in manycast mode*/
	if(manycast == 1){
		if(serverArg == 0){
			argIP = malloc(sizeof(serverIP));
			strcpy(argIP,serverIP);
		}
		printf("Resolving Mulicast Group IP %s: ",argIP);
		/* resolve server host name or IP address */
		if ((he = gethostbyname(argIP)) == NULL) {
			perror("Server gethostbyname");
			exit(1);
		}
		printf("OK\n");
	}


	/*
	 * Setup the listen socket */
	printf("Setting up socket Port %d: ", LISTEN_PORT);
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Listen socket");
		exit(1);
	}
	printf("OK\n");


	/*
	 * Listen*/
	printf("Setting Listen Socket address: ");
	memset(&group_addr, 0, sizeof(group_addr));
	group_addr.sin_family = AF_INET; /* host byte order .. */
	group_addr.sin_port = htons(LISTEN_PORT); /* .. short, netwk byte order */
	printf("OK\n");

	/*
	 * Bind to port 1234 by default if argument -p is not passed. */
	printf("Binding Socket:");
	if (bind(sockfd, (struct sockaddr *) &group_addr, sizeof(struct sockaddr)) == -1) {
		perror("Listener bind");
		exit(1);
	}
	printf("OK\n");


	/*
	 * Manycast Mode Only */
	if(manycast == 1){
		/*
		 * Listen to multicast address only */
		group_addr.sin_addr = *((struct in_addr *) he->h_addr);

		printf("Setting socket to be reusable %d: ", LISTEN_PORT);
		/* Allow multiple sockets to use the same PORT number.
		 * Used if other processes want to listen to multicast messages on this port. */
		u_int yes = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
			perror("Reusing ADDR failed");
			exit(1);
		}
		printf("OK\n");

		/*
		 *  use setsockopt() to request the kernel join a multicast group */
		printf("Joining the multicast group: ");
		mreq.imr_multiaddr.s_addr = inet_addr(argIP);
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
			perror("setsockopt");
			exit(1);
		}
		printf("OK\n");
	} else /* If unicast, listen to any address */
		group_addr.sin_addr.s_addr = INADDR_ANY;

	while (1) { /* Main loop for listening and sending. */

		printf("Listening...");
		if ((numbytes = recvfrom(sockfd, (struct sntpMsgFormat *) &recvBuffer,
				sizeof(struct sntpMsgFormat), 0, (struct sockaddr *) &group_addr, &addr_len)) == -1) {
			perror("Listener recvfrom");
			exit(1);
		}
		struct timeval myTime;
		gettimeofday(&myTime, NULL);


		if (numbytes == sizeof(struct sntpMsgFormat)) { /* Only accepts packets that are size 48 */
			printf("Recieved from existing client...");
			reverseMsgFormat(&recvBuffer);

			/* Checks that all the data received is a valid reply*/
			int modeCheck = recvBuffer.flags & CLIENT_MODE;
			int clientVersion = (recvBuffer.flags >> 3) & 0x07;

			if(modeCheck != CLIENT_MODE) printf("Message not from a valid server (Mode is not %d).\n",CLIENT_MODE);
			else if(clientVersion > 4 || clientVersion < 1) printf("Client Version is not between 1 and 4.\n");
			//else if (recvBuffer.stratum != 0) printf("Client Stratum is not 0.\n");
			else{
				msg = recvBuffer;
				setServerMessage(&msg, myTime);
				reverseMsgFormat(&msg);

				printf("Sending data to client\n");
				/* Sends the data to the server. */
				if ((numbytes = sendto(sockfd, (struct sntpMsgFormat *) &msg, sizeof(struct sntpMsgFormat),
						0, (struct sockaddr *) &group_addr, addr_len)) == -1) {
					perror("Server sendto error");
					exit(1);
				}
			}
		} else printf("Packet is not 48 bytes in Length. Ignoring. \n");
	}
	close(sockfd);
	return 0;
}

/* Sets the default values for the server message to the client. */
void setServerMessage( struct sntpMsgFormat* msg,struct timeval myTime) {
	/* Input the receive time of a message. */
	gettimeofday(&myTime, NULL);

	msg->revcTimestamp = tv_to_ntp(myTime);

	/* Sets up the initial information so the server knows I am a server*/
	msg->flags = 0; /* LI */
	msg->flags <<= 3;

	msg->flags |= 4; /* VN */
	msg->flags <<= 3;

	msg->flags |= 4; /* Mode. Client = 3, Server = 4 */

	msg->stratum = 1; /* Show accuracy of the clock. */
	msg->precision = 0;
	msg->rootDelay = 0;
	msg->rootDispersion = 0;
	msg->originateTimestamp = msg->transmitTimestamp;

	/* Gets the current system time and converts it into a 64bit timestamp*/
	gettimeofday(&myTime, NULL);
	msg->transmitTimestamp = tv_to_ntp(myTime);
}

