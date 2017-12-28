#include <gtest/gtest.h>
#include <memory>
#include <cmath>
#include <Eigen/Dense>

#include "core/discretization/basis.h"
#include "core/discretization/scaled_basis.h"
#include "core/discretization/orthogonal_legendre.h"
#include "core/state/wave_function.h"
#include "core/state/wigner_function.h"
#include "core/map/map_wavefunction.h"
#include "core/map/map_density_operator.h"
#include "core/propagation/propagator.h"
#include "core/propagation/runge_kutta.h"
#include "core/propagation/diagonal_propagator.h"

using Basis = core::discretization::Basis;
using ScaledBasis = core::discretization::ScaledBasis;
using WaveFunction = core::state::WaveFunction;
using WignerFunction = core::state::WignerFunction;
using DensityOperator = core::state::DensityOperator;
using OrthogonalLegendre = core::discretization::OrthogonalLegendre;
using MapWaveFunction = core::map::MapWaveFunction;
using MapDensityOperator = core::map::MapDensityOperator;

template<class StateClass>
using Propagator = core::propagation::Propagator<StateClass>;

template<class StateClass>
using RungeKutta = core::propagation::RungeKutta<StateClass>;

template<class StateClass>
using DiagonalPropagator = core::propagation::DiagonalPropagator<StateClass>;


TEST(runge_kutta, wavefunction_harmonic_gaussian){ 
    std::shared_ptr<Basis> basis = std::make_shared<ScaledBasis>(std::make_shared<OrthogonalLegendre>(70), -7., 7.); 
    std::shared_ptr<MapWaveFunction> op = std::make_shared<MapWaveFunction>(basis, 1, 1, [](double x){ return 1.; }, 0); 
    *op += MapWaveFunction(basis, 0, 0, [](double x){ return x*x; }, 2);
    *op *= std::complex<double>{0., -1.};

    WaveFunction wf(basis, [](double x){ return exp(-(x-2.)*(x-2.)/2.); }, 0);
    WaveFunction wf_check(basis, [](double x){ return std::complex<double>{0., -exp(-(x+2.)*(x+2.)/2.) }; }, 0);

    RungeKutta<WaveFunction> prop(op, "RK4");

    wf.plot_to_terminal();

    auto hist = prop.propagate(wf, 0., M_PI/2.); 

    wf.plot_to_terminal();

    EXPECT_NEAR((wf-wf_check).norm(), 0., 1.e-5);
    EXPECT_TRUE(hist.avg()>1.e-5);
}

#ifndef _EXCLUDE_EXPENSIVE_TESTS_

TEST(runge_kutta, density_operator_harmonic_gaussian){ 
    std::shared_ptr<Basis> basis = std::make_shared<ScaledBasis>(std::make_shared<OrthogonalLegendre>(70), -7., 7.); 
    std::shared_ptr<MapDensityOperator> op = std::make_shared<MapDensityOperator>(basis);

    op->add_left(1, 1, [](double x){ return 1.; }, 0);
    op->add_left(0, 0, [](double x){ return x*x; }, 2);
    op->add_right(1, 1, [](double x){ return -1.; }, 0);
    op->add_right(0, 0, [](double x){ return -x*x; }, 2);
    *op *= std::complex<double>{0., -1.};

    std::shared_ptr<WaveFunction> wf = std::make_shared<WaveFunction>(basis, [](double x){ return exp(-(x-2.)*(x-2.)/2.); }, 0);
    std::shared_ptr<WaveFunction> wf_check = std::make_shared<WaveFunction>(basis, [](double x){ return std::complex<double>{0., -exp(-(x+2.)*(x+2.)/2.) }; }, 0);

    DensityOperator rho{{{1., wf}}};
    DensityOperator rho_check{{{1., wf_check}}};

    RungeKutta<DensityOperator> prop(op, "RK4");

    {WignerFunction wigner{ rho, 128 }; wigner.plot_to_terminal();}

    double t_final = M_PI/2.;
    double t_step = 1.e-5;
    prop.propagate_const_step(rho, 0., t_final, t_step);

    {WignerFunction wigner{ rho, 128 }; wigner.plot_to_terminal();}

    EXPECT_NEAR((rho-rho_check).norm(), 0., 1.e-5);
}

#endif

TEST(runge_kutta, consistency){
    std::shared_ptr<Basis> basis = std::make_shared<ScaledBasis>(std::make_shared<OrthogonalLegendre>(70), -10., 10.);
    std::shared_ptr<MapWaveFunction> op = std::make_shared<MapWaveFunction>(basis, 1, 1, [](double x){ return 1.; }, 0);
    *op += MapWaveFunction(basis, 0, 0, [](double x){ return x*x; }, 2);
    *op *= std::complex<double>{0., -1.};

    WaveFunction wf(basis, [](double x){ return exp(-x*x)*x; }, 0);

    RungeKutta<WaveFunction> prop(op, "RK4");
    DiagonalPropagator<WaveFunction> prop_check(op);

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
