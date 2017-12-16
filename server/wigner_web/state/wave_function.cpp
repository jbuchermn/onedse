#include <complex>
#include <iostream>
#include <memory>
#include <functional>
#include <Eigen/Dense>

#include "wigner_web/state/wave_function.h"
#include "wigner_web/discretization/basis.h"

using Basis = wigner_web::discretization::Basis;

namespace wigner_web::state{

    WaveFunction::WaveFunction(std::shared_ptr<const Basis> _basis): basis(_basis){}
    WaveFunction::WaveFunction(std::shared_ptr<const Basis> _basis, Eigen::VectorXcd&& _vector): basis(_basis), vector(_vector){}
    WaveFunction::WaveFunction(std::shared_ptr<const Basis> _basis, std::function<std::complex<double>(double)> psi, int order): basis(_basis){
        vector = basis->get_metric_contrav()*basis->discretize_function_cov(psi, order);
    }

    std::complex<double> WaveFunction::operator()(double x) const{
        Eigen::VectorXcd basis_vals = basis->evaluate(x);
        return basis_vals.conjugate().dot(vector);
    }

    double WaveFunction::norm() const{
        return std::sqrt( vector.dot(basis->get_metric_cov()*vector).real() );
    }
        
    Eigen::VectorXcd WaveFunction::grid(const Eigen::VectorXd& x) const{
        Eigen::VectorXcd values(x.rows());
        for(int i=0; i<x.rows(); i++){
            values(i) = (*this)(x(i));
        }

        return values;
    }

    void WaveFunction::set_from_components_cov(const Eigen::VectorXcd& components){
        vector = basis->get_metric_contrav() * components;
    }

    void WaveFunction::set_from_components_contrav(const Eigen::VectorXcd& components){
        vector = components;
    }
}


