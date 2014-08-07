#include <stdio.h>
#include <stdlib.h>
#include <libvirt/libvirt.h>
virConnectPtr Openvirt_connect(void)
{
    virConnectPtr conn;
    conn = virConnectOpen("qemu:///system");
    if (conn == NULL) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
        perror("virConnectOpen");
        exit -1;
    }
//    virConnectClose(conn);
    return conn;
}
