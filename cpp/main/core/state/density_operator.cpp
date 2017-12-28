#include <iostream>
#include <Eigen/Dense>
#include <boost/variant.hpp>

#include "core/state/density_operator.h"
#include "core/state/wave_function.h"
#include "core/discretization/basis.h"

using Basis = core::discretization::Basis;
using State = core::state::State;
using WaveFunction = core::state::WaveFunction;

namespace core::state{

    DensityOperator::DensityOperator(std::shared_ptr<const Basis> basis): State(basis), matrix(Eigen::MatrixXcd::Zero(basis->size, basis->size)), vector(matrix.data(), basis->size*basis->size){}

    DensityOperator::DensityOperator(std::shared_ptr<const Basis> basis, const Eigen::MatrixXcd& matrix_): State(basis), matrix(matrix_), vector(matrix.data(), basis->size*basis->size) {}
    
    DensityOperator::DensityOperator(DiagonalRepresentation wavefunctions): State(wavefunctions[0].second->basis), matrix(Eigen::MatrixXcd::Zero(basis->size, basis->size)), vector(matrix.data(), basis->size*basis->size){
        set_from_wavefunctions(wavefunctions);
    }
        
    DensityOperator::DensityOperator(std::shared_ptr<WaveFunction> wf): State(wf->basis), matrix(Eigen::MatrixXcd::Zero(basis->size, basis->size)), vector(matrix.data(), basis->size*basis->size){
        set_from_wavefunctions(DiagonalRepresentation{{1., wf}});
    }


    double DensityOperator::norm() const{
        double trace = (matrix.conjugate() * basis->get_metric_cov() * matrix * basis->get_metric_cov()).trace().real();

        // Zero operator might get us left with trace = -2.e-18, producing nan in the norm.
        return trace > 0. ? std::sqrt(trace) : 0.;
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
            if(std::abs(solver.eigenvalues()(i))<1.e-6) continue;

            Eigen::VectorXcd components = solver.eigenvectors().block(0, i, solver.eigenvectors().rows(), 1);
            auto wf = std::make_shared<WaveFunction>(basis);
            wf->set_from_components_cov(components);
            result.push_back({ solver.eigenvalues()(i), wf });
        }

        return result;

    }
        
}
