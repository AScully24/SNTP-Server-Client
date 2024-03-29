
#include <netdb.h> /* for gethostbyname() */
/* 
 * File:   ntp_time_conversion.h
 * Author: Anthony Scully
 *
 * Created on 28 March 2015, 14:06
 */

#ifndef NTP_TIME_CONVERSION_H
#define	NTP_TIME_CONVERSION_H

/* Message structure for SNTP. 48 Bytes in size*/
struct sntpMsgFormat {
    u_int8_t flags;
    u_int8_t stratum;
    u_int8_t poll;
    int8_t precision;
    int32_t rootDelay;
    uint32_t rootDispersion;
    uint32_t refIdentifier;
    uint64_t refTimestamp;
    uint64_t originateTimestamp;
    uint64_t revcTimestamp;
    uint64_t transmitTimestamp;
};

uint64_t tv_to_ntp(struct timeval);
struct timeval ntp_to_tv(unsigned long long);
void print_tv(struct timeval);
void initialiseMsgFormat(struct sntpMsgFormat*);
void reverseMsgFormat(struct sntpMsgFormat*);
void printMsgDetails(struct sntpMsgFormat);

#ifdef	__cplusplus
extern "C" {
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* NTP_TIME_CONVERSION_H */
