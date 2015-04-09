/* 
 * File:   client.h
 * Author: Anthony Scully
 *
 * Created on 30 March 2015, 20:31
 */

#ifndef CLIENT_H
#define	CLIENT_H

void clientHandler(int , struct sockaddr_in);
void sendClientMessage(int, struct sockaddr_in);

#endif	/* CLIENT_H */

