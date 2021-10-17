#include <stdio.h> 
#include <string.h>  
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 

#define 8080

int main(int argc, char *argv[]){


    
    int sockfd, newsockfd;

    //client and sever address
    struct sockaddr_in cli_addr, srv_addr; 

    //socket(int family, int type, int protocol)
    //sockfd: Socket-Diskriptor, wird behandelt wie Datei oder bindirektionale Pipe. (wenn <0, Fehler)
    //family: Adressfamilie normalerweise AF_INET
    //type: gewünschter Protokolltyp bei AF_INET -> SOCK_STREAM(TCP), SOCK_DGRAM(UDP), SOCK_RAW(IP)
    //protocol: normalerweise Null
    if(sockfd = socket(AF_INET, SOCK_STREAM, 0) < 0){
        err_abort((char*) "kann socket nicht oeffnen!");
    }

    return 0;
}a