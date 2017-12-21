#pragma once

#include <memory>

#include "wigner_web/discretization/basis.h"

namespace wigner_web::state{
    class State{
    public:
        std::shared_ptr<const wigner_web::discretization::Basis> basis;

        State(): basis(0){}
        State(std::shared_ptr<const wigner_web::discretization::Basis> basis_): basis(basis_){}

        virtual double norm() const=0;
    };
}
