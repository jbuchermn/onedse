#include <Eigen/Dense>
#include <alglib/integration.h>

#include "wigner_web/discretization/orthogonal_polynomial.h"

namespace wigner_web::discretization{

    OrthogonalPolynomial::alglib_exception::alglib_exception(std::string message): std::runtime_error(message){}

    OrthogonalPolynomial::OrthogonalPolynomial(double _lower, double _upper, int _size):
        Basis(_lower, _upper, _size){}

    void OrthogonalPolynomial::evaluate(double x, Eigen::VectorXcd& values, Eigen::VectorXcd& derivatives) const{
        values = Eigen::VectorXcd(size);
        derivatives = Eigen::VectorXcd(size);
        if(size>0){
            values[0] = value0();
            derivatives[0] = 0;
        }

        if(size>1){
            values[1] = a(1)*x*values[0] + b(1);
            derivatives[1] = a(1)*values[0];
        }

        for(int i=2; i<size; i++){
            values[i] = (a(i)*x + b(i))*values[i-1] + c(i)*values[i-2];
            derivatives[i] = (a(i)*x + b(i))*derivatives[i-1] + a(i)*values[i-1] + c(i)*derivatives[i-2]; 
        }
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
            throw alglib_exception("QuadratureRule: internal eigenproblem solver hasn't converged");
        case -2:
            throw alglib_exception("QuadratureRule: Beta[i]<=0");
        case -1: 
            throw alglib_exception("QuadratureRule: incorrect N was passed");
        default: 
            throw alglib_exception("QuadratureRule: error value info = "+std::to_string(info));
        }

        // transfer to std::vector
        points = Eigen::VectorXd(order);
        weights = Eigen::VectorXd(order);
        for (int i=0; i<order; i++) {
            points(i)=xq[i];
            weights(i)=wq[i];
        };

    }

}





