#include <gtest/gtest.h>
#include <memory>
#include <cmath>
#include <Eigen/Dense>

#include "wigner_web/discretization/basis.h"
#include "wigner_web/discretization/scaled_basis.h"
#include "wigner_web/discretization/orthogonal_legendre.h"
#include "wigner_web/state/wave_function.h"
#include "wigner_web/map/operator_wavefunction.h"
#include "wigner_web/propagation/propagator.h"
#include "wigner_web/propagation/runge_kutta.h"
#include "wigner_web/propagation/diagonal_propagator.h"

using Basis = wigner_web::discretization::Basis;
using ScaledBasis = wigner_web::discretization::ScaledBasis;
using WaveFunction = wigner_web::state::WaveFunction;
using OrthogonalLegendre = wigner_web::discretization::OrthogonalLegendre;
using OperatorWaveFunction = wigner_web::map::OperatorWaveFunction;

template<class StateClass>
using Propagator = wigner_web::propagation::Propagator<StateClass>;

template<class StateClass>
using RungeKutta = wigner_web::propagation::RungeKutta<StateClass>;

using DiagonalPropagator = wigner_web::propagation::DiagonalPropagator;

TEST(runge_kutta, wavefunction_harmonic_gaussian){ 
    std::shared_ptr<Basis> basis = std::make_shared<ScaledBasis>(std::make_shared<OrthogonalLegendre>(70), -7., 7.); 
    std::shared_ptr<OperatorWaveFunction> op = std::make_shared<OperatorWaveFunction>(basis, 1, 1, [](double x){ return 1.; }, 0); 
    *op += OperatorWaveFunction(basis, 0, 0, [](double x){ return x*x; }, 2);
    *op *= std::complex<double>{0., -1.};

    WaveFunction wf(basis, [](double x){ return exp(-(x-2.)*(x-2.)/2.); }, 0);
    WaveFunction wf_check(basis, [](double x){ return std::complex<double>{0., -exp(-(x+2.)*(x+2.)/2.) }; }, 0);

    RungeKutta<WaveFunction> prop("RK4", op);

    double t_step = 1.e-5;
    prop.propagate(wf, 0., M_PI/2., t_step); 

    EXPECT_NEAR((wf-wf_check).norm(), 0., 1.e-5);
    EXPECT_TRUE(t_step>1.e-5);
}

TEST(runge_kutta, consistency){
    std::shared_ptr<Basis> basis = std::make_shared<ScaledBasis>(std::make_shared<OrthogonalLegendre>(70), -10., 10.);
    std::shared_ptr<OperatorWaveFunction> op = std::make_shared<OperatorWaveFunction>(basis, 1, 1, [](double x){ return 1.; }, 0);
    *op += OperatorWaveFunction(basis, 0, 0, [](double x){ return x*x; }, 2);
    *op *= std::complex<double>{0., -1.};

    WaveFunction wf(basis, [](double x){ return exp(-x*x)*x; }, 0);

    RungeKutta<WaveFunction> prop("RK4", op);
    DiagonalPropagator prop_check(op);

    double error1 = 1.;
    double error2 = 1.;
    for(double h=1.; error1+error2>1.e-8 && h>1.e-14; h/=2){
        WaveFunction test1 = wf; 
        prop.step(test1, 0., h);

        WaveFunction test2 = wf;
        prop_check.step(test2, 0., h);
        
        error1 = error2;
        error2 = (test1 - test2).norm();
    }

    double consistency = std::log(error1/error2)/std::log(2.) - 1.;

    EXPECT_NEAR(consistency, prop.get_consistency_order(), .1);
}
