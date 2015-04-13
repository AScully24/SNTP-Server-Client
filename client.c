#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h> /* gettimeofday */
#include "ntp_time_conversion.h" /* Custom made header file for converting types.*/
#include <sys/select.h>

#define POLL_TIME 5

struct timeval myTime;

void setMessageClient(struct sntpMsgFormat* msg){
	struct timeval myTime;
	/* Sets up the initial information so the server knows I am a client*/
	gettimeofday(&myTime, NULL);

	// LI
    msg->flags = 0;
    msg->flags <<= 3;
    // VN
    msg->flags |= 4;
    msg->flags <<= 3;
    // Mode
    msg->flags |= 3;

    msg->transmitTimestamp = msg->originateTimestamp = tv_to_ntp(myTime);
}

struct sntpMsgFormat sendClientMessage(int sockfd, struct sockaddr_in their_addr,struct sntpMsgFormat msg) {
    /* Gets the current system time and converts it into a 64bit timestamp. */

    int bytesReceived;
    socklen_t addr_len = (socklen_t)sizeof (their_addr);
    struct sntpMsgFormat recvBuffer;
    
    // Set all values of msg to 0.
    //initialiseMsgFormat(&msg);
    gettimeofday(&myTime, NULL);

    printf("Time Before: ");
    print_tv(myTime);
    printf("\t");

//    /* Sets up the initial information so the server knows I am a client*/
//    // LI
//    msg.flags = 0;
//    msg.flags <<= 3;
//    // VN
//    msg.flags |= 4;
//    msg.flags <<= 3;
//    // Mode
//    msg.flags |= 3;
//
//    msg.transmitTimestamp = msg.originateTimestamp = tv_to_ntp(myTime);
    setMessageClient(&msg);

    reverseMsgFormat(&msg);

    /* Sends the data to the server. */
    if ((bytesReceived = sendto(sockfd, &msg, sizeof (msg), 0,
            (struct sockaddr *) &their_addr, addr_len)) == -1) {
        perror("Clent Send Error");
        exit(1);
    }


    /* TESTING SELECT */
    fd_set readfds;
    struct timeval tv;
    int n;

    // clear the set ahead of time
    FD_ZERO(&readfds);

    // add our descriptors to the set
    FD_SET(sockfd, &readfds);

    //Sockfd is our only  socket to check.
    n  = sockfd +1;

    // wait until either socket has data ready to be recv()d (timeout 10.5 secs)
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    int rv = select(n, &readfds, NULL, NULL, &tv);

    bytesReceived = 0;

    if (rv == -1) {
    	perror("RV Error");
    }
    else if (rv == 0) {
        printf("Timeout occurred!  No data after 0.1 seconds.\n");
    } else {
        // one or both of the descriptors have data
        if (FD_ISSET(sockfd, &readfds)) {
            if ((bytesReceived = recvfrom(sockfd, (struct sntpMsgFormat *) &recvBuffer, sizeof (struct sntpMsgFormat), 0,
                    (struct sockaddr *) &their_addr, &addr_len)) == -1) {
                perror("Client Recieve Error");
                exit(1);
            }
        }
    }


    /* END TEST */
    /* Server sends back its reply. ORIGINAL*/
//    bytesReceived = 0;
//    if ((bytesReceived = recvfrom(sockfd, (struct sntpMsgFormat *) &recvBuffer, sizeof (struct sntpMsgFormat), 0,
//            (struct sockaddr *) &their_addr, &addr_len)) == -1) {
//        perror("Client Recieve Error");
//        exit(1);
//    }
    gettimeofday(&myTime, NULL);

    if (bytesReceived > 0) {
        /* Analyse the server data and set the new system time. */
        //ntpTime = htobe64(recvBuffer.transmitTimestamp);
    	reverseMsgFormat(&recvBuffer);
    	reverseMsgFormat(&msg);

    	msg.refTimestamp = recvBuffer.transmitTimestamp;
    	msg.revcTimestamp = tv_to_ntp(myTime);

        myTime = ntp_to_tv(recvBuffer.transmitTimestamp);
        printf("Time After: ");
        print_tv(myTime);
        printf("\n");
    } else printf("No valid server reply.\n");

    return msg;
}
