#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 2048
#define PORT 8080
#define DEFAULTPAGE "index.html"

char error403[] = 
"HTTP/1.1 403 Forbidden\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html><head><title>:(</title></head>\r\n"
"<body><center><h1>Error 403 sori</h1></center></body></html>\r\n";

char error404[] = 
"HTTP/1.1 404 Forbidden\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html><head><title>:(</title></head>\r\n"
"<body><center><h1>No such file sori</h1></center></body></html>\r\n";

int main(int argc, char *argv[]){
  /* initialize the server */
  FILE *stream;
  struct sockaddr_in server_addr, client_addr;
  socklen_t sin_len = sizeof(client_addr);
  int fd_server, fd_client;
  char buf[BUFSIZE];
  char uri[BUFSIZE];
  char method[BUFSIZE];
  char version[BUFSIZE];
  char filename[BUFSIZE];
  char filetype[BUFSIZE];
  int on = 1;
  struct stat sizebuf;

  /* try creating a socket */
  fd_server = socket(AF_INET, SOCK_STREAM, 0);
  if(fd_server < 0){
    error("socket");
    exit(1);
  }

  setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);
  
  /* try binding */
  if(bind(fd_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1){
    error("bind");
    close(fd_server);
    exit(1);
  }

  /* if more than 10 connections are queued than stop */
  if(listen(fd_server, 10) == -1){
    error("listen");
    close(fd_server);
    exit(1);
  }

  while(1){
    fd_client = accept(fd_server, (struct sockaddr *) &client_addr, &sin_len);

    if(fd_client == -1){
      perror("failed connection\n");
      continue;
    }

    if((stream = fdopen(childfd, "r+")) == NULL){
      error("ERROR on fdopen");
    }

    printf("connection\n");

    fgets(buf, BUFSIZE, stream);

    printf("%s\n", buf);
    sscanf(buf, "%s %s %s\n", method, uri, version);

    strcpy(filename, ".");
    strcat(filename, uri);
    if(uri[strlen(uri)-1] == '/'){
      strcat(filename, DEFAULTPAGE);
    }

    /* get file size and check if it exists */
    if(stat(filename, &sizebuf) < 0){
      write(fd_client, error404, sizeof(error404) - 1);
      fclose(stream);
      close(childfd);
      continue;
    }

    /* get filetype */
    if(strstr(filename, ".html")){
      strcpy(filetype, "text/html");
    } else if(strstr(filename, ".gif")){
      strcpy(filetype, "image/gif");
    } else if(strstr(filename, ".jpg")){
      strcpy(filetype, "image/jpg");
    } else {
      strcpy(filetype, "text/plain");
    }

    /* print response header */
    fprintf(stream, "HTTP/1.1 200 OK\n");
    fprintf(stream, "Server: Lol\n");
    fprintf(stream, "Content-length: %d\n", (int)sizebuf.st_size);
    fprintf(stream, "Content-type: %s\n", filetype);
    fprintf(stream, "\r\n"); 
    fflush(stream);
    
    /* Use mmap to return arbitrary-sized response body */
    fd = open(filename, O_RDONLY);
    p = mmap(0, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    fwrite(p, 1, sbuf.st_size, stream);
    munmap(p, sbuf.st_size);

    /* close the stream */
    fclose(stream);
    close(childfd);
  }

  return 0;
}