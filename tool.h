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
#define MAX_CLIENT 5


// struct Sock_info{
//     int sock;
//     struct addrinfo p;
// }

struct port_score_t{
    int port;
    int score;
    int checksum;
};

struct machIDlaten{
    char machID[20];
    int lats[MAX_CLIENTS];
    int ports[MAX_CLIENTS];
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