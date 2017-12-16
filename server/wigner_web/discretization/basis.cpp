#include <iostream>
#include <memory>
#include <LuaContext.hpp>

#include "wigner_web/discretization/basis.h"
#include "wigner_web/discretization/orthogonal_hermite.h"
#include "wigner_web/discretization/scaled_orthogonal_chebyshev.h"

using OrthogonalHermite = wigner_web::discretization::OrthogonalHermite;
using ScaledOrthogonalChebyshev = wigner_web::discretization::ScaledOrthogonalChebyshev;

namespace wigner_web::discretization{
    Basis::Basis(double _lower, double _upper, int _size, BoundaryConditions _boundary_conditions): 
        lower(_lower), upper(_upper), size(_size), boundary_conditions(_boundary_conditions), metric_cov(0,0), metric_contrav(0,0){}

    Eigen::MatrixXcd Basis::evaluate(const Eigen::VectorXd& x, int derivative) const{
        Eigen::MatrixXcd values(x.rows(), size);
        for(int i=0; i<x.rows(); i++){
            Eigen::VectorXcd vals = evaluate(x(i), derivative);
            values.block(i, 0, 1, size) = vals.transpose();
        }

        return values;
    }

    const Eigen::MatrixXcd& Basis::get_metric_cov() const{
        if(metric_cov.rows() != size){
            Eigen::VectorXd points, weights;
            quadrature(0, points, weights);

            Eigen::MatrixXcd basis_vals = evaluate(points);
            metric_cov = basis_vals.adjoint() * weights.asDiagonal() * basis_vals;
        } 
        return metric_cov;
    }

    const Eigen::MatrixXcd& Basis::get_metric_contrav() const{
        if(metric_contrav.rows() != size){
            metric_contrav = get_metric_cov().inverse();
        } 
        return metric_contrav;
    }


    Eigen::VectorXcd Basis::discretize_function_cov(std::function<std::complex<double>(double)> psi, int order) const{
        Eigen::VectorXd points, weights;
        quadrature(order, points, weights);

        Eigen::MatrixXcd basis_vals = evaluate(points);

        Eigen::VectorXcd function_vals(points.rows());
        for(int i=0; i<points.rows(); i++){
            function_vals(i) = psi(points(i));
        }

        return basis_vals.adjoint() * weights.asDiagonal() * function_vals;
    }
    
    Eigen::MatrixXcd Basis::discretize_laplacian_cov() const{
        Eigen::VectorXd points, weights;
        quadrature(0, points, weights);

        Eigen::MatrixXcd basis_derivs = evaluate(points, 1);
        return -1. * basis_derivs.adjoint() * weights.asDiagonal() * basis_derivs;
    }
        
    Eigen::MatrixXcd Basis::discretize_mult_op_cov(std::function<std::complex<double>(double)> V, int order) const{
        Eigen::VectorXd points, weights;
        quadrature(order, points, weights);

        Eigen::MatrixXcd basis_vals = evaluate(points);

        Eigen::VectorXcd function_vals(points.rows());
        for(int i=0; i<points.rows(); i++){
            function_vals(i) = V(points(i));
        }

        return basis_vals.adjoint() * weights.asDiagonal() * function_vals.asDiagonal() * basis_vals;
    }

}
