#pragma once

#include <memory>
#include <boost/type_traits.hpp>

#include "wigner_web/state/state.h"
#include "wigner_web/discretization/basis.h"

namespace wigner_web::map{
    
    template <class StateClass>
    class Operator{
        BOOST_STATIC_ASSERT(boost::is_base_of<wigner_web::state::State, StateClass>::value);

    public:
        std::shared_ptr<const wigner_web::discretization::Basis> basis;

        Operator(std::shared_ptr<const wigner_web::discretization::Basis> basis_): basis(basis_){}

        virtual void set_time(double time){  }
        virtual void apply(StateClass& vector) const=0;
    };
}
