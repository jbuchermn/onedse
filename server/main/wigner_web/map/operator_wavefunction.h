#pragma once

#include <memory>
#include <functional>
#include <Eigen/Dense>

#include "wigner_web/discretization/basis.h"
#include "wigner_web/map/operator.h"
#include "wigner_web/state/wave_function.h"

namespace wigner_web::map{

    class OperatorWaveFunction: public wigner_web::map::Operator<wigner_web::state::WaveFunction>{
    public:
        /// Matrix with mixed indices M^î_j
        Eigen::MatrixXcd matrix;
        
        OperatorWaveFunction(std::shared_ptr<const wigner_web::discretization::Basis> basis);
        OperatorWaveFunction(std::shared_ptr<const wigner_web::discretization::Basis> basis, int left_derivative, int right_derivative, std::function<std::complex<double>(double)> V, int order);
        OperatorWaveFunction(std::shared_ptr<const wigner_web::discretization::Basis> basis, const Eigen::MatrixXcd& matrix);

        void set_from_components_cov(const Eigen::MatrixXcd& components);

        void apply(wigner_web::state::WaveFunction& wavefunction) const override;
        
        OperatorWaveFunction& operator/=(const std::complex<double>& scalar){ matrix/=scalar;       return *this; }
        OperatorWaveFunction& operator*=(const std::complex<double>& scalar){ matrix*=scalar;       return *this; }
        OperatorWaveFunction& operator+=(const OperatorWaveFunction& other) { matrix+=other.matrix; return *this; }
        OperatorWaveFunction& operator-=(const OperatorWaveFunction& other) { matrix-=other.matrix; return *this; }

        OperatorWaveFunction operator/(const std::complex<double>& scalar) const { return OperatorWaveFunction{basis, matrix/scalar}; }
        OperatorWaveFunction operator*(const std::complex<double>& scalar) const { return OperatorWaveFunction{basis, matrix*scalar}; }
        OperatorWaveFunction operator+(const OperatorWaveFunction& other) const  { return OperatorWaveFunction{basis, matrix+other.matrix}; }
        OperatorWaveFunction operator-(const OperatorWaveFunction& other) const  { return OperatorWaveFunction{basis, matrix-other.matrix}; }

        friend OperatorWaveFunction operator*(const std::complex<double>& scalar, const OperatorWaveFunction& op){
            return op*scalar;
        }
    };
}
