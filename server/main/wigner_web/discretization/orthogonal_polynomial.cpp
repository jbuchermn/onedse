#include <Eigen/Dense>
#include <alglib/integration.h>
#include <boost/math/special_functions/binomial.hpp>

#include "wigner_web/discretization/orthogonal_polynomial.h"

namespace wigner_web::discretization{

    OrthogonalPolynomial::alglib_error::alglib_error(std::string message): std::runtime_error(message){}
    OrthogonalPolynomial::unsupported_derivative_error::unsupported_derivative_error(int derivative): std::runtime_error("Derivative not supported: "+std::to_string(derivative)){}

    OrthogonalPolynomial::OrthogonalPolynomial(double _lower, double _upper, int _size, Basis::BoundaryConditions boundary_conditions):
        Basis(_lower, _upper, _size, boundary_conditions){}

    Eigen::VectorXcd OrthogonalPolynomial::evaluate_polynomials(double x, int derivative) const{
        Eigen::VectorXcd values(size);

        Eigen::VectorXcd base_values;
        if(derivative>0) base_values = evaluate_polynomials(x, derivative-1);

        values(0) = derivative==0 ? value0() : 0;

        for(int i=1; i<size; i++){
            values(i) = (a(i)*x + b(i))*values(i-1);
            if(i>1) values(i) += c(i)*values(i-2);

            if(derivative>0) values(i) += derivative*a(i)*base_values(i-1);
        }

        return values;
    }

    Eigen::VectorXcd OrthogonalPolynomial::evaluate(double x, int derivative) const{
        Eigen::VectorXcd res = Eigen::VectorXcd::Zero(size); 
        for(int i=0; i<=derivative; i++){
            res += boost::math::binomial_coefficient<double>(derivative, i)*evaluate_sqrtweight(x, i)*evaluate_polynomials(x, derivative-i);
        }
        return res;
    }

    void OrthogonalPolynomial::quadrature(int order, Eigen::VectorXd& points, Eigen::VectorXd& weights) const{
        order = quadrature_order_overlap() + order;

        alglib::real_1d_array alpha;
        alglib::real_1d_array beta;
        alglib::ae_int_t info;
        alglib::real_1d_array xq;
        alglib::real_1d_array wq;

        alpha.setlength(order);
        beta.setlength(order);
        if(order>0){
            alpha[0] = -b(1)/a(1);
            beta[0] = 0.;
        }
        for(int n=1; n<order; n++){
            alpha[n] = -b(n+1)/a(n+1);
            beta[n] = -c(n+1)/(a(n+1)*a(n));
        }

        alglib::gqgeneraterec(alpha, beta, normsq0(), order, info, xq, wq);

        switch(info){
        case  1: 
            break; // success
        case -3:
            throw alglib_error("QuadratureRule: internal eigenproblem solver hasn't converged");
        case -2:
            throw alglib_error("QuadratureRule: Beta[i]<=0");
        case -1: 
            throw alglib_error("QuadratureRule: incorrect N was passed");
        default: 
            throw alglib_error("QuadratureRule: error value info = "+std::to_string(info));
        }

        // transfer to std::vector
        points = Eigen::VectorXd(order);
        weights = Eigen::VectorXd(order);
        for (int i=0; i<order; i++) {
            points(i)=xq[i];
            weights(i)=wq[i];
        };

        // We want to integrate against basis functions polynomial times sqrt(weight)
        for(int i=0; i<order; i++){
            weights(i)/=std::pow(evaluate_sqrtweight(points(i)), 2.);
        }

    }

}





