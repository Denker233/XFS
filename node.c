#include "tool.h"


char server_IP[20];
int server_port = 8002;
struct machIDlaten map[MAXNODES]; //key value pairs to store latency
char ID[15];
char dirname[100];//"/home/tian0138/csci-5105/XFS/share/ID"
char IP[20]="127.0.0.1";
int node_node_port; // port reserved for node to node connection/request their self
int load_index=0;
char local_storage[100]; //"/home/tian0138/csci-5105/XFS/share/"
char filelist[200]; //boot;ID;file1.txt;file2.txt
int min_score=INT_MAX;//score to select peers

int min(int x,int y){
    return (x < y) ? x : y;
}


int get_latency(char* machID){
    printf("in get latency loop\n");
    for(int i =0;i<MAXNODES;i++){
        if (strcmp(map[i].machID,machID)==0){
            printf("map latency: %d\n", map[i].latency);
            return map[i].latency;
        }
    }
    return -1;
}

int get_port(char* machID){
    for(int i =0;i<MAXNODES;i++){
        if (strcmp(map[i].machID,machID)==0){
            return map[i].port;
        }
    }
    return -1;
}

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
        exit(1);
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
        exit(1);
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

void read_latency(){//read latency.txt file and store the key value pair into map
    printf("witin read latency\n");
    // FILE *file = fopen(strcat(local_storage,"latency.txt"), "r");
    FILE *file = fopen("./share/latency.txt", "r");
    if (file == NULL) {
        printf("Error: could not open file\n");
        exit(1);
    }
    char* token;
    char line[100];
    int i =0;
    while (fgets(line, sizeof(line), file) != NULL && i<MAXNODES) {
        printf("%s\n", line);
        token = strtok(line,":");
        strcpy(map[i].machID,token);
        printf("%s\n", token);
        token = strtok(NULL,":");
        map[i].latency=atoi(token);
        printf("%d\n", atoi(token));
        token = strtok(NULL,":");
        map[i].port=atoi(token);
        if(strcmp(map[i].machID,ID)==0){ //if node already in the txt then assign the port to the global
            node_node_port=map[i].port;
        }
        printf("%d\n", atoi(token));
        i++;
    }
    fclose(file);
}

void boot(){
    char dir_share[50] = "./share/";
    strcat(dir_share, ID);
    strcat(dir_share, "/");
    char message[1024];
    pthread_t upload_thread;
    strcpy(message,"boot;");
    strcat(message,ID);
    // strcat(message,";");
    DIR* dir;
    struct dirent *ent;
    if ((dir = opendir (dir_share)) != NULL) {//loop through and get every filename under the machID
        while ((ent = readdir (dir)) != NULL) {
            if(strncmp((ent->d_name),".",1)==0){//only transmit the file name
                continue;
            }
            /* get check sum for each file */
            int checksum = 0;
            char buf[100];
            char file_name[50];
            strcpy(file_name, dir_share);
            strcat(file_name, ent->d_name);
            FILE* fp = fopen(file_name, "r");
            while (fgets(buf, sizeof(buf), fp)){
                checksum += get_checksum(buf);
            }
            char str[512];
            sprintf(str, ";%s;%d", ent->d_name, checksum);
            strcat(message,str);
        }
        closedir (dir);
    } else {
        perror ("");
        exit(0);
    }
    strcpy(server_IP,"127.0.0.1");
    send_message(server_IP,server_port, message);
    strcpy(filelist,message);
}

int getload(int port){
    puts("in get load\n");
    char message[20];
    strcpy(message,"getload");
    puts("before send_message\n");
    int sock=send_message(IP,port, message);
    char buf[MAXBUFLEN];
    struct sockaddr_storage sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    int numbytes = recvfrom(sock, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&sender_addr, &addr_len);
    if (numbytes == -1) {
        perror("recvfrom");
        exit(1);
    }
    printf("%s",buf);
    printf("\n");
    buf[numbytes] = '\0';  // get back a list of nodes with that filename
    printf("Received subsribed article from server: %s\n", buf);
    return atoi(buf);
}

