#ifndef STREAM_WORK_H
#define STREAM_WORK_H

#include "common.h"
#include "tcp_worker.h"

#define STREAMWORK_SHUTDOWN  2
#define STREAMWORK_FINISHED  1
#define STREAMWORK_CONTINUE  0
#define STREAMWORK_ERROR    -1

class StreamWork;

////////////////////////////////////////////////////////////////////////////////
// Creates stream work classes
////////////////////////////////////////////////////////////////////////////////
class StreamWorkMaker {
public:
    static StreamWork* make(tcp_worker_params_t *params,
                            int sock);
};

////////////////////////////////////////////////////////////////////////////////
// Base class for Work
////////////////////////////////////////////////////////////////////////////////
class StreamWork {
    stream_work_params_t *params;
public:
    int sock;
    StreamWork(stream_work_params_t *params,
               int sock);
    virtual ~StreamWork();
    virtual int handler(char *recvbuf, size_t recvlen,
                        char *sendbuf, size_t &sendlen);
    virtual int read_handler(char *recvbuf, size_t recvlen);
    virtual int write_handler(char *sendbuf, size_t &sendlen);
};


////////////////////////////////////////////////////////////////////////////////
// Echo work
////////////////////////////////////////////////////////////////////////////////
class EchoStreamWork : public StreamWork {
    stream_work_echo_params_t *params;
public:
    EchoStreamWork(stream_work_echo_params_t *params,
                   int sock);
    virtual ~EchoStreamWork();
    virtual int handler(char *recvbuf, size_t recvlen,
                        char *sendbuf, size_t &sendlen);
};


////////////////////////////////////////////////////////////////////////////////
// Random sender and echo work
////////////////////////////////////////////////////////////////////////////////
class RandomStreamWork : public StreamWork {
    stream_work_random_params_t *params;
    uint64_t bytes_remaining;
public:
    RandomStreamWork(stream_work_random_params_t *params,
                     int sock);
    virtual ~RandomStreamWork();
    virtual int write_handler(char *sendbuf, size_t &sendlen);
};


////////////////////////////////////////////////////////////////////////////////
// HTTP client work
////////////////////////////////////////////////////////////////////////////////
class HttpClientStreamWork : public StreamWork {
    stream_work_httpclient_params_t *params;
public:
    HttpClientStreamWork(stream_work_httpclient_params_t *params,
                         int sock);
    virtual ~HttpClientStreamWork();
    virtual int write_handler(char *sendbuf, size_t &sendlen);
};


#endif
