#pragma once

#include <cmath>
#include <float.h>

#include "core/discretization/orthogonal_polynomial.h"

namespace core::discretization{

    /**
     * Hermite polynomials are defined over the whole real line with weight function w(x) = exp(-x^2).
     * We redefine the polynomials H(x) -> H(x)/sqrt(2^n n!), otherwise the norms blow up
     * way too fast.
     */
    class OrthogonalHermite: public OrthogonalPolynomial{
    protected:
        inline double a(int n) const override{ return 2. /* Normalization */ *std::sqrt(.5/n); }
        inline double b(int n) const override{ return 0.; }
        inline double c(int n) const override{ return -2.*(n-1) /* Normalization*/ *std::sqrt(.25/n/(n-1)); }
        inline double value0() const override{ return 1.; }
        inline double normsq0() const override{ return std::sqrt(M_PI); }
        inline double evaluate_sqrtweight(double x, int derivative) const override{
            if(derivative==0){
                return std::exp(-x*x/2.);
            }else if(derivative==1){
                return -x*std::exp(-x*x/2.);
            }else if(derivative==2){
                return (x*x - 1.)*std::exp(-x*x/2.);
            }else{
                throw unsupported_derivative_error(derivative);
            }
        }

    public:
        OrthogonalHermite(int size): OrthogonalPolynomial(DBL_MIN/2., DBL_MAX/2., size, Basis::BoundaryConditions::None){}
    };
}
