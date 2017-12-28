#pragma once

#include <memory>
#include <vector>
#include <string>
#include <Eigen/Dense>

#include "core/propagation/propagator.h"

namespace core::propagation{

    template<class StateClass>
    class DiagonalPropagator: public Propagator<StateClass>{
        Eigen::VectorXcd eigenvalues;
        Eigen::MatrixXcd trafo_to_diagonal;
        Eigen::MatrixXcd trafo_from_diagonal;
    public:
        DiagonalPropagator(std::shared_ptr<core::map::Map<StateClass>> map): Propagator<StateClass>(map){
            Eigen::ComplexEigenSolver<Eigen::MatrixXcd> solver(map->matrix_representation());
            eigenvalues = std::move(solver.eigenvalues());
            trafo_from_diagonal = std::move(solver.eigenvectors());
            trafo_to_diagonal = trafo_from_diagonal.inverse();

            if((trafo_from_diagonal * eigenvalues.asDiagonal() * trafo_to_diagonal - map->matrix_representation()).norm()/map->matrix_representation().norm() > 1.e-12) throw std::runtime_error("Diagonalization failed");

        }

        void step(StateClass& state, double t_start, double t_step) override{
            Eigen::VectorXcd exponential(eigenvalues.rows());
            for(int i=0; i<exponential.rows(); i++) exponential(i) = exp(t_step*eigenvalues(i));

            state.vector = trafo_from_diagonal * exponential.asDiagonal() * trafo_to_diagonal *  state.vector;
        }

        typename Propagator<StateClass>::StepSizeHistogram propagate(StateClass& state, double t_start, double t_final, double epsilon=1.e-9) override{
            step(state, t_start, t_final - t_start);

            typename Propagator<StateClass>::StepSizeHistogram result;
            result.add(t_final-t_start);
            return result;
        }
    };
}
