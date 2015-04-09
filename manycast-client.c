/* client - Used to communicate with the server for clock sync. */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h> /* gettimeofday */
#include "ntp_time_conversion.h" /* Custom made header file for converting types.*/
#include "client.h"

#define SERVER_PORT 123 /* server port the client connects to */
#define POLL_TIME   1 /* Number of seconds to wait until sending to the server again */
#define MAX_SERVER_COUNT 3

in_addr_t serverList[MAX_SERVER_COUNT];
int serverCount = 0;

int isServerInList(in_addr_t addr) {
    int i;
    for (i = 0; i < 3; i++) {
        if (addr == serverList[i]) {
            return 1;
        }
    }
    return 0;
}

int main(int argc, char * argv[]) {

    int sockfd, numbytes;
    struct hostent *he;
    struct sockaddr_in their_addr; /* server address info */
    struct sntpMsgFormat msg;
    struct timeval myTime; /* Stores the time from the system clock or the server*/
    char serverIP[] = "time-a.nist.gov";
    //    char serverIP[] = "192.168.0.1";
    struct sntpMsgFormat recvBuffer;
    socklen_t addr_len = (socklen_t)sizeof (struct sockaddr);

    initialiseMsgFormat(&msg);
    initialiseMsgFormat(&recvBuffer);

    printf("Manycast Client\n");

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
    
    /* allow multiple sockets to use the same PORT number */
    u_int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes)) < 0) {
        perror("Reusing ADDR failed");
        exit(1);
    }
    
    printf("OK\n");


    memset(&their_addr, 0, sizeof (their_addr)); /* zero struct */
    /* Server details */
    their_addr.sin_family = AF_INET; /* host byte order .. */
    their_addr.sin_port = htons(SERVER_PORT); /* .. short, network byte order */
    their_addr.sin_addr = *((struct in_addr *) he->h_addr);

    struct sockaddr_in any_addr;
    memset(&any_addr, 0, sizeof (any_addr)); /* zero struct */
    /* Server details */
    any_addr.sin_family = AF_INET; /* host byte order .. */
    any_addr.sin_port = htons(SERVER_PORT); /* .. short, network byte order */
    any_addr.sin_addr.s_addr = INADDR_ANY;
    
    /* Sets up the initial information so the server knows I am a client*/
    printf("Initialising flags: ");
    // LI
    msg.flags = 0;
    msg.flags <<= 3;
    // VN
    msg.flags |= 4;
    msg.flags <<= 3;
    // Mode
    msg.flags |= 3;

    printf("OK\n");

    // Sets up the initial connection with the server
    while (1) {
        reverseMsgFormat(&msg);
        
        /* Sends the data to the server. */
        if ((numbytes = sendto(sockfd, &msg, sizeof (msg), 0,
                (struct sockaddr *) &their_addr, addr_len)) == -1) {
            perror("Server sendto error");
            exit(1);
        }
        
        /* Server sends back its reply. */
        numbytes = 0;
        if ((numbytes = recvfrom(sockfd, (struct sntpMsgFormat *) &recvBuffer, sizeof (struct sntpMsgFormat), 0,
                (struct sockaddr *) &any_addr, &addr_len)) == -1) {
            perror("Listener recvfrom");
            exit(1);
        }
        
        if (numbytes > 0) {
            if (serverCount < 3 && isServerInList(any_addr.sin_addr.s_addr) == 0) {
                serverList[serverCount++] = any_addr.sin_addr.s_addr;
                
                // Converts IP to char for debugging.
                char newServerIP[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(any_addr.sin_addr), newServerIP, INET_ADDRSTRLEN);
                printf("Adding: %s\n", newServerIP);
            }
        } else printf("Data not received attempting again.\n");
        
        int i;
        for (i = 0; i < serverCount; i++) {
            any_addr.sin_addr.s_addr = serverList[i];
            sendClientMessage(sockfd,any_addr);
        }

        any_addr.sin_addr.s_addr = INADDR_ANY;
        sleep(POLL_TIME);
    }
    close(sockfd);

    return 0;
}