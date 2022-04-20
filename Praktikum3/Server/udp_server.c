/* Iterativer UDP Echo-Server  
 * Basiert auf Stevens: Unix Network Programming  
 * getestet unter Ubuntu 20.04 64 Bit 
 */ 

#include <stdio.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <stdlib.h>
#include <map>

#define SRV_PORT 8999
#define MAXLINE  32768
#define HEADERSIZE 80

// Vorwaertsdeklarationen 
void dg_echo (int);  
void err_abort (char *str); 

// Explizite Deklaration zur Vermeidung von Warnungen 
// void *memset (void *s, int c, size_t n); 

int main (int argc, char *argv[]) { 
	// Deskriptor 
	int sockfd; 
	// Socket Adresse 
	struct sockaddr_in srv_addr; 

	// UDP-Socket erzeugen 
	if ((sockfd=socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
		err_abort((char*) "Kann Datagram-Socket nicht oeffnen!"); 
	} 

	// Binden der lokalen Adresse damit Clients uns erreichen 
	memset ((void *)&srv_addr, '\0', sizeof(srv_addr)); 
	srv_addr.sin_family = AF_INET; 
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	srv_addr.sin_port = htons(SRV_PORT); 
	if (bind (sockfd, (struct sockaddr *)&srv_addr, 
		sizeof(srv_addr)) < 0 ) { 
			err_abort((char*) "Kann  lokale  Adresse  nicht  binden,  laeuft  fremder Server?"); 
	} 
	printf ("UDP Echo-Server: bereit unter %d %d %d\n",AF_INET,INADDR_ANY,SRV_PORT);
	dg_echo(sockfd); 
}  

/* dg_echo: Lesen von Daten vom Socket und an den Client zuruecksenden */ 
void dg_echo (int sockfd) { 
	socklen_t alen;
	int n;
	struct sockaddr_in cli_addr;
    std::map<int,int> chunksizeMap;
    std::map<int,char[255]> filenameMap;
    static int eindeutigeID = 0;
    int sessionkey = 0;
    int lesezeichen = 0;
    struct stat info;
    FILE *fp;
    char in[MAXLINE], out[MAXLINE];
    char header[HEADERSIZE], data[MAXLINE - HEADERSIZE];
    char input[MAXLINE];
    char inputPart[MAXLINE];
    int chunksize = 0;
    char filename[255];

	for(;;) {
        alen = sizeof(cli_addr);
        memset((void *) &in, '\0', sizeof(in));
        sprintf(out," ");
        sprintf(header," ");
        // Daten vom Socket lesen -------------------------------------------------------------------------------------
        n = recvfrom(sockfd, in, MAXLINE, 0, (struct sockaddr *) &cli_addr, &alen);
        if (n < 0) {
            err_abort((char *) "Fehler beim Lesen des Sockets!");
        }
        printf("client: %s\n", in);
        strcpy(input, in);
        strcpy(inputPart, strtok(input, ";"));
        // Init Input zerlegen in filesize und filename ---------------------------------------------------------------
        if (strcmp(inputPart, "HSOSSTP_INITX") == 0) {
            chunksizeMap[eindeutigeID] = atoi(strtok(NULL, ";"));
            strcpy(filename, strtok(NULL, ";"));
            strcpy(filenameMap[eindeutigeID], filename);
            // pruefen ob Datei existiert
            fp = fopen(filename, "rb");
            if (fp == 0) {
                chunksizeMap.erase(chunksizeMap.find(eindeutigeID));
                filenameMap.erase(filenameMap.find(eindeutigeID));
                printf("Filename beim inti: %s\n",filename);
                sprintf(header, "HSOSSTP_ERROR;FNF");
            } else {
                sprintf(header, "HSOSSTP_SIDXX;%d", eindeutigeID++);
                fclose(fp);
            }
            sprintf(out, "%s", header);
            // Get Input zerlegen und bearbeiten ----------------------------------------------------------------------
        } else if (strcmp(inputPart, "HSOSSTP_GETXX") == 0) {
            sessionkey = atoi(strtok(NULL,";"));
            // chunksize anhand von Sessionkey auswaehlen
            auto search = chunksizeMap.find(sessionkey);
            if (search != chunksizeMap.end()) {
                chunksize = search->second;
            }else{
                sprintf(header, "HSOSSTP_ERROR;NOS");
            }
            // Filename anhand von Sessionkey auswaehlen
            auto search2 = filenameMap.find(sessionkey);
            if (search2 != filenameMap.end()) {
                sprintf(filename,"%s",search2->second);
            }else{
                sprintf(header, "HSOSSTP_ERROR;NOS");
            }
            // Datei oeffnen
            fp = fopen(filename, "rb");
            if (fp == 0) {
                auto search = filenameMap.find(sessionkey);
                if (search != filenameMap.end()) {
                    chunksizeMap.erase(chunksizeMap.find(sessionkey));
                    filenameMap.erase(filenameMap.find(sessionkey));
                }
                printf("Filename: %s\n",filename);
                sprintf(header, "HSOSSTP_ERROR;FNF");
            }
            // chunknummer pruefen und uebrnehmen
            lesezeichen = atoi(strtok(NULL, ";"));
            lstat(filename, &info);
            if (lesezeichen * chunksize >= info.st_size) {
                auto search = filenameMap.find(sessionkey);
                if (search != filenameMap.end()) {
                    chunksizeMap.erase(chunksizeMap.find(sessionkey));
                    filenameMap.erase(filenameMap.find(sessionkey));
                }
                sprintf(header, "HSOSSTP_ERROR;CNF");
            }
            // Dateinhalt mit header in buffer laden
            if (strcmp(header," ") == 0) {
                sprintf(header, "HSOSSTP_DATAX;%d;%d;", lesezeichen, chunksize);
                fseek(fp,lesezeichen * chunksize ,SEEK_SET);
                int fd = fileno(fp);
                int counter = lesezeichen * chunksize;
                while (counter >= 0 && counter < info.st_size && counter < (lesezeichen +1) * chunksize) {
                    // Daten aus Datei lesen
                    char temp[1];
                    fread(temp,1,1,fp);
                    char c = temp[0];
                    if (counter == lesezeichen * chunksize){
                        sprintf(data, "%c",c);
                    }else{
                        sprintf(data, "%s%c",data,c);
                    }
                    counter = counter +1;
                }
                fclose(fp);
                sprintf(out, "%s%s", header, data);
            }else{
                sprintf(out, "%s", header);
            }
        }
        // Daten aus Buffer senden ------------------------------------------------------------------------------------
        if (strcmp(out, " ") != 0) {
            printf("server: %s\n", out);
            n = strlen(out);
            if (sendto(sockfd, out, n, 0, (struct sockaddr *) &cli_addr, alen) != n) {
                err_abort((char *) "Fehler beim Schreiben des Sockets!");
            }
        }
    }
}


/* Ausgabe von Fehlermeldungen */ 
void err_abort(char *str){ 
	fprintf(stderr," UDP-Server: %s\n",str);
	fflush(stdout); 
	fflush(stderr); 
	_exit(1); 
} 
