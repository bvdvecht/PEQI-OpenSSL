#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct {
    int socket;
    struct addrinfo* info;
} socket_info;

void setup_classical_server(char* hostname, char* portname, socket_info* sinfo);
void setup_classical_client(char* hostname, char* portname, socket_info* sinfo);
int wait_for_classical(socket_info sinfo, char msg);
int send_classical(socket_info sinfo, char msg);