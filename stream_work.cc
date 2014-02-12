#include <cstdlib>

#include "stream_work.h"

StreamWork* StreamWorkMaker::make(tcp_worker_params_t *params,
                                  int sock) {

    StreamWork* work;

    switch(params->stream_work_type) {
    case tcp_worker_params_t::ECHO:
        work = new EchoStreamWork(&params->echo_work, sock);
        break;
    case tcp_worker_params_t::RANDOM:
        work = new RandomStreamWork(&params->random_work, sock);
        break;
    case tcp_worker_params_t::HTTP_CLIENT:
        work = new HttpClientStreamWork(&params->httpclient_work, sock);
        break;
    default:
        perror("invalid stream_work_type\n");
        exit(EXIT_FAILURE);
    }

    return work;

}

////////////////////////////////////////////////////////////////////////////////
StreamWork::StreamWork(stream_work_params_t *params,
                       int sock)
    : params(params),
      sock(sock) {
}

StreamWork::~StreamWork() {
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
EchoStreamWork::EchoStreamWork(stream_work_echo_params_t *params,
                               int sock)
    : StreamWork(params, sock),
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
RandomStreamWork::RandomStreamWork(stream_work_random_params_t *params,
                                   int sock)
    : StreamWork(params, sock),
      params(params),
      bytes_remaining(params->bytes) {
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
        if(params->shutdown) {
            return STREAMWORK_SHUTDOWN;
        } else {
            return STREAMWORK_FINISHED;
        }
    }
    return STREAMWORK_CONTINUE;
}

////////////////////////////////////////////////////////////////////////////////
HttpClientStreamWork::HttpClientStreamWork(stream_work_httpclient_params_t *params,
                                           int sock)
    : StreamWork(params, sock),
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
