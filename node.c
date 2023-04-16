#include "tool.h"


char server_IP[20];
int server_port = 8000;
struct machIDlaten map[MAXNODES]; //key value pairs to store latency
char ID[15];
char dirname[100];
char IP[20]="127.0.0.1";
int node_node_port;
int load_index=0;

int min(int x,int y){
    return (x < y) ? x : y;
}

int get_latency(char* machID){
    for(int i =0;i<MAXNODES;i++){
        if (strcmp(map[i].machID,machID)==0){
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
    FILE *file = fopen("latency.txt", "r");
    if (file == NULL) {
        printf("Error: could not open file\n");
        exit(1);
    }
    char* token;
    char line[100];
    int i =0;
    while (fgets(line, sizeof(line), file) != NULL && i<MAXNODES) {
        printf("%s", line);
        token = strtok(line,":");
        strcpy(map[i].machID,token);
        printf("%s", token);
        token = strtok(NULL,":");
        map[i].latency=atoi(token);
        printf("%d", atoi(token));
        token = strtok(NULL,":");
        map[i].port=atoi(token);
        if(strcmp(map[i].machID,ID)==0){ //if node already in the txt then assign the port to the global
            node_node_port=map[i].port;
        }
        printf("%d", atoi(token));
        i++;
    }
    fclose(file);
}

void boot(){
    char message[100];
    pthread_t upload_thread;
    strcpy(message,"boot;");
    strcat(message,ID);
    strcat(message,";");
    DIR* dir;
    struct dirent *ent;
    if ((dir = opendir (dirname)) != NULL) {//loop through and get every filename under the machID
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

int getload(int port){
    char message[20];
    strcpy(message,"getload");
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
    char path[100];
    int min_score=INT_MAX;
    pthread_t thread_ids[20];
    char* token;
    char message[100];
    int load,latency,score,port,min_port;
    strcpy(message,"download;");
    strcat(message,filename);
    int sock=send_message(server_IP,server_port,message);//send the intended filename to server
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
    close(sock);
    token = strtok(buf,";");
    while(token!=NULL){//loop through every node; token here means the machID
        port=get_port(token);
        load=getload(port);//get the load of the node
        latency = get_latency(token);
        score=load+0.1*latency;
        min_score=min(score,min_score);
        if(min_score==score){min_port=port;}//update the minport to best peer
        token=strtok(NULL,";");
    }
    sock=send_message(IP,min_port,message);
    memset(buf,0,sizeof(buf));
    struct sockaddr_storage sender_addr2;
    socklen_t addr_len2 = sizeof(sender_addr2);
    numbytes = recvfrom(sock, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&sender_addr2, &addr_len2);
    if (numbytes == -1) {
        perror("recvfrom");
        exit(1);
    }
    printf("%s",buf);
    printf("\n");
    buf[numbytes] = '\0';  // get back a list of nodes with that filename
    printf("Received subsribed article from server: %s\n", buf);
    sprintf(path,"%s%s",dirname,filename);
    FILE *fp=fopen(path,"w");
    if(fp==NULL){
        printf("error creating file\n");
    }
    else{
        printf("create file success\n");
        fseek(fp, 0, SEEK_END);
        fwrite(buf,sizeof(char),sizeof(buf),fp);
        fclose(fp);
    }
    close(sock);
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

void* receive_udp_message(int arg) {
    int sockfd = arg;
    char buf[MAXBUFLEN];
    struct sockaddr_storage sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
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
    }

    pthread_exit(NULL);
}

void* receive_node(){
    int sock=bind_udp(node_node_port);
}




int main(void) {
    pthread_t node_node_thread,download_thread;
    strcpy(dirname,"/home/tian0138/csci-5105/XFS/share/");
    strcat(dirname,ID); 
    read_latency();//read latency.txt to map struct
    printf("Input machID\n");
    scanf("%s",ID);
    if(get_latency(ID)==-1){//add new node to latency file 
        FILE *fp=fopen(strcat(dirname,"latency.txt"),"w");
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
    boot();
    if(pthread_create(&node_node_thread,NULL,receive_node,NULL)<0){//bind node waiting for other node's request
        fprintf(stderr, "Error creating thread\n");
        return -1;
    }
    printf("Input function and filename");
    scanf("%s %s", func,filename);
    if(strcmp(func,"download")==0){
        if(pthread_create(&download_thread,NULL,(void*)download,filename)<0){
            fprintf(stderr, "Error creating thread\n");
            return -1;
        }
        load_index++;
    }

    
   


}

