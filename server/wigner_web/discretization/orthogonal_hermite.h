#pragma once

#include <cmath>

#include "wigner_web/discretization/orthogonal_polynomial.h"

namespace wigner_web::discretization{

    /**
     * Hermite polynomials are defined over the whole real line
     */
    class OrthogonalHermite: public OrthogonalPolynomial{
    protected:
        inline double a(int n) const{ return 2.; }
        inline double b(int n) const{ return 0.; }
        inline double c(int n) const{ return -2.*(n-1); }
        inline double value0() const{ return 1.; }
        inline double normsq0() const{ return std::sqrt(M_PI); }

    public:
        OrthogonalHermite(double lower, double upper, int size): OrthogonalPolynomial(lower, upper, size, Basis::BoundaryConditions::None){}
    };
}
