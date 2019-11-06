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

#define BUFSIZE 1024

qos_t current_qos;

uint16_t remote_port;
uint32_t requested_length;

int own_socket;
struct addrinfo* own_addr_info = 0;
int remote_socket;
struct addrinfo* remote_addr_info = 0;



void error(char *msg) {
    perror(msg);
    exit(1);
}

bool search_handle(key_handle_t key_handle) {
    return false;
}

uint32_t QKD_OPEN(destination_t dest, qos_t qos, key_handle_t key_handle) {
    remote_port = dest.port;
    requested_length = qos.requested_length;

    if (search_handle(key_handle)) {
        return QKD_OPEN_FAILED;
    }

    if (key_handle == NULL) {
        for (size_t i = 0; i < KEY_HANDLE_SIZE; i++) {
            key_handle[i] = (char) (rand() % 256);
        }
    }

    return SUCCESS;
}

uint32_t QKD_CONNECT_NONBLOCK(key_handle_t key_handle, uint32_t timeout) {
    if (remote_port == BOB_PORT) // we are Alice
    {
        setup_classical_server("localhost", ALICE_CLASS_PORT);
        setup_remote_server("localhost", BOB_CLASS_PORT);
    }
    else // we are Bob
    {
        setup_classical_server("localhost", BOB_CLASS_PORT);
        setup_remote_server("localhost", ALICE_CLASS_PORT);
    }

    return SUCCESS;
}

uint32_t QKD_CONNECT_BLOCKING(key_handle_t key_handle, uint32_t timeout) {
    if (remote_port == BOB_PORT) // we are Alice
    {
        setup_classical_server("localhost", ALICE_CLASS_PORT);
        setup_remote_server("localhost", BOB_CLASS_PORT);

        printf("waiting for classical msg\n");
        wait_for_classical(CLASS_MSG_HELLO);
        printf("classical msg received\n");
    }
    else // we are Bob
    {
        setup_classical_server("localhost", BOB_CLASS_PORT);
        setup_remote_server("localhost", ALICE_CLASS_PORT);

        printf("sending classical msg\n");
        send_classical(CLASS_MSG_HELLO);
    }

    return SUCCESS;
}

void setup_classical_server(char* hostname, char* portname)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_ADDRCONFIG;
    int err = getaddrinfo(hostname, portname, &hints, &own_addr_info);
    if (err != 0) {
        error("ERROR with getaddrinfo");
    }

    own_socket = socket(own_addr_info->ai_family, own_addr_info->ai_socktype, own_addr_info->ai_protocol);
    if (own_socket == -1) {
        error("ERROR opening socket");
    }

    int reuseaddr = 1;
    if (setsockopt(own_socket, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1) {
        error("ERROR with setsockopt");
    }

    if (bind(own_socket, own_addr_info->ai_addr, own_addr_info->ai_addrlen) == -1) {
        error("ERROR binding to socket");
    }
}

void setup_remote_server(char* hostname, char* portname)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_ADDRCONFIG;

    int err = getaddrinfo(hostname, portname, &hints, &remote_addr_info);
    if (err != 0) {
        error("ERROR with getaddrinfo");
    }

    remote_socket = socket(remote_addr_info->ai_family, remote_addr_info->ai_socktype, remote_addr_info->ai_protocol);
    if (remote_socket == -1) {
        error("ERROR opening socket");
    }

    int reuseaddr = 1;
    if (setsockopt(remote_socket, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1) {
        error("ERROR with setsockopt");
    }
}

int wait_for_classical(char msg)
{
    if (listen(own_socket, SOMAXCONN)) {
        error("FAILED to listen for connections");
    }
    int session_fd = accept(own_socket, 0, 0);

    char buf[BUFSIZE];
    int n = read(session_fd, buf, sizeof(char));
    if (n < 0) {
        error("ERROR reading from socket");
    }
    
    if (buf[0] != msg) {
        printf("Received wrong message\n");
        freeaddrinfo(own_addr_info);
        close(own_socket);
        return -1;
    }

    close(session_fd);
}

int send_classical(char msg)
{
    if (connect(remote_socket, remote_addr_info->ai_addr, remote_addr_info->ai_addrlen) < 0) {
        error("ERROR connecting");
    }
    
    char m = msg;
    int n = write(remote_socket, &m, 1);
    if (n < 0) {
        error("ERROR writing to socket");
    }

    // close(remote_socket);
}

uint32_t QKD_GET_KEY(key_handle_t key_handle, char* key_buffer) {
    cqc_ctx* cqc = cqc_init(10);

    if (remote_port == BOB_PORT) // we are Alice
    {
        if (cqc_connect(cqc, HOSTNAME, ALICE_PORT) != CQC_LIB_OK)
            return 0;
        
        printf("Connected.\n");

        struct hostent* server = gethostbyname(HOSTNAME);
        uint32_t remote_node = ntohl(*((uint32_t *)server->h_addr_list[0]));

        

        uint16_t qubits[100];
        uint8_t outcomes[100];
        entanglementHeader ent_info;
        for (int i = 0; i < requested_length; i++)
        {
            if (cqc_epr(cqc, 10, BOB_PORT, remote_node, &qubits[i], &ent_info) != CQC_LIB_OK)
                return 0;

            if (cqc_measure(cqc, qubits[i], &outcomes[i]) != CQC_LIB_OK)
                return 0;

            printf("waiting for Bob to confirm recv\n");
            wait_for_classical(CLASS_MSG_HELLO);       
            printf("confirmation received\n");
        }
        
        printf("Sent all EPRs and measured own half.\n");

        cqc_close(cqc);
        cqc_destroy(cqc);

        printf("Alice: outcomes: ");
        for (int i = 0; i < requested_length; i++)
        {
            printf("%d", outcomes[i]);
        }
        printf("\n");
    }
    else // we are Bob
    {
        if (cqc_connect(cqc, HOSTNAME, BOB_PORT) != CQC_LIB_OK)
            return 0;
        
        printf("Connected.\n");

        uint16_t qubits[100];
        uint8_t outcomes[100];
        entanglementHeader ent_info;
        for (int i = 0; i < requested_length; i++)
        {
            if (cqc_epr_recv(cqc, &qubits[i], &ent_info) != CQC_LIB_OK)
                return 0;

            if (cqc_measure(cqc, qubits[i], &outcomes[i]) != CQC_LIB_OK)
                return 0;

            setup_remote_server("localhost", ALICE_CLASS_PORT);
            printf("sending classical message\n");
            send_classical(CLASS_MSG_HELLO);
        }
        
        printf("Received all EPRs and measured all.\n");

        cqc_close(cqc);
        cqc_destroy(cqc);

        printf("Bob: outcomes: ");
        for (int i = 0; i < requested_length; i++)
        {
            printf("%d", outcomes[i]);
        }
        printf("\n");
    }

    

    return SUCCESS;
}

uint32_t QKD_CLOSE(key_handle_t key_handle) {

    return SUCCESS;
}
