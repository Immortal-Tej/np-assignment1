#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/select.h>

#include <calcLib.h>

#define DEBUG

using namespace std;


int main(int argc, char *argv[]){
  
  char delim[]=":";
  char *Desthost=strtok(argv[1],delim);
  char *Destport=strtok(NULL,delim);

  int port=atoi(Destport);
#ifdef DEBUG  
  printf("Host %s, and port %d.\n",Desthost,port);
#endif

  initCalcLib();

  struct addrinfo hints, *servinfo;
  int sockfd;
  
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  
  int rv = getaddrinfo(Desthost, Destport, &hints, &servinfo);
  if (rv != 0) {
    printf("getaddrinfo error: %s\n", gai_strerror(rv));
    return 1;
  }
  
  sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  if (sockfd == -1) {
    printf("Socket creation failed\n");
    freeaddrinfo(servinfo);
    return 1;
  }
  
  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    printf("setsockopt failed\n");
    close(sockfd);
    freeaddrinfo(servinfo);
    return 1;
  }
  
  if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
    printf("Bind failed\n");
    close(sockfd);
    freeaddrinfo(servinfo);
    return 1;
  }
  
  freeaddrinfo(servinfo);
  
  if (listen(sockfd, 5) == -1) {
    printf("Listen failed\n");
    close(sockfd);
    return 1;
  }
  
#ifdef DEBUG
  printf("Server listening on %s:%d\n", Desthost, port);
#endif

  while (1) {
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof(client_addr);
    
    int clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);
    if (clientfd == -1) {
      continue;
    }
    
    const char *protocol_msg = "TEXT TCP 1.0\n\n";
    send(clientfd, protocol_msg, strlen(protocol_msg), 0);
    
    fd_set readfds;
    struct timeval timeout;
    
    FD_ZERO(&readfds);
    FD_SET(clientfd, &readfds);
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    
    int select_result = select(clientfd + 1, &readfds, NULL, NULL, &timeout);
    
    if (select_result == 0) {
      const char *timeout_msg = "ERROR TO\n";
      send(clientfd, timeout_msg, strlen(timeout_msg), 0);
      close(clientfd);
      continue;
    }
    
    if (select_result > 0) {
      char buffer[256];
      int bytes_received = recv(clientfd, buffer, sizeof(buffer) - 1, 0);
      
      if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        
        if (strcmp(buffer, "OK\n") == 0) {
          // For now, just close after handshake
          close(clientfd);
        } else {
          close(clientfd);
        }
      } else {
        close(clientfd);
      }
    } else {
      close(clientfd);
    }
  }
  
  close(sockfd);
  return 0;
}
