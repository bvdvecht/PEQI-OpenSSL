#ifndef QKD_API_H
#define QKD_API_H

#include <stdint.h>
#include "cqc.h"

#define KEY_HANDLE_SIZE 64
#define IP_ADDR_MAX_LEN 16

enum RETURN_CODES {
    SUCCESS = 0,
    QKD_GET_KEY_FAILED = 1,
    NO_QKD_CONNECTION = 2,
    QKD_OPEN_FAILED = 3,
    TIMEOUT_ERROR = 4,
};

typedef char key_handle_t[KEY_HANDLE_SIZE];

typedef struct {
    uint32_t requested_length;
    uint32_t max_bps;
    uint32_t priority;
    uint32_t timeout;
} qos_t;

typedef struct {
    uint8_t length;
    char address[IP_ADDR_MAX_LEN];
} ip_address_t;

typedef struct {
    uint8_t length;
    char address[IP_ADDR_MAX_LEN];
    uint16_t port;
} destination_t;

typedef struct {
    const key_handle_t key;
    qos_t qos;
    destination_t dest;
    cqc_ctx *cqc;
} connection_t;

typedef struct {
    key_handle_t *key;
    connection_t *conn;
} dict_entry_t;

typedef struct {
    int len;
    int cap;
    dict_entry_t *entry;
} dict_t;

int dict_find_index(dict_t *dict, key_handle_t *key) {
    for (int i = 0; i < dict->len; i++) {
        if (!strcmp(dict->entry[i].key, key)) {
            return i;
        }
    }
    return -1;
}

connection_t* dict_find(dict_t *dict, key_handle_t *key) {
    int idx = dict_find_index(dict, key);
    return idx == -1 ? NULL : dict->entry[idx].conn;
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

uint32_t QKD_OPEN(destination_t dest, qos_t QoS, key_handle_t key_handle);

uint32_t QKD_CONNECT_NONBLOCK(key_handle_t key_handle, uint32_t timeout);

uint32_t QKD_CONNECT_BLOCKING(key_handle_t key_handle, uint32_t timeout);

uint32_t QKD_GET_KEY(key_handle_t key_handle, char *key_buffer);

uint32_t QKD_CLOSE(key_handle_t key_handle);

#endif
