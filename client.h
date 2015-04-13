/* 
 * File:   client.h
 * Author: Anthony Scully
 *
 * Created on 30 March 2015, 20:31
 */

#ifndef CLIENT_H
#define	CLIENT_H

void setMessageClient(struct sntpMsgFormat*);
struct sntpMsgFormat sendClientMessage(int, struct sockaddr_in,struct sntpMsgFormat);

#endif	/* CLIENT_H */

