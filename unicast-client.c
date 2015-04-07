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
#define POLL_TIME   5 /* Number of seconds to wait until sending to the server again */

int main(int argc, char * argv[]) {
    
    int sockfd;
    struct hostent *he;
    struct sockaddr_in their_addr; /* server address info */
//    char serverIP[] = "time-a.nist.gov";
    char serverIP[] = "time.nist.gov";
    
    
    printf("Argc: %d\n",argc);
    if (argc == 1) {
        printf("IP/Server address not input. Using default server.\n");

        printf("Resolving server IP %s: ",serverIP);
        /* resolve server host name or IP address */
        if ((he = gethostbyname(serverIP)) == NULL) {
            perror("Server gethostbyname");
            exit(1);
        }
    } else if (argc == 2) {

        printf("Resolving server IP %s: \n",argv[1]);
        /* resolve server host name or IP address */
        if ((he = gethostbyname(argv[1])) == NULL) {
            perror("Server gethostbyname");
            exit(1);
        }
    } else {
        printf("Invalid argument count.\n");
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
    
    clientHandler(sockfd, their_addr);
    
    return 0;
}