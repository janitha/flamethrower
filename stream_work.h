#ifndef STREAM_WORK_H
#define STREAM_WORK_H

#include "common.h"
#include "tcp_worker.h"

// TODO(Janitha): Make this an enum inside StreamWork
#define STREAMWORK_SHUTDOWN  2
#define STREAMWORK_FINISHED  1
#define STREAMWORK_CONTINUE  0
#define STREAMWORK_ERROR    -1

////////////////////////////////////////////////////////////////////////////////
// Base class for Work
////////////////////////////////////////////////////////////////////////////////
class StreamWork {
public:
    StreamWorkParams &params;
    int sock;

    StreamWork(StreamWorkParams &params);
    virtual ~StreamWork();
    virtual int handler(char *recvbuf, size_t recvlen,
                        char *sendbuf, size_t sendlen, size_t &sentlen);
    virtual int read_handler(char *recvbuf, size_t recvlen);
    virtual int write_handler(char *sendbuf, size_t sendlen, size_t &sentlen);

    static StreamWork* make(StreamWorkParams &params);
};


////////////////////////////////////////////////////////////////////////////////
// Echo work
////////////////////////////////////////////////////////////////////////////////
class EchoStreamWork : public StreamWork {
public:
    StreamWorkEchoParams &params;

    EchoStreamWork(StreamWorkEchoParams &params);
    virtual ~EchoStreamWork();
    virtual int handler(char *recvbuf, size_t recvlen,
                        char *sendbuf, size_t sendlen, size_t &sentlen);
};


////////////////////////////////////////////////////////////////////////////////
// Random sender and echo work
////////////////////////////////////////////////////////////////////////////////
class RandomStreamWork : public StreamWork {
public:
    StreamWorkRandomParams &params;
    uint32_t bytes_remaining;

    RandomStreamWork(StreamWorkRandomParams &params);
    virtual ~RandomStreamWork();
    virtual int write_handler(char *sendbuf, size_t sendlen, size_t &sentlen);
};


////////////////////////////////////////////////////////////////////////////////
// HTTP client work
////////////////////////////////////////////////////////////////////////////////
class HttpClientStreamWork : public StreamWork {
public:
    StreamWorkHttpClientParams &params;

    HttpClientStreamWork(StreamWorkHttpClientParams &params);
    virtual ~HttpClientStreamWork();
    virtual int write_handler(char *sendbuf, size_t sendlen, size_t &sentlen);
};

////////////////////////////////////////////////////////////////////////////////
// HTTP server work
////////////////////////////////////////////////////////////////////////////////
class HttpServerStreamWork : public StreamWork {
public:
    StreamWorkHttpServerParams &params;

    const char *res_header_ptr;
    uint32_t res_header_remaining;

    const char *res_body_ptr;
    uint32_t res_body_remaining;

    enum class ServerState {
        REQUEST_HEADER,
        REQUEST_BODY,
        REQUEST_DONE,
        RESPONSE_HEADER,
        RESPONSE_BODY,
        RESPONSE_DONE,
        SERVER_DONE
    } state;

    HttpServerStreamWork(StreamWorkHttpServerParams &params);
    virtual ~HttpServerStreamWork();
    virtual int handler(char *recvbuf, size_t recvlen,
                        char *sendbuf, size_t sendlen, size_t &sentlen);
};


#endif
