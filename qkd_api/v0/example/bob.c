#include "qkd_api.h"
#include "../defines.h"

int main()
{
    destination_t dest;
    dest.port = ALICE_PORT;
    
    qos_t qos;
    key_handle_t handle;
    QKD_OPEN(dest, qos, handle);

    char buffer;
    QKD_GET_KEY(handle, &buffer);
}