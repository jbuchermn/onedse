#include <complex>
#include <iostream>
#include <memory>
#include <functional>
#include <utility>
#include <float.h>
#include <cmath>
#include <stdexcept>
#include <Eigen/Dense>
#include <json.hpp>

#include <boost/math/special_functions/fpclassify.hpp>

#include "core/state/wave_function.h"
#include "core/discretization/basis.h"
#include "core/state/state.h"

using State = core::state::State;
using Basis = core::discretization::Basis;

namespace core::state{

    WaveFunction::WaveFunction(std::shared_ptr<const Basis> basis): State(basis), vector(Eigen::VectorXcd::Zero(basis->size)){}
    WaveFunction::WaveFunction(std::shared_ptr<const Basis> basis, const Eigen::VectorXcd& vector_): State(basis), vector(vector_) {}

    WaveFunction::WaveFunction(std::shared_ptr<const Basis> basis_, std::function<std::complex<double>(double)> psi, int order): core::state::State(basis_){
        vector = basis->get_metric_contrav()*basis->discretize_function_cov(psi, order);

        // Normalize when setting by std::function
        double n = norm();
        if(n>1.e-12) (*this)/=n;
    }

    std::complex<double> WaveFunction::operator()(double x) const{
        Eigen::VectorXcd basis_vals = basis->evaluate(x);
        return basis_vals.conjugate().dot(vector);
    }

    double WaveFunction::norm() const{
        return std::sqrt(dot(*this).real());
    }
        
    std::complex<double> WaveFunction::dot(const WaveFunction& other) const{
        return vector.dot(basis->get_metric_cov()*other.vector);
    }
        
    void WaveFunction::validate() const{
        if(vector.hasNaN()) throw std::out_of_range("Wavefunction has NaN values");
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
                if(std::abs(vals(i)) > range.second && std::abs(vals(i))<range.first){
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
        
    void WaveFunction::to_json(nlohmann::json& json, int points) const{
        Eigen::VectorXd x(points);
        for(int i=0; i<points; i++) x(i) = basis->lower + 1.*i/points*(basis->upper - basis->lower);
        Eigen::VectorXcd vals = this->grid(x);

        json["lower"] = basis->lower;
        json["upper"] = basis->upper;
        json["data"] = nlohmann::json();
        for(int i=0; i<vals.rows(); i++){
            json["data"].push_back(std::vector<double>{vals(i).real(), vals(i).imag()});
        }

    }
}


