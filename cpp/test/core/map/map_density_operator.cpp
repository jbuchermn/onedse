#include <gtest/gtest.h>
#include <memory>

#include "core/discretization/basis.h"
#include "core/discretization/orthogonal_legendre.h"
#include "core/state/wave_function.h"
#include "core/state/density_operator.h"
#include "core/map/map_density_operator.h"

using Basis = core::discretization::Basis;
using OrthogonalLegendre = core::discretization::OrthogonalLegendre;
using WaveFunction = core::state::WaveFunction;
using DensityOperator = core::state::DensityOperator;
using MapDensityOperator = core::map::MapDensityOperator;

TEST(map_density_operator, matrix){
    std::shared_ptr<Basis> basis = std::make_shared<OrthogonalLegendre>(50);
    std::shared_ptr<WaveFunction> wf = std::make_shared<WaveFunction>(basis, [](double x){ return x*exp(-x*x); }, 0);
    DensityOperator rho{ wf };
    DensityOperator rho_check = rho;

    MapDensityOperator map(basis);
    map.add_left(1, 1, [](double x){ return 1.; }, 0);
    map.add_right(0, 0, [](double x){ return x*x*x*x; }, 4);
    map.add_both(1, 1, [](double x){ return x; }, 1, 
                 0, 1, [](double x){ return std::sqrt(x); }, 2);

    map.apply(rho_check);

    map.setup_matrix();

    map.apply(rho);

    EXPECT_NEAR((rho - rho_check).norm(), 0., 1.e-12);
}
