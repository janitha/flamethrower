#ifndef FLAMETHROWER_H
#define FLAMETHROWER_H

#include "common.h"

#include "tcp_factory.h"


////////////////////////////////////////////////////////////////////////////////
// Flamethrower main class
////////////////////////////////////////////////////////////////////////////////
class Flamethrower {
private:
public:
    struct ev_loop *loop;
    FlamethrowerParams params;

    std::list<Factory*> factories;

    Flamethrower(boost::property_tree::ptree &params_ptree);
    virtual ~Flamethrower();

    void loop_iteration();
};


#endif
