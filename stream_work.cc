#include <cstdlib>

#include "stream_work.h"

StreamWork* StreamWorkMaker::make(tcp_worker_params_t *params,
                                  int sock) {

    StreamWork* work;

    switch(params->stream_work_type) {
    case tcp_worker_params_t::ECHO:
        work = new EchoStreamWork(&params->echo_work,
                                  sock);
        break;
    case tcp_worker_params_t::RANDOM:
        work = new RandomStreamWork(&params->random_work,
                                    sock);
        break;
    case tcp_worker_params_t::HTTP_CLIENT:
        work = new HttpClientStreamWork(&params->httpclient_work,
                                        sock);
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

int StreamWork::read_handler(char *recvbuf, ssize_t recvlen) {
    debug_print("dumb:%s", recvbuf);
    return recvlen;
}


int StreamWork::write_handler(char *sendbuf, ssize_t &sendlen) {
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


int EchoStreamWork::read_handler(char *recvbuf, ssize_t recvlen) {
    debug_print("echo:%s", recvbuf);

    ssize_t sentsz;

    // TODO(Janitha): Should loop over send to make sure everything was sent
    // TODO(Janitha): Handle non blocking send
    sentsz = send(sock, recvbuf, recvlen, 0);

    return STREAMWORK_CONTINUE;
}

////////////////////////////////////////////////////////////////////////////////
RandomStreamWork::RandomStreamWork(stream_work_random_params_t *params,
                                   int sock)
    : EchoStreamWork(params, sock),
      params(params) {
}


RandomStreamWork::~RandomStreamWork() {
}


int RandomStreamWork::write_handler(char *sendbuf, ssize_t &sendlen) {

    for(int i=0; i<sendlen; i++) {
        // Randomize just printable ascii chars
        sendbuf[i] = '!' + (std::rand()%93);
    }

    return STREAMWORK_FINISHED;
}

////////////////////////////////////////////////////////////////////////////////
HttpClientStreamWork::HttpClientStreamWork(stream_work_httpclient_params_t *params,
                                           int sock)
    : StreamWork(params, sock),
      params(params) {
}


HttpClientStreamWork::~HttpClientStreamWork() {
}


int HttpClientStreamWork::write_handler(char *sendbuf, ssize_t &sendlen) {

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
