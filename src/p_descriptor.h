#include "prims.h"

void prims_concount    (__P_PROTO);
void prims_connections (__P_PROTO);
void prims_condbref    (__P_PROTO);
void prims_conidle     (__P_PROTO);
void prims_contime     (__P_PROTO);
void prims_conhost     (__P_PROTO);
void prims_conboot     (__P_PROTO);
void prims_connotify   (__P_PROTO);
void prims_connected   (__P_PROTO);
void prims_awakep      (__P_PROTO);
void prims_conlast     (__P_PROTO);
void prims_pconnectors (__P_PROTO);
void prims_socket      (__P_PROTO);
void prims_connect     (__P_PROTO);
void prims_close       (__P_PROTO);
void prims_ready_write (__P_PROTO);
void prims_ready_read  (__P_PROTO);
void prims_socket_read (__P_PROTO);
void prims_socket_write        (__P_PROTO);
void prims_socket_last         (__P_PROTO);
void prims_socket_connected_at (__P_PROTO);
void prims_socket_connected    (__P_PROTO);
void prims_socket_host         (__P_PROTO);
void prims_socket_fgets        (__P_PROTO);
void prims_is_socket           (__P_PROTO);

#define PRIMS_DESCRIPTOR_FL prims_concount, prims_connections, prims_condbref, \
        prims_conidle, prims_contime, prims_conhost, prims_conboot, \
        prims_connotify, prims_connected, prims_awakep, prims_conlast, \
        prims_pconnectors, prims_socket, prims_connect, prims_close,\
        prims_ready_write, prims_ready_read, prims_socket_read, \
        prims_socket_write, prims_socket_last, prims_socket_connected_at, \
        prims_socket_connected, prims_socket_host, prims_socket_fgets, \
        prims_is_socket

#define PRIMS_DESCRIPTOR_TL "CONCOUNT", "CONNECTIONS", "CONDBREF", "CONIDLE", \
        "CONTIME", "CONHOST", "CONBOOT", "CONNOTIFY", "CONNECTED?", "AWAKE?", \
        "CONLAST", "CONNECTORS", "SOCKET", "CONNECT", "CLOSE", "READY_WRITE", \
        "READY_READ", "SOCKET_READ", "SOCKET_WRITE", "SOCKET_LAST", \
        "SOCKET_CONNECTED_AT", "SOCKET_CONNECTED", "SOCKET_HOST", \
        "SOCKET_FGETS", "SOCKET?"

#define PRIMS_DESCRIPTOR_LEN 25
