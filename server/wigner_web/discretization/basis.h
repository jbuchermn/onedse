#ifndef BASIS_H
#define BASIS_H

#include <functional>

#include <Eigen/Dense>

namespace wigner_web::discretization{

    /**
     * Basis of the discrtized subspace of L^2(dx, [lower, upper]) of dimension size
     */
    class Basis{
        mutable Eigen::MatrixXcd metric_cov;
        mutable Eigen::MatrixXcd metric_contrav;

    public:
        enum class BoundaryConditions{
            None,
            Dirichlet,
            Periodic
        };

        const double lower;
        const double upper;
        const BoundaryConditions boundary_conditions;
        const int size;

        Basis(double _lower, double _upper, int _size, BoundaryConditions _boundary_conditions);

        /// Evaluate basis functions at point x
        virtual Eigen::VectorXcd evaluate(double x, int derivative=0) const=0;

        // Matrix(i,j) = e_j(x_i)
        virtual Eigen::MatrixXcd evaluate(const Eigen::VectorXd& x, int derivative=0) const;

        /// Quadrature rule that exactly integrates <e_i|f(x)|e_j> where f has maximal degree order
        virtual void quadrature(int order, Eigen::VectorXd& points, Eigen::VectorXd& weights) const=0;

        /// <e_i|e_j>
        const Eigen::MatrixXcd& get_metric_cov() const;
        
        /// Inverse of metric_cov
        const Eigen::MatrixXcd& get_metric_contrav() const;

        Eigen::VectorXcd discretize_function_cov(std::function<std::complex<double>(double)> psi, int order) const;
        
        Eigen::MatrixXcd discretize_laplacian_cov() const;
        Eigen::MatrixXcd discretize_mult_op_cov(std::function<std::complex<double>(double)> V, int order) const;

    };

}

#endif
