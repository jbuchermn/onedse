#pragma once

#include <memory>
#include <vector>
#include <string>
#include <Eigen/Dense>

#include "wigner_web/state/wave_function.h"
#include "wigner_web/map/map_wavefunction.h"
#include "wigner_web/propagation/propagator.h"

namespace wigner_web::propagation{

    class DiagonalPropagator: public Propagator<wigner_web::state::WaveFunction>{
        Eigen::VectorXcd eigenvalues;
        Eigen::MatrixXcd trafo_to_diagonal;
        Eigen::MatrixXcd trafo_from_diagonal;
    public:
        DiagonalPropagator(std::shared_ptr<wigner_web::map::MapWaveFunction> map);

        void step(wigner_web::state::WaveFunction& state, double t_start, double t_step) override;
        void propagate(wigner_web::state::WaveFunction& state, double t_start, double t_final, double& t_step, double epsilon=1.e-9) override;
        
        // Without this line, overriding propagate hides propagate(WaveFunction&, double, double)
        using Propagator<wigner_web::state::WaveFunction>::propagate;
    };
}
