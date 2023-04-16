#include "tool.h"


char server_IP[20];
int server_port = 8000;

int send_message(char *IP, int Port, char *message)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;

    char port_str[6];
    sprintf(port_str, "%d", Port);

    if ((rv = getaddrinfo(IP, port_str, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // loop through all the results and make a socket
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "talker: failed to create socket\n");
        return -1;
    }

    if ((numbytes = sendto(sockfd, message, strlen(message), 0,
                           p->ai_addr, p->ai_addrlen)) == -1)
    {
        perror("talker: sendto");
        exit(1);
    }

    freeaddrinfo(servinfo);

    printf("talker: sent %d bytes to %s\n", numbytes, IP);
    // close(sockfd);
    return sockfd;
}

void boot(){
    char message[100];
    strcpy(message,"boot;");
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ("/home/tian0138/csci-5105/XFS/share/machID")) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            if(strncmp((ent->d_name),".",1)==0){//only transmit the file name
                continue;
            }
            printf ("%s\n", ent->d_name);
            char str[30]=";";
            strcat(str,ent->d_name);
            strcat(message,str);
        }
        closedir (dir);
    } else {
        perror ("");
        exit(0);
    }
    strcpy(server_IP,"127.0.0.1");
    printf("%s\n", message);
    send_message(server_IP,server_port, message);
}
void download(filename){
    char message[100];
    strcpy(message,"download;");
    strcat(message,filename);
    int sock=send_message(server_IP,server_port,message);//send the intended filename to server
    char buf[MAXBUFLEN];
    struct sockaddr_storage sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    int numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&sender_addr, &addr_len);
    if (numbytes == -1) {
        perror("recvfrom");
        exit(1);
    }
    printf("%s",buf);
    printf("\n");
    buf[numbytes] = '\0';  // get back a list of nodes with that filename
    printf("Received subsribed article from server: %s\n", buf);

    


    


}
void getload(int port){

    send_message(IP="127.0.0.1",port, message);
}

int main(void) {
    char func[10];
    char filename[15];
    boot();
    // scanf("%s %s", func,filename);
    // if(strcmp(func,"download")==0){
    //     download(filename);
    // }


}