#include <cstdlib>

#include "stream_work.h"


StreamWork::StreamWork(int sock)
    : sock(sock) {

}


StreamWork::~StreamWork() {
}



int StreamWork::read_handler(char *recvbuf, ssize_t recvlen) {
    printf("dumb:%s", recvbuf);
    return recvlen;
}


int StreamWork::write_handler(char *sendbuf, ssize_t &sendlen) {
    sendlen = 0;
    return STREAMWORK_FINISHED;
}


EchoStreamWork::EchoStreamWork(int sock)
        : StreamWork(sock) {
}


EchoStreamWork::~EchoStreamWork() {
}


int EchoStreamWork::read_handler(char *recvbuf, ssize_t recvlen) {
    printf("echo:%s", recvbuf);

    ssize_t sentsz;

    // TODO(Janitha): Should loop over send to make sure everything was sent
    // TODO(Janitha): Handle non blocking send
    sentsz = send(sock, recvbuf, recvlen, 0);

    return STREAMWORK_CONTINUE;
}


RandomStreamWork::RandomStreamWork(int sock)
        : EchoStreamWork(sock) {
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


HttpClientStreamWork::HttpClientStreamWork(int sock)
    : StreamWork(sock) {
}


HttpClientStreamWork::~HttpClientStreamWork() {
}


int HttpClientStreamWork::write_handler(char *sendbuf, ssize_t &sendlen) {

    char mybuf[] = "GET / HTTP/1.1\r\nSome-Header: Herp Derp\r\n\r\n";

    if(sizeof(mybuf) <= sendlen) {
        memcpy(sendbuf, mybuf, sizeof(mybuf));
    } else {
        // TODO(Janitha): Handle this gracefully by sending little by little
        printf("Trickling data not implemented, this is a critical TODO\n");
        exit(EXIT_FAILURE);
    }

    return STREAMWORK_FINISHED;
}
