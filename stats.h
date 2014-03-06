#ifndef STATS_H
#define STATS_H

#include <ctime>
#include <cstdint>
#include <list>

#include <msgpack.hpp>

uint64_t timestamp_ns_now();

class Stats;

////////////////////////////////////////////////////////////////////////////////
class StatsList {
private:
    std::list<Stats*> stats;
public:
    StatsList();
    virtual ~StatsList();

    void push(Stats *stats);
};

////////////////////////////////////////////////////////////////////////////////
class Stats {
public:
    Stats();
    virtual ~Stats();

    virtual void print();
};


////////////////////////////////////////////////////////////////////////////////
class FactoryStats : public Stats {
};

class TcpFactoryStats : public FactoryStats {
public:
    uint64_t bytes_in;
    uint64_t bytes_out;
    uint64_t cumulative_count;

    TcpFactoryStats();
    virtual ~TcpFactoryStats();

    virtual void print();
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
};

////////////////////////////////////////////////////////////////////////////////
class TcpServerWorkerStats : public TcpWorkerStats {
public:

    uint64_t established_time;

    TcpServerWorkerStats();
    virtual ~TcpServerWorkerStats();

    virtual void print();
};

////////////////////////////////////////////////////////////////////////////////
class TcpClientWorkerStats : public TcpWorkerStats {
public:

    uint64_t connect_time;
    uint64_t established_time;

    TcpClientWorkerStats();
    virtual ~TcpClientWorkerStats();

    virtual void print();
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
