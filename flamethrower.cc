#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "flamethrower.h"

Flamethrower::Flamethrower(boost::property_tree::ptree &params_ptree)
    : params(params_ptree),
      statsbucket(*params.stats) {

    // Initialize the event loop
    loop = ev_loop_new(EVFLAG_AUTO);

    // Create factories
    for(auto &factory_params : params.factories) {
        factories.push_back(Factory::maker(*this, *factory_params));
    }

    uuid = boost::uuids::to_string(boost::uuids::random_generator()());
    debug_print("Flamethrower uuid=%s\n", uuid.c_str());

}

Flamethrower::~Flamethrower() {
    ev_loop_destroy(loop);
}

void Flamethrower::loop_iteration() {
    ev_loop(loop, 0);
}
