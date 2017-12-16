#include "wigner_web/discretization/scaled_orthogonal_chebyshev.h"

namespace wigner_web::discretization{
    Eigen::VectorXcd ScaledOrthogonalChebyshev::evaluate(double x, int derivative) const{
        x = 2/(upper-lower)*x + 1 - 2*upper/(upper-lower);
        return OrthogonalPolynomial::evaluate(x, derivative)*std::pow(2/(upper-lower), derivative);
    }

    void ScaledOrthogonalChebyshev::quadrature(int order, Eigen::VectorXd& points, Eigen::VectorXd& weights) const{
        OrthogonalPolynomial::quadrature(order, points, weights);
        for(int i=0; i<points.rows(); i++) points(i) = (upper-lower)/2.*(points(i) - 1. + 2.*upper/(upper-lower));
        for(int i=0; i<weights.rows(); i++) weights(i)*=2./(upper-lower);
    }
}



// a*lower + b = -1
// a*upper + b = 1
//
// => a*(upper-lower) = 2 => a=2/(upper-lower)
// b = 1- a*upper = 1- 2*upper/(upper-lower)
