#ifndef EVENTS_H
#define EVENTS_H

#include <ctime>
#include <cstdint>

uint64_t timestamp_ns_now();

class Event {
public:
    Event() {}
    virtual ~Event() {}
};

class TcpClientEvent : public Event {
public:
    uint64_t established_time;

    TcpClientEvent() : Event() {}
    virtual ~TcpClientEvent() {}
};




#endif
