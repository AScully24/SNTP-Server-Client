#include "ntp_time_conversion.h"
#include <time.h>
/* These are used to create a timestamp in the correct format (1st January 1900) */
const unsigned long long EPOCH = 2208988800ULL;
const unsigned long long NTP_SCALE_FRAC = 4294967295ULL;

/* Returns an NTP timestamp. a = (b * x) / c TRY TO IMPROVE THIS.*/
uint64_t tv_to_ntp(struct timeval tv) {
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
    strftime(tmbuf, sizeof (tmbuf), "%Y-%m-%d %H:%M:%S", nowtm);
    
    snprintf(buf, sizeof buf, "%s.%06d", tmbuf, (int) tv.tv_usec);
    printf("%s", buf);
}

void initialiseMsgFormat(struct msgFormat *msg) {
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
    //msg->keyIdentifier = 0;
    //msg->messageDigest = 0;
}

void reverseMsgFormat(struct msgFormat * msg) {
    msg->rootDelay = htobe32(msg->rootDelay);
    msg->rootDispersion = htobe32(msg->rootDispersion);
    msg->refIdentifier = htobe32(msg->refIdentifier);
    msg->refTimestamp = htobe64(msg->refTimestamp);
    msg->originateTimestamp = htobe64(msg->originateTimestamp);
    msg->revcTimestamp = htobe64(msg->revcTimestamp);
    msg->transmitTimestamp = htobe64(msg->transmitTimestamp);
}