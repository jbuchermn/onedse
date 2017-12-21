#include <gtest/gtest.h>

#include <memory>
#include <cmath>

#include "wigner_web/discretization/basis.h"
#include "wigner_web/discretization/orthogonal_legendre.h"
#include "wigner_web/state/wave_function.h"

using Basis = wigner_web::discretization::Basis;
using OrthogonalLegendre = wigner_web::discretization::OrthogonalLegendre;
using WaveFunction = wigner_web::state::WaveFunction;

TEST(wave_function, norm){
    std::shared_ptr<Basis> basis = std::make_shared<OrthogonalLegendre>(100);
    WaveFunction wf(basis, [](double x){ return exp(-25.*x*x)*std::pow(50./M_PI, .25); }, 0);

    EXPECT_NEAR(wf.norm(), 1., 1.e-12);
}
