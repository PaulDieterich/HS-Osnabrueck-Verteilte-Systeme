#include <stdio.h> 
#include <string.h>  
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#define SRV_PORT 8080
#define MAX_SOCK 10 
#define MAXLINE 512 



char readFile();
void err_abort(char* str); 
void err_notfound(char* str);

int main(int argc, char *argv[]){
    //char* filepath = argv[1];
    //int SRV_PORT = 8080;
    //if(argc > 2){ SRV_PORT = argv[2]; }
    //client and sever address

    //socket(int family, int type, int protocol)
    //sockfd: Socket-Diskriptor, wird behandelt wie Datei oder bindirektionale Pipe. (wenn <0, Fehler)
    //family: Adressfamilie normalerweise AF_INET
    //type: gewünschter Protokolltyp bei AF_INET -> SOCK_STREAM(TCP), SOCK_DGRAM(UDP), SOCK_RAW(IP)
    //protocol: normalerweise Null
   int server_fd, new_socket, pid; long valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

     char *hello = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\r\nHello world!";
     
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
       err_abort((char*) "Socket konnte nicht erstellt werden");
    }
    

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( SRV_PORT);
    
    memset(address.sin_zero, '\0', sizeof address.sin_zero);
    
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
       err_abort((char*) "Kann locale Adrese nicht binden");
    }

    //listen(int sockfd, int backlog)
    //backlog = Anzahl noch nicht behandelter Anfragen in Warteschlange
    listen(server_fd, 5);
    printf((char*) "Web-Server: bereit... \n");
    FILE *file;
    file = fopen("./docroot/index.html","rb");

   

	while(1){ 
	  printf("\n+++++++ Waiting for new connection ++++++++\n\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
           err_abort((char*) "HTTP 400 - Fehler beim Verbindungsaufbau!"); 
        } 
        char* f = readFile();
        char buffer[30000] = {0};

        valread = read(new_socket , buffer, sizeof(buffer));
        printf("%s\n",buffer);
        write(new_socket , f, strlen(buffer));
        printf("------------------Hello message sent-------------------\n");
		
        
	close (new_socket); 
	}
	
}

char readFile(){
    char buffer[3000] = {0}, *current=buffer;
    int bytes, chunk = 20, size=sizeof(buffer)/sizeof(char);
    FILE *file = fopen("./docroot/folder/index.html","rb");
    if(file){
        do{ 
            bytes =fread(current, sizeof(char), chunk, file);
            current+=bytes;
        }while(bytes==chunk);
        fclose(file);
    }
return &buffer;
}
/*
char* readFile(char* file){
    char* host = strcat("localhost", ":");
    char* context = "text/html";
    char sendline[MAXLINE +1], recvline[MAXLINE +1];
    size_t MAXLENGTH, n;
    snprintf(sendline, MAXLENGTH,
    "GET %s HTTP/1.0\r\n"
    "HOST: %s%d \r\n"
    "Content-type: %s"
    "Content-length: %d\r\n\r\n"
    "%s"
    , file,host,PORT, context);

    return sendline;

}
*/
/* Ausgabe von Fehlermeldungen */ 
void err_abort(char *str){ 
	fprintf(stderr," Web-Server: %s\n",str); 
	fflush(stdout); 
	fflush(stderr); 
	_exit(1); 
}
void err_notfound(char *str){ 
	fprintf(stderr," Web-Server: %s\n",str); 
	fflush(stdout); 
	fflush(stderr); 
	_exit(1); 
}