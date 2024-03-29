#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h> /* gettimeofday */
#include "ntp_time_conversion.h" /* Custom made header file for converting types.*/



void setServerMessage( struct sntpMsgFormat* msg) {
	/* Input the receive time of a message. */
	struct timeval myTime;
	gettimeofday(&myTime, NULL);

	msg->revcTimestamp = tv_to_ntp(myTime);

	/* Sets up the initial information so the server knows I am a client*/
	// LI
	msg->flags = 0;
	msg->flags <<= 3;
	// VN
	msg->flags |= 4;
	msg->flags <<= 3;
	// Mode
	msg->flags |= 4;

	msg->stratum = 2;
	msg->precision = 0;
	msg->rootDelay = 0;
	msg->rootDispersion = 0;

	/* Gets the current system time and converts it into a 64bit timestamp-> */
	gettimeofday(&myTime, NULL);
	msg->transmitTimestamp = tv_to_ntp(myTime);
}

void serverHandler(int sockfd, struct sockaddr_in listen_addr) {

	struct sntpMsgFormat msg;
	struct sntpMsgFormat recvBuffer;

	int numbytes = 0;
	socklen_t addr_len = (socklen_t) sizeof(listen_addr);
	socklen_t msgFormatSize = (socklen_t) sizeof(struct sntpMsgFormat);

	initialiseMsgFormat(&msg);
	initialiseMsgFormat(&recvBuffer);

	/* Server sends back its reply. */
	printf("Waiting for client request...");
	numbytes = 0;

	if ((numbytes = recvfrom(sockfd, (struct sntpMsgFormat *) &recvBuffer,
			msgFormatSize, 0, (struct sockaddr *) &listen_addr, &addr_len))
			== -1) {
		perror("Listener recvfrom");
		exit(1);
	}
	//printf("Received message from %s\n", inet_ntoa(listen_addr.sin_addr));
	reverseMsgFormat(&recvBuffer);
	msg = recvBuffer;

	setServerMessage(&msg);

	reverseMsgFormat(&msg);

	/* Sends the data to the server. */
	if ((numbytes = sendto(sockfd, (struct sntpMsgFormat *) &msg, msgFormatSize,
			0, (struct sockaddr *) &listen_addr, addr_len)) == -1) {
		perror("Server sendto error");
		exit(1);
	}

	printf("Reply sent to client\n");

}
