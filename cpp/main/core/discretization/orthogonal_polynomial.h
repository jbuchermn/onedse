#pragma once

#include <exception>
#include <string>
#include <complex>
#include <Eigen/Dense>

#include "core/discretization/basis.h"

namespace core::discretization{

    /** 
     * Orthogonal polynomials are defined by their recursion relation a_n, b_n, c_n
     * and the norm of the zero'th polynomial normsq0 and the (constant) value of the zero'th
     * polynomial value0
     */
    class OrthogonalPolynomial: public Basis{
    protected:
        virtual int quadrature_order_overlap() const{ return size; }

        virtual double a(int n) const=0;
        virtual double b(int n) const=0;
        virtual double c(int n) const=0;
        virtual double value0() const=0;
        virtual double normsq0() const=0;
        virtual double evaluate_sqrtweight(double x, int derviative=0) const=0;

        Eigen::VectorXcd evaluate_polynomials(double x, int derivative=0) const;

    public:
        class alglib_error: public std::runtime_error{
        public:
            alglib_error(std::string message);
        };

        class unsupported_derivative_error: public std::runtime_error{
        public:
            unsupported_derivative_error(int derivative);
        };


        OrthogonalPolynomial(double _lower, double _upper, int _size, Basis::BoundaryConditions boundary_conditions);

        Eigen::VectorXcd evaluate(double x, int derivative=0) const override;
        void quadrature(int order, Eigen::VectorXd& points, Eigen::VectorXd& weights) const override;
    };

}
