#include <list>

#include "common.h"

class TcpWorker;
class Payload;

////////////////////////////////////////////////////////////////////////////////
// Collection of Payloads
////////////////////////////////////////////////////////////////////////////////
class PayloadList {
private:
    TcpWorker &worker;
    std::list<Payload*> payloads;
public:
    PayloadList(TcpWorker &worker, std::list<PayloadParams*> &paramslist);
    virtual ~PayloadList();

    char* peek(size_t maxlen, size_t &len);
    size_t advance(size_t len);
};

////////////////////////////////////////////////////////////////////////////////
// Base class Payload
////////////////////////////////////////////////////////////////////////////////
class Payload {
public:
    TcpWorker &worker;
    PayloadParams &params;

    Payload(TcpWorker &worker, PayloadParams &params);
    virtual ~Payload();

    static Payload* maker(TcpWorker &worker, PayloadParams &params);

    virtual char* peek(size_t maxlen, size_t &len) = 0;
    virtual size_t advance(size_t len) = 0;
};

////////////////////////////////////////////////////////////////////////////////
// Random data payload
////////////////////////////////////////////////////////////////////////////////
class PayloadRandom : public Payload {
public:
    PayloadRandomParams &params;

    size_t remaining;

    PayloadRandom(TcpWorker &worker, PayloadRandomParams &params);
    virtual ~PayloadRandom();

    virtual char* peek(size_t maxlen, size_t &len);
    virtual size_t advance(size_t len);
};

////////////////////////////////////////////////////////////////////////////////
// File based payload
////////////////////////////////////////////////////////////////////////////////
class PayloadFile : public Payload {
public:
    PayloadFileParams &params;

    char *payload_ptr;
    size_t remaining;

    PayloadFile(TcpWorker &worker, PayloadFileParams &params);
    virtual ~PayloadFile();

    virtual char* peek(size_t maxlen, size_t &len);
    virtual size_t advance(size_t len);
};
