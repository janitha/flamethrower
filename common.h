#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <map>
#include <queue>
#include <utility>
#include <algorithm>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <ev.h>

#include "flamethrower_params.h"

#define DEBUG 1

#define debug_print(fmt, ...)                                           \
    do{ if (DEBUG) {                                                    \
        fprintf(stderr, "%s:%d:%s: " fmt,                               \
                __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);       \
    }} while (0)

#define debug_pprint(fmt, ...)                                          \
    do { if (DEBUG) {                                                   \
        fprintf(stderr, "%s:%d:%s: " fmt,                               \
                __FILE__, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__);\
    }} while (0)


#define debug_socket_print(sd, fmt, ...)                                \
    do { if (DEBUG) {                                                   \
        struct sockaddr_in sa_from, sa_to;                              \
        socklen_t saf_len = sizeof(sa_from);                            \
        socklen_t sat_len = sizeof(sa_to);                              \
        getsockname(sd, (sockaddr*)&sa_from, &saf_len);                 \
        getpeername(sd, (sockaddr*)&sa_to,   &sat_len);                 \
        fprintf(stderr, "%s:%d:%s:[%s:%d->%s:%d]: " fmt,                \
                __FILE__, __LINE__, __FUNCTION__,                       \
                inet_ntoa(sa_from.sin_addr), ntohs(sa_from.sin_port),   \
                inet_ntoa(sa_to.sin_addr),   ntohs(sa_to.sin_port),     \
                ##__VA_ARGS__);                                         \
    }} while (0)
