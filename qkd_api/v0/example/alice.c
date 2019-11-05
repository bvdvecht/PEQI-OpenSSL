#include "qkd_api.h"

int main()
{
    qos_t qos;
    key_handle_t handle;
    QKD_OPEN(8006, qos, handle);

    char buffer;
    QKD_GET_KEY(handle, &buffer);
}