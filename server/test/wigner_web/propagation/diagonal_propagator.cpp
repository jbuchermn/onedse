#include <gtest/gtest.h>
#include <memory>
#include <cmath>

#include "wigner_web/discretization/basis.h"
#include "wigner_web/discretization/scaled_basis.h"
#include "wigner_web/discretization/orthogonal_legendre.h"
#include "wigner_web/state/wave_function.h"
#include "wigner_web/map/operator_wavefunction.h"
#include "wigner_web/propagation/diagonal_propagator.h"
#include "wigner_web/propagation/propagator.h"

using Basis = wigner_web::discretization::Basis;
using ScaledBasis = wigner_web::discretization::ScaledBasis;
using WaveFunction = wigner_web::state::WaveFunction;
using OrthogonalLegendre = wigner_web::discretization::OrthogonalLegendre;
using OperatorWaveFunction = wigner_web::map::OperatorWaveFunction;
using DiagonalPropagator = wigner_web::propagation::DiagonalPropagator;

template <class StateClass>
using Propagator = wigner_web::propagation::Propagator<StateClass>;

TEST(diagonal_propagator, wavefunction_harmonic_gaussian){ 
    std::shared_ptr<Basis> basis = std::make_shared<ScaledBasis>(std::make_shared<OrthogonalLegendre>(70), -7., 7.); 
    std::shared_ptr<OperatorWaveFunction> op = std::make_shared<OperatorWaveFunction>(basis, 1, 1, [](double x){ return 1.; }, 0); 
    *op += OperatorWaveFunction(basis, 0, 0, [](double x){ return x*x; }, 2);
    *op *= std::complex<double>{0., -1.};

    WaveFunction wf(basis, [](double x){ return exp(-(x-2.)*(x-2.)/2.); }, 0);
    WaveFunction wf_check = -1.*wf;

    DiagonalPropagator* prop = new DiagonalPropagator(op);

    prop->propagate(wf, 0., M_PI); 

    EXPECT_NEAR((wf-wf_check).norm(), 0., 1.e-5);
}

