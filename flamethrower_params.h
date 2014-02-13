#ifndef FLAMETHROWER_PARAMS_H
#define FLAMETHROWER_PARAMS_H

#include <string>
#include <list>
#include <map>

#include <boost/property_tree/ptree_fwd.hpp>

// TODO(Janitha): Move the corresponding settings to their respective

struct Params {
    Params(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct StreamWorkParams : public Params {

    enum class StreamWorkType {
        NONE,
        ECHO,
        RANDOM,
        HTTP_CLIENT,
        HTTP_SERVER
    } type;

    StreamWorkParams(boost::property_tree::ptree &ptree);
    static StreamWorkParams* maker(boost::property_tree::ptree &ptree);
};

struct StreamWorkEchoParams : public StreamWorkParams {
    StreamWorkEchoParams(boost::property_tree::ptree &ptree);
};

struct StreamWorkRandomParams : public StreamWorkParams {
    uint32_t bytes;
    bool shutdown;

    StreamWorkRandomParams(boost::property_tree::ptree &ptree);
};

struct StreamWorkHttpClientParams : public StreamWorkParams {
    StreamWorkHttpClientParams(boost::property_tree::ptree &ptree);
};

struct StreamWorkHttpServerParams : public StreamWorkParams {
    StreamWorkHttpServerParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct TcpWorkerParams : public Params {

    StreamWorkParams *work;
    int linger; // tcp linger as set via SO_LINGER

    TcpWorkerParams(boost::property_tree::ptree &ptree);
};

struct TcpServerWorkerParams : public TcpWorkerParams {

    TcpServerWorkerParams(boost::property_tree::ptree &ptree);
};

struct TcpClientWorkerParams : public TcpWorkerParams {

    TcpClientWorkerParams(boost::property_tree::ptree &ptree);
};

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

struct TcpFactoryParams : public FactoryParams {
    uint32_t bind_addr;       // htonl(INADDR_ANY)
    uint16_t bind_port;       // htons(9999)
    uint32_t concurrency;
    uint32_t count;

    TcpFactoryParams(boost::property_tree::ptree &ptree);
};

struct TcpServerFactoryParams : public TcpFactoryParams {
    uint32_t accept_backlog;
    TcpServerWorkerParams *worker;

    TcpServerFactoryParams(boost::property_tree::ptree &ptree);
};

struct TcpClientFactoryParams : public TcpFactoryParams {
    uint32_t server_addr;     // htonl or inet_addr("1.2.3.4")
    uint16_t server_port;     // htons(12345)
    float    connect_timeout; // 5.0
    TcpClientWorkerParams *worker;

    TcpClientFactoryParams(boost::property_tree::ptree &ptree);
};

////////////////////////////////////////////////////////////////////////////////
struct FlamethrowerParams : public Params{
    uint32_t version;
    std::list<FactoryParams*> factories;

    FlamethrowerParams(boost::property_tree::ptree &ptree);
};

#endif
