#pragma once

#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <Eigen/Dense>
#include <json.hpp>

#include "wigner_web/state/state.h"
#include "wigner_web/discretization/basis.h"
#include "wigner_web/state/wave_function.h"

namespace wigner_web::state{
    class DensityOperator: public wigner_web::state::State{
    public:
        using DiagonalRepresentation = std::vector<std::pair<double, std::shared_ptr<wigner_web::state::WaveFunction>>>;

        /// Representation with contravariant indices rho^{ij}
        Eigen::MatrixXcd matrix;

        DensityOperator(std::shared_ptr<const wigner_web::discretization::Basis> basis);
        DensityOperator(std::shared_ptr<const wigner_web::discretization::Basis> basis, Eigen::MatrixXcd&& matrix_);
        DensityOperator(const DensityOperator& other);
        DensityOperator(DensityOperator&& other);
        DensityOperator& operator=(const DensityOperator& other);
        DensityOperator& operator=(DensityOperator&& other);

        DensityOperator(DiagonalRepresentation wavefunctions);

        void add_wavefunction(double probability, std::shared_ptr<wigner_web::state::WaveFunction> wavefunction);
        void set_from_wavefunctions(DiagonalRepresentation wavefunctions);
        DiagonalRepresentation diagonalize() const;
        
        void operator/=(const std::complex<double>& scalar){ matrix/=scalar; }
        void operator*=(const std::complex<double>& scalar){ matrix*=scalar; }
        void operator+=(const DensityOperator& other){ matrix+=other.matrix; }
        void operator-=(const DensityOperator& other){ matrix-=other.matrix; }

        DensityOperator operator/(const std::complex<double>& scalar) const { return DensityOperator{basis, matrix/scalar}; }
        DensityOperator operator*(const std::complex<double>& scalar) const { return DensityOperator{basis, matrix*scalar}; }
        DensityOperator operator+(const DensityOperator& other) const { return DensityOperator{basis, matrix+other.matrix}; }
        DensityOperator operator-(const DensityOperator& other) const { return DensityOperator{basis, matrix-other.matrix}; }

        friend DensityOperator operator*(const std::complex<double>& scalar, const DensityOperator& op){
            return op*scalar;
        }
    };
}
