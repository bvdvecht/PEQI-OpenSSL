#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "qkd_api.h"
#include "cqc.h"
#include "defines.h"
#include "class_comm.h"

#define BUFSIZE 1024

void error(char *msg) {
    perror(msg);
    exit(1);
}

void setup_classical_server(char* hostname, char* portname, socket_info* sinfo)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_ADDRCONFIG;
    int err = getaddrinfo(hostname, portname, &hints, &sinfo->info);
    if (err != 0) {
        error("ERROR with getaddrinfo");
    }

    sinfo->socket = socket(sinfo->info->ai_family, sinfo->info->ai_socktype, sinfo->info->ai_protocol);
    if (sinfo->socket == -1) {
        error("ERROR opening socket");
    }

    int reuseaddr = 1;
    if (setsockopt(sinfo->socket, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1) {
        error("ERROR with setsockopt");
    }

    if (bind(sinfo->socket, sinfo->info->ai_addr, sinfo->info->ai_addrlen) == -1) {
        error("ERROR binding to socket");
    }
}

void setup_classical_client(char* hostname, char* portname, socket_info* sinfo)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_ADDRCONFIG;

    int err = getaddrinfo(hostname, portname, &hints, &sinfo->info);
    if (err != 0) {
        error("ERROR with getaddrinfo");
    }

    sinfo->socket = socket(sinfo->info->ai_family, sinfo->info->ai_socktype, sinfo->info->ai_protocol);
    if (sinfo->socket == -1) {
        error("ERROR opening socket");
    }

    int reuseaddr = 1;
    if (setsockopt(sinfo->socket, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1) {
        error("ERROR with setsockopt");
    }
}

int wait_for_classical(socket_info sinfo, char msg)
{
    if (listen(sinfo.socket, SOMAXCONN)) {
        error("FAILED to listen for connections");
    }
    int session_fd = accept(sinfo.socket, 0, 0);

    char buf[BUFSIZE];
    int n = read(session_fd, buf, sizeof(char));
    if (n < 0) {
        error("ERROR reading from socket");
    }
    
    if (buf[0] != msg) {
        printf("Received wrong message\n");
        freeaddrinfo(sinfo.info);
        close(sinfo.socket);
        return -1;
    }

    close(session_fd);
}

int send_classical(socket_info sinfo, char msg)
{
    if (connect(sinfo.socket, sinfo.info->ai_addr, sinfo.info->ai_addrlen) < 0) {
        error("ERROR connecting");
    }
    
    char m = msg;
    int n = write(sinfo.socket, &m, 1);
    if (n < 0) {
        error("ERROR writing to socket");
    }

    // close(remote_socket);
}