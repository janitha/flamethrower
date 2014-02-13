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
    default:
        perror("invalid streamwork type\n");
        exit(EXIT_FAILURE);
    }
    return nullptr;
}

int StreamWork::handler(char *recvbuf, size_t recvlen,
                        char *sendbuf, size_t &sendlen) {
    int rret, wret;

    if(recvbuf) {
        if((rret = read_handler(recvbuf, recvlen)) < 0) {
            perror("read handler error");
            exit(EXIT_FAILURE);
        }
    }
    if(sendbuf) {
        if((wret = write_handler(sendbuf, sendlen)) < 0) {
            perror("write handler error");
            exit(EXIT_FAILURE);
        }
    }

    // TODO(Janitha): Is this the returning the highest code right?
    return std::max<int>(rret, wret);

}

int StreamWork::read_handler(char *recvbuf, size_t recvlen) {
    debug_print("dumb:%s", recvbuf);
    return STREAMWORK_FINISHED;
}

int StreamWork::write_handler(char *sendbuf, size_t &sendlen) {
    sendlen = 0;
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
                            char *sendbuf, size_t &sendlen) {

    debug_print("echo:%s", recvbuf);

    if(recvlen > sendlen) {
        perror("recvlen>sendlen, queueing up send data isn't supported yet");
        exit(EXIT_FAILURE);
    }

    memcpy(sendbuf, recvbuf, recvlen);
    sendlen = recvlen;

    // TODO(Janitha): Should loop over send to make sure everything was sent
    // TODO(Janitha): Handle non blocking send
    // TODO(Janitha): ARRGGG.. the send should be here
    //sentsz = send(sock, recvbuf, recvlen, 0);

    if(!recvbuf && sendbuf) {
        // Arrived from write_cb, so bug out
        return STREAMWORK_FINISHED;
    }

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


int RandomStreamWork::write_handler(char *sendbuf, size_t &sendlen) {

    if(bytes_remaining <= sendlen) {
        sendlen = bytes_remaining;
    }
    bytes_remaining -= sendlen;

    for(size_t i=0; i<sendlen; i++) {
        // Randomize just printable ascii chars
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


int HttpClientStreamWork::write_handler(char *sendbuf, size_t &sendlen) {

    char mybuf[] = "GET / HTTP/1.1\r\nSome-Header: Herp Derp\r\n\r\n";

    if(sizeof(mybuf) <= sendlen) {
        memcpy(sendbuf, mybuf, sizeof(mybuf));
    } else {
        // TODO(Janitha): Handle this gracefully by sending little by little
        perror("Trickling data not implemented, this is a critical TODO\n");
        exit(EXIT_FAILURE);
    }

    return STREAMWORK_FINISHED;
}
