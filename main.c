#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

char webpage[] = 
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html><head><title>LOL</title>\r\n"
"<style>body { background-color: blue }</style></head>\r\n"
"<body><center><h1>Lol hi there</h1></center>\r\n"
"<img src=\"doctest.png\"></body></html>\r\n";

int main(int argc, char *argv[]){
  /* Initialize the server */
  struct sockaddr_in server_addr, client_addr;
  socklen_t sin_len = sizeof(client_addr);
  int fd_server , fd_client;
  char buf[2048];
  int fdimg;
  int on = 1;

  fd_server = socket(AF_INET, SOCK_STREAM, 0);
  if(fd_server < 0){
    perror("socket");
    exit(1);
  }

  setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(8080);
  
  if(bind(fd_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1){
    perror("bind");
    close(fd_server);
    exit(1);
  }

  if(listen(fd_server, 10) == -1){
    perror("listen");
    close(fd_server);
    exit(1);
  }

  while(1){
    fd_client = accept(fd_server, (struct sockaddr *) &client_addr, &sin_len);

    if(fd_client == -1){
      perror("failed connection\n");
      continue;
    }

    printf("connection\n");

    if(!fork()){
      /* Child process */
      close(fd_server);
      memset(buf, 0, 2048);
      read(fd_client, buf, 2047);

      printf("%s\n", buf);

      if(!strncmp(buf, "GET /favicon.ico", 16)){
        printf("sending favicon.ico\n");
        fdimg = open("favicon.ico", O_RDONLY);
        sendfile(fd_client, fdimg, NULL, 5000);
        close(fdimg);
      } else if(!strncmp(buf, "GET /doctest.png", 16)){
        printf("sending doctest.png\n");
        fdimg = open("doctest.png", O_RDONLY);
        sendfile(fd_client, fdimg, NULL, 6000);
        close(fdimg);
      } else {
        write(fd_client, webpage, sizeof(webpage) - 1);
      }

      close(fd_client);
      printf("closing\n");
      exit(0);
    }

    /* Parent process */
    close(fd_client);
  }

  return 0;
}