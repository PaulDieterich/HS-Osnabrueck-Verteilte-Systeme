/*
* Autor(en):    Paul Dieterich, Christina B.
* Praktikum:    Verteilte Systeme WiSe 2021
* Programm:     Einfacher TCP Web-Server
*
* Hinweise zum kompilieren/starten:
*       Kompilieren:    gcc -o HTTPServer_TCP HTTPServer_TCP.c
*       Starten:        ./HTTPServer_TCP <DIRECTORY>  |OPTIONAL: <PORT>|
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#define PORT 8080
#define MAX_SOCK 10
#define MAXLINE 30000

void getRequest(char *request, char *dateiName, char *dir_path);
void getContentType(char *dateiName, char *contentType, char *status, char *file_path, char *dir_path);
void request(int socket, char *dir_path);
void sendHeader(int socket, char *status, char *type);
void sendBody(int socket, DIR *dir, char *status, char *type, char *dir_path, char *content, char *dateiName);
void err_abort(char *str);


int main(int argc, char const *argv[])
{
    int server_fd, new_socket;
    int port;
    long valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int reuse = 1;
    char *dir_path = NULL;

    if (argc < 2)
    {
        err_abort((char *)"es fehlen argumente");
    }
    dir_path = argv[1];
    if (argc == 3){port = atoi(argv[2]);}
    else{ port = PORT;}

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
         err_abort((char *)"In socket");
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        err_abort((char *)"Kann Socketoption nicht setzen!");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    memset(address.sin_zero, '\0', sizeof address.sin_zero);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        err_abort((char *)"Kann locale Adrese nicht binden");
    }

    listen(server_fd, 5);

    printf((char *)"Web-Server: bereit... \n");
    while (1)
    {
        printf("\n+++++++ Waiting for new connection ++++++++\n\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("In accept"); exit(EXIT_FAILURE);
        }
       
        request(new_socket, dir_path);

        close(new_socket);
        printf("\n+++++++ Socket Closed ++++++++\n\n");
    }
    return 0;
}
void request(int socket, char *dir_path)
{
    DIR *dir = NULL;
    dir = opendir(dir_path);
    char request[MAXLINE] = {0};
    char contentType[20] = {0};
    char content[MAXLINE] = {0};
    char dateiName[MAXLINE] = {0};
    char file_path[MAXLINE] = {0};
    char status[16] = {0};

    long valread = read(socket, request, sizeof(request));
    printf("%s\n", request);
   
    getRequest(request, dateiName, dir_path);
   
    getContentType(dateiName, contentType, status, file_path, dir_path);
  
    sendHeader(socket, status, contentType);

    sendBody(socket, dir, status, contentType, dir_path, content, dateiName);

   // write(socket, content, strlen(content));
    printf("write to socket - content: %s", content);
}

//mit Hilfe von lukas Lübberding
void getRequest(char *request, char *dateiName, char *dir_path)
{
    char *pattern1 = "GET ";
    char *pattern2 = " HTTP/1.1";
    char *target = NULL;
    char *start, *end;
    /* Wenn pattern1 gefunden */
    if (start = strstr(request, pattern1))
    {
        /* pattern1 überspringen */
        start += strlen(pattern1);
        /* wenn pattern2 gefunden */
        if (end = strstr(start, pattern2))
        {
            /* target setzen */
            target = (char *)malloc(end - start + 1);
            memcpy(target, start, end - start);
            target[end - start] = '\0';
        }
    }
    strcpy(dateiName, target);
    printf("getRequest %s\n", dateiName);
}

void getContentType(char *dateiName, char *contentType, char *status, char *file_path, char *dir_path)
{
    char tmp[1000] = {0};
    char stat[1000] = {0};
    char type[1000] = {0};
    struct stat *buf;
    snprintf(tmp, sizeof(tmp), "%s%s", file_path, dateiName);
    strcpy(file_path, tmp);
    printf("getContent %s\n", file_path);

    if (lstat(dir_path, &buf) == 0)
    {
        strcpy(stat, "200 OK");
    }
    else
    {
        strcpy(stat, "404 ERROR");
    }
    printf("getContent path %s\n", dir_path); //dir_path
    char *extension = strrchr(file_path, '.');
    printf("getContent extension %s\n", extension);
    if (extension)
    {
        if (strstr(extension, "html"))
        {
            strcpy(type, "text/html; charset=iso-8859-1");
        }
        else if (strstr(extension, "txt"))
        {
            strcpy(type, "text/plain");
        }
        else if (strstr(extension, "jpg"))
        {
            strcpy(type, "image/jpg");
        }
    }
    else
    {
        strcpy(type, "text/html; charset=iso-8859-1");
    }
    strcpy(status, stat);
    strcpy(contentType, type);
   
    

}

void sendHeader(int socket, char *status, char *type)
{
    char sh[1000] = {0};
    snprintf(sh, sizeof(sh), "HTTP/1.1 %s\r\nContent-Type: %s\r\n\r\n", status, type);
    write(socket, sh, sizeof(sh));
    printf("sendHeader %s\n", sh);
}

void sendBody(int socket, DIR *dir, char *status, char *type, char *file_path, char *content, char *dateiName)
{

    dir = opendir(file_path);
    int content_length = 0;
    char response[MAXLINE] = {0};
    printf("sendBody %s\n", dateiName);
    if (strstr(status, "200"))
    {
        if (strstr(type, "text/html") || strstr(type, "text/plane"))
        {
            char *extension = strrchr(file_path, '.');
            printf("sendBody extension %s\n", extension);
            if (extension)
            {
                FILE *file = fopen(file_path, "r");
                fread(content, 2000, 2000, file);
                write(socket, content, strlen(content));
                fclose(file);
            }
            else
            {
                struct dirent *entry;
                struct stat attribut;

                char data[MAXLINE] = {0};
                char filepath[MAXLINE] = {0};
                char url[MAXLINE] = {0};

               
                strcpy(content, "<h1>Verzeichnisse</h1>");
                
                while (entry = readdir(dir))
                {
                   
                    if (!strcmp(entry->d_name, "..") || !strcmp(entry->d_name, "."))
                        continue;

                   
                    sprintf(url, "%s%s", file_path, entry->d_name);

                    stat(url, &attribut);
                    sprintf(url, "%s", dateiName);
                  
                    strcat(url, entry->d_name);
                      printf("sendBody url %s\n", url);
                    if (S_ISDIR(attribut.st_mode))
                    {
                        strcat(url, "/");
                    }

                    
                    sprintf(data, "<p><a href=\"%s\">%s</a></p>", url, entry->d_name);

    
                    strcat(content, data);
                    
                }
                write(socket, content, strlen(content));
            }
        }
       else if (strstr(type, "image/jpg")) {
            printf("sendBody type: image/jpg\n");
            int buffer[512] = {0};

            FILE* file = fopen(file_path, "rb");
            printf(file);
            fseek (file , 0 , SEEK_END);
            long lSize = ftell(file);
            fseek(file, 0, SEEK_SET);

        
            char response[MAXLINE] = {0};
            while(fread(buffer, sizeof(int), sizeof(buffer)/sizeof(int), file))
            {
            
                if(write(socket, buffer, sizeof(buffer)) != sizeof(buffer))
                {
                
                    printf("image error\r\n");
                    return;
                }
            }
            fclose(file);
        }else
        {
           
            snprintf(response, sizeof(response), "Content-Length: 9\r\n\r\nERROR 404");
            write(socket, response, strlen(response));
        }

    }
    printf("\n ende sendbody\n");
}
void err_abort(char *str)
{
    fprintf(stderr, " Web-Server: %s\n", str);
    fflush(stdout);
    fflush(stderr);
    _exit(1);
}