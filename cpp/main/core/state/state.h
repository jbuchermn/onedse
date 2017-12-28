#pragma once

#include <memory>

#include "core/discretization/basis.h"

namespace core::state{
    class State{
    public:
        std::shared_ptr<const core::discretization::Basis> basis;

        State(): basis(0){}
        State(std::shared_ptr<const core::discretization::Basis> basis_): basis(basis_){}

        virtual double norm() const=0;
    };
}
