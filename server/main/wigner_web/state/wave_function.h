#pragma once

#include <complex>
#include <functional>
#include <memory>
#include <iomanip>
#include <Eigen/Dense>
#include <json.hpp>

#include "wigner_web/state/state.h"
#include "wigner_web/discretization/basis.h"

namespace wigner_web::state{

    class WaveFunction: public wigner_web::state::State{
    public:

        /// Representation with contravariant indices
        Eigen::VectorXcd vector;

        WaveFunction(std::shared_ptr<const wigner_web::discretization::Basis> basis);
        WaveFunction(std::shared_ptr<const wigner_web::discretization::Basis> basis, Eigen::VectorXcd&& vector_);
        WaveFunction(const WaveFunction& other);
        WaveFunction(WaveFunction&& other);
        WaveFunction& operator=(const WaveFunction& other);
        WaveFunction& operator=(WaveFunction&& other);
        
        WaveFunction(std::shared_ptr<const wigner_web::discretization::Basis> basis, std::function<std::complex<double>(double)> psi, int order);


        std::complex<double> operator()(double x) const;
        double norm() const;
        Eigen::VectorXcd grid(const Eigen::VectorXd& x) const;

        void plot_to_terminal(int points=200) const;

        void set_from_components_cov(const Eigen::VectorXcd& components);
        void set_from_components_contrav(const Eigen::VectorXcd& components);

        void operator/=(const std::complex<double>& scalar){ vector/=scalar; }
        void operator*=(const std::complex<double>& scalar){ vector*=scalar; }
        void operator+=(const WaveFunction& other){ vector+=other.vector; }
        void operator-=(const WaveFunction& other){ vector-=other.vector; }

        WaveFunction operator/(const std::complex<double>& scalar) const { return WaveFunction{basis, vector/scalar}; }
        WaveFunction operator*(const std::complex<double>& scalar) const { return WaveFunction{basis, vector*scalar}; }
        WaveFunction operator+(const WaveFunction& other) const { return WaveFunction{basis, vector+other.vector}; }
        WaveFunction operator-(const WaveFunction& other) const { return WaveFunction{basis, vector-other.vector}; }

        friend WaveFunction operator*(const std::complex<double>& scalar, const WaveFunction& wf){
            return wf*scalar;
        }

        void to_json(nlohmann::json& json, int points) const;
    };
}
