#pragma once

#include <cmath>

#include "wigner_web/discretization/orthogonal_polynomial.h"

namespace wigner_web::discretization{

    /**
     * Chebyshev polynomials are defined over [-1,1]
     */
    class OrthogonalChebyshev: public OrthogonalPolynomial{
    protected:
        inline double a(int n) const override{ return n==1 ? 1. : 2.; }
        inline double b(int n) const override{ return 0.; }
        inline double c(int n) const override{ return -1; }
        inline double value0() const override{ return 1.; }
        inline double normsq0() const override{ return M_PI/2.*(upper-lower); }

    public:
        OrthogonalChebyshev(int size): OrthogonalPolynomial(-1., 1., size, Basis::BoundaryConditions::None){}
    };
}
