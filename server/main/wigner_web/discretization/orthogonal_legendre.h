#pragma once

#include <cmath>
#include <float.h>

#include "wigner_web/discretization/orthogonal_polynomial.h"

namespace wigner_web::discretization{

    /**
     * Legendre polynomials are defined over [-1, 1]
     */
    class OrthogonalLegendre: public OrthogonalPolynomial{
    protected:
        inline double a(int n) const override{ return (2.*n-1.)/n; }
        inline double b(int n) const override{ return 0.; }
        inline double c(int n) const override{ return -(n-1.)/n; }
        inline double value0() const override{ return 1.; }
        inline double normsq0() const override{ return 2.; }

    public:
        OrthogonalLegendre(int size): OrthogonalPolynomial(-1., 1., size, Basis::BoundaryConditions::None){}
    };
}