void* download(char* filename){
    int empty=1;
    char path[100];
    pthread_t thread_ids[20];
    char* checksum;
    int min_checksum = 0;
    FILE *fp;
    char message[100];
    int load,latency,score,port,min_port;
    strcpy(message,"download;");
    strcat(message,filename);
    int sock=send_message(server_IP,server_port,message);//send the intended filename to server
    char buf[MAXBUFLEN];
    // char* ptr = buf;
    struct sockaddr_storage sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    int numbytes = recvfrom(sock, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&sender_addr, &addr_len);
    if (numbytes == -1) {
        perror("recvfrom");
        exit(1);
    }
    printf("%s",buf);
    printf("\n");
    buf[numbytes] = '\0';  // get back a list of nodes with that filename
    printf("Received subsribed article from server: %s\n", buf);
    close(sock);                 // close the port with server 
    // token = strtok(buf,";");
    // printf("token is %s\n", token);
    // if(token==NULL){ // What is this chunk for ????
    //     checksum = strtok(NULL,";");
    //     port=get_port(token);
    //     puts("if token == NULL\n");
    //     load=getload(port);//get the load of the node
    //     latency = get_latency(token);
    //     score=load+0.1*latency;
    //     min_score=min(score,min_score);
    //     if(min_score==score){min_port=port;}//update the minport to best peer
    // }

    // loop through every node for the smallest latency; token here means the machID
    // while(token!=NULL){
    char* token = buf;
    char cs_str[4];
    while ((token = strtok(token, ";")) != NULL){
        printf("loop11111\n");
        checksum = strtok(NULL, ";");
        printf("token and checksum: %s and %s", token, checksum);

        port=get_port(token);
        load = getload(port);//get the load of the node
        printf("load is %d\n", load);
        latency = get_latency(token);
        score=load+0.1*latency;
        min_score=min(score,min_score);
        // update the minport to best peer
        if(min_score==score){
            min_port=port;
            printf("min checksm:");
            strcpy(cs_str, checksum);
            min_checksum = atoi(cs_str);
            printf("min checksm: %d", min_checksum);
        }
        printf("above token = NULL\n");
        token = NULL;
    }
    printf("before retry loop\n");

    /* send download request to selected peer */
    /* re-fetch on mismatch */
    
    while (checksum_check(min_checksum, token) == -1) {
        printf("retry loop\n");
        sock=send_message(IP,min_port,message); //send download;filename to the intended file
        puts("send yes\n");
        memset(buf,0,sizeof(buf));
        struct sockaddr_storage sender_addr2;
        socklen_t addr_len2 = sizeof(sender_addr2);
        numbytes = recvfrom(sock, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&sender_addr2, &addr_len2);
        puts("receive yes\n");
        if (numbytes == -1) {
            perror("recvfrom");
            exit(1);
        }
        printf("%s",buf);
        printf("\n");
        buf[numbytes] = '\0';  // get back a list of nodes with that filename
        printf("Received subsribed article from server: %s\n", buf);
        strcpy(path,dirname);
        strcat(path,"/");
        strcat(path,filename);
        token=strtok(buf,";");
        token=strtok(NULL,";");
        token=strtok(NULL,";");
    }

    fp=fopen(path,"w");
    if(fp==NULL){
        printf("error creating file\n");
    }
    else{
        printf("create file success\n");
        if(token!=NULL){
            fprintf(fp, "%s", token);
        }
        fclose(fp);
        printf("after file success\n");
    }
    close(sock);
    printf("after close sock\n");
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

void read_intended_file(char* filename,char* message){
    FILE *fp;
    char path[100];
    char content[MAXBUFLEN];
    strcpy(path,dirname);
    strcat(path,"/");
    strcat(path,filename);
    if ((fp = fopen(path, "r")) == NULL){
        puts("error read intended file");
        exit(1);
    }
    puts(path);
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    strcat(message,filename);
    fread(content, size, 1, fp);
    puts(content);
    strcat(message,";");
    strcat(message,content);
    message[strlen(message)] = '\0';
    // if(size==0){
    //     message[strlen(message)-1]='\0';
    //     puts("in ");
    //     puts(message);
    // }
    puts(message);
    memset(content,0,sizeof(content));
    fclose(fp);
    
    
}

char* receive_udp_message(int sock,struct sockaddr_storage sender_addr,socklen_t addr_len) {
    int sockfd = sock;
    char message[200];
    char* token;
    char filename[20];
    char buf[MAXBUFLEN];
    char* content;
    char path[100];
    FILE *fp;
    while (1) {
        int numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&sender_addr, &addr_len);
        if (numbytes == -1) {
            perror("recvfrom");
            exit(1);
        }
        printf("%s",buf);
        printf("\n");
        buf[numbytes] = '\0';
        printf("Received subsribed article from server: %s\n", buf);
        strcpy(message,buf);
        if(message==NULL){
            puts("empty message");
            exit(1);
        }
        puts(buf);
        token = strtok(message,";");
        puts(token);
        if(strcmp(token,"download")==0){
            token=strtok(NULL,";");
            strcpy(filename,token);
            memset(message,0,sizeof(message));
            strcat(message,"downloaded;");
            read_intended_file(filename,message);
            int bytes=sendto(sock, message, sizeof(message), 0, (struct sockaddr *)&sender_addr, addr_len);
            if (bytes == -1) {
                puts("sendto in receive fail");
                exit(1);
            }
        }
        else if(strcmp(token,"getload")==0){
            memset(message,0,sizeof(message));
            sprintf(message,"%d",load_index);
            int bytes=sendto(sock, message, sizeof(message), 0, (struct sockaddr *)&sender_addr, addr_len);
        }
        // else if (strcmp(token,"downloaded")==0){
        //     puts("inside downloaded");
        //     strcpy(path,dirname);
        //     strcat(path,"/");
        //     strcat(path,filename);
        //     puts(path);
        //     // printf("path after downloaded:%s\n",path);
        //     content=strtok(NULL,";");
        //     fp = fopen(path, "w");
        //     fprintf(fp, "%s", content);
        //     fclose(fp);
        //     memset(path,0,sizeof(path));
        //     memset(content,0,sizeof(content));
        // }
        memset(buf,0,sizeof(buf));
    }
}


