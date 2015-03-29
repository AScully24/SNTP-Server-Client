
#include <netdb.h> /* for gethostbyname() */
/* 
 * File:   ntp_time_conversion.h
 * Author: Anthony Scully
 *
 * Created on 28 March 2015, 14:06
 */

#ifndef NTP_TIME_CONVERSION_H
#define	NTP_TIME_CONVERSION_H

struct msgFormat {
    u_int8_t flags;
    u_int8_t stratum;
    u_int8_t poll;
    u_int8_t precision;
    uint32_t rootDelay;
    uint32_t rootDispersion;
    uint32_t refIdentifier;
    uint64_t refTimestamp;
    uint64_t originateTimestamp;
    uint64_t revcTimestamp;
    uint64_t transmitTimestamp;
    //uint32_t keyIdentifier;
    //uint64_t messageDigest;
};

uint64_t tv_to_ntp(struct timeval);
struct timeval ntp_to_tv(unsigned long long);
void print_tv(struct timeval);
void initialiseMsgFormat(struct msgFormat*);
void reverseMsgFormat(struct msgFormat*);

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* NTP_TIME_CONVERSION_H */