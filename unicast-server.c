/* Server - Waits for a client to send a message, then sends back an updated time. */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h> /* gettimeofday */
#include "ntp_time_conversion.h" /* Custom made header file for converting types.*/
#include "server.h"

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

    printf("Listen Socket address: ");

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
        serverHandler(sockfd, listen_addr);
    }
    close(sockfd);

    return 0;
}