#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h> /* gettimeofday */
#include "ntp_time_conversion.h" /* Custom made header file for converting types.*/

#define POLL_TIME 5

void sendClientMessage(int, struct sockaddr_in);

void clientHandler(int sockfd, struct sockaddr_in their_addr) {
    struct sntpMsgFormat msg;
    struct sntpMsgFormat recvBuffer;
    struct timeval myTime;
    int bytesReceived;

    // Initialise Variables
    socklen_t addr_len = (socklen_t)sizeof (their_addr);
    initialiseMsgFormat(&msg);
    initialiseMsgFormat(&recvBuffer);

    char theirIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(their_addr.sin_addr), theirIP, INET_ADDRSTRLEN);

    /* Sets up the initial information so the server knows I am a client*/
    // LI
    msg.flags = 0;
    msg.flags <<= 3;
    // VN
    msg.flags |= 4;
    msg.flags <<= 3;
    // Mode
    msg.flags |= 3;

    while (1) {
        sendClientMessage(sockfd, their_addr);
        sleep(POLL_TIME);
    }
}

void sendClientMessage(int sockfd, struct sockaddr_in their_addr) {
    /* Gets the current system time and converts it into a 64bit timestamp. */
    struct timeval myTime;
    int bytesReceived;
    socklen_t addr_len = (socklen_t)sizeof (their_addr);
    gettimeofday(&myTime, NULL);
    struct sntpMsgFormat msg;
    
    // Set all values of msg to 0.
    initialiseMsgFormat(&msg);
    msg.originateTimestamp = tv_to_ntp(myTime);

    printf("Time Before: ");
    print_tv(myTime);
    printf("\t");


    /* Sets up the initial information so the server knows I am a client*/
    // LI
    msg.flags = 0;
    msg.flags <<= 3;
    // VN
    msg.flags |= 4;
    msg.flags <<= 3;
    // Mode
    msg.flags |= 3;

    reverseMsgFormat(&msg);

    /* Sends the data to the server. */
    if ((bytesReceived = sendto(sockfd, &msg, sizeof (msg), 0,
            (struct sockaddr *) &their_addr, addr_len)) == -1) {
        perror("Send Error");
        exit(1);
    }
    printf("bytes sent: %ld\t", sizeof (msg));

    /* Server sends back its reply. */
    bytesReceived = 0;
    if ((bytesReceived = recvfrom(sockfd, (struct sntpMsgFormat *) &msg, sizeof (struct sntpMsgFormat), 0,
            (struct sockaddr *) &their_addr, &addr_len)) == -1) {
        perror("Listener recvfrom");
        exit(1);
    }

    if (bytesReceived > 0) {
        /* Analyse the server data and set the new system time. */
        //ntpTime = htobe64(recvBuffer.transmitTimestamp);
        myTime = ntp_to_tv(htobe64(msg.transmitTimestamp));
        printf("Time After: ");
        print_tv(myTime);
        printf("\n");
        sleep(POLL_TIME);
    } else printf("No server reply. Resending packet.\n");
}