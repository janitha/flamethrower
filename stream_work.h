#ifndef STREAM_WORK_H
#define STREAM_WORK_H

#include "common.h"

#define STREAMWORK_SHUTDOWN  2
#define STREAMWORK_FINISHED  1
#define STREAMWORK_CONTINUE  0
#define STREAMWORK_ERROR    -1


////////////////////////////////////////////////////////////////////////////////
// Base class for Work
////////////////////////////////////////////////////////////////////////////////
class StreamWork {
public:
    int sock;
    StreamWork(int sock);
    virtual ~StreamWork();
    virtual int read_handler(char *recvbuf, ssize_t recvlen);
    virtual int write_handler(char *sendbuf, ssize_t &sendlen);
};


////////////////////////////////////////////////////////////////////////////////
// Echo work
////////////////////////////////////////////////////////////////////////////////
class EchoStreamWork : public StreamWork {
public:
    EchoStreamWork(int sock);
    virtual ~EchoStreamWork();
    virtual int read_handler(char *recvbuf, ssize_t recvlen);
};


////////////////////////////////////////////////////////////////////////////////
// Random sender and echo work
////////////////////////////////////////////////////////////////////////////////
class RandomStreamWork : public EchoStreamWork {
public:
    RandomStreamWork(int sock);
    virtual ~RandomStreamWork();
    virtual int write_handler(char *sendbuf, ssize_t &sendlen);
};


////////////////////////////////////////////////////////////////////////////////
// HTTP client work
////////////////////////////////////////////////////////////////////////////////
class HttpClientStreamWork : public StreamWork {
public:
    HttpClientStreamWork(int sock);
    virtual ~HttpClientStreamWork();
    virtual int write_handler(char *sendbuf, ssize_t &sendlen);
};


#endif
