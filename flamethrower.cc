#include "flamethrower.h"

Flamethrower::Flamethrower(boost::property_tree::ptree &params_ptree)
    : params(params_ptree) {

    // Initialize the event loop
    loop = ev_loop_new(EVFLAG_AUTO);

    // Create factories
    for(auto &factory_params : params.factories) {
        factories.push_back(Factory::maker(loop, *factory_params));
    }
}

Flamethrower::~Flamethrower() {
    ev_loop_destroy(loop);
}

void Flamethrower::loop_iteration() {
    ev_loop(loop, 0);
}
