#ifndef SCALED_ORTHOGONAL_CHEBYSHEV_H
#define SCALED_ORTHOGONAL_CHEBYSHEV_H

#include <cmath>

#include "wigner_web/discretization/orthogonal_polynomial.h"

namespace wigner_web::discretization{

    /**
     * Chebyshev polynomials are defined over [-1,1]
     */
    class ScaledOrthogonalChebyshev: public OrthogonalPolynomial{
    protected:
        inline double a(int n) const{ return n==1 ? 1. : 2.; }
        inline double b(int n) const{ return 0.; }
        inline double c(int n) const{ return -1; }
        inline double value0() const{ return 1.; }
        inline double normsq0() const{ return M_PI/2.*(upper-lower); }

    public:
        ScaledOrthogonalChebyshev(double lower, double upper, int size): OrthogonalPolynomial(lower, upper, size, Basis::BoundaryConditions::None){}

        Eigen::VectorXcd evaluate(double x, int derivative=0) const;
        void quadrature(int order, Eigen::VectorXd& points, Eigen::VectorXd& weights) const;
    };
}


#endif
