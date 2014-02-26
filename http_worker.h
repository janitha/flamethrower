#ifndef HTTP_WORKER_H
#define HTTP_WORKER_H

#include <iostream>

#include "common.h"
#include "memhunter.h"



class HttpWorker {
    
    // TODO(Janitha): BROKEN WORK IN PROGRESS
    
private:
public:
    
    enum class HttpState {
        START,
        FIRSTLINE,
        HEADERS,
        BODY,
        DONE
    } http_state;

    enum class FirstlineState {
        START,
        DATA,
        DONE
    } firstline_state;

    MemHunter firstline_crlf_mh;
    
    enum class HeaderState {
        START,
        DATA,
        DONE
    } header_state;

    MemHunter header_crlfcrlf_mh;

    enum class BodyState {
        START,
        DATA,
        DONE
    } body_state;

    HttpWorker();
    virtual ~HttpWorker();

    virtual HttpState consume(char *recvbuf, size_t recvlen, size_t &remlen) = 0;    
    virtual void consume_firstline(char *recvbuf, size_t recvlen, size_t &remlen) = 0;
    virtual void consume_header(char *recvbuf, size_t recvlen, size_t &remlen) = 0;
    virtual void consume_body(char *recvbuf, size_t recvlen, size_t &remlen) = 0;
    
};


class HttpServerWorker : public HttpWorker  {

    HttpServerWorker();
    virtual ~HttpServerWorker();
    
};


#endif
