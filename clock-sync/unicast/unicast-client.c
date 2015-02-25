/* client - Used to communicate with the server for clock sync.
 * 
 * 
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
#include <sys/time.h> /* gettimeofday */
#include <time.h> /* clock_gettime */

#define SERVER_PORT 123 /* server port the client connects to */
#define LISTENPORT 59765 /* server port the client recieves on */

/* These are used to create a timestamp in the correct format (1st January 1900) */
const unsigned long long EPOCH = 2208988800ULL;
const unsigned long long NTP_SCALE_FRAC = 4294967295ULL;

struct msgFormat {
    unsigned char flags;
    unsigned char stratum;
    unsigned char poll;
    unsigned char precision;
    unsigned char rootDelay[4];
    unsigned char rootDispersion[4];
    unsigned char refIdentifier[4];
    unsigned char refTimestamp[8];
    unsigned char originateTimestamp[8];
    unsigned char revcTimestamp[8];
    unsigned char transmitTimestamp[8];
    unsigned char keyIdentifier[4];
    unsigned char messageDigestPart1[4];
    unsigned char messageDigestPart2[4];
};

unsigned long long tv_to_ntp(struct timeval tv) {
    unsigned long long tv_ntp, tv_usecs;

    tv_ntp = tv.tv_sec + EPOCH;
    tv_usecs = (NTP_SCALE_FRAC * tv.tv_usec) / 1000000UL;

    return (tv_ntp << 32) | tv_usecs;
}

int main(int argc, char * argv[]) {

    int sockfd, numbytes;
    struct hostent *he;
    struct sockaddr_in their_addr; /* server address info */
    struct msgFormat *msg;
    struct timeval myTime;

    msg = (struct msgFormat*) malloc(sizeof (struct msgFormat));

    // LI
    msg->flags = 0;
    msg->flags = msg->flags << 3;
    // VN
    msg->flags += 4;
    msg->flags = msg->flags << 3;
    // Mode
    msg->flags += 3;

    char serverIP[] = "time-a.nist.gov";

    /* resolve server host name or IP address */
    if ((he = gethostbyname(serverIP)) == NULL) {
        perror("Server gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Talker s1ocket");
        exit(1);
    }

    // Server details
    memset(&their_addr, 0, sizeof (their_addr)); /* zero struct */
    their_addr.sin_family = AF_INET; /* host byte order .. */
    their_addr.sin_port = htons(SERVER_PORT); /* .. short, netwk byte order */
    their_addr.sin_addr = *((struct in_addr *) he->h_addr);

    gettimeofday(&myTime, NULL);
    unsigned long long ntpTime = tv_to_ntp(myTime);

    //ntp_to_char_arr(&msg->originateTimestamp, ntpTime);

    int i;
    for (i = 7; i > -1; i--) {
        //unsigned char revcTimestamp[8];
        msg->originateTimestamp[i] = ntpTime & 0xFF;
        msg->transmitTimestamp[i] = ntpTime & 0xFF;
        ntpTime = ntpTime >> 8;
    }

    if ((numbytes = sendto(sockfd, msg, sizeof (struct msgFormat), 0, (struct sockaddr *) &their_addr, sizeof (struct sockaddr)))
            == -1) {
        perror("Server sendto error");
        exit(1);
    }

    printf("Sent %d bytes to %s\n", numbytes, inet_ntoa(their_addr.sin_addr));

    /* THIS WILL BE FOR WHEN THE SERVER SENDS INFO BACK.*/
    char recvBuffer[9999];
    socklen_t addr_len = (socklen_t)sizeof (struct sockaddr);
    numbytes = 0;

    if ((numbytes = recvfrom(sockfd, recvBuffer, strlen(recvBuffer) - 1, 0,
            (struct sockaddr *) &their_addr, &addr_len)) == -1) {
        perror("Listener recvfrom");
        exit(1);
    }

    if (numbytes > 0) {
        printf("Received from server %d bytes from %s\n", numbytes,
                inet_ntoa(their_addr.sin_addr));
    }

    close(sockfd);

    return 0;
}