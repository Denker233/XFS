#include "tool.h"

#define MAX_CLIENT 10

int client_count=1;
int server_port = 8002;
int port_list[10] = {7072, 8022, 8030, 8040}; // port of the existing node
char* node_list[10] = {"node1", "node2", "node3", "node4", "\0"};
char files[MAXNODES][152] = {{0}}; // files[0][j] = "123.txt;118"



int update_list(char* node_name, char* new_resource){
    puts(new_resource);
    char* name;
    int i;
    char* checksum;
    name = strtok(new_resource, ";");
    name = strtok(NULL, ";");
    name=strtok(NULL, ";");

    // find the corresponding node index to update
    for (i = 0; i < client_count; i++){
        if (strcmp(node_list[i], node_name) == 0){
            break;
        }
    }
    // loop over the file list and update to files[]
    checksum = strtok(NULL, ";");
    strcat(files[i], name);
    strcat(files[i], ";");

    strcat(files[i], checksum);
    name=strtok(NULL, ";");
    while (name  != NULL) {
        checksum = strtok(NULL, ";");
        strcat(files[i], ";");
        strcat(files[i], name);
        strcat(files[i], ";");
        strcat(files[i], checksum);
        printf("file: %s\n",files[i]);
        name=strtok(NULL, ";");
    }
    puts(files[i]);
    return 0;
}

int resource_locate(char* resource, char* result) {
    char current[200];
   
    for (int i = 0; i <MAXNODES; i++) {
        for (int j = 0; j < 152; j++) {
            printf("%c ", files[i][j]);
        }
        printf("\n");
    }
    for (int i = 0; i < MAXNODES; i++){
        strcpy(current,files[i]);
        // printf("current and loop: %s %d\n",current,i);
        char* name;
        name=strtok(current, ";");
        
        // name = strtok_r(name, ";", &saveptr);
        if(current==NULL){
            memset(current,0,sizeof(current));
            continue;
        }
        char* checksum = strtok(NULL, ";");
        // printf("name first: %s\n",name);
        if (name!=NULL&&strcmp(name, resource) == 0) {
            strcat(result, node_list[i]);
            strcat(result, ";");
            strcat(result, checksum);
            strcat(result, ";");
            printf("result first: %s\n",result);
            memset(current,0,sizeof(current));
            continue;
        }
        while ((name = strtok(NULL, ";")) != NULL) {
            checksum = strtok(NULL, ";");
            if (strcmp(name, resource) == 0) {
                strcat(result, node_list[i]);
                strcat(result, ";");
                strcat(result, checksum);
                strcat(result, ";");
                printf("result while: %s\n",result);
                memset(current,0,sizeof(current));
                continue;
            }
        }
        memset(current,0,sizeof(current));
    }
    result[strlen(result)-1]='\0';
    printf("result: %s\n",result);
    return 0;
}

int bind_udp(int port){
    int sockfd;
    struct sockaddr_in cli_addr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&cli_addr, '0', sizeof(cli_addr));

    /* Hardcoded IP and Port for every client*/
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(port);
    cli_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* Bind the socket to a specific port */
    if (bind(sockfd, (const struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0) {
        printf("\nBind failed\n");
        return -1;
    }

    /* Set IP and Port */
    struct sockaddr_in tmp_addr;
    socklen_t len = sizeof(tmp_addr);
    getsockname(sockfd, (struct sockaddr *) &tmp_addr, &len);
    // client_IP = inet_ntoa(tmp_addr.sin_addr);
    // client_Port = ntohs(tmp_addr.sin_port);
    
    // printf("Client IP: %s, Port: %d\n", client_IP, client_Port);
    return sockfd;
}

char* receive_udp_message(int arg) {
    int sockfd = arg;
    char buf[MAXBUFLEN];
    memset(buf,0,sizeof(buf));
    struct sockaddr_storage sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    while (1) {
        int numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&sender_addr, &addr_len);
        if (numbytes == -1) {
            perror("recvfrom");
            exit(1);
        }
        buf[numbytes] = '\0';
        printf("Received request from node: %s\n", buf);
        /* parse the request from node */
        char* request;
        char buf_copy[100];
        strcpy(buf_copy, buf);
        // char* saveptr;
        request = strtok(buf_copy, ";");
        printf("request:%s\n",request);
        /* Calling methods according to request */
        printf("after request\n");
        struct addrinfo* sender;
        printf("after request\n");
        if (strcmp(request, "find") == 0) {
            char message[100];
            char* resource;
            resource = strtok(NULL, ";");
            resource_locate(resource, message);
            sendto(sockfd, message, sizeof(node_list), 0, (struct sockaddr *) &sender_addr, addr_len);
        }
        else if (strcmp(request, "download") == 0 || strcmp(request, "find") == 0) {
            printf("in download\n");
            char* name;
            name = strtok(NULL, ";");
            printf("filename: %s\n",name);
            char nodes[100];
            memset(nodes, 0, sizeof(nodes));
            resource_locate(name, nodes);
            printf("all nodes with that file: %s",nodes);
            sendto(sockfd, nodes, sizeof(nodes), 0, (struct sockaddr *)&sender_addr, addr_len);
        }
        else if (strcmp(request, "boot")==0) { // update;file1;file2;file3
            char* node_name;
            node_name = strtok(NULL, ";");
            printf("before updatelist\n");
            update_list(node_name, buf);
            char* notify;
            notify = "Update succeed\n";
            sendto(sockfd, notify, sizeof(notify), 0, (struct sockaddr *)&sender_addr, addr_len);
        }
        else if (strcmp(request, "ping")==0){
            printf("in ping branch\n");
            char* pong="pong";
            sendto(sockfd, pong, sizeof(pong), 0, (struct sockaddr *)&sender_addr, addr_len);
        }
        
    }
}




int main(){
    // Initial server port
    int fd_list[MAX_CLIENT];
    // for (int i; i < MAX_CLIENT; i++){
    //     fd_list[i] = (bind_udp(port_list[i]));
    // }
    fd_list[0] = (bind_udp(server_port));
    while (1) {
        char* msg;
        msg = receive_udp_message(fd_list[0]);

        
    }
}

