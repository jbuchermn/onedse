#pragma once

#include <memory>
#include <functional>
#include <utility>

#include <Eigen/Dense>

namespace core::discretization{

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

        // Find boundaries for plots or Wigner functions (i. e. do not include lower or upper, and pick sth useful for (DBL_MIN, DBL_MAX))
        std::pair<double, double> plot_boundaries() const;

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
        Eigen::MatrixXcd discretize_op_cov(int left_derivative, int right_derivative, std::function<std::complex<double>(double)> V, int order) const;

    };

}
