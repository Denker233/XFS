#include "tool.h"


int client_count=0,thread_index=0;//port)_index to send for ping thread to use
int server_port = 8002;
int port_list[MAX_CLIENT+1] = {7075,6000,6001,6002,6003,8002}; // ping port for each client and one more for other request
char* node_list[10] = {"node1", "node2", "node3", "node4", "node5","\0"};
char files[MAX_CLIENT][152] = {{0}}; // files[0][j] = "123.txt;118"
int fd_list[MAX_CLIENT];
pthread_mutex_t num_client_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t num_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t ping_thread[MAX_CLIENT];



int update_list(char* node_name, char* new_resource){
    puts(new_resource);
    char* name;
    int i;
    char* checksum;
    name = strtok(new_resource, ";");
    // printf("%s\n",name);
    name = strtok(NULL, ";");
    // printf("%s\n",name);
    if((name=strtok(NULL, ";"))==NULL){//empty repo
        printf("empty repo\n");
        // printf("%s\n",name);
        return 0;
    }
    printf("no return\n");
    // printf("%s\n",name);

    // find the corresponding node index to update
    for (i = 0; i < client_count; i++){
        if (strcmp(node_list[i], node_name) == 0){
            break;
        }
    }
    memset(files[i],0,sizeof(files[0]));//clear previous boot info
    // loop over the file list and update to files[]
    if((checksum = strtok(NULL, ";"))==NULL){
        printf("in check sum\n");
        // printf("check sum:%s\n",name);
        return 0;
    }
    printf("after check sum  and client count: %d\n",client_count);
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
   
    for (int i = 0; i <MAX_CLIENT; i++) {
        for (int j = 0; j < 152; j++) {
            printf("%c ", files[i][j]);
        }
        printf("\n");
    }
    for (int i = 0; i < MAX_CLIENT; i++){
        strcpy(current,files[i]);
        // printf("current and loop: %s %d\n",current,i);
        char* name;
        // name=strtok(current, ";");
        if((name=strtok(current, ";"))==NULL){//empty repo
        // strcat(result, ";");
        printf("inside");
        continue;
    }
        
        // name = strtok_r(name, ";", &saveptr);
        if(current==NULL){
            memset(current,0,sizeof(current));
            continue;
        }
        char* checksum = strtok(NULL, ";");
        printf("token and checksum: %s and %s", name, checksum);

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

void* set_up_connection();
char* receive_udp_message(int arg) {
    char notify[50];
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
            printf("all nodes with that file: %s\n",nodes);
            sendto(sockfd, nodes, sizeof(nodes), 0, (struct sockaddr *)&sender_addr, addr_len);
        }
        else if (strcmp(request, "boot")==0) { // update;file1;file2;file3
            char* node_name;
            node_name = strtok(NULL, ";");
            printf(" boot!!!!!!!\n");
            int index_in_files = atoi(&node_name[strlen(node_name)-1])-1;
            update_list(node_name, buf);
            if(index_in_files>(sizeof(port_list)/sizeof(int))){
                strcpy(notify,"no port");
                sendto(sockfd, notify, sizeof(notify), 0, (struct sockaddr *)&sender_addr, addr_len);
                memset(notify, 0, sizeof(notify));
                continue;
            }
            sprintf(notify,"%d",port_list[index_in_files]);
            printf("before sendto in boot\n");
            sendto(sockfd, notify, sizeof(notify), 0, (struct sockaddr *)&sender_addr, addr_len);
            memset(notify, 0, sizeof(notify));
        }
        else if (strcmp(request, "update")==0){
            char* node_name;
            node_name = strtok(NULL, ";");
            printf("before updatelist in regular update\n");
            int index_in_files = atoi(&node_name[strlen(node_name)-1])-1;
            update_list(node_name, buf);
            strcpy(notify,"not boot");
            printf("not boot send from server\n");
            sendto(sockfd, notify, sizeof(notify), 0, (struct sockaddr *)&sender_addr, addr_len);
            memset(notify, 0, sizeof(notify));
        }
        else if (strcmp(request, "ping")==0){
            printf("in ping branch\n");
            char* pong="pong";
            sendto(sockfd, pong, sizeof(pong), 0, (struct sockaddr *)&sender_addr, addr_len);
        }
        else if(strcmp(request, "fail")==0){
            char* node_index;
            node_index = strtok(NULL, ";");
            memset(files[atoi(node_index)],0,sizeof(files[atoi(node_index)]));
            sendto(sockfd, "fail_update", sizeof("fail_update"), 0, (struct sockaddr *)&sender_addr, addr_len);
        }
        
    }
}
void* set_up_connection(){
    pthread_mutex_lock(&num_client_lock);
    int local_index = client_count;
    client_count++;
    pthread_mutex_unlock(&num_client_lock);
    printf("port number: %d\n",port_list[local_index]);
    fd_list[local_index] = (bind_udp(port_list[local_index]));
    while (1) {
        receive_udp_message(fd_list[local_index]);

    }
}




int main(){
    // Initial server port for 
    for (int i=0; i < MAX_CLIENT; i++){//extra one is boot/download connection
        pthread_mutex_lock(&num_thread_lock);
        if(pthread_create(&ping_thread[thread_index++],NULL,set_up_connection,NULL)<0)
            {
                fprintf(stderr, "Error creating thread\n");
                exit(1);
            }
        pthread_mutex_unlock(&num_thread_lock);
    }
    set_up_connection();
    
}

