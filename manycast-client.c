/* client - Used o communicate with the server for clock sync. */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h> /* gettimeofday */
#include <math.h>
#include <errno.h>
#include "ntp_time_conversion.h" /* Custom made header file for converting types.*/

#define POLL_TIME   15 /* Number of seconds to wait until sending to the server again */
#define SERVER_MODE	4 /* The Mode value that the server should have */
int SERVER_PORT = 123; /* server port the client connects to. Default 123., Can be change with arguements */

void setMessageClient(struct sntpMsgFormat*);

int main(int argc, char * argv[]) {
    /* 
     * Variable Declaration*/
    int sockfd, numbytes;
    struct hostent *he;
    struct sockaddr_in their_addr; /* server address info */
    struct sntpMsgFormat msg, recvBuffer;
    struct timeval myTime;
    char serverIP[] = "time-a.nist.gov";
    socklen_t addr_len = (socklen_t) sizeof (struct sockaddr);

    /* Argument handler */
    int serverArg = 0, manycast = 0;
    char *argIP;
    int i;
    for (i = 1; i < argc; i++) /* Skip argv[0] (program name). */ {
        /* Enters manycast mode. */
        if (strcmp(argv[i], "-m") == 0) {
            manycast = 1;
        }

        /* Allows the user to input a specific port*/
        if (strcmp(argv[i], "-p") == 0) {
            i++;
            int newPort = 0;
            int x = 0;
            while (argv[i][x] != '\0') {
                newPort *= 10;
                newPort += argv[i][x] - '0';
                x++;
            }
            SERVER_PORT = newPort;
        }
        /* Allows the user to input an ip address to connect to. */
        if (strcmp(argv[i], "-ip") == 0) {
            i++;
            serverArg = 1;
            argIP = malloc(sizeof (argv[i]));
            strcpy(argIP, argv[i]);
        }
    }

    /* 
     * Sets the default ip if a user did not the argument. */
    if (serverArg == 0) {
        argIP = malloc(sizeof (serverIP));
        strcpy(argIP, serverIP);
    }

    if (manycast == 0)
        printf("Entering Unicast Mode\n");
    else
        printf("Entering Manycast Mode\n");

    printf("Resolving server IP %s: ", argIP);
    /* 
     * Server host name or IP address handler. */
    if ((he = gethostbyname(argIP)) == NULL) {
        perror("gethostbyname");
        exit(1);
    }
    printf("OK\n");

    /* 
     * Setup the socket */
    printf("Setting up socket Port %d: ", SERVER_PORT);
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Talker socket");
        exit(1);
    }

    printf("OK\n");

    /* 
     * Server details */
    memset(&their_addr, 0, sizeof (their_addr)); /* zero struct */
    their_addr.sin_family = AF_INET; /* host byte order .. */
    their_addr.sin_port = htons(SERVER_PORT); /* .. short, network byte order */
    their_addr.sin_addr = *((struct in_addr *) he->h_addr);
    unsigned long serverAddr = their_addr.sin_addr.s_addr;

    /* 
     * Timeout setting for listening. */
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0; // 100ms
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof (tv)) < 0) {
        perror("timeout set error");
    }
    
     /* 
      * Main loop for sending and listening. */
    while (1) {
        their_addr.sin_addr = *((struct in_addr *) he->h_addr);
        numbytes = 0;
        initialiseMsgFormat(&msg);
        setMessageClient(&msg);
        reverseMsgFormat(&msg);

        /* 
         * Sends the data to the server. */
        if ((numbytes = sendto(sockfd, &msg, sizeof (msg), 0,
                (struct sockaddr *) &their_addr, addr_len)) == -1) {
            perror("Client sendto");
            exit(1);
        }

        /* 
         * Will listen to any address if in Manycast Mode. */
        printf("data sent, awaiting reply...");
        if (manycast == 1)
            their_addr.sin_addr.s_addr = INADDR_ANY;

        initialiseMsgFormat(&recvBuffer);
        numbytes = 0;

        /* Wait for a reply from the server. 
         * Timeout after 10 seconds if no reply is received.*/
        if ((numbytes = recvfrom(sockfd, (struct sntpMsgFormat *) &recvBuffer,
                sizeof (struct sntpMsgFormat), 0, (struct sockaddr *) &their_addr,
                &addr_len)) == -1) {

            if (errno == EAGAIN) {
                /* Ignores error.*/
            } else {
                perror("Manycast Client recvfrom Error");
                exit(1);
            }
        }
        gettimeofday(&myTime, NULL); /* Time data was received from server. */
        printf("Finished listening... ");

        if (numbytes == sizeof (struct sntpMsgFormat)) { /* Only accepts packets that are size 48 */
            printf("Processing Packet... ");
            reverseMsgFormat(&recvBuffer);
            printMsgDetails(recvBuffer);
            reverseMsgFormat(&msg);

            /* 
             * Variables for checking for bogus messages. */
            int modeCheck = recvBuffer.flags & SERVER_MODE;
            int myVersion = (msg.flags >> 3) & 0x07;
            int serverVersion = (recvBuffer.flags >> 3) & 0x07;

            /* 
             * Further packet checks. See print statements for details. */
            if (modeCheck != SERVER_MODE) printf("Message not from a valid server (Mode is not %d).\n", SERVER_MODE);
            else if (myVersion != serverVersion) printf("Version sent in packet does not match server version.\n");
            else if (recvBuffer.stratum > 15) printf("Server stratum too high.\n");
            else if (recvBuffer.stratum == 0) printf("Kiss o' death message received. Better stop me soon..\n");
            else if (msg.transmitTimestamp != recvBuffer.originateTimestamp) printf("Server Originate does not match Client Transmit.\n");
            else if (manycast == 0 && their_addr.sin_addr.s_addr != serverAddr) printf("Message not from selected server. Message ignored.\n");
            else { /* Valid server reply. Prints the packet */
                myTime = ntp_to_tv(recvBuffer.transmitTimestamp);
                printf("Time After: ");
                print_tv(myTime);
                printf("\n");
            }
        } else printf("No Data received. Will attempt again in %d seconds.\n", POLL_TIME);
        
        sleep(POLL_TIME);
    }
    
    close(sockfd);
    return 0;
}

/* Sets the default values for the client message to the server. */
void setMessageClient(struct sntpMsgFormat* msg) {
    struct timeval myTime;
    /* Shifting bits required for retrieving flags.
     * Sets up the initial information so the server knows I am a client*/
    gettimeofday(&myTime, NULL);

    msg->flags = 0; /* Leap Indicator */
    msg->flags <<= 3;

    msg->flags |= 4; /* Version Number */
    msg->flags <<= 3;

    msg->flags |= 3; /* Mode. Client = 3, Server = 4 */

    msg->poll = log2(POLL_TIME);

    msg->transmitTimestamp = tv_to_ntp(myTime);
}
