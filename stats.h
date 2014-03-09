#ifndef STATS_H
#define STATS_H

#include <ctime>
#include <cstdint>
#include <list>

#include <zmq.hpp>
#include <msgpack.hpp>

#include "common.h"

#define STATSQUEUE_CAPACITY 9000

uint64_t timestamp_ns_now();

class Stats;

////////////////////////////////////////////////////////////////////////////////
class StatsCollector {
private:
    StatsParams &params;

    zmq::context_t zcontext;
    zmq::socket_t zpubsocket;

public:

    StatsCollector(StatsParams &params);
    virtual ~StatsCollector();

    void push(Stats *stats);
};

////////////////////////////////////////////////////////////////////////////////
class Stats {
public:

    uint64_t version;

    Stats();
    virtual ~Stats();

    virtual void print();
    virtual void serialize(msgpack::packer<msgpack::sbuffer> &packer);
};


////////////////////////////////////////////////////////////////////////////////
class TcpFactoryStats : public Stats {
public:
    uint64_t bytes_in;
    uint64_t bytes_out;
    uint64_t cumulative_workers;
    uint64_t active_workers;

    TcpFactoryStats();
    virtual ~TcpFactoryStats();

    virtual void print();
    virtual void serialize(msgpack::packer<msgpack::sbuffer> &packer);
};

class TcpServerFactoryStats : public TcpFactoryStats {
};

class TcpClientFactoryStats : public TcpFactoryStats {
};


////////////////////////////////////////////////////////////////////////////////
class TcpWorkerStats : public Stats {
public:

    uint64_t readable_time;
    uint64_t writable_time;
    uint64_t close_time;

    uint64_t bytes_in;
    uint64_t bytes_out;

    TcpWorkerStats();
    virtual ~TcpWorkerStats();

    virtual void print();
    virtual void serialize(msgpack::packer<msgpack::sbuffer> &packer);
};

////////////////////////////////////////////////////////////////////////////////
class TcpServerWorkerStats : public TcpWorkerStats {
public:

    uint64_t established_time;

    TcpServerWorkerStats();
    virtual ~TcpServerWorkerStats();

    virtual void print();
    virtual void serialize(msgpack::packer<msgpack::sbuffer> &packer);
};

////////////////////////////////////////////////////////////////////////////////
class TcpClientWorkerStats : public TcpWorkerStats {
public:

    uint64_t connect_time;
    uint64_t established_time;

    TcpClientWorkerStats();
    virtual ~TcpClientWorkerStats();

    virtual void print();
    virtual void serialize(msgpack::packer<msgpack::sbuffer> &packer);
};

////////////////////////////////////////////////////////////////////////////////
class TcpServerEchoStats : public TcpServerWorkerStats {
};

class TcpClientEchoStats : public TcpClientWorkerStats {
};

class TcpServerRawStats : public TcpServerWorkerStats {
};

class TcpClientRawStats : public TcpClientWorkerStats {
};

class TcpServerHttpStats : public TcpServerWorkerStats {
};

class TcpClientHttpStats : public TcpClientWorkerStats {
};


#endif
