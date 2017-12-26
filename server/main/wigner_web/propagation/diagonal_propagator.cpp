#include <Eigen/Dense>
#include <memory>
#include <exception>
#include <utility>

#include "wigner_web/state/wave_function.h"
#include "wigner_web/propagation/diagonal_propagator.h"
#include "wigner_web/propagation/propagator.h"
#include "wigner_web/map/map_wavefunction.h"

template <class StateClass>
using Propagator = wigner_web::propagation::Propagator<StateClass>;

using WaveFunction = wigner_web::state::WaveFunction;
using MapWaveFunction = wigner_web::map::MapWaveFunction;

namespace wigner_web::propagation{

    DiagonalPropagator::DiagonalPropagator(std::shared_ptr<MapWaveFunction> map): Propagator<WaveFunction>(map){
        Eigen::ComplexEigenSolver<Eigen::MatrixXcd> solver(map->matrix);
        eigenvalues = std::move(solver.eigenvalues());
        trafo_from_diagonal = std::move(solver.eigenvectors());
        trafo_to_diagonal = trafo_from_diagonal.inverse();

        if((trafo_from_diagonal * eigenvalues.asDiagonal() * trafo_to_diagonal - map->matrix).norm()/map->matrix.norm() > 1.e-12) throw std::runtime_error("Diagonalization failed");
    }

    void DiagonalPropagator::step(WaveFunction& state, double t_start, double t_step){
        Eigen::VectorXcd exponential(eigenvalues.rows());
        for(int i=0; i<exponential.rows(); i++) exponential(i) = exp(t_step*eigenvalues(i));

        state.vector = trafo_from_diagonal * exponential.asDiagonal() * trafo_to_diagonal *  state.vector;
    }

    void DiagonalPropagator::propagate(WaveFunction& state, double t_start, double t_final, double& t_step, double epsilon){
        t_step = t_final-t_start;
        step(state, t_start, t_step);
    }
}

