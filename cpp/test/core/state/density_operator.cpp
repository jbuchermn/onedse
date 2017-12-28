#include <gtest/gtest.h>

#include <memory>
#include <cmath>
#include <complex>

#include "core/discretization/basis.h"
#include "core/discretization/orthogonal_legendre.h"
#include "core/state/wave_function.h"
#include "core/state/density_operator.h"

using Basis = core::discretization::Basis;
using OrthogonalLegendre = core::discretization::OrthogonalLegendre;
using WaveFunction = core::state::WaveFunction;
using DensityOperator = core::state::DensityOperator;

TEST(density_operator, trace_one){
    std::shared_ptr<Basis> basis = std::make_shared<OrthogonalLegendre>(100);
    std::shared_ptr<WaveFunction> wf1 = std::make_shared<WaveFunction>(basis, [](double x){ return exp(-25.*x*x)*std::pow(50./M_PI, .25); }, 0);
    std::shared_ptr<WaveFunction> wf2 = std::make_shared<WaveFunction>(basis, [](double x){ return exp(-25.*(x-.1)*(x-1.))*std::pow(50./M_PI, .25); }, 0);

    DensityOperator rho{{{.63, wf1}, {.37, wf2}}};
    DensityOperator rho_pure{{{1., wf1}}};

    std::complex<double> trace = (basis->get_metric_cov() * rho.matrix).trace();
    std::complex<double> trace_pure = (basis->get_metric_cov() * rho_pure.matrix).trace();
    EXPECT_NEAR(trace.real(), 1., 1.e-12);
    EXPECT_NEAR(trace.imag(), 0., 1.e-12);
    EXPECT_NEAR(trace_pure.real(), 1., 1.e-12);
    EXPECT_NEAR(trace_pure.imag(), 0., 1.e-12);
}

TEST(density_operator, norm){
    std::shared_ptr<Basis> basis = std::make_shared<OrthogonalLegendre>(100);
    std::shared_ptr<WaveFunction> wf1 = std::make_shared<WaveFunction>(basis, [](double x){ return exp(-25.*x*x)*std::pow(50./M_PI, .25); }, 0);
    std::shared_ptr<WaveFunction> wf2 = std::make_shared<WaveFunction>(basis, [](double x){ return exp(-25.*(x-.1)*(x-1.))*std::pow(50./M_PI, .25); }, 0);

    // Gram-Schmidt orthonormalize
    *wf2 -= wf1->dot(*wf2) * (*wf1);
    *wf2 /= wf2->norm();

    DensityOperator rho{{{.63, wf1}, {.37, wf2}}};
    DensityOperator rho_pure{{{1., wf1}}};

    EXPECT_NEAR(rho.norm(), std::sqrt(.63*.63 + .37*.37), 1.e-12);
    EXPECT_NEAR(rho_pure.norm(), 1., 1.e-12);
}
