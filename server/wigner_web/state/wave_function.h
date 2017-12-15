#ifndef WAVE_FUNCTION_H
#define WAVE_FUNCTION_H

#include <complex>
#include <memory>
#include <Eigen/Dense>

#include "wigner_web/discretization/basis.h"

namespace wigner_web::state{

    class WaveFunction{
    public:
        const std::shared_ptr<const wigner_web::discretization::Basis> basis;

        /// Representation with contravariant indices
        Eigen::VectorXcd vector;

        WaveFunction(std::shared_ptr<const wigner_web::discretization::Basis> _basis);
        WaveFunction(std::shared_ptr<const wigner_web::discretization::Basis> _basis, Eigen::VectorXcd&& _vector);

        std::complex<double> operator()(double x) const;
        double norm() const;
        Eigen::VectorXcd grid(const Eigen::VectorXd& x) const;

        void set_from_components_cov(const Eigen::VectorXcd& components);
        void set_from_components_contrav(const Eigen::VectorXcd& components);

        WaveFunction& operator/=(const std::complex<double>& scalar){ vector/=scalar; return *this; }
        WaveFunction& operator*=(const std::complex<double>& scalar){ vector*=scalar; return *this; }
        WaveFunction& operator+=(const WaveFunction& other){ vector-=other.vector; return *this; }
        WaveFunction& operator-=(const WaveFunction& other){ vector+=other.vector; return *this; }

        WaveFunction operator/(const std::complex<double>& scalar) const { return WaveFunction{basis, vector/scalar}; }
        WaveFunction operator*(const std::complex<double>& scalar) const { return WaveFunction{basis, vector*scalar}; }
        WaveFunction operator+(const WaveFunction& other) const { return WaveFunction{basis, vector+other.vector}; }
        WaveFunction operator-(const WaveFunction& other) const { return WaveFunction{basis, vector-other.vector}; }

        friend WaveFunction operator*(const std::complex<double>& scalar, const WaveFunction& wf){
            return wf*scalar;
        }
    };
}



#endif
