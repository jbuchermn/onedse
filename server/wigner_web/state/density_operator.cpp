#include <iostream>
#include <Eigen/Dense>
#include <boost/variant.hpp>

#include "wigner_web/state/density_operator.h"
#include "wigner_web/state/wave_function.h"
#include "wigner_web/discretization/basis.h"

using Basis = wigner_web::discretization::Basis;
using WaveFunction = wigner_web::state::WaveFunction;

namespace wigner_web::state{

    DensityOperator::DensityOperator(std::shared_ptr<const Basis> _basis): basis(_basis){
        matrix = Eigen::MatrixXcd::Zero(basis->size, basis->size);
    }
    DensityOperator::DensityOperator(std::shared_ptr<const Basis> _basis, Eigen::MatrixXcd&& _matrix): basis(_basis), matrix(_matrix){}
    DensityOperator::DensityOperator(DiagonalRepresentation wavefunctions): basis(wavefunctions[0].second->basis){
        set_from_wavefunctions(wavefunctions);
    }
        
    void DensityOperator::add_wavefunction(double probability, std::shared_ptr<WaveFunction> wavefunction){
        matrix += probability * wavefunction->vector * wavefunction->vector.adjoint();
    }

    void DensityOperator::set_from_wavefunctions(DiagonalRepresentation wavefunctions){
        matrix = Eigen::MatrixXcd::Zero(basis->size, basis->size);
        for(auto wf: wavefunctions){
            add_wavefunction(wf.first, wf.second);
        } 
    }

    DensityOperator::DiagonalRepresentation DensityOperator::diagonalize() const{
        DiagonalRepresentation result;

        // Set up eigenvalue eqn in contravariant indices: rho^{ij}psi_j = lambda g^{ik}psi_k to obtain covariant vectors
        Eigen::GeneralizedSelfAdjointEigenSolver<Eigen::MatrixXcd> solver(matrix, basis->get_metric_contrav());
        for(int i=0; i<solver.eigenvalues().rows(); i++){
            if(std::abs(solver.eigenvalues()(i))<1.e-12) continue;

            Eigen::VectorXcd components = solver.eigenvectors().block(0, i, solver.eigenvectors().rows(), 1);
            auto wf = std::make_shared<WaveFunction>(basis);
            wf->set_from_components_cov(components);
            result.push_back({ solver.eigenvalues()(i), wf });
        }

        return result;

    }
        
}
