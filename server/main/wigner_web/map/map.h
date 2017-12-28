#pragma once

#include <memory>
#include <exception>
#include <boost/type_traits.hpp>
#include <Eigen/Dense>

#include "wigner_web/state/state.h"
#include "wigner_web/discretization/basis.h"

namespace wigner_web::map{
    
    template <class StateClass>
    class Map{
        BOOST_STATIC_ASSERT(boost::is_base_of<wigner_web::state::State, StateClass>::value);

    public:
        std::shared_ptr<const wigner_web::discretization::Basis> basis;

        Map(std::shared_ptr<const wigner_web::discretization::Basis> basis_): basis(basis_){}

        virtual void set_time(double time){  }
        virtual void apply(StateClass& vector) const=0;
        virtual const Eigen::MatrixXcd& matrix_representation() const{ throw std::runtime_error("No matrix representation implemented!"); }
    };
}
