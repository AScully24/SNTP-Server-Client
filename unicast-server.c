/* Server - Waits for a client to send a message, then sends back an updated time.
 * 
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#//include <errno.h>
//#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <netdb.h> /* for gethostbyname() */
#include <sys/time.h> /* gettimeofday */
//#include <time.h> /* clock_gettime */
//#include <endian.h>
#include "ntp_time_conversion.h"

#define SERVER_PORT 123 /* server port the client connects to */
#define LISTEN_PORT 1234 /* The port the server listens on. */
#define POLL_TIME   10 /* Number of seconds to wait until sending to the client again */

int main(int argc, char * argv[]) {

    int sockfd, numbytes;
    struct sockaddr_in listen_addr; /* server address info */
    struct msgFormat msg;
    struct timeval myTime;
    struct msgFormat recvBuffer;
    socklen_t addr_len = (socklen_t)sizeof (struct sockaddr);

    initialiseMsgFormat(&msg);
    initialiseMsgFormat(&recvBuffer);

    printf("Setting up socket Port %d: ", SERVER_PORT);
    /* Setup the listen socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Listen socket");
        exit(1);
    }
    printf("OK\n");


    printf("Setting to listen on Port %d: ", SERVER_PORT);

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
        /* 
         * 
         * 
         * Server sends back its reply. */
        printf("Waiting for client request...");
        numbytes = 0;
        if ((numbytes = recvfrom(sockfd, (struct msgFormat *) &recvBuffer, sizeof (struct msgFormat), 0,
                (struct sockaddr *) &listen_addr, &addr_len)) == -1) {
            perror("Listener recvfrom");
            exit(1);
        }
        printf("Received message from %s\n", inet_ntoa(listen_addr.sin_addr));
        /* 
         * 
         * 
         * Input the receive time of a message. */
        gettimeofday(&myTime, NULL);
        uint64_t recieveTime = tv_to_ntp(myTime);
        reverseMsgFormat(&recvBuffer);
        msg = recvBuffer;
        msg.revcTimestamp = recieveTime;

        /* 
         * 
         * 
         * Sets up the initial information so the server knows I am a client*/
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

        /* 
         * 
         * 
         * Gets the current system time and converts it into a 64bit timestamp. */
        gettimeofday(&myTime, NULL);
        msg.transmitTimestamp = tv_to_ntp(myTime);
        reverseMsgFormat(&msg);

        /*
         * 
         * 
         *  Sends the data to the server. */
        if ((numbytes = sendto(sockfd, (struct msgFormat *) &msg, sizeof (struct msgFormat), 0,
                (struct sockaddr *) &listen_addr, sizeof (struct sockaddr))) == -1) {
            perror("Server sendto error");
            exit(1);
        }
        
        printf("Reply sent to client\n");
    }
    close(sockfd);

    return 0;
}