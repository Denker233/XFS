#include "tool.h"


char server_IP[20];
int server_port = 8002,ping_port;//port to do ping
struct machIDlaten map[MAX_CLIENT]; //key value pairs to store latency
char ID[15];
int port_index=0;
int download_index=0;
char dirname[100];//"/home/tian0138/csci-5105/XFS/share/ID"
char IP[20]="127.0.0.1";
int node_node_port; // port reserved for node to node connection/request their self
int load_index=0;
char local_storage[100]; //"/home/tian0138/csci-5105/XFS/share/"
char filelist[200]; //boot;ID;file1.txt;file2.txt
int min_score=INT_MAX;//score to select peers
bool server_down = false;
pthread_mutex_t serverdown_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t download_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t port_lock = PTHREAD_MUTEX_INITIALIZER;
int handler_fired=0;

int min(int x,int y){
    return (x < y) ? x : y;
}


int get_latency(char* machID2){
    printf("in get latency loop\n");
    int ID_1 = atoi(&ID[strlen(ID)-1])-1;
    int ID_2 = atoi(&machID2[strlen(machID2)-1])-1;
    return map[ID_1].lats[ID_2]+map[ID_2].lats[ID_1];
}

int get_port(char* machID2){
    int ID_1 = atoi(&ID[strlen(ID)-1])-1;
    int ID_2 = atoi(&machID2[strlen(machID2)-1])-1;
    printf("ID1 and 2: %d %d\n",ID_1,ID_2);
    return map[ID_2].ports[ID_1];
}

int get_indexfrom_port(int port){//return the index of that port
    int ID_1 = atoi(&ID[strlen(ID)-1])-1;
    for(int i =0;i<MAX_CLIENT;i++){
        if(map[ID_1].ports[i]==port){
            return i;
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

    // printf("talker: sent %d bytes to %s\n", numbytes, IP);
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
    int i =0,j=0;
    while (fgets(line, sizeof(line), file) != NULL && i<MAX_CLIENT) {
        // printf("%s\n", line);
        token = strtok(line,":");
        strcpy(map[i].machID,token);
        // printf("%s\n", token);
        token = strtok(NULL,":");
        while(j<MAX_CLIENT&&token!=NULL){
            map[i].lats[j]=atoi(token);
            token=strtok(NULL,":");
            j++;
        }
        j=0;
        while(j<MAX_CLIENT&&token!=NULL){
            map[i].ports[j]=atoi(token);
            token=strtok(NULL,":");
            j++;
        }
        j=0;
        // if(strcmp(map[i].machID,ID)==0){ //if node already in the txt then assign the port to the global
        //     node_node_port=map[i].port;
        // }
        i++;
    }
    fclose(file);
}
void send_granted_receive(char* message,int receiver_port,int interval){//0 for to ndoe 1 for to server -1 for queue up
    char buf[MAXBUFLEN];
    struct sockaddr_storage sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    int loop;
    int sock_ping;
    fd_set readfds;
    int rv; 
    struct timeval tv;
    int index_each_port;
    while(1){// keep trying only if server is not down
        // printf("pong loop: %d\n",loop);
        printf("before send in send_granted\n");
        sock_ping=send_message(server_IP,receiver_port, message);
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
            // printf("buf: %s\n",buf);
            strcpy(message,buf); 
            return;
        }
        close(sock_ping);
        memset(buf, 0, sizeof(buf));
        loop++;
    }
}

void updatelist(){
    char dir_share[50] = "./share/";
    strcat(dir_share, ID);
    strcat(dir_share, "/");
    char message[1024];
    pthread_t upload_thread;
    strcpy(message,"update;");
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
    char message[20] ,buf[MAXBUFLEN-1];
    strcpy(message,"getload");
    struct sockaddr_storage sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    int loop;
    int sock_ping;
    fd_set readfds;
    int rv; 
    struct timeval tv;
    int index_each_port;
    int sock;
    printf("retry loop\n");
    printf("before send in send_granted in get load %s , %d, %s\n",server_IP, port, message);
    while(1){//time out then return -1
        printf("before send in send_granted in get load: %s , %d, %s\n",server_IP, port, message);
        if(port==0){//exclude this one
            return -1;
        }
        sock_ping=send_message(server_IP,port, message);
        FD_ZERO(&readfds);
        FD_SET(sock_ping, &readfds);
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        rv = select(sock_ping + 1, &readfds, NULL, NULL, &tv);
        if(rv){//try different port from the list when it times out
            int numbytes = recvfrom(sock_ping, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&sender_addr, &addr_len);
            if (numbytes == -1) {
                perror("recvfrom");
                exit(1);
            }
            printf("%s",buf);
            printf("\n");
            // buf[numbytes] = '\0';  // get back a list of nodes with that filename
            printf("Received load from another node: %s\n", buf);
            if(buf==0){
                printf("return 0\n");
                return 0;
            }
            return atoi(buf);
        }
        printf("client down and fail\n");
        char notify[20],fail_index[5];
        strcpy(notify,"fail;");
        sprintf(fail_index,"%d",get_indexfrom_port(port));
        strcat(notify,fail_index);
        close(sock_ping);
        while(!server_down){
            send_granted_receive(notify,server_port,1);//send fail index to server side 
        }
        memset(buf, 0, sizeof(buf));
        loop++;
        return -1;
    }
}
char* find(char* filename){
    char message[100];
    char* buf = malloc(sizeof(char)*MAXBUFLEN);
    strcpy(message,"download;");
    strcat(message,filename);
    strcpy(buf,message);
    while(!server_down){
        printf("request send to another node: %s\n", message);
        send_granted_receive(buf,server_port,1);
        printf("receive from another node: %s\n", buf);
        // strcpy(buf,message);
        return buf;
    }
    return "";
}


void* download(char* filename){
    int empty=1,rank=0;
    pthread_t thread_ids[20];
    char* checksum;
    int min_checksum = 0;
    char* token;
    FILE *fp;
    int nodes_len;
    char message[100],buf[MAXBUFLEN],path[100];
    struct port_score_t ports[MAX_CLIENT];
    strcpy(buf,"");
    int load,latency,score,port,min_port;
    // char* buf2=find(filename);
    // strcpy(buf,buf2);
    // free(buf2);
    strcpy(message,"download;");
    strcat(message,filename);
    strcpy(buf,message);
    while(!server_down){
        printf("request send to another node: %s\n", message);
        send_granted_receive(buf,server_port,1);
        printf("receive from another node: %s\n", buf);
        // strcpy(buf,message);
        break;
    }
    if(strcmp(buf,"")!=0){
        printf("downloaded message received from server: %s\n",buf);
        token = buf;
        char cs_str[4];
        while ((token = strtok(token, ";")) != NULL){
            printf("loop11111\n");
            checksum = strtok(NULL, ";");
            printf("token and checksum: %s and %s", token, checksum);

            port=get_port(token);
            if(port==0){
                token = NULL;
                continue;
            }
            load = getload(port);//get the load of the node
            printf("load is %d\n", load);
            latency = get_latency(token);
            score=0.5*load+0.001*latency;
            if(load==-1){score=INT_MAX;};
            ports[rank].port=port;
            ports[rank].score=score;
            ports[rank].checksum=atoi(checksum);
            printf("port score checksum and rank: %d %d %d %d\n",ports[rank].port,score,atoi(checksum),rank);
            printf("above token = NULL\n");
            token = NULL;
            rank++;
            printf("download success\n");
        }
        nodes_len=rank+1;
        printf("port score checksum and rank 0: %d %d %d %d\n",ports[0].port,ports[0].score,ports[0].checksum,rank);
        for(int i =0;i<nodes_len;i++){
            printf("port score checksum and rank: %d %d %d %d\n",ports[rank].port,ports[rank].score,ports[rank].checksum,rank);
        }
        int num_ports = sizeof(ports) / sizeof(ports[0]);
        for (int i = 0; i < rank - 1; i++) {//bubble sort to sort based on score
            for (int j = 0; j < rank - i - 1; j++) {
                if (ports[j].score > ports[j + 1].score) {
                    struct port_score_t temp = ports[j];
                    ports[j] = ports[j + 1];
                    ports[j + 1] = temp;
                }
            }
        }
        printf("port score checksum and rank 0: %d %d %d %d\n",ports[0].port,ports[0].score,ports[0].checksum,rank);
        for(int i =0;i<rank;i++){
            printf("after sort port score checksum and rank: %d %d %d %d\n",ports[i].port,ports[i].score,ports[i].checksum,rank);
        }
        printf("if there is no download success ahead then download was interrupted\n");
    }
    else{//fail to find a file then just go back to input filename and function
        printf("no file is found\n");
        return NULL;
    }
        rank=0;//start from the min

        /* send download request to selected peer */
        /* re-fetch on mismatch */
        memset(buf, 0, sizeof(buf));
        struct sockaddr_storage sender_addr;
        socklen_t addr_len = sizeof(sender_addr);
        int loop;
        int sock_ping;
        fd_set readfds;
        int rv; 
        struct timeval tv;
        int index_each_port;
        int sock;
        printf("retry loop\n");
        while(rank<nodes_len){//keep try from the smallest til the end of avaible list
            printf("before send in send_granted in download\n");
            printf("ports before exclude 0: %d %d\n",ports[rank].port,rank);

            if(ports[rank].port==0||ports[rank].score==INT_MAX){//not including itself or the client is down
                rank++;
            };
            printf("ports after exclude 0: %d %d\n",ports[rank].port,rank);
            sock_ping=send_message(server_IP,ports[rank].port, message);
            FD_ZERO(&readfds);
            FD_SET(sock_ping, &readfds);
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            rv = select(sock_ping + 1, &readfds, NULL, NULL, &tv);
            if(rv){//try different port from the list when it times out
                int numbytes = recvfrom(sock_ping, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&sender_addr, &addr_len);
                if (numbytes == -1&&!handler_fired) {
                    perror("recvfrom");
                    exit(1);
                }
                printf("refetch from other node: %s\n", buf);
                printf("%s",buf);
                printf("\n");
                buf[numbytes] = '\0';  
                strcpy(path,dirname);
                strcat(path,"/");
                strcat(path,filename);
                token=strtok(buf,";");
                token=strtok(NULL,";");
                token=strtok(NULL,";");//content
                if(rand()%4==0){//modify a byte
                    strcpy(&token[1],"");
                    printf("change value\n");
                }
                if(checksum_check(ports[rank].checksum, token) == -1){// send fail then resend and not updatting rank
                    memset(buf,0,sizeof(buf));
                    continue;
                }
                break;
            }
            char notify[20],fail_index[5];
            strcpy(notify,"fail;");
            sprintf(fail_index,"%d",get_indexfrom_port(ports[rank].port));
            strcat(notify,fail_index);
            close(sock_ping);
            while(!server_down){
                printf("in while loop before fail send");
                send_granted_receive(notify,server_port,1);//send fail index to server side 
            }
            memset(buf, 0, sizeof(buf));
            loop++;
            rank++;
        }
        if(buf==0){
            printf("all nodes have that file is down\n");
            return NULL;
        };
        

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
        updatelist();//The peer who downloaded the file should send the new updated file list to the tracking server.
        load_index--;
        // free(buf);
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
            load_index++;
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
            load_index--;//finish uploading
        }
        else if(strcmp(token,"getload")==0){//request from another node
            load_index++;
            printf("inside receive get load\n");
            memset(message,0,sizeof(message));
            sprintf(message,"%d",load_index);
            int bytes=sendto(sock, message, sizeof(message), 0, (struct sockaddr *)&sender_addr, addr_len);
            load_index--;
            printf("inside receive get load after\n");
        }
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
    pthread_mutex_lock(&port_lock);
    int sock=bind_udp(map[atoi(&ID[strlen(ID)-1])-1].ports[port_index++]);
    pthread_mutex_unlock(&port_lock);
    puts("after bind");
    printf("receive port: %d\n",map[atoi(&ID[strlen(ID)-1])-1].ports[port_index++]);
    receive_udp_message(sock,sender_addr,addr_len);
    puts("after receive");    
}



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
        // printf("pong loop: %d\n",loop);
        strcpy(message,"ping;");
        // printf("before send\n");
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
            // printf("should be pong: %s\n",buf);
            continue;
        }
        // printf("should be pong: %s\n",buf);
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

void* update_loop(){
    while(1){
        sleep(10);
        printf("periodecally update files to server\n");
        updatelist(); // when thread
        
    }
}




int main(int argc, char* argv[]) {
    if (argc == 3) {
        strcpy(ID,argv[1]);
        freopen(argv[2], "r", stdin);
        printf("choose to run script\n");
    }
    pthread_t node_node_thread[MAX_CLIENT-1],download_thread[MAX_CLIENT-1],ping_thread,update_thread;
    // strcpy(dirname,"/home/tian0138/csci-5105/XFS/share/");
    strcpy(dirname, "/home/wan00807/5105/XFS/share/");
    strcpy(local_storage,dirname);
    if (argc != 3) {
        printf("Input machID\n");
        scanf("%s",ID);
        printf("%s\n",ID);
    }
    strcat(dirname,ID); 
    read_latency();//read latency.txt to map struct
    char func[10];
    char filename[15];
    int main_detect=0, node_thread=0;
    bool ping_created=false,nodetonode_created=false,update_created=false;
    int maxfd;
    fd_set readfds;
    puts("before boots");
    int boot_time=0;
    while(1){//this loop is blocked in while(!server_down); if server is down then reboot
        if(!server_down){
            boot();//when server is back or run for the first time make sure update first
            if(!update_created){
                if(pthread_create(&update_thread,NULL,update_loop,NULL)<0){//bind node waiting for other node's request
                fprintf(stderr, "Error creating thread\n");
                return -1;
            }
            update_created=true;
            }
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
            for(int i=0;i<MAX_CLIENT-1;i++){//create ports for each other clients
                if(pthread_create(&node_node_thread[node_thread++],NULL,receive_node,NULL)<0){//bind node waiting for other node's request
                    fprintf(stderr, "Error creating thread\n");
                    return -1;
                }
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
                    pthread_mutex_lock(&download_lock);
                    if(pthread_create(&download_thread[download_index++],NULL,(void*)download,filename)<0){
                        fprintf(stderr, "Error creating thread\n");
                        return -1;
                    }
                    load_index++;
                    pthread_mutex_unlock(&download_lock);
                }
                else if (strcmp(func,"find") == 0) {
                    char* node_sum=find(filename);
                    char nodelist[MAXBUFLEN];
                    int i=0;
                    if(node_sum!=""){
                        char* token = strtok(node_sum, ";");
                        while(token!=NULL){
                            strcpy(&nodelist[i],token);
                            strcat(&nodelist[i],";");
                            token = strtok(NULL, ";");//checksum
                            token = strtok(NULL, ";");
                            i+=6;
                        }
                        printf("all nodes with that file:%s\n",nodelist);
                    }
                    else{
                        printf("no node has that file\n");
                    }
                    free(node_sum);

                }
            }
        }//must input filename and function otherwise it won't reboot
        printf("main thread detect server_down\n");
        if(main_detect==0){
            // printf("main thread detect server_down\n");
        }
        main_detect=1;
    }
    for(int i=0;i<MAX_CLIENT-1;i++){
        pthread_join(node_node_thread[i],NULL);
    }
    for(int i = 0;i<download_index+1;i++){
        pthread_join(download_thread[i],NULL);
    }
    pthread_join(ping_thread,NULL);
    pthread_join(update_thread,NULL);

    
}

