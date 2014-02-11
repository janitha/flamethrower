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
    flamethrower_params_t params;

    Flamethrower(char *params_filename);
    virtual ~Flamethrower();

    void loop_iteration();
};


#endif
