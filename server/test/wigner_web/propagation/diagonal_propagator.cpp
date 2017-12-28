#include <gtest/gtest.h>
#include <memory>
#include <cmath>

#include "wigner_web/discretization/basis.h"
#include "wigner_web/discretization/scaled_basis.h"
#include "wigner_web/discretization/orthogonal_legendre.h"
#include "wigner_web/state/wave_function.h"
#include "wigner_web/state/density_operator.h"
#include "wigner_web/state/wigner_function.h"
#include "wigner_web/map/map_wavefunction.h"
#include "wigner_web/map/map_density_operator.h"
#include "wigner_web/propagation/diagonal_propagator.h"
#include "wigner_web/propagation/propagator.h"

using Basis = wigner_web::discretization::Basis;
using ScaledBasis = wigner_web::discretization::ScaledBasis;
using WaveFunction = wigner_web::state::WaveFunction;
using DensityOperator = wigner_web::state::DensityOperator;
using OrthogonalLegendre = wigner_web::discretization::OrthogonalLegendre;
using MapWaveFunction = wigner_web::map::MapWaveFunction;
using MapDensityOperator = wigner_web::map::MapDensityOperator;
using WignerFunction = wigner_web::state::WignerFunction;

template <class StateClass>
using Propagator = wigner_web::propagation::Propagator<StateClass>;

template<class StateClass>
using DiagonalPropagator = wigner_web::propagation::DiagonalPropagator<StateClass>;

TEST(diagonal_propagator, wavefunction_harmonic_gaussian){ 
    std::shared_ptr<Basis> basis = std::make_shared<ScaledBasis>(std::make_shared<OrthogonalLegendre>(70), -7., 7.); 
    std::shared_ptr<MapWaveFunction> op = std::make_shared<MapWaveFunction>(basis, 1, 1, [](double x){ return 1.; }, 0); 
    *op += MapWaveFunction(basis, 0, 0, [](double x){ return x*x; }, 2);
    *op *= std::complex<double>{0., -1.};

    WaveFunction wf(basis, [](double x){ return exp(-(x-2.)*(x-2.)/2.); }, 0);
    WaveFunction wf_check = -1.*wf;

    DiagonalPropagator<WaveFunction> prop(op);

    wf.plot_to_terminal();
    prop.propagate(wf, 0., M_PI/2); 
    wf.plot_to_terminal();
    EXPECT_TRUE((wf-wf_check).norm() > 1.e-2);
    prop.propagate(wf, M_PI/2., M_PI);
    wf.plot_to_terminal();
    EXPECT_NEAR((wf-wf_check).norm(), 0., 1.e-5);
}

#ifndef _EXCLUDE_EXPENSIVE_TESTS_

TEST(diagonal_propagator, density_operator_harmonic_gaussian){ 
    std::shared_ptr<Basis> basis = std::make_shared<ScaledBasis>(std::make_shared<OrthogonalLegendre>(35), -7., 7.); 
    std::shared_ptr<MapDensityOperator> op = std::make_shared<MapDensityOperator>(basis);
    op->add_left(1, 1, [](double x){ return 1.; }, 0); 
    op->add_left(0, 0, [](double x){ return x*x; }, 2);
    op->add_right(1, 1, [](double x){ return -1.; }, 0); 
    op->add_right(0, 0, [](double x){ return -x*x; }, 2);
    *op *= std::complex<double>{0., -1.};

    std::shared_ptr<WaveFunction> wf = std::make_shared<WaveFunction>(basis, [](double x){ return exp(-(x-2.)*(x-2.)/2.); }, 0);
    std::shared_ptr<WaveFunction> wf_check = std::make_shared<WaveFunction>(-1.*(*wf));

    DensityOperator rho{ wf };
    DensityOperator rho_check{ wf_check };

    DiagonalPropagator<DensityOperator> prop(op);

    { WignerFunction wigner{rho, 128}; wigner.plot_to_terminal(); }
    prop.propagate(rho, 0., M_PI/2.); 
    { WignerFunction wigner{rho, 128}; wigner.plot_to_terminal(); }
    EXPECT_TRUE((rho-rho_check).norm() > 1.e-2);
    prop.propagate(rho, M_PI/2., M_PI);
    { WignerFunction wigner{rho, 128}; wigner.plot_to_terminal(); }
    EXPECT_NEAR((rho-rho_check).norm(), 0., 1.e-5);
}

#endif