void* receive_node(){
    struct sockaddr_storage sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    char message[200];
    char* token;
    char filename[20];
    puts("in receive node");
    int sock=bind_udp(node_node_port);
    puts("after bind");
    receive_udp_message(sock,sender_addr,addr_len);
    puts("after receive");    
}

int find(char* resource){
    char message[100] = "find;";
    strcat(message, resource);
    int sockfd = send_message(server_IP, server_port, message);
    memset(message, 0, sizeof(message));
    struct sockaddr_storage sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    recvfrom(sockfd, message, sizeof(message), 0, (struct sockaddr *)&sender_addr, &addr_len);
    printf("File list: %s", message);
    return 0;
}




int main(void) {
    pthread_t node_node_thread,download_thread;
    // strcpy(dirname,"/home/tian0138/csci-5105/XFS/share/");
    strcpy(dirname, "/home/wan00807/5105/XFS/share/");
    strcpy(local_storage,dirname);
    // strcat(local_storage,ID); 
    printf("Input machID\n");
    scanf("%s",ID);
    printf("%s\n",ID);
    strcat(dirname,ID); 
    read_latency();//read latency.txt to map struct
    if(get_latency(ID)==-1){ //add new node to latency file 
        // FILE *fp=fopen(strcat("/home/tian0138/csci-5105/XFS/share/","latency.txt"),"w");
        FILE *fp=fopen("./share/latency.txt","w");
        if(fp == NULL) {
            printf("fail open nlatency txt\n");
        }
        char new_node[50];
        int latency;
        printf("Input new node's port and latency\n");
        scanf("%d %d",&node_node_port,&latency);
        sprintf(new_node,"%s",ID);
        fseek(fp, 0, SEEK_END);
        fwrite(new_node,sizeof(char),sizeof(new_node),fp);
        sprintf(new_node,":%d:%d",latency,node_node_port);
        fseek(fp, 0, SEEK_END);
        fwrite(new_node,sizeof(int),2,fp);
    }
    char func[10];
    char filename[15];
    puts("before boots");
    boot();
    /* Node-node connection thread */
    if(pthread_create(&node_node_thread,NULL,receive_node,NULL)<0){//bind node waiting for other node's request
        fprintf(stderr, "Error creating thread\n");
        return -1;
    }
    printf("Input function and filename:\n");
    while(1){
        scanf("%s %s", func,filename);
        if(strcmp(func,"download")==0){
            if(pthread_create(&download_thread,NULL,(void*)download,filename)<0){
                fprintf(stderr, "Error creating thread\n");
                return -1;
            }
            load_index++;
        }
        else if (strcmp(func,"find") == 0) {
            find(filename);
        }
    }
    pthread_join(download_thread,NULL);
}

