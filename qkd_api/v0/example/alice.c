#include "qkd_api.h"
#include "../defines.h"


key_handle_t handle = {0};

int main()
{
    destination_t dest;
    dest.port = BOB_PORT;
    
    qos_t qos;
    qos.requested_length = REQUESTED_LENGTH;
    // key_handle_t handle;
    printf("srhlghf\n");
    printf("%d\n", handle[0]);
    QKD_OPEN(dest, qos, handle);
    printf("srhlghf\n");
    printf("%d, %d, %d\n", handle[0], handle[1], handle[2]);
    QKD_CONNECT_BLOCKING(handle, 0);

    char buffer;
    QKD_GET_KEY(handle, &buffer);
}