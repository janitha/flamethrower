#include "common.h"
#include "flamethrower.h"
#include "tcp_factory.h"

int main(int argc, char *argv[]) {

    struct ev_loop *loop = ev_default_loop(0);

    TcpServerFactory *tsf = new TcpServerFactory(loop,
                                                 htonl(INADDR_ANY),
                                                 htons(9999),
                                                 5);

    TcpClientFactory *tcf = new TcpClientFactory(loop,
                                                 htonl(INADDR_LOOPBACK),
                                                 htons(0),
                                                 htonl(INADDR_LOOPBACK),
                                                 htons(5555),
                                                 5);

    printf("Starting event loop\n");

    while(1) {
        ev_loop(loop, 0);
    }

    delete tsf;
    delete tcf;

}
