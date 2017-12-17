#ifndef DENSITY_OPERATOR_H
#define DENSITY_OPERATOR_H

#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <Eigen/Dense>
#include <json.hpp>

#include "wigner_web/discretization/basis.h"
#include "wigner_web/state/wave_function.h"

namespace wigner_web::state{
    class DensityOperator{
    public:
        using DiagonalRepresentation = std::vector<std::pair<double, std::shared_ptr<wigner_web::state::WaveFunction>>>;

        const std::shared_ptr<const wigner_web::discretization::Basis> basis;

        /// Representation with contravariant indices rho^{ij}
        Eigen::MatrixXcd matrix;

        DensityOperator(std::shared_ptr<const wigner_web::discretization::Basis> _basis);
        DensityOperator(std::shared_ptr<const wigner_web::discretization::Basis> _basis, Eigen::MatrixXcd&& _matrix);
        DensityOperator(DiagonalRepresentation wavefunctions);

        void add_wavefunction(double probability, std::shared_ptr<wigner_web::state::WaveFunction> wavefunction);
        void set_from_wavefunctions(DiagonalRepresentation wavefunctions);
        DiagonalRepresentation diagonalize() const;
    };
}

#endif
