#include <cstdlib>

#include "stream_work.h"



////////////////////////////////////////////////////////////////////////////////
StreamWork::StreamWork(StreamWorkParams &params)
    : params(params) {
}

StreamWork::~StreamWork() {
}

StreamWork* StreamWork::make(StreamWorkParams &params) {

    switch(params.type) {
    case StreamWorkParams::StreamWorkType::ECHO:
        return new EchoStreamWork((StreamWorkEchoParams&)params);
        break;
    case StreamWorkParams::StreamWorkType::RANDOM:
        return new RandomStreamWork((StreamWorkRandomParams&)params);
        break;
    case StreamWorkParams::StreamWorkType::HTTP_CLIENT:
        return new HttpClientStreamWork((StreamWorkHttpClientParams&)params);
        break;
    case StreamWorkParams::StreamWorkType::HTTP_SERVER:
        return new HttpServerStreamWork((StreamWorkHttpServerParams&)params);
        break;
    default:
        perror("invalid streamwork type\n");
        exit(EXIT_FAILURE);
    }
    return nullptr;
}

int StreamWork::handler(char *recvbuf, size_t recvlen,
                        char *sendbuf, size_t sendlen, size_t &sentlen) {
    int rret = STREAMWORK_CONTINUE;
    int wret = STREAMWORK_CONTINUE;

    if(recvbuf && (rret = read_handler(recvbuf, recvlen)) < 0) {
        perror("read handler error");
        exit(EXIT_FAILURE);
    }
    if(sendbuf && (wret = write_handler(sendbuf, sendlen, sentlen)) < 0) {
        perror("write handler error");
        exit(EXIT_FAILURE);
    }

    if(recvbuf) {
        return rret;
    } else {
        // TODO(Janitha): Does this even make sense?
        return std::max<int>(rret, wret);
    }

}

int StreamWork::read_handler(char *recvbuf, size_t recvlen) {
    debug_print("dumb:%s", recvbuf);
    return STREAMWORK_CONTINUE;
}

int StreamWork::write_handler(char *sendbuf, size_t sendlen, size_t &sentlen) {
    sentlen = 0;
    return STREAMWORK_FINISHED;
}

////////////////////////////////////////////////////////////////////////////////
EchoStreamWork::EchoStreamWork(StreamWorkEchoParams &params)
    : StreamWork(params),
      params(params) {
}


EchoStreamWork::~EchoStreamWork() {
}


int EchoStreamWork::handler(char *recvbuf, size_t recvlen,
                            char *sendbuf, size_t sendlen, size_t &sentlen) {

    if(recvbuf && sendbuf) {
        debug_print("recvbuf:%s\n", recvbuf);
    } else {
        return STREAMWORK_FINISHED;
    }

    if(recvlen > sendlen) {
        perror("recvlen>sendlen, queueing up send data isn't supported yet");
        exit(EXIT_FAILURE);
    }

    memcpy(sendbuf, recvbuf, recvlen);
    sentlen = recvlen;

    // TODO(Janitha): Should loop over send to make sure everything was sent
    // TODO(Janitha): Handle non blocking send
    // TODO(Janitha): ARRGGG.. the send should be here
    //sentsz = send(sock, recvbuf, recvlen, 0);

    return STREAMWORK_CONTINUE;
}

////////////////////////////////////////////////////////////////////////////////
RandomStreamWork::RandomStreamWork(StreamWorkRandomParams &params)
    : StreamWork(params),
      params(params),
      bytes_remaining(params.bytes) {
}


RandomStreamWork::~RandomStreamWork() {
}


int RandomStreamWork::write_handler(char *sendbuf, size_t sendlen, size_t &sentlen) {

    sentlen = (sendlen <= bytes_remaining) ? sendlen : bytes_remaining;
    bytes_remaining -= sentlen;

    for(size_t i=0; i<sentlen; i++) {
        // Randomize printable ascii
        sendbuf[i] = '!' + (std::rand()%93);
    }

    if(!bytes_remaining) {
        if(params.shutdown) {
            return STREAMWORK_SHUTDOWN;
        } else {
            return STREAMWORK_FINISHED;
        }
    }
    return STREAMWORK_CONTINUE;
}

////////////////////////////////////////////////////////////////////////////////
HttpClientStreamWork::HttpClientStreamWork(StreamWorkHttpClientParams &params)
    : StreamWork(params),
      params(params) {
}


