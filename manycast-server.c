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

    int sockfd;
    int numbytes = 0;
    struct ip_mreq mreq;
    struct sockaddr_in group_addr; /* Group address to listen on */
    struct sockaddr_in their_addr; /* Incoming  */
    struct sntpMsgFormat recvBuffer;
    socklen_t addr_len = (socklen_t)sizeof (struct sockaddr);

    struct hostent *he;
    char serverIP[] = "time.nist.gov";

    printf("Resolving Group IP: ");
    /* resolve server host name or IP address */
    if ((he = gethostbyname(serverIP)) == NULL) {
        perror("Server gethostbyname");
        exit(1);
    }
    printf("OK\n");

    initialiseMsgFormat(&recvBuffer);

    printf("Setting up socket Port %d: ", LISTEN_PORT);
    /* Setup the listen socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Listen socket");
        exit(1);
    }

    /* allow multiple sockets to use the same PORT number */
    u_int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes)) < 0) {
        perror("Reusing ADDR failed");
        exit(1);
    }

    printf("OK\n");

    printf("Listen Socket address: ");

    memset(&group_addr, 0, sizeof (group_addr));
    /* Group Listen*/
    group_addr.sin_family = AF_INET; /* host byte order .. */
    group_addr.sin_port = htons(LISTEN_PORT); /* .. short, netwk byte order */
    group_addr.sin_addr = *((struct in_addr *) he->h_addr); // CHANGE THIS - Needs to be a specific IP. Set default.

    memset(&their_addr, 0, sizeof (their_addr));
    /* Any Address Listen*/
    their_addr.sin_family = AF_INET; /* host byte order .. */
    their_addr.sin_port = htons(LISTEN_PORT); /* .. short, network byte order */
    their_addr.sin_addr.s_addr = INADDR_ANY; // CHANGE THIS - Needs to be a specific IP. Set default.


    if (bind(sockfd, (struct sockaddr *) &group_addr, sizeof (struct sockaddr)) == -1) {
        perror("Listener bind");
        exit(1);
    }
    printf("OK\n");

    /* use setsockopt() to request that the kernel join a multicast group */
    mreq.imr_multiaddr.s_addr = inet_addr(serverIP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof (mreq)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    while (1) {

        if ((numbytes = recvfrom(sockfd, (struct sntpMsgFormat *) &recvBuffer, sizeof (struct sntpMsgFormat), 0,
                (struct sockaddr *) &group_addr, &addr_len)) == -1) {
            perror("Listener recvfrom");
            exit(1);
        }

        // Establish a connection with the client
        if (numbytes > 0) {
            serverHandler(sockfd, group_addr);
        }
    }
    close(sockfd);
    return 0;
}