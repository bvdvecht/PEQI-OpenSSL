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

qos_t current_qos;

uint16_t remote_port;
uint32_t requested_length;

socket_info own_socket;
socket_info remote_socket;

dict_t *connections;

/*
* searches for an existing key_handle
* will initialize connections if currently NULL
* return true iff key_handle exists
*/
connection_t* search_handle(key_handle_t key_handle) {
    if(connections == NULL) {
        //create new connections dict
        connections = dict_new();
        return NULL;
    } else {
        return dict_find(connections, &key_handle);
    }
}

void init_key_handle(key_handle_t *key_handle) {
    for (size_t i = 0; i < KEY_HANDLE_SIZE; i++) {
            *key_handle[i] = (char) (rand() % 256);
        }
}

uint32_t QKD_OPEN(destination_t dest, qos_t qos, key_handle_t key_handle) {
    if (key_handle == NULL) {

        //TODO we can get rid of this as we have our dict
        remote_port = dest.port;
        requested_length = qos.requested_length;
    
        init_key_handle(&key_handle);

        if(connections == NULL) {
            connections = dict_new();
        }
        
        connection_t conn = { .qos = qos, .dest = dest};
        dict_add(connections, key_handle, &conn);
        return SUCCESS;

    } else {
        connection_t *existing = search_handle(key_handle);
        if (existing == NULL) {
            connection_t conn = { .qos = qos, .dest = dest};
            dict_add(connections, key_handle, &conn);
            return SUCCESS;
        } else {
            if(existing->dest.address == dest.address
              && existing->dest.length == dest.length
              && existing->dest.port == dest.port
              && existing->qos.max_bps == qos.max_bps
              && existing->qos.priority == qos.priority
              && existing->qos.requested_length == qos.requested_length
              && existing->qos.timeout == qos.timeout) {
                return SUCCESS;
            } else {
                return QKD_OPEN_FAILED;
            }
        }
    }
}

uint32_t QKD_CONNECT_NONBLOCK(key_handle_t key_handle, uint32_t timeout) {
    cqc_ctx* cqc = cqc_init(10);
    if (!cqc_connect(cqc, HOSTNAME, ALICE_PORT) != CQC_LIB_OK) {
        return NO_QKD_CONNECTION;
    }
    
    if (remote_port == BOB_PORT) // we are Alice
    {
        setup_classical_server("localhost", ALICE_CLASS_PORT, &own_socket);
        setup_classical_client("localhost", BOB_CLASS_PORT, &remote_socket);
    }
    else // we are Bob
    {
        setup_classical_server("localhost", BOB_CLASS_PORT, &own_socket);
        setup_classical_client("localhost", ALICE_CLASS_PORT, &remote_socket);
    }

    return SUCCESS;
}

uint32_t QKD_CONNECT_BLOCKING(key_handle_t key_handle, uint32_t timeout) {
    cqc_ctx* cqc = cqc_init(10);
    for(int t=0; t < timeout/10; t++) {
        if (!cqc_connect(cqc, HOSTNAME, ALICE_PORT) != CQC_LIB_OK) {
            return NO_QKD_CONNECTION;
        }
        sleep(10);
    }
    
    if (remote_port == BOB_PORT) // we are Alice
    {
        setup_classical_server("localhost", ALICE_CLASS_PORT, &own_socket);
        setup_classical_client("localhost", BOB_CLASS_PORT, &remote_socket);

        printf("waiting for classical msg\n");
        wait_for_classical(own_socket, CLASS_MSG_HELLO);
        printf("classical msg received\n");
    }
    else // we are Bob
    {
        setup_classical_server("localhost", BOB_CLASS_PORT, &own_socket);
        setup_classical_client("localhost", ALICE_CLASS_PORT, &remote_socket);

        printf("sending classical msg\n");
        send_classical(remote_socket, CLASS_MSG_HELLO);
    }

    return SUCCESS;
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
            wait_for_classical(own_socket, CLASS_MSG_HELLO);
            printf("confirmation received\n");
        }
        
        printf("Sent all EPRs and measured own half.\n");

        cqc_close(cqc);
        cqc_destroy(cqc);

        printf("Alice: outcomes: ");
        for (int i = 0; i < requested_length; i++)
        {
            printf("%d", outcomes[i]);
            key_buffer[i] = outcomes[i];
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

            setup_classical_client("localhost", ALICE_CLASS_PORT, &remote_socket);
            printf("sending classical message\n");
            send_classical(remote_socket, CLASS_MSG_HELLO);
        }
        
        printf("Received all EPRs and measured all.\n");

        cqc_close(cqc);
        cqc_destroy(cqc);

        printf("Bob: outcomes: ");
        for (int i = 0; i < requested_length; i++)
        {
            printf("%d", outcomes[i]);
            key_buffer[i] = outcomes[i];
        }
        printf("\n");
    }

    return SUCCESS;
}

uint32_t QKD_CLOSE(key_handle_t key_handle) {

    return SUCCESS;
}
