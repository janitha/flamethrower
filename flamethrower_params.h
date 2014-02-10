#ifndef FLAMETHROWER_PARAMS_H
#define FLAMETHROWER_PARAMS_H

// TODO(Janitha): Move the corresponding settings to their respective

////////////////////////////////////////////////////////////////////////////////
typedef struct stream_work_params_t {
} stream_work_params_t;

typedef struct stream_work_echo_params_t : public stream_work_params_t {
} stream_work_echo_params_t;

typedef struct stream_work_random_params_t : public stream_work_echo_params_t {
} stream_work_random_params_t;

typedef struct stream_work_httpclient_params_t : public stream_work_params_t {
} stream_work_httpclient_params_t;

////////////////////////////////////////////////////////////////////////////////
typedef struct tcp_worker_params_t {
    enum {
        ECHO,
        RANDOM,
        HTTP_CLIENT,
    } stream_work_type;
    union {
        stream_work_echo_params_t echo_work;
        stream_work_random_params_t random_work;
        stream_work_httpclient_params_t httpclient_work;
    };
} tcp_worker_params_t;

typedef struct tcp_server_worker_params_t : public tcp_worker_params_t {
} tcp_server_worker_params_t;

typedef struct tcp_client_worker_params_t : public tcp_worker_params_t {
    uint32_t bind_addr;       // htonl(INADDRY_ANY)
    uint16_t bind_port;       // htons(0)
    uint32_t server_addr;     // htonl or inet_addr("1.2.3.4")
    uint16_t server_port;     // htons(12345)
    float    timeout;         // 5.0
} tcp_client_worker_params_t;

////////////////////////////////////////////////////////////////////////////////
typedef struct tcp_factory_params_t{
} tcp_factory_params_t;

typedef struct tcp_server_factory_params_t : tcp_factory_params_t {
    uint32_t bind_addr;       // htonl(INADDR_ANY)
    uint16_t bind_port;       // htons(9999)
    uint32_t accept_backlog;  // 5
    tcp_server_worker_params_t server_worker;
} tcp_server_factory_params_t;

typedef struct tcp_client_factory_params_t : tcp_factory_params_t {
    tcp_client_worker_params_t client_worker;
} tcp_client_factory_params_t;

////////////////////////////////////////////////////////////////////////////////
typedef struct factory_params_t {
    enum {
        TCP_SERVER,
        TCP_CLIENT,
    } factory_type;
    union {
        tcp_server_factory_params_t tcp_server;
        tcp_client_factory_params_t tcp_client;
    };
} factory_params_t;

////////////////////////////////////////////////////////////////////////////////
typedef struct {
    uint32_t version;
    factory_params_t factory;
} flamethrower_params_t;

////////////////////////////////////////////////////////////////////////////////
flamethrower_params_t* flamethrower_params_from_file(char* filename);

#endif
