#include <gtest/gtest.h>

#include <memory>
#include <cmath>

#include "core/discretization/basis.h"
#include "core/discretization/orthogonal_legendre.h"
#include "core/state/wave_function.h"

using Basis = core::discretization::Basis;
using OrthogonalLegendre = core::discretization::OrthogonalLegendre;
using WaveFunction = core::state::WaveFunction;

TEST(wave_function, norm){
    std::shared_ptr<Basis> basis = std::make_shared<OrthogonalLegendre>(100);
    WaveFunction wf(basis, [](double x){ return exp(-25.*x*x)*std::pow(50./M_PI, .25); }, 0);

    EXPECT_NEAR(wf.norm(), 1., 1.e-12);
}
