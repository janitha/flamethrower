#ifndef PARAMS_H
#define PARAMS_H

#include <string>
#include <list>
#include <map>

#include <boost/property_tree/ptree_fwd.hpp>

////////////////////////////////////////////////////////////////////////////////
// Params base class
////////////////////////////////////////////////////////////////////////////////
struct Params {
    Params(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
// Payloads
////////////////////////////////////////////////////////////////////////////////
struct PayloadParams : Params {

    enum class PayloadType {
        RANDOM,
        STRING,
        FILE,
        HTTP_HEADERS
    } type;

    PayloadParams(boost::property_tree::ptree &ptree);
    static PayloadParams* maker(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct PayloadRandomParams : PayloadParams {

    static const size_t RANDOM_BUF_SIZE = 1024*1024;
    char random_buf[RANDOM_BUF_SIZE];
    size_t payload_len;

    PayloadRandomParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct PayloadBufAbstractParams : PayloadParams {
    char *payload_ptr;
    size_t payload_len;

    PayloadBufAbstractParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct PayloadStringParams : PayloadBufAbstractParams {
    PayloadStringParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct PayloadFileParams : PayloadBufAbstractParams {
    PayloadFileParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct PayloadHttpHeadersParams : PayloadBufAbstractParams {
    PayloadHttpHeadersParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
// Workers
////////////////////////////////////////////////////////////////////////////////
struct TcpWorkerParams : public Params {

    bool initiate_close;
    float delay_close;

    int tcp_linger; // tcp linger as set via SO_LINGER

    TcpWorkerParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct TcpServerWorkerParams : public TcpWorkerParams {

    enum class WorkerType {
        NONE,
        ECHO,
        RAW,
        HTTP
    } type;

    TcpServerWorkerParams(boost::property_tree::ptree &ptree);
    static TcpServerWorkerParams* maker(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct TcpClientWorkerParams : public TcpWorkerParams {

    enum class WorkerType {
        NONE,
        ECHO,
        RAW,
        HTTP
    } type;

    TcpClientWorkerParams(boost::property_tree::ptree &ptree);
    static TcpClientWorkerParams* maker(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct TcpServerEchoParams : public TcpServerWorkerParams {
    TcpServerEchoParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct TcpClientEchoParams : public TcpClientWorkerParams {
    TcpClientEchoParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct TcpServerRawParams : public TcpServerWorkerParams {

    std::list<PayloadParams*> payloads;

    TcpServerRawParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct TcpClientRawParams : public TcpClientWorkerParams {

    std::list<PayloadParams*> payloads;

    TcpClientRawParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct TcpServerHttpParams : public TcpServerWorkerParams {

    std::list<PayloadParams*> firstline_payloads;
    std::list<PayloadParams*> header_payloads;
    std::list<PayloadParams*> body_payloads;

    TcpServerHttpParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct TcpClientHttpParams : public TcpClientWorkerParams {

    std::list<PayloadParams*> firstline_payloads;
    std::list<PayloadParams*> header_payloads;
    std::list<PayloadParams*> body_payloads;

    TcpClientHttpParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
// Factories
////////////////////////////////////////////////////////////////////////////////
struct FactoryParams : public Params {

    enum class FactoryType {
        NONE,
        TCP_SERVER,
        TCP_CLIENT
    } type;

    FactoryParams(boost::property_tree::ptree &ptree);
    static FactoryParams* maker(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct TcpFactoryParams : public FactoryParams {
    uint32_t bind_addr;       // htonl(INADDR_ANY)
    uint16_t bind_port;       // htons(9999)
    uint32_t concurrency;
    uint64_t count;

    TcpFactoryParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct TcpServerFactoryParams : public TcpFactoryParams {
    uint32_t accept_backlog;
    TcpServerWorkerParams *worker;

    TcpServerFactoryParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct TcpClientFactoryParams : public TcpFactoryParams {
    uint32_t server_addr;     // htonl or inet_addr("1.2.3.4")
    uint16_t server_port;     // htons(12345)
    float    connect_timeout; // 5.0
    TcpClientWorkerParams *worker;

    TcpClientFactoryParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
// Stats
////////////////////////////////////////////////////////////////////////////////
struct StatsParams : Params {
    std::string listener;

    StatsParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
// Main flamethrower parameters
////////////////////////////////////////////////////////////////////////////////
struct FlamethrowerParams : public Params{
    uint32_t version;
    StatsParams *stats;
    std::list<FactoryParams*> factories;

    FlamethrowerParams(boost::property_tree::ptree &ptree);
};

#endif
