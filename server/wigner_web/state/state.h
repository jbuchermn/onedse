#pragma once

#include <memory>

#include "wigner_web/discretization/basis.h"

namespace wigner_web::state{
    class State{
    public:
        std::shared_ptr<const wigner_web::discretization::Basis> basis;

        State(std::shared_ptr<const wigner_web::discretization::Basis> basis_): basis(basis_){}

    };
}
