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
    StreamWorkParams &params;
public:
    int sock;
    StreamWork(StreamWorkParams &params);
    virtual ~StreamWork();
    virtual int handler(char *recvbuf, size_t recvlen,
                        char *sendbuf, size_t &sendlen);
    virtual int read_handler(char *recvbuf, size_t recvlen);
    virtual int write_handler(char *sendbuf, size_t &sendlen);

    static StreamWork* make(StreamWorkParams &params);
};


////////////////////////////////////////////////////////////////////////////////
// Echo work
////////////////////////////////////////////////////////////////////////////////
class EchoStreamWork : public StreamWork {
    StreamWorkEchoParams &params;
public:
    EchoStreamWork(StreamWorkEchoParams &params);
    virtual ~EchoStreamWork();
    virtual int handler(char *recvbuf, size_t recvlen,
                        char *sendbuf, size_t &sendlen);
};


////////////////////////////////////////////////////////////////////////////////
// Random sender and echo work
////////////////////////////////////////////////////////////////////////////////
class RandomStreamWork : public StreamWork {
    StreamWorkRandomParams &params;
    uint32_t bytes_remaining;
public:
    RandomStreamWork(StreamWorkRandomParams &params);
    virtual ~RandomStreamWork();
    virtual int write_handler(char *sendbuf, size_t &sendlen);
};


////////////////////////////////////////////////////////////////////////////////
// HTTP client work
////////////////////////////////////////////////////////////////////////////////
class HttpClientStreamWork : public StreamWork {
    StreamWorkHttpClientParams &params;
public:
    HttpClientStreamWork(StreamWorkHttpClientParams &params);
    virtual ~HttpClientStreamWork();
    virtual int write_handler(char *sendbuf, size_t &sendlen);
};


#endif
