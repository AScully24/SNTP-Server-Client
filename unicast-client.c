/* client - Used to communicate with the server for clock sync.
 * 
 * 
 */
#include <stdio.h>
#//include <stdlib.h>
//#include <unistd.h>
#include <errno.h>
//#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <netdb.h> /* for gethostbyname() */
#include <sys/time.h> /* gettimeofday */
//#include <time.h> /* clock_gettime */
//#include <endian.h> /* For converting to various endians */
#include "ntp_time_conversion.h" /* Custom made header file for converting types.*/

#define SERVER_PORT 123 /* server port the client connects to */
#define POLL_TIME   5//10 /* Number of seconds to wait until sending to the server again */

int main(int argc, char * argv[]) {

    int sockfd, numbytes;
    struct hostent *he;
    struct sockaddr_in their_addr; /* server address info */
    struct msgFormat msg;
    struct timeval myTime;
    char serverIP[] = "time-a.nist.gov";
    //char serverIP[] = "127.0.0.1";
    struct msgFormat recvBuffer;
    socklen_t addr_len = (socklen_t)sizeof (struct sockaddr);

    initialiseMsgFormat(&msg);
    initialiseMsgFormat(&recvBuffer);

    printf("Resolving server IP: ");
    /* resolve server host name or IP address */
    if ((he = gethostbyname(serverIP)) == NULL) {
        perror("Server gethostbyname");
        exit(1);
    }
    printf("OK\n");

    printf("Setting up socket Port %d: ", SERVER_PORT);
    /* Setup the socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Talker socket");
        exit(1);
    }
    printf("OK\n");


    memset(&their_addr, 0, sizeof (their_addr)); /* zero struct */
    /* Server details */
    their_addr.sin_family = AF_INET; /* host byte order .. */
    their_addr.sin_port = htons(SERVER_PORT); /* .. short, netwk byte order */
    their_addr.sin_addr = *((struct in_addr *) he->h_addr);

    //while (1) {
    /* 
     * 
     * 
     * Sets up the initial information so the server knows I am a client*/
    printf("Initialising flags: ");
    // LI
    msg.flags = 0;
    msg.flags <<= 3;
    // VN
    msg.flags |= 4;
    msg.flags <<= 3;
    // Mode
    msg.flags |= 3;
    
    //memset(&myTime,0,sizeof(myTime));
    printf("OK\n");

    
    
    while (1) {
        /* 
         * 
         * 
         * Gets the current system time and converts it into a 64bit timestamp. */
        gettimeofday(&myTime, NULL);
        msg.originateTimestamp = tv_to_ntp(myTime);

        printf("Time Before: ");
        print_tv(myTime);
        printf("\t");

        reverseMsgFormat(&msg);

        /*
         * 
         * 
         *  Sends the data to the server. */
        if ((numbytes = sendto(sockfd, &msg, sizeof (msg), 0,
                (struct sockaddr *) &their_addr, sizeof (struct sockaddr))) == -1) {
            perror("Server sendto error");
            exit(1);
        }
        printf("bytes sent: %ld\t", sizeof (msg));

        /* 
         * 
         * 
         * Server sends back its reply. */
        numbytes = 0;
        if ((numbytes = recvfrom(sockfd, (struct msgFormat *) &recvBuffer, sizeof (struct msgFormat), 0,
                (struct sockaddr *) &their_addr, &addr_len)) == -1) {
            perror("Listener recvfrom");
            exit(1);
        }

        /* 
         * 
         * 
         * Analyse the server data and set the new system time. */
        //ntpTime = htobe64(recvBuffer.transmitTimestamp);
        myTime = ntp_to_tv(htobe64(recvBuffer.transmitTimestamp));
        printf("Time After: ");
        print_tv(myTime);
        printf("\n");
        sleep(POLL_TIME);
    }
    close(sockfd);

    return 0;
}