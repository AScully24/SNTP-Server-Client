/* Shared functions used by both the client and the server. */

#include <time.h>
#include <stdio.h>
#include <math.h>
#include "ntp_time_conversion.h"

/* These are used to create a timestamp in the correct format (1st January 1900 / 1st January 1970) */
const unsigned long long SECONDS_OFFSET = 2208988800ULL;
const unsigned long long N_SECONDS_OFFSET = 4294967295ULL;

/* Converts a TimeVal struct to an NTP timestamp. */
uint64_t tv_to_ntp(struct timeval tv) {
    unsigned long long tv_ntp, tv_usecs;

    tv_ntp = tv.tv_sec + SECONDS_OFFSET;
    tv_usecs = (N_SECONDS_OFFSET * tv.tv_usec) / 1000000UL;

    return (tv_ntp << 32) | tv_usecs;
}

/* Converts an NTP timestamp to a TimeVal struct. */
struct timeval ntp_to_tv(unsigned long long ntp) {
    unsigned long long tv_secs, tv_usecs;
    tv_usecs = ntp & 0xFFFFFFFF;
    tv_secs = (ntp >> 32) & 0xFFFFFFFF;

    struct timeval temp;

    tv_secs = tv_secs - SECONDS_OFFSET;
    //         a        c           b
    tv_usecs = (tv_usecs + 1) * 1000000UL / N_SECONDS_OFFSET;

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
    strftime(tmbuf, sizeof (tmbuf), "%Y-%m-%d %H:%M:%S", nowtm);

    snprintf(buf, sizeof buf, "%s.%06d", tmbuf, (int) tv.tv_usec);
    printf("%s\n", buf);
}

/* Set all values of a struct sntpMsgFormat to 0. */
void initialiseMsgFormat(struct sntpMsgFormat *msg) {
    msg->flags = 0;
    msg->stratum = 0;
    msg->poll = 0;
    msg->precision = 0;
    msg->rootDelay = 0;
    msg->rootDispersion = 0;
    msg->refIdentifier = 0;
    msg->refTimestamp = 0;
    msg->originateTimestamp = 0;
    msg->revcTimestamp = 0;
    msg->transmitTimestamp = 0;
}

/* Converts data from/to Network Byte Order/Host Byte Order */
void reverseMsgFormat(struct sntpMsgFormat * msg) {
    msg->rootDelay = htobe32(msg->rootDelay);
    msg->rootDispersion = htobe32(msg->rootDispersion);
    msg->refIdentifier = htobe32(msg->refIdentifier);
    msg->refTimestamp = htobe64(msg->refTimestamp);
    msg->originateTimestamp = htobe64(msg->originateTimestamp);
    msg->revcTimestamp = htobe64(msg->revcTimestamp);
    msg->transmitTimestamp = htobe64(msg->transmitTimestamp);
}

/* Print an SNTP message in a human readable format. */
void printMsgDetails(struct sntpMsgFormat msg) {
    int mode = msg.flags & 0x07;
    int version = (msg.flags >> 3) & 0x07;

    /* Shifting bits required for retrieving flags. */
    printf("Leap Indicator: %d\n", msg.flags >> 6);
    printf("Mode: %d\n", mode);
    printf("Version: %d\n", version);
    printf("Stratum: %d\n", msg.stratum);
    printf("Poll: %f\n", pow(2,msg.poll));
    
    printf("Precision: %f\n", pow(2,msg.precision));
    printf("Root Delay: %d\n", msg.rootDelay);
    printf("Root Dispersion: %d\n", msg.rootDispersion);
    
    /* Extracts a reference identifier into a char array. */
    char refChar[5];
    int i;
    int32_t temp = msg.refIdentifier;
    for (i = 3; i > -1; i--) {
        refChar[i] = temp & 0xFF;
        temp = temp >> 8;
    }
    
    refChar[4] = '\0';
    printf("Reference Identifier: %s\n", refChar); /* IP addresses not yet supported. */

    printf("Reference Timestamp: ");
    print_tv(ntp_to_tv(msg.refTimestamp));

    printf("Originate Timestamp: ");
    print_tv(ntp_to_tv(msg.originateTimestamp));

    printf("Receive Timestamp: ");
    print_tv(ntp_to_tv(msg.revcTimestamp));

    printf("Transmit Timestamp: ");
    print_tv(ntp_to_tv(msg.transmitTimestamp));
}