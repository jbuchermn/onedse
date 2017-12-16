#include <complex>
#include <iostream>
#include <memory>
#include <functional>
#include <utility>
#include <float.h>
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

    void WaveFunction::plot_to_terminal(int points) const{
        Eigen::VectorXd x(points);
        for(int i=0; i<points; i++) x(i) = basis->lower + 1.*i/points*(basis->upper - basis->lower);

        Eigen::VectorXcd vals = this->grid(x);
        for(auto range: std::vector<std::pair<double, double>>{{DBL_MAX, 1.}, {1., .5}, {.5, .1}, {.1, 1.e-4}, {1.e-4, -1.e-4}, {-1.e-4, -.1}, {-.1, -.5}, {-.5, -1.}, {-1., DBL_MIN}}){
            for(int i=0; i<vals.rows(); i++){
                if(vals(i).real() > range.second && vals(i).real()<range.first){
                    std::cout<<"*";
                }else{
                    if(range.second<0 && range.first>0){
                        std::cout<<"-";
                    }else{
                        std::cout<<" ";
                    }
                }
            }
            std::cout<<std::endl;
        }
    }

    void WaveFunction::set_from_components_cov(const Eigen::VectorXcd& components){
        vector = basis->get_metric_contrav() * components;
    }

    void WaveFunction::set_from_components_contrav(const Eigen::VectorXcd& components){
        vector = components;
    }
}


