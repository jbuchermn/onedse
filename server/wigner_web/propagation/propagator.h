#pragma once

#include <boost/type_traits.hpp>

namespace wigner_web::propagation{
    template <class StateClass>
    class Propagator{
        BOOST_STATIC_ASSERT(boost::is_base_of<wigner_web::state::State, StateClass>::value);

    protected:
        std::shared_ptr<wigner_web::map::Operator<StateClass>> map;

    public:
        Propagator(std::shared_ptr<wigner_web::map::Operator<StateClass>> map_): map(map_) {}

        virtual void step(StateClass& state, double t_start, double t_step)=0;

    };
}
