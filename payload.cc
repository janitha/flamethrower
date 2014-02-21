#include <cstdlib>

#include "payload.h"


////////////////////////////////////////////////////////////////////////////////
PayloadList::PayloadList(TcpWorker &worker, std::list<PayloadParams*> &paramslist)
    : worker(worker) {

    for(auto &payload_params : paramslist) {
        payloads.push_back(Payload::maker(worker, *payload_params));
    }

}

PayloadList::~PayloadList() {

    while(!payloads.empty()) {
        delete payloads.front();
        payloads.pop_front();
    }

}

char* PayloadList::peek(size_t maxlen, size_t &len) {

    while(!payloads.empty()) {

        Payload &payload = *payloads.front();
        char *payload_ptr = payload.peek(maxlen, len);

        if(payload_ptr) {
            return payload_ptr;
        } else {
            delete payloads.front();
            payloads.pop_front();
        }
    }

    len = 0;
    return nullptr;

}

size_t PayloadList::advance(size_t len) {

    if(payloads.empty()) {
        perror("error attempting to advance non existing payloads");
        exit(EXIT_FAILURE);
    }

    while(!payloads.empty()) {

        Payload &cur_payload = *payloads.front();
        size_t remaining = cur_payload.advance(len);

        if(remaining) {
            return remaining;
        } else {
            delete payloads.front();
            payloads.pop_front();
            len = 0;
        }
    };

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
Payload::Payload(TcpWorker &worker, PayloadParams &params)
    : worker(worker),
      params(params) {
}

Payload::~Payload() {
}

Payload* Payload::maker(TcpWorker &worker, PayloadParams &params) {

    switch(params.type) {
    case PayloadParams::PayloadType::RANDOM:
        return new PayloadRandom(worker, (PayloadRandomParams&)params);
        break;
    case PayloadParams::PayloadType::FILE:
        return new PayloadFile(worker, (PayloadFileParams&)params);
        break;
    case PayloadParams::PayloadType::STRING:
        return new PayloadString(worker, (PayloadStringParams&)params);
        break;
    case PayloadParams::PayloadType::HTTP_HEADERS:
        return new PayloadHttpHeaders(worker, (PayloadHttpHeadersParams&)params);
        break;
    default:
        perror("error: invalid payload type\n");
        exit(EXIT_FAILURE);
    };
    return nullptr;

}

////////////////////////////////////////////////////////////////////////////////
PayloadRandom::PayloadRandom(TcpWorker &worker, PayloadRandomParams &params)
    : Payload(worker, params),
      params(params) {

    remaining = params.payload_len;

}

PayloadRandom::~PayloadRandom() {
}

char* PayloadRandom::peek(size_t maxlen, size_t &len) {

    // NOTE: This is cheating way to get ptr to random data very cheaply.
    //       The params already has a buffer filled with RANDOM_BUF_SIZE (1M) bytes,
    //       and this function simply picks a random offset of that and provides
    //       a ptr. This way every call to this doesn't need to regenerat
    //       or reallocate random bytes.

    if(!remaining) {
        return nullptr;
    }

    len = (maxlen < remaining) ? maxlen : remaining;

    if(len > params.RANDOM_BUF_SIZE) {
        perror("Random buffer was not large enough for requested buffer");
        exit(EXIT_FAILURE);
    }

    char *offset = params.random_buf + (std::rand() % (params.RANDOM_BUF_SIZE - len));
    return offset;
}

size_t PayloadRandom::advance(size_t len) {

    if(len > remaining) {
        perror("error: attempt to advance the payload too much");
        exit(EXIT_FAILURE);
    }

    remaining -= len;
    return remaining;
}

////////////////////////////////////////////////////////////////////////////////
PayloadBufAbstract::PayloadBufAbstract(TcpWorker &worker, PayloadBufAbstractParams &params)
    : Payload(worker, params),
      params(params) {

    remaining = params.payload_len;
    payload_ptr = params.payload_ptr;
}

PayloadBufAbstract::~PayloadBufAbstract() {
}

char* PayloadBufAbstract::peek(size_t maxlen, size_t &len) {

    if(!remaining) {
        return nullptr;
    }

    len = (maxlen < remaining) ? maxlen : remaining;
    return payload_ptr;
}

size_t PayloadBufAbstract::advance(size_t len) {

    if(len > remaining) {
        perror("error: attempt to advance the payload too much");
        exit(EXIT_FAILURE);
    }

    remaining -= len;
    payload_ptr += len;

    return remaining;
}
////////////////////////////////////////////////////////////////////////////////
PayloadString::PayloadString(TcpWorker &worker, PayloadStringParams &params)
    : PayloadBufAbstract(worker, params),
      params(params) {
}

PayloadString::~PayloadString() {
}

////////////////////////////////////////////////////////////////////////////////
PayloadFile::PayloadFile(TcpWorker &worker, PayloadFileParams &params)
    : PayloadBufAbstract(worker, params),
      params(params) {
}

PayloadFile::~PayloadFile() {
}

////////////////////////////////////////////////////////////////////////////////
PayloadHttpHeaders::PayloadHttpHeaders(TcpWorker &worker, PayloadHttpHeadersParams &params)
    : PayloadBufAbstract(worker, params),
      params(params) {
}

PayloadHttpHeaders::~PayloadHttpHeaders() {
}
