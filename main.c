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

FILE *fdopen(int fd, const char *mode);

void error404(FILE *stream, char *DEFAULT404){
  int fd;
  char *p;
  struct stat sizebuf;

  /* read file size */
  stat(DEFAULT404, &sizebuf);

  /* print response header */
  fprintf(stream, "HTTP/1.1 404 Forbidden\n");
  fprintf(stream, "Content-Type: text/html; charset=UTF-8\r\n\r\n");
  
  /* open file and write it to response */
  fd = open(DEFAULT404, O_RDONLY);
  p = mmap(0, sizebuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  fwrite(p, 1, sizebuf.st_size, stream);
  munmap(p, sizebuf.st_size);
}

void error403(FILE *stream, char *DEFAULT403){
  int fd;
  char *p;
  struct stat sizebuf;

  /* read file size */
  stat(DEFAULT403, &sizebuf);

  /* print response header */
  fprintf(stream, "HTTP/1.1 403 Forbidden\n");
  fprintf(stream, "Content-Type: text/html; charset=UTF-8\r\n\r\n");
  
  /* open file and write it to response */
  fd = open(DEFAULT403, O_RDONLY);
  p = mmap(0, sizebuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  fwrite(p, 1, sizebuf.st_size, stream);
  munmap(p, sizebuf.st_size);
}

int main(int argc, char *argv[]){
  /* initialize config */
  struct stat cfgsize;
  FILE *cfg;
  char* buffer = NULL;
  size_t len;
  ssize_t bytes_read;

  int PORT = 8080;
  char DEFAULTPAGE[BUFSIZE];
  char DEFAULT404[BUFSIZE];
  char DEFAULT403[BUFSIZE];

  if(stat("config.cfg", &cfgsize) < 0){
    printf("cant load config\n");
    exit(1);
  }

  cfg = fopen("./config.cfg", "r");
  bytes_read = getdelim(&buffer, &len, '\0', cfg);

  if(bytes_read == -1){
    printf("cant load config\n");
    exit(1);
  }

  sscanf(buffer, "%d %s %s %s\n", PORT, DEFAULTPAGE, DEFAULT404, DEFAULT403);

  /* initialize process and logging */
  FILE *fp= NULL;
  pid_t process_id = 0;
  pid_t sid = 0;

  /* create fork */
  process_id = fork();

  if(process_id < 0){
    printf("fork failed!\n");
    exit(1);
  }
  
  /* kill parent process */
  if(process_id > 0){
    printf("process_id of child process %d \n", process_id);
    exit(0);
  }

  umask(0);

  sid = setsid();
  if(sid < 0){
    exit(1);
  }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  /* open log file */
  fp = fopen("log.txt", "w+");
  fclose(fp);

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
  int fd;
  int on = 1;
  char *p;
  struct stat sizebuf;

  /* try creating a socket */
  fd_server = socket(AF_INET, SOCK_STREAM, 0);
  if(fd_server < 0){
    perror("socket");
    exit(1);
  }

  setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);
  
  /* try binding */
  if(bind(fd_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1){
    perror("bind");
    close(fd_server);
    exit(1);
  }

  /* if more than 10 connections are queued than stop */
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

    if((stream = fdopen(fd_client, "r+")) == NULL){
      perror("ERROR on fdopen\n");
      continue;
    }

    printf("connection\n");

    /* get method, uri and version */
    fgets(buf, BUFSIZE, stream);

    printf("%s\n", buf);
    sscanf(buf, "%s %s %s\n", method, uri, version);

    /* get full buffer */
    fgets(buf, BUFSIZE, stream);
    printf("%s\m", buf);

    /* logging */
    fprintf(fp, "%s\n");
    fflush(fp);

    while(strcmp(buf, "\r\n")) {
      fgets(buf, BUFSIZE, stream);
      printf("%s", buf);
    }

    strcpy(filename, ".");
    strcat(filename, uri);
    if(uri[strlen(uri)-1] == '/'){
      strcat(filename, DEFAULTPAGE);
    }

    /* get file size and check if it exists */
    if(stat(filename, &sizebuf) < 0){
      printf("no such file %s\n", filename);
      error404(stream, DEFAULT404);
      fclose(stream);
      close(fd_client);
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

    /* open file and write it to response */
    fd = open(filename, O_RDONLY);
    
    if(fd < 0){
      printf("access denied %s\n", filename);
      error403(stream, DEFAULT403);
      fclose(stream);
      close(fd_client);
      continue;
    }

    /* print response header */
    fprintf(stream, "HTTP/1.1 200 OK\n");
    fprintf(stream, "Server: Lol\n");
    fprintf(stream, "Content-length: %d\n", (int)sizebuf.st_size);
    fprintf(stream, "Content-type: %s\n", filetype);
    fprintf(stream, "\r\n"); 
    fflush(stream);

    p = mmap(0, sizebuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    fwrite(p, 1, sizebuf.st_size, stream);
    munmap(p, sizebuf.st_size);

    /* close the stream */
    fclose(stream);
    close(fd_client);
  }

  return 0;
}