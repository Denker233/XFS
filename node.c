#include "tool.h"


char server_IP[20];
int server_port = 8002,ping_port;//port to do ping
struct machIDlaten map[MAXNODES]; //key value pairs to store latency
char ID[15];
char dirname[100];//"/home/tian0138/csci-5105/XFS/share/ID"
char IP[20]="127.0.0.1";
int node_node_port; // port reserved for node to node connection/request their self
int load_index=0;
char local_storage[100]; //"/home/tian0138/csci-5105/XFS/share/"
char filelist[200]; //boot;ID;file1.txt;file2.txt
int min_score=INT_MAX;//score to select peers
bool server_down = false;
pthread_mutex_t serverdown_lock = PTHREAD_MUTEX_INITIALIZER;
int handler_fired=0;

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
    if ((rv = getaddrinfo(IP, port_str, &hints, &servinfo)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }
    // loop through all the results and make a socket
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1){
            perror("talker: socket");
            continue;}
        break;
    }

    if (p == NULL){
        fprintf(stderr, "talker: failed to create socket\n");
        exit(1);
    }
    if ((numbytes = sendto(sockfd, message, strlen(message), 0,
                           p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1); }

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
void send_granted_receive(char* message,int receiver_port,int interval){
    char buf[MAXBUFLEN];
    struct sockaddr_storage sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    int loop;
    int sock_ping;
    fd_set readfds;
    int rv; 
    struct timeval tv;
    while(1){// keep trying only if server is not down
        // printf("pong loop: %d\n",loop);
        printf("before send in send_granted\n");
        sock_ping=send_message(server_IP,server_port, message);
        FD_ZERO(&readfds);
        FD_SET(sock_ping, &readfds);
        tv.tv_sec = interval;
        tv.tv_usec = 0;
        rv = select(sock_ping + 1, &readfds, NULL, NULL, &tv);
        if(rv){//when there is something in sock then call recvfrom;  times out then go over the loop again
            int numbytes = recvfrom(sock_ping, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&sender_addr, &addr_len);
            if (numbytes == -1&&!handler_fired) {
                perror("recvfrom");
                exit(1);
            }
            printf("should be port for ping: %s\n",buf);
            strcpy(message,buf);
            // pthread_mutex_lock(&serverdown_lock);
            // server_down = false;
            // printf("server is not down\n");
            pthread_mutex_unlock(&serverdown_lock);
            return;
        }
        // alarm(0);
        close(sock_ping);
        memset(buf, 0, sizeof(buf));
        // memset(buf, 0, sizeof(buf));
        // pthread_mutex_lock(&serverdown_lock);
        // server_down = true;
        // pthread_mutex_unlock(&serverdown_lock);
        loop++;
    }
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
    send_granted_receive(message,server_port,1);
    if(strcmp(message,"no port")==0){
        puts("there is no port to ping\n");
        exit(1);
    }
    ping_port = atoi(message);

}

int getload(int port){//get load from another node
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
    pthread_t thread_ids[20];
    char* checksum;
    int min_checksum = 0;
    FILE *fp;
    char message[100],buf[MAXBUFLEN]="",path[100];
    strcpy(buf,"");
    int load,latency,score,port,min_port;
    strcpy(message,"download;");
    strcat(message,filename);
    strcpy(buf,message);
    while(!server_down){
        printf("request send to another node: %s\n", message);
        send_granted_receive(buf,server_port,1);
        // strcpy(buf,message);
        break;
    }
    if(strcmp(buf,"")!=0){
        printf("downloaded message received from server: %s\n",buf);
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
            printf("download success\n");
            }
        printf("if there is no download success ahead then download was interrupted\n");

        /* send download request to selected peer */
        /* re-fetch on mismatch */
        int sock;
        while (checksum_check(min_checksum, token) == -1) {
            printf("retry loop\n");
            sock=send_message(IP,min_port,message); //send download;filename to the intended file
            puts("send yes\n");
            memset(buf,0,sizeof(buf));
            struct sockaddr_storage sender_addr2;
            socklen_t addr_len2 = sizeof(sender_addr2);
            int numbytes = recvfrom(sock, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&sender_addr2, &addr_len2);
            puts("receive yes\n");
            if (numbytes == -1) {
                perror("recvfrom");
                exit(1);
            }
            printf("%s",buf);
            printf("\n");
            buf[numbytes] = '\0';  // get back a list of nodes with that filename
            printf("refetch from server: %s\n", buf);
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
    printf("inside read intended file");
    printf("inside read intended file, filename:%s\n",filename);
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
    printf("message in read_intended file: %s\n",message);
    // if(size==0){
    //     message[strlen(message)-1]='\0';
    //     puts("in ");
    //     puts(message);
    // }
    puts(message);
    memset(content,0,sizeof(content));
    fclose(fp);
    
    
}

char* receive_udp_message(int sock,struct sockaddr_storage sender_addr,socklen_t addr_len) {//
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
        printf("Received message from client: %s\n", buf);
        strcpy(message,buf);
        if(message==NULL){
            puts("empty message");
            exit(1);
        }
        puts(buf);
        token = strtok(message,";");
        puts(token);
        if(strcmp(token,"download")==0){//request from another node
            token=strtok(NULL,";");
            strcpy(filename,token);
            memset(message,0,sizeof(message));
            strcat(message,"downloaded;");
            printf("before read intended file\n");
            read_intended_file(filename,message);
            int bytes=sendto(sock, message, sizeof(message), 0, (struct sockaddr *)&sender_addr, addr_len);
            if (bytes == -1) {
                puts("sendto in receive fail");
                exit(1);
            }
        }
        else if(strcmp(token,"getload")==0){//request from another node
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


void* receive_node(){//receive node to node request
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
    while(!server_down){
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
    return -1;
}
// void ping_handler(){
//     close(sock_ping);
//     printf("ping hanlder fired\n");
//     pthread_mutex_lock(&serverdown_lock);
//     server_down = true;
//     pthread_mutex_unlock(&serverdown_lock);
//     handler_fired=1;
//     printf("before send message\n");
//     send_message(server_IP,server_port, "ping;");
//     printf("after send message\n");
//     return;
// }


void* ping(){
    printf("in ping:\n");
    char message[200];
    char buf[MAXBUFLEN];
    struct sockaddr_storage sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    int loop;
    int sock_ping;
    // struct sigaction act={
    //     .sa_handler = ping_handler,
    //     .sa_flags = 0,
    // };
    fd_set readfds; 
    // fcntl(sock_ping, F_SETFL, O_NONBLOCK); //no need to set it non-blocking
    int rv; 
    struct timeval tv;
    while(1){
        sleep(3);
        printf("pong loop: %d\n",loop);
        strcpy(message,"ping;");
        printf("before send\n");
        sock_ping=send_message(server_IP,ping_port,message);
        // alarm(3);//should go back after handler
        // sigaction (SIGALRM, &act, NULL); 
        // printf("after alarm\n");
        FD_ZERO(&readfds);
        FD_SET(sock_ping, &readfds);
        tv.tv_sec = 3;
        tv.tv_usec = 0;
        rv = select(sock_ping + 1, &readfds, NULL, NULL, &tv);
        if(rv){//when there is something in sock then call recvfrom; 5 sec times out then go over the loop again
            int numbytes = recvfrom(sock_ping, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&sender_addr, &addr_len);
            if (numbytes == -1&&!handler_fired) {
                perror("recvfrom");
                exit(1);
            }
            pthread_mutex_lock(&serverdown_lock);
            server_down = false;
            printf("server is not down\n");
            pthread_mutex_unlock(&serverdown_lock);
            printf("should be pong: %s\n",buf);
            continue;
        }
        printf("should be pong: %s\n",buf);
        // alarm(0);
        close(sock_ping);
        memset(buf, 0, sizeof(buf));
        // memset(buf, 0, sizeof(buf));
        pthread_mutex_lock(&serverdown_lock);
        server_down = true;
        pthread_mutex_unlock(&serverdown_lock);
        loop++;
    }
    

} 




int main(void) {
    pthread_t node_node_thread,download_thread,ping_thread;
    strcpy(dirname,"/home/tian0138/csci-5105/XFS/share/");
    // strcpy(dirname, "/home/wan00807/5105/XFS/share/");
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
    int main_detect=0;
    bool ping_created=false,nodetonode_created=false;
    int maxfd;
    fd_set readfds;
    puts("before boots");
    int boot_time=0;
    // int flags = fcntl(STDIN_FILENO, F_GETFL); //make scanf non-blocking
    // fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK); 
    while(1){//this loop is blocked in while(!server_down); if server is down then reboot
        if(!server_down){
            boot();
            boot_time++;
            printf("boot timeeeeeeeeee: %d\n",boot_time);
        }
        if(!ping_created){
            if(pthread_create(&ping_thread,NULL,ping,NULL)<0){//bind node waiting for other node's request
            fprintf(stderr, "Error creating thread\n");
            return -1;
            }
            ping_created=true;
        }
        if(!nodetonode_created){
            if(pthread_create(&node_node_thread,NULL,receive_node,NULL)<0){//bind node waiting for other node's request
            fprintf(stderr, "Error creating thread\n");
            return -1;
            }
            nodetonode_created=true;
        }
        /* Node-node connection thread */
        while(!server_down){
            FD_ZERO(&readfds); // clear the file descriptor set
            FD_SET(STDIN_FILENO, &readfds); // add stdin to the set
            maxfd = STDIN_FILENO + 1;
            struct timeval timeout;
            timeout.tv_sec = 1; // set the timeout to 1 second
            timeout.tv_usec = 0;
            int ready = select(maxfd, &readfds, NULL, NULL, &timeout); // wait for input or timeout
            if(ready>0){
                if(boot_time==1){printf("Input function and filename:\n");}
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
        }//must input filename and function otherwise it won't reboot
        printf("main thread detect server_down\n");
        if(main_detect==0){
            // printf("main thread detect server_down\n");
        }
        main_detect=1;
    }


    pthread_join(download_thread,NULL);
}

