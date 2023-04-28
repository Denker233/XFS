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
#include <stdbool.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>

#define MAX_CLIENTS 100
#define MAX_SUBSCRIPTIONS 10
#define MAXSTRING 120
#define MAXBUFLEN 120
#define MAXNODES 10


// struct Sock_info{
//     int sock;
//     struct addrinfo p;
// }
struct machIDlaten{
    char machID[20];
    int latency;
    int port;
};

int check_if_in(char* element,char* list,char* sym){
    char *token;
    token = strtok(list, sym);

    while (token != NULL) {
    // Compare the token with the target element
    if (strcmp(token, element) == 0) {
        printf("%s is in\n",sym);
        return 1;
    }
    // Get the next token
    token = strtok(NULL, sym);
  }
  return -1;
}



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

/*
 * This function computes the SHA-256 checksum of a file using OpenSSL.
 * The code for computing the checksum is adapted from the OpenSSL documentation.
 * See https://www.tutorialspoint.com/c-program-to-implement-checksum 
 */
unsigned int get_checksum(char *str) {
    printf("get checksum\n");
    if (str == NULL) {
        return 0;
    }
   unsigned int sum = 0;
   while (*str) {
      sum += *str;
      str++;
   } 
   return sum;
}


/*
 * Check the string with expected checksum
 * return: 0, same; -1, mismatch
 */
int checksum_check(int expected, char* doc) {
    int checksum = get_checksum(doc);
    printf("in checksum_check\n");
    if (checksum == expected){return 0;}
    printf("Checksum mismatch! Expected: %d, Actual: %d\n", expected, checksum);
    return -1;
}