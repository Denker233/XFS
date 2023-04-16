#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/times.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <dirent.h>
#include <math.h>
#include <limits.h>

#define MAX_CLIENTS 100
#define MAX_SUBSCRIPTIONS 10
#define MAXSTRING 120
#define MAXBUFLEN 120
#define MAXNODES 20


// struct Sock_info{
//     int sock;
//     struct addrinfo p;
// }
struct machIDlaten{
    char machID[20];
    int latency;
    int port;
};

// struct 

// int send_message_thread(char *IP, int Port, char *message)
// {
//     int sockfd;
//     struct addrinfo hints, *servinfo, *p;
//     int rv;
//     int numbytes;

//     memset(&hints, 0, sizeof hints);
//     hints.ai_family = AF_INET; // set to AF_INET to use IPv4
//     hints.ai_socktype = SOCK_DGRAM;

//     char port_str[6];
//     sprintf(port_str, "%d", Port);

//     if ((rv = getaddrinfo(IP, port_str, &hints, &servinfo)) != 0)
//     {
//         fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
//         exit(1);
//     }

//     // loop through all the results and make a socket
//     for (p = servinfo; p != NULL; p = p->ai_next)
//     {
//         if ((sockfd = socket(p->ai_family, p->ai_socktype,
//                              p->ai_protocol)) == -1)
//         {
//             perror("talker: socket");
//             continue;
//         }

//         break;
//     }

//     if (p == NULL)
//     {
//         fprintf(stderr, "talker: failed to create socket\n");
//         exit(1);
//     }

//     if ((numbytes = sendto(sockfd, message, strlen(message), 0,
//                            p->ai_addr, p->ai_addrlen)) == -1)
//     {
//         perror("talker: sendto");
//         exit(1);
//     }

//     freeaddrinfo(servinfo);

//     printf("talker: sent %d bytes to %s\n", numbytes, IP);
//     // close(sockfd);
//     return sockfd;
// }







