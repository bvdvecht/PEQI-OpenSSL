#include "qkd_api.h"

int main()
{
    destination_t dest;
    dest.port = 8006;
    
    qos_t qos;
    key_handle_t handle;
    QKD_OPEN(dest, qos, handle);

    char buffer;
    QKD_GET_KEY(handle, &buffer);
}