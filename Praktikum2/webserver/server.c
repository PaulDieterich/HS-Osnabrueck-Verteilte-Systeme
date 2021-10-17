#include <stdio.h> 
#include <string.h>  
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 

int main(int argc, char *argv[]){


    int sockfd, newsockfd, id; 

    //client and sever address
    struct sockaddr_in cli_addr, srv_addr; 

    //socket(int family, int type, int protocol)

    if(sockfd = socket(AF_INET, SOCK_STREAM, 0) < 0){
        err_abort((char*) "kann socket nicht oeffnen!");
    }

    return 0;
}