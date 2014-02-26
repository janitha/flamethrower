#include "http_worker.h"

////////////////////////////////////////////////////////////////////////////////
HttpWorker::HttpWorker()
    : http_state(HttpState::START),
      firstline_crlf_mh("\r\n", 2),
      header_crlfcrlf_mh("\r\n\r\n", 4) {

    // TODO(Janitha): BROKEN WORK IN PROGRESS

}

HttpWorker::~HttpWorker() {

}

HttpWorker::HttpState HttpWorker::consume(char *recvbuf, size_t recvlen, size_t &remlen) {
    debug_print("called\n");

    // TODO(Janitha): BROKEN WORK IN PROGRESS

    remlen = recvlen;

    switch(http_state) {
    case HttpState::START:

        http_state = HttpState::FIRSTLINE;
        firstline_state = FirstlineState::START;

    case HttpState::FIRSTLINE:
        consume_firstline(recvbuf, recvlen, remlen);
        if(!remlen) break;

        recvbuf += recvlen - remlen;
        recvlen  = remlen;
        http_state = HttpState::HEADERS;
        header_state = HeaderState::START;

    case HttpState::HEADERS:
        consume_header(recvbuf, recvlen, remlen);
        if(!remlen) break;

        recvbuf += recvlen - remlen;
        recvlen  = remlen;
        http_state = HttpState::BODY;
        body_state = BodyState::START;

    case HttpState::BODY:
        consume_body(recvbuf, recvlen, remlen);
        if(!remlen) break;

        recvbuf += recvlen - remlen;
        recvlen  = remlen;
        http_state= HttpState::DONE;

    case HttpState::DONE:
    default:
        break;
    }

    if(remlen) {
        perror("error: unconsumed data in http");
        exit(EXIT_FAILURE);
    }

    return http_state;

}

void HttpWorker::consume_firstline(char *recvbuf, size_t recvlen, size_t &remlen) {
    debug_print("called\n");

    // TODO(Janitha): BROKEN WORK IN PROGRESS

    remlen = recvlen;
    char *end;

    switch(firstline_state) {
    case FirstlineState::START:
        firstline_state = FirstlineState::DATA;

    case FirstlineState::DATA:
        end  = firstline_crlf_mh.findend(recvbuf, recvlen);
        if(!end) { 
            remlen = 0;
            break;
        }

        remlen = end - recvbuf;
        recvbuf = end;
        recvlen = remlen;        
        firstline_state = FirstlineState::DONE;

    case FirstlineState::DONE:
    default:
        break;
    }

}

void HttpWorker::consume_header(char *recvbuf, size_t recvlen, size_t &remlen) {
    debug_print("called\n");

    // TODO(Janitha): BROKEN WORK IN PROGRESS

    remlen = recvlen;
    char *end;

    switch(header_state) {
    case HeaderState::START:
        header_state = HeaderState::DATA;

    case HeaderState::DATA:
        end = header_crlfcrlf_mh.findend(recvbuf, recvlen);
        if(!end) {
            remlen = 0;
            break;
        }

        remlen = end - recvbuf;
        recvbuf = end;
        recvlen = remlen;
        header_state = HeaderState::DONE;
        
    case HeaderState::DONE:
    default:
        break;
    }
}

void HttpWorker::consume_body(char *recvbuf, size_t recvlen, size_t &remlen) {
    debug_print("called\n");

    // TODO(Janitha): BROKEN WORK IN PROGRESS

    remlen = recvlen;
    
    switch(body_state) {
    case BodyState::START:
        body_state = BodyState::DATA;

    case BodyState::DATA:
        // TODO(Janitha): What do?
        remlen = 0;
        break;

    case BodyState::DONE:
    default:
        break;

    }
    
}

////////////////////////////////////////////////////////////////////////////////
HttpServerWorker::HttpServerWorker()
    : HttpWorker() {
}

HttpServerWorker::~HttpServerWorker() {
}
