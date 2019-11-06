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
#define APPLICATION_ID 10

qos_t current_qos;
key_handle_t zeros_array = {0};

uint16_t remote_port;
uint32_t requested_length;

dict_t *connections;

void error(char *msg) {
    perror(msg);
    exit(1);
}


int dict_find_index(dict_t *dict, key_handle_t *key) {
    for (int i = 0; i < dict->len; i++) {
        if (!strcmp(dict->entry[i].key, *key)) {
            return i;
        }
    }
    return -1;
}

connection_t* dict_find(dict_t *dict, key_handle_t *key) {
    int idx = dict_find_index(dict, key);
    return idx == -1 ? 0 : dict->entry[idx].conn;
}

void dict_add(dict_t *dict, const key_handle_t key, connection_t *conn) {
   int idx = dict_find_index(dict, key);
   if (idx != -1) {
       dict->entry[idx].key = key;
       dict->entry[idx].conn = conn;
       return;
   }
   if (dict->len == dict->cap) {
       dict->cap *= 2;
       dict->entry = realloc(dict->entry, dict->cap * sizeof(connection_t));
   }
   dict->entry[dict->len].key = key;
   dict->entry[dict->len].conn = conn;
   dict->len++;
}

dict_t *dict_new(void) {
    dict_t proto = {0, 10, malloc(10 * sizeof(dict_entry_t))};
    dict_t *d = malloc(sizeof(dict_t));
    *d = proto;
    return d;
}

void dict_free(dict_t *dict) {
    for (int i = 0; i < dict->len; i++) {
        free(dict->entry[i].key);
    }
    free(dict->entry);
    free(dict);
}

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

uint32_t QKD_CONNECT_NONBLOCK(key_handle_t key_handle) {
    connection_t *conn = search_handle(key_handle);

    if(conn == NULL) {
        return NO_QKD_CONNECTION;
    }

    cqc_ctx* cqc = cqc_init(APPLICATION_ID);
    if (cqc_connect(cqc, conn->dest.address, conn->dest.port) != CQC_LIB_OK) {
        return SUCCESS;
    }
    return NO_QKD_CONNECTION;
}

uint32_t QKD_CONNECT_BLOCKING(key_handle_t key_handle, uint32_t timeout) {
    connection_t *conn = search_handle(key_handle);

    if(conn == NULL) {
        return NO_QKD_CONNECTION;
    }

    cqc_ctx* cqc = cqc_init(APPLICATION_ID);
    for(int t=0; t < timeout/10; t++) {
        if (cqc_connect(cqc, conn->dest.address, conn->dest.port) != CQC_LIB_OK) {
            return SUCCESS;
        }
        sleep(10);
    }
    return NO_QKD_CONNECTION;
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

        uint16_t qubit;
        entanglementHeader ent_info;
        if (cqc_epr(cqc, 10, BOB_PORT, remote_node, &qubit, &ent_info) != CQC_LIB_OK)
            return 0;
        
        printf("Created EPR.\n");

        uint8_t outcome;
        if (cqc_measure(cqc, qubit, &outcome) != CQC_LIB_OK)
            return 0;
        
        printf("Measured qubit.\n");

        cqc_close(cqc);
        cqc_destroy(cqc);

        printf("Alice: outcome: %d.\n", outcome);
    }
    else // we are Bob
    {
        if (cqc_connect(cqc, HOSTNAME, BOB_PORT) != CQC_LIB_OK)
            return 0;
        
        printf("Connected.\n");

        uint16_t qubit;
        entanglementHeader ent_info;
        if (cqc_epr_recv(cqc, &qubit, &ent_info) != CQC_LIB_OK)
            return 0;
        
        printf("Received EPR.\n");

        uint8_t outcome;
        if (cqc_measure(cqc, qubit, &outcome) != CQC_LIB_OK)
            return 0;
        
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
