#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <boost/math/special_functions/binomial.hpp>

#include "core/discretization/orthogonal_polynomial.h"

namespace core::discretization{

    OrthogonalPolynomial::quadrature_error::quadrature_error(std::string message): std::runtime_error(message){}
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

        // Golub-Welsch: nodes are the eigenvalues of the Jacobi matrix built from the
        // three-term recurrence, weights follow from the first eigenvector components.
        if(order<1) throw quadrature_error("QuadratureRule: order must be positive");

        Eigen::VectorXd diag(order);
        Eigen::VectorXd subdiag(std::max(order-1, 0));
        for(int n=0; n<order; n++){
            diag(n) = -b(n+1)/a(n+1);
            if(n>0){
                double beta = -c(n+1)/(a(n+1)*a(n));
                if(!(beta>0.)) throw quadrature_error("QuadratureRule: Beta[i]<=0");
                subdiag(n-1) = std::sqrt(beta);
            }
        }

        Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver;
        solver.computeFromTridiagonal(diag, subdiag, Eigen::EigenvaluesOnly);
        if(solver.info() != Eigen::Success){
            throw quadrature_error("QuadratureRule: internal eigenproblem solver hasn't converged");
        }

        // Weights: w_i = 1 / sum_k p_k(x_i)^2 with the orthonormal polynomials p_k. Unlike
        // the textbook formula via eigenvector components, this keeps full relative accuracy
        // for the tiny weights in the tails (Hermite), which get divided by the weight function below.
        points = solver.eigenvalues();
        weights = Eigen::VectorXd(order);
        for(int i=0; i<order; i++){
            double previous = 0.;
            double current = 1./std::sqrt(normsq0());
            double sum = current*current;
            for(int k=0; k+1<order; k++){
                double next = ((points(i) - diag(k))*current - (k>0 ? subdiag(k-1)*previous : 0.))/subdiag(k);
                previous = current;
                current = next;
                sum += current*current;
            }
            weights(i) = 1./sum;
        }

        // We want to integrate against basis functions polynomial times sqrt(weight)
        for(int i=0; i<order; i++){
            weights(i)/=std::pow(evaluate_sqrtweight(points(i)), 2.);
        }

    }

}





