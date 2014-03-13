#ifndef FLAMETHROWER_H
#define FLAMETHROWER_H

#include <list>
#include <string>

#include "common.h"

#include "tcp_factory.h"

class Factory;

////////////////////////////////////////////////////////////////////////////////
// Flamethrower main class
////////////////////////////////////////////////////////////////////////////////
class Flamethrower {
private:
public:
    std::string uuid;

    struct ev_loop *loop;
    FlamethrowerParams params;

    std::list<Factory*> factories;

    StatsCollector statsbucket;

    Flamethrower(boost::property_tree::ptree &params_ptree);
    virtual ~Flamethrower();

    void loop_iteration();
};


#endif
