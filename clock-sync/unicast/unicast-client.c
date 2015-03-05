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
#define POLL_TIME   10 /* Number of seconds to wait until sending to the server again */
#define arrSize 5

/* These are used to create a timestamp in the correct format (1st January 1900) */
const unsigned long long EPOCH = 2208988800ULL;
const unsigned long long NTP_SCALE_FRAC = 4294967295ULL;

struct msgFormat {
    unsigned char flags;
    unsigned char stratum;
    unsigned char poll;
    unsigned char precision;
    unsigned long rootDelay;
    unsigned long rootDispersion;
    unsigned long refIdentifier;
    unsigned long long refTimestamp;
    unsigned long long originateTimestamp;
    unsigned long long revcTimestamp;
    unsigned long long transmitTimestamp;
    unsigned long keyIdentifier;
    unsigned long long messageDigest;
};

/* Returns an NTP timestamp. a = (b * x) / c TRY TO IMPROVE THIS.*/
unsigned long long tv_to_ntp(struct timeval tv) {
    unsigned long long tv_ntp, tv_usecs;

    tv_ntp = tv.tv_sec + EPOCH;
    tv_usecs = (NTP_SCALE_FRAC * tv.tv_usec) / 1000000UL;

    return (tv_ntp << 32) | tv_usecs;
}

/* Returns an timvalue struct from an NTP timestamp. x = a * c / b TRY TO IMPROVE THIS.*/
struct timeval ntp_to_tv(unsigned long long ntp) {
    unsigned long long tv_secs, tv_usecs;
    tv_usecs = ntp & 0xFFFFFFFF;
    tv_secs = (ntp >> 32) & 0xFFFFFFFF;

    struct timeval temp;

    tv_secs = tv_secs - EPOCH;
    //         a        c           b
    tv_usecs = (tv_usecs + 1) * 1000000UL / NTP_SCALE_FRAC;
    //    tv_usecs = (NTP_SCALE_FRAC * tv.tv_usec) / 1000000UL;

    temp.tv_sec = (time_t) tv_secs;
    temp.tv_usec = (suseconds_t) tv_usecs;
    return temp;
}

/* Prints a timeval in a human readable format */
void print_tv(struct timeval tv) {
    time_t nowtime;
    struct tm *nowtm;
    char tmbuf[64], buf[64];

    nowtime = tv.tv_sec;
    nowtm = localtime(&nowtime);
    strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
    snprintf(buf, sizeof buf, "%s.%06d", tmbuf, (int) tv.tv_usec);
    printf("%s", buf);
}

/* Reverse the order of a char array. */
void reverseCharArray(unsigned char *array, int size) {

    int i;
    int end = size-1;
    for (i = 0; i < (size/2); i++) {
        *(array + i) = *(array + i) ^ *(array + end);
        *(array + end) = *(array + i) ^ *(array + end);
        *(array + i) = *(array + i) ^ *(array + end);
        end--;
        //printf("\n%d\n",i);
    }
}

void reverseMsgFormat(struct msgFormat *msg){
    reverseCharArray(&msg->originateTimestamp[0], 8);
}

int main(int argc, char * argv[]) {

    int sockfd, numbytes;
    struct hostent *he;
    struct sockaddr_in their_addr; /* server address info */
    struct msgFormat msg;
    struct timeval myTime;
    char serverIP[] = "time-a.nist.gov";
    //char serverIP[] = "10.167.158.236";
    struct msgFormat recvBuffer;
    socklen_t addr_len = (socklen_t)sizeof (struct sockaddr);
//
//    char testing[arrSize] = "12345";
//
//    reverseCharArray(&testing[0], arrSize);
//    
//    int t;
//    for (t = 0; t < arrSize; t++) {
//        printf("%c", testing[t]);
//    }
//
//    printf("\n\n\n\n");
//    //    

    /* resolve server host name or IP address */
    if ((he = gethostbyname(serverIP)) == NULL) {
        perror("Server gethostbyname");
        exit(1);
    }

    /* Setup the socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Talker socket");
        exit(1);
    }

    /* Server details */
    memset(&their_addr, 0, sizeof (their_addr)); /* zero struct */
    their_addr.sin_family = AF_INET; /* host byte order .. */
    their_addr.sin_port = htons(SERVER_PORT); /* .. short, netwk byte order */
    their_addr.sin_addr = *((struct in_addr *) he->h_addr);

    //while (1) {
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
    msg.flags |= 3;

    /* 
     * 
     * 
     * Gets the current system time and converts it into a 64bit timestamp. */
    gettimeofday(&myTime, NULL);
    unsigned long long ntpTime = tv_to_ntp(myTime);

    printf("Time Before: ");
    print_tv(myTime);
    printf("\t");
    int i;
    
    
    for (i = 7; i > -1; i--) {
        msg.originateTimestamp[i] = ntpTime & 0xFF;
        msg.transmitTimestamp[i] = ntpTime & 0xFF;
        ntpTime = ntpTime >> 8;
    }
    
    reverseMsgFormat(&msg);
    /*
     * 
     * 
     *  Sends the data to the server. */
    if ((numbytes = sendto(sockfd, &msg, sizeof (struct msgFormat), 0,
            (struct sockaddr *) &their_addr, sizeof (struct sockaddr))) == -1) {
        perror("Server sendto error");
        exit(1);
    }
    printf("bytes sent: %d\t", numbytes);

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
    //msg = recvBuffer;

    ntpTime = 0x00;
    i = 0;
    for (i = 0; i < 8; i++) {
        //        for (i = 7; i > -1; i--) {
        //unsigned char revcTimestamp[8];
        ntpTime = ntpTime << 8;
        ntpTime = ntpTime | recvBuffer.transmitTimestamp[i];
    }

    myTime = ntp_to_tv(ntpTime);
    printf("Time After: ");
    print_tv(myTime);
    printf("\n");
    /* Sets the system clock to the new time */
    //        int timeError = 0;
    //        if ((timeError = settimeofday(&myTime, NULL)) == -1) {
    //            perror("Set Time ");
    //            exit(1);
    //        }
    //sleep(POLL_TIME);
    //}
    close(sockfd);

    return 0;
}