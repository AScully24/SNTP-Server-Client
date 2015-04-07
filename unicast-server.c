/* Server - Waits for a client to send a message, then sends back an updated time. */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h> /* gettimeofday */
#include "ntp_time_conversion.h" /* Custom made header file for converting types.*/

#define LISTEN_PORT 1234 /* The port the server listens on. */

int main(int argc, char * argv[]) {

    int sockfd, numbytes;
    struct sockaddr_in listen_addr; /* server address info */
    struct sntpMsgFormat msg;
    struct timeval myTime;
    struct sntpMsgFormat recvBuffer;
    socklen_t addr_len = (socklen_t)sizeof (struct sockaddr);
    socklen_t msgFormatSize = (socklen_t) sizeof (struct sntpMsgFormat);

    initialiseMsgFormat(&msg);
    initialiseMsgFormat(&recvBuffer);

    printf("Setting up socket Port %d: ", LISTEN_PORT);
    /* Setup the listen socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Listen socket");
        exit(1);
    }
    printf("OK\n");

    printf("Setting to listen on Port %d: ", LISTEN_PORT);

    memset(&listen_addr, 0, sizeof (listen_addr));
    /* Listen port*/
    listen_addr.sin_family = AF_INET; /* host byte order .. */
    listen_addr.sin_port = htons(LISTEN_PORT); /* .. short, netwk byte order */
    listen_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *) &listen_addr, sizeof (struct sockaddr)) == -1) {
        perror("Listener bind");
        exit(1);
    }
    printf("OK\n");

    while (1) {
        /* Server sends back its reply. */
        printf("Waiting for client request...");
        numbytes = 0;
        if ((numbytes = recvfrom(sockfd, (struct sntpMsgFormat *) &recvBuffer, msgFormatSize, 0,
                (struct sockaddr *) &listen_addr, &addr_len)) == -1) {
            perror("Listener recvfrom");
            exit(1);
        }
        printf("Received message from %s\n", inet_ntoa(listen_addr.sin_addr));

        /* Input the receive time of a message. */
        gettimeofday(&myTime, NULL);
        uint64_t recieveTime = tv_to_ntp(myTime);
        reverseMsgFormat(&recvBuffer);
        msg = recvBuffer;
        msg.revcTimestamp = recieveTime;

        /* Sets up the initial information so the server knows I am a client*/
        // LI
        msg.flags = 0;
        msg.flags <<= 3;
        // VN
        msg.flags |= 4;
        msg.flags <<= 3;
        // Mode
        msg.flags |= 4;

        msg.stratum = 2;
        msg.precision = 0;
        msg.rootDelay = 0;
        msg.rootDispersion = 0;

        /* Gets the current system time and converts it into a 64bit timestamp. */
        gettimeofday(&myTime, NULL);
        msg.transmitTimestamp = tv_to_ntp(myTime);
        reverseMsgFormat(&msg);

        /* Sends the data to the server. */
        if ((numbytes = sendto(sockfd, (struct sntpMsgFormat *) &msg, msgFormatSize, 0,
                (struct sockaddr *) &listen_addr, &addr_len)) == -1) {
            perror("Server sendto error");
            exit(1);
        }

        printf("Reply sent to client\n");
    }
    close(sockfd);

    return 0;
}