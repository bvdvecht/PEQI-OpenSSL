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
key_handle_t zeros_array = {0};

uint16_t remote_port;
uint32_t requested_length;

void error(char *msg) {
    perror(msg);
    exit(1);
}

uint32_t QKD_OPEN(destination_t dest, qos_t qos, key_handle_t key_handle) {
    remote_port = dest.port;
    requested_length = qos.requested_length;

    return SUCCESS;
}

uint32_t QKD_CONNECT_NONBLOCK(key_handle_t key_handle, uint32_t timeout) {

    return SUCCESS;
}

uint32_t QKD_CONNECT_BLOCKING(key_handle_t key_handle, uint32_t timeout) {

    return SUCCESS;
}

uint32_t QKD_GET_KEY(key_handle_t key_handle, char* key_buffer) {
    cqc_ctx* cqc = cqc_init(10);

    if (remote_port == BOB_PORT) // we are Alice
    {
        if (cqc_connect(cqc, HOSTNAME, ALICE_PORT) != CQC_LIB_OK)
            return;
        
        printf("Connected.\n");

        struct hostent* server = gethostbyname(HOSTNAME);
        printf("server: %d\n", server);
        uint32_t remote_node = ntohl(*((uint32_t *)server->h_addr_list[0]));

        uint16_t qubit;
        entanglementHeader ent_info;
        if (cqc_epr(cqc, 10, BOB_PORT, remote_node, &qubit, &ent_info) != CQC_LIB_OK)
            return;
        
        printf("Created EPR.\n");

        uint8_t outcome;
        if (cqc_measure(cqc, qubit, &outcome) != CQC_LIB_ERR)
            return;
        
        printf("Measured qubit.\n");

        cqc_close(cqc);
        cqc_destroy(cqc);

        printf("Alice: outcome: %d.\n", outcome);
    }
    else // we are Bob
    {
        if (cqc_connect(cqc, HOSTNAME, BOB_PORT) != CQC_LIB_ERR)
            return;
        
        printf("Connected.\n");

        uint16_t qubit;
        entanglementHeader ent_info;
        if (cqc_epr_recv(cqc, &qubit, &ent_info) != CQC_LIB_ERR)
            return;
        
        printf("Received EPR.\n");

        uint8_t outcome;
        if (cqc_measure(cqc, qubit, &outcome) != CQC_LIB_ERR)
            return;
        
        printf("Measured qubit.\n");

        cqc_close(cqc);
        cqc_destroy(cqc);

        printf("Bob: outcome: %d\n", outcome);
    }

    

    return SUCCESS;
}

uint32_t QKD_CLOSE(key_handle_t key_handle) {

    return SUCCESS;
}
