#include <iostream>

#include "wigner_web/discretization/basis.h"

namespace wigner_web::discretization{
    Basis::Basis(double _lower, double _upper, int _size): lower(_lower), upper(_upper), size(_size), metric_cov(0,0), metric_contrav(0,0){}
        
    void Basis::evaluate(const Eigen::VectorXd& x, Eigen::MatrixXcd& values, Eigen::MatrixXcd& derivatives) const{
        values = Eigen::MatrixXcd(x.rows(), size);
        derivatives = Eigen::MatrixXcd(x.rows(), size);
        for(int i=0; i<x.rows(); i++){
            Eigen::VectorXcd vals, derivs;
            evaluate(x(i), vals, derivs);
            values.block(i, 0, 1, size) = vals.transpose();
            derivatives.block(i, 0, 1, size) = derivs.transpose();
        }

    }

    const Eigen::MatrixXcd& Basis::get_metric_cov() const{
        if(metric_cov.rows() != size){
            Eigen::VectorXd points, weights;
            quadrature(0, points, weights);

            Eigen::MatrixXcd basis_vals, basis_derivs;
            evaluate(points, basis_vals, basis_derivs);

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

        Eigen::MatrixXcd basis_vals, basis_derivs;
        evaluate(points, basis_vals, basis_derivs);

        Eigen::VectorXcd function_vals(points.rows());
        for(int i=0; i<points.rows(); i++){
            function_vals(i) = psi(points(i));
        }

        return basis_vals.adjoint() * weights.asDiagonal() * function_vals;
    }
    
    Eigen::MatrixXcd Basis::discretize_laplacian_cov() const{
        Eigen::VectorXd points, weights;
        quadrature(0, points, weights);

        Eigen::MatrixXcd basis_vals, basis_derivs;
        evaluate(points, basis_vals, basis_derivs);

        return -1. * basis_derivs.adjoint() * weights.asDiagonal() * basis_derivs;
    }
        
    Eigen::MatrixXcd Basis::discretize_mult_op_cov(std::function<std::complex<double>(double)> V, int order) const{
        Eigen::VectorXd points, weights;
        quadrature(order, points, weights);

        Eigen::MatrixXcd basis_vals, basis_derivs;
        evaluate(points, basis_vals, basis_derivs);

        Eigen::VectorXcd function_vals(points.rows());
        for(int i=0; i<points.rows(); i++){
            function_vals(i) = V(points(i));
        }

        return basis_vals.adjoint() * weights.asDiagonal() * function_vals.asDiagonal() * basis_vals;
    }

}