HttpClientStreamWork::~HttpClientStreamWork() {
}


int HttpClientStreamWork::write_handler(char *sendbuf, size_t sendlen, size_t &sentlen) {

    debug_print("httpclient");

    debug_print("BROKEN\n");
    sentlen = 0;

    /*
    const char mybuf[] = "GET / HTTP/1.1\r\nSome-Header: Herp Derp \r\n\r\n";

    if(sizeof(mybuf) <= sendlen) {
        memcpy(sendbuf, mybuf, sizeof(mybuf));
    } else {
        // TODO(Janitha): Handle this gracefully by sending little by little
        perror("Trickling data not implemented, this is a critical TODO\n");
        exit(EXIT_FAILURE);
    }
    */

    return STREAMWORK_SHUTDOWN;
}


////////////////////////////////////////////////////////////////////////////////
HttpServerStreamWork::HttpServerStreamWork(StreamWorkHttpServerParams &params)
    : StreamWork(params),
      params(params),
      state(ServerState::REQUEST_HEADER) {

    static const char* const default_header = "HTTP/1.1 200 OK\r\nServer: Firehose\r\n\r\n";
    static const char* const default_body =
        "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod "
        "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
        "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
        "commodo consequat. Duis aute irure dolor in reprehenderit in voluptate "
        "velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat "
        "cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id "
        "est laborum.";

    res_header_ptr = default_header;
    res_header_remaining = sizeof(default_header);

    res_body_ptr = default_body;
    res_body_remaining = sizeof(default_body);

}

HttpServerStreamWork::~HttpServerStreamWork() {
}

int HttpServerStreamWork::handler(char *recvbuf, size_t recvlen,
                                  char *sendbuf, size_t sendlen, size_t &sentlen) {
    sentlen = 0;
    size_t dosize = 0;

    switch(state) {

    case ServerState::REQUEST_HEADER:

        if(recvbuf && recvlen) {
            debug_print("request_header: buf=%zu bytes\n", recvlen);
        } else {
            break;
        }

        // TODO(Janitha): For lazy/pragmetic/perf reasons, lets just "assume"
        //                that the header termination "0D0A0D0A" isn't cross bufs
        static const char header_terminator[] = { 0x0D, 0x0A, 0x0D, 0x0A };

        char *header_end;
        if((header_end = strstr(recvbuf, header_terminator)) == nullptr) {
            // Header teminator not found
            break;
        }

        header_end += sizeof(header_terminator);

        // TODO(Janitha): Do something with the header

        recvlen -= header_end - recvbuf;
        recvbuf = header_end;

        state = ServerState::REQUEST_BODY;

    case ServerState::REQUEST_BODY:

        // TODO(Janitha): Implement handling request body, RFC 2616 4.4
        state = ServerState::REQUEST_DONE;

    case ServerState::REQUEST_DONE:

        state = ServerState::RESPONSE_HEADER;

    case ServerState::RESPONSE_HEADER:

        if(sendbuf && sendlen) {
            debug_print("response_header\n");
        } else {
            break;
        }

        dosize = (sendlen <= res_header_remaining) ? sendlen : res_header_remaining;

        memcpy(sendbuf, res_header_ptr, dosize);
        sendbuf += dosize;
        sendlen -= dosize;
        sentlen += dosize;

        res_header_ptr += dosize;
        res_header_remaining -= dosize;

        if(res_header_remaining) {
            break;
        }

        state = ServerState::RESPONSE_BODY;

    case ServerState::RESPONSE_BODY:

        if(sendbuf && sendlen) {
            debug_print("response_body\n");
        } else {
            break;
        }

        dosize = (sendlen <= res_body_remaining) ? sendlen : res_body_remaining;

        memcpy(sendbuf, res_body_ptr, dosize);
        sendbuf += dosize;
        sendlen -= dosize;
        sentlen += dosize;

        res_body_ptr += dosize;
        res_body_remaining -= dosize;

        if(res_header_remaining) {
            break;
        }

        state = ServerState::RESPONSE_BODY;

    case ServerState::RESPONSE_DONE:

        state = ServerState::SERVER_DONE;

    case ServerState::SERVER_DONE:
    default:
        return STREAMWORK_SHUTDOWN;
    }

    return STREAMWORK_CONTINUE;

}
