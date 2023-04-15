#include "tool.h"

int server_port = 8000;

int main(){
    int sockfd = (bind_udp(server_port));
    receive_udp_message(sockfd);
}
