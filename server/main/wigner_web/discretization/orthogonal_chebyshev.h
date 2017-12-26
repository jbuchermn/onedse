#pragma once

#include <cmath>

#include "wigner_web/discretization/orthogonal_polynomial.h"

namespace wigner_web::discretization{

    /**
     * Chebyshev polynomials are defined over [-1,1] with weight function w(x) = (1-x^2)^{-1/2}
     */
    class OrthogonalChebyshev: public OrthogonalPolynomial{
    protected:
        inline double a(int n) const override{ return n==1 ? 1. : 2.; }
        inline double b(int n) const override{ return 0.; }
        inline double c(int n) const override{ return -1; }
        inline double value0() const override{ return 1.; }
        inline double normsq0() const override{ return M_PI/2.*(upper-lower); }
        inline double evaluate_sqrtweight(double x, int derivative=0) const override{
            if(derivative==0){
                return std::pow(1-x*x, -.25);
            }else if(derivative==1){
                return .5*x*std::pow(1-x*x, -1.25);
            }else if(derivative==2){
                return .5*std::pow(1-x*x, -1.25) + 1.25*x*x*std::pow(1-x*x, -2.25);
            }else{
                throw unsupported_derivative_error(derivative);
            }
        }

    public:
        OrthogonalChebyshev(int size): OrthogonalPolynomial(-1., 1., size, Basis::BoundaryConditions::None){}
    };
}
