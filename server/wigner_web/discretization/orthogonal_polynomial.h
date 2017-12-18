#pragma once

#include <exception>
#include <string>
#include <Eigen/Dense>

#include "wigner_web/discretization/basis.h"

namespace wigner_web::discretization{

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

    public:
        class alglib_exception: public std::runtime_error{
        public:
            alglib_exception(std::string message);
        };


        OrthogonalPolynomial(double _lower, double _upper, int _size, Basis::BoundaryConditions boundary_conditions);

        Eigen::VectorXcd evaluate(double x, int derivative=0) const override;
        void quadrature(int order, Eigen::VectorXd& points, Eigen::VectorXd& weights) const override;
    };

}
