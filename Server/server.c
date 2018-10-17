
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}

void SendtoClient(FILE *fd, int newsock, char *status, char *contentType, char *version) //creates header and sends file to client
{
    char header[128];
    
    if(fseek(fd, 0, SEEK_END) == -1)   // move pointer to end of file
        error("ERROR on fseek\n");
    
    long filesize = ftell(fd);        //get filesize
    if(filesize == -1)
        error("ERROR on ftell\n");
    
    if(fseek(fd, 0, SEEK_SET) != 0)     //return pointer to beginning of file
        error("ERROR on fseek\n");
    
    char *data = (char*)malloc(filesize);
    if(data == NULL)
        error("ERROR on allocating data\n");
    
    if(fread(data, filesize, 1, fd) != 1) //read contents of file into allocated data
        error("ERROR on reading from file\n");
    
    fclose(fd);

    sprintf(header,"%s %s\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n", version, status, filesize, contentType);

    int sendsize;
    
    sendsize = send(newsock, header, strlen(header), 0); //send header to client
    if(sendsize == -1)
    {
        error("ERROR on sending header\n");
    }
    
    while(filesize > 0) //keep sending parts of the file to the client until the whole file is sent
    {
        sendsize = send(newsock, data, filesize, 0);
        if(sendsize == -1)
        {
            error("ERROR on sending file\n");
        }
    
        data += sendsize;
        filesize -= sendsize;
    }
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    FILE *fp;

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // get socket descriptor
    if (sockfd < 0)
        error("ERROR opening socket");
    memset((char *) &serv_addr, 0, sizeof(serv_addr));

    // fill in address info
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) // bind socket descriptor to host
        error("ERROR on binding");

    listen(sockfd, 5);  // wait for connections from clients, only 5 allowed in queue

    //accept client socket descriptor
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0)
     error("ERROR on accept");

    int n;
    char buffer[256];
    char namebuf[256];
    char filename[256];
    char filetype[8];


    memset(buffer, 0, 256);

    //read client's message
    n = read(newsockfd, buffer, 255);
    if (n < 0) error("ERROR reading from socket");
    printf("Here is the request: %s\n", buffer);
    
    
    int dot = 0; //filetype flag
    int i = 0;
    int j = 5; //skip "GET /"
    int k = 0;
    while(1) //parse message for requested filename
    {
        if(buffer[j] == ' ')
        {
            namebuf[i] = '\0';
            filetype[k] = '\0';
            j++;
            break;
        }
        
        if(buffer[j] == '%') //replace %20 with spaces
        {
            if(buffer[j+1] == '2' && buffer[j+2] == '0')
            {
                namebuf[i] = ' ';
                if(dot == 1)
                {
                    filetype[k] = ' ';
                    k++;
                }
                j += 3;
                i++;
                continue;
            }
        }
        
        if(dot == 1)
        {
            filetype[k] = tolower(buffer[j]);
            k++;
            j++;
            continue;
        }
        
        if(buffer[j] == '.')
        {
            dot = 1;
        }
        
        namebuf[i] = buffer[j];
        i++;
        j++;
    }
    
    char version[10] = "HTTP/1.1";
    i = 0;
    while(1) //parse request for HTTP version
    {
        if(i == 8)
        {
            version[i] = '\0';
            break;
        }
        version[i] = buffer[j];
        i++;
        j++;
    }
    fprintf(stderr, "Here is the HTTP version: %s\n", version);

    
    sprintf(filename,"%s%s", namebuf, filetype);
    fprintf(stderr, "Here is the requested file: %s\n", filename);

    char contentType[20];   //create header for filetype
    if (strcmp(filetype, "jpeg") == 0 || strcmp(filetype, "jpg") == 0)
        sprintf(contentType, "image/jpeg");
    else if (strcmp(filetype, "gif") == 0)
        sprintf(contentType, "image/gif");
    else if (strcmp(filetype, "html") == 0 || strcmp(filetype, "htm") == 0)
        sprintf(contentType, "text/html");
    else
        sprintf(contentType, "text/html"); //if filetype is unknown, use default
    
    fprintf(stderr, "Here is the filetype and Content-Type: %s, %s\n", filetype, contentType);
    
    fp = fopen(filename, "rb"); //open file as binary

    if(fp == NULL) //if file not found, switch filetype to uppercase and check again
    {
        i = 0;
        while(1)
        {
            if(filetype[i] == '\0')
            {
                break;
            }
            filetype[i] = toupper(filetype[i]);
            i++;
        }
        sprintf(filename,"%s%s", namebuf, filetype);
        fprintf(stderr, "Cannot find file, here is the revised filename: %s\n", filename);
        fp = fopen(filename, "rb");
    }
    
    if(fp == NULL) //if file is still not found, send 404 page
    {
        fprintf(stderr, "File not found, opening 404 page\n");
        fp = fopen("404.html", "rb");
        if(fp == NULL)
            error("ERROR opening file\n");
        SendtoClient(fp, newsockfd, "404 Not Found", "text/html", version);
    }
    else
    {
        SendtoClient(fp, newsockfd, "200 OK", contentType, version);
    }

    close(newsockfd);  // close connection
    close(sockfd);

    return 0;
}


