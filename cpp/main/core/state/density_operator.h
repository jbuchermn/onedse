#pragma once

#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <Eigen/Dense>
#include <json.hpp>

#include "core/state/state.h"
#include "core/discretization/basis.h"
#include "core/state/wave_function.h"

namespace core::state{
    class DensityOperator: public core::state::State{
    public:
        using DiagonalRepresentation = std::vector<std::pair<double, std::shared_ptr<core::state::WaveFunction>>>;

        /// Representation with contravariant indices rho^{ij}
        Eigen::MatrixXcd matrix;

        /// Representation of the matrix rho^{ij} = v^{i + N*j}
        Eigen::Map<Eigen::VectorXcd> vector;

        DensityOperator(std::shared_ptr<const core::discretization::Basis> basis);
        DensityOperator(std::shared_ptr<const core::discretization::Basis> basis, const Eigen::MatrixXcd& matrix_);

        DensityOperator(DiagonalRepresentation wavefunctions);
        DensityOperator(std::shared_ptr<WaveFunction> wf);
        
        /// \sqrt(tr \rho^\dagger\rho)
        double norm() const override;
        void validate() const override;

        void add_wavefunction(double probability, std::shared_ptr<core::state::WaveFunction> wavefunction);
        void set_from_wavefunctions(DiagonalRepresentation wavefunctions);
        DiagonalRepresentation diagonalize() const;
        
        DensityOperator& operator/=(const std::complex<double>& scalar){ matrix/=scalar;       return *this; }
        DensityOperator& operator*=(const std::complex<double>& scalar){ matrix*=scalar;       return *this; }
        DensityOperator& operator+=(const DensityOperator& other)      { matrix+=other.matrix; return *this; }
        DensityOperator& operator-=(const DensityOperator& other)      { matrix-=other.matrix; return *this; }

        DensityOperator operator/(const std::complex<double>& scalar) const { return DensityOperator{basis, matrix/scalar}; }
        DensityOperator operator*(const std::complex<double>& scalar) const { return DensityOperator{basis, matrix*scalar}; }
        DensityOperator operator+(const DensityOperator& other) const       { return DensityOperator{basis, matrix+other.matrix}; }
        DensityOperator operator-(const DensityOperator& other) const       { return DensityOperator{basis, matrix-other.matrix}; }

        friend DensityOperator operator*(const std::complex<double>& scalar, const DensityOperator& op){
            return op*scalar;
        }
    };
}
