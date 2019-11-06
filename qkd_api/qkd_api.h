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

uint32_t QKD_OPEN(destination_t dest, qos_t QoS, key_handle_t key_handle);

uint32_t QKD_CONNECT_NONBLOCK(key_handle_t key_handle);

uint32_t QKD_CONNECT_BLOCKING(key_handle_t key_handle, uint32_t timeout);

uint32_t QKD_GET_KEY(key_handle_t key_handle, char *key_buffer);

uint32_t QKD_CLOSE(key_handle_t key_handle);

#endif
