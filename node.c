#include "tool.h"


char server_IP[20];
int server_port = 8000;


int main(void) {
    char message[100];
    strcpy(message,"I am fine");
    strcpy(server_IP,"127.0.0.1");
    send_message(server_IP,server_port, message);
}