#pragma once

#include <complex>
#include <functional>
#include <memory>
#include <iomanip>
#include <Eigen/Dense>
#include <json.hpp>

#include "core/state/state.h"
#include "core/discretization/basis.h"

namespace core::state{

    class WaveFunction: public core::state::State{
    public:

        /// Representation with contravariant indices
        Eigen::VectorXcd vector;

        WaveFunction(std::shared_ptr<const core::discretization::Basis> basis);
        WaveFunction(std::shared_ptr<const core::discretization::Basis> basis, const Eigen::VectorXcd& vector_);
        
        WaveFunction(std::shared_ptr<const core::discretization::Basis> basis, std::function<std::complex<double>(double)> psi, int order);

        double norm() const override;
        std::complex<double> dot(const WaveFunction& other) const;

        std::complex<double> operator()(double x) const;
        Eigen::VectorXcd grid(const Eigen::VectorXd& x) const;

        void plot_to_terminal(int points=200) const;

        void set_from_components_cov(const Eigen::VectorXcd& components);
        void set_from_components_contrav(const Eigen::VectorXcd& components);

        WaveFunction& operator/=(const std::complex<double>& scalar){ vector/=scalar;       return *this; }
        WaveFunction& operator*=(const std::complex<double>& scalar){ vector*=scalar;       return *this; }
        WaveFunction& operator+=(const WaveFunction& other)         { vector+=other.vector; return *this; }
        WaveFunction& operator-=(const WaveFunction& other)         { vector-=other.vector; return *this; }

        WaveFunction operator/(const std::complex<double>& scalar) const { return WaveFunction{basis, vector/scalar}; }
        WaveFunction operator*(const std::complex<double>& scalar) const { return WaveFunction{basis, vector*scalar}; }
        WaveFunction operator+(const WaveFunction& other) const          { return WaveFunction{basis, vector+other.vector}; }
        WaveFunction operator-(const WaveFunction& other) const          { return WaveFunction{basis, vector-other.vector}; }

        friend WaveFunction operator*(const std::complex<double>& scalar, const WaveFunction& wf){
            return wf*scalar;
        }

        void to_json(nlohmann::json& json, int points) const;
    };
}
