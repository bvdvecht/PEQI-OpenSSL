#include "qkd_api.h"
#include "../defines.h"

int main()
{
    destination_t dest;
    dest.port = BOB_PORT;
    
    qos_t qos;
    qos.requested_length = REQUESTED_LENGTH;
    key_handle_t handle;
    QKD_OPEN(dest, qos, handle);
    QKD_CONNECT_BLOCKING(handle, 0);

    char buffer;
    QKD_GET_KEY(handle, &buffer);
}