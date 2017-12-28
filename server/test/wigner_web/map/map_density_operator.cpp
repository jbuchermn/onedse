#include <gtest/gtest.h>
#include <memory>

#include "wigner_web/discretization/basis.h"
#include "wigner_web/discretization/orthogonal_legendre.h"
#include "wigner_web/state/wave_function.h"
#include "wigner_web/state/density_operator.h"
#include "wigner_web/map/map_density_operator.h"

using Basis = wigner_web::discretization::Basis;
using OrthogonalLegendre = wigner_web::discretization::OrthogonalLegendre;
using WaveFunction = wigner_web::state::WaveFunction;
using DensityOperator = wigner_web::state::DensityOperator;
using MapDensityOperator = wigner_web::map::MapDensityOperator;

TEST(map_density_operator, matrix){
    std::shared_ptr<Basis> basis = std::make_shared<OrthogonalLegendre>(50);
    std::shared_ptr<WaveFunction> wf = std::make_shared<WaveFunction>(basis, [](double x){ return x*exp(-x*x); }, 0);
    DensityOperator rho{ wf };
    DensityOperator rho_check = rho;

    MapDensityOperator map(basis);
    map.add_left(1, 1, [](double x){ return 1.; }, 0);
    map.add_right(0, 0, [](double x){ return x*x*x*x; }, 4);
    map.add(1, 1, [](double x){ return x; }, 1, 
            0, 1, [](double x){ return std::sqrt(x); }, 2);

    map.apply(rho_check);

    map.setup_matrix();

    map.apply(rho);

    EXPECT_NEAR((rho - rho_check).norm(), 0., 1.e-12);
}
