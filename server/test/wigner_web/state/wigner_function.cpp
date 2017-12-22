#include <gtest/gtest.h>

#include <memory>
#include <cmath>

#include "wigner_web/discretization/basis.h"
#include "wigner_web/discretization/orthogonal_legendre.h"
#include "wigner_web/discretization/scaled_basis.h"
#include "wigner_web/state/wave_function.h"
#include "wigner_web/state/wigner_function.h"

using Basis = wigner_web::discretization::Basis;
using ScaledBasis = wigner_web::discretization::ScaledBasis;
using OrthogonalLegendre = wigner_web::discretization::OrthogonalLegendre;
using WaveFunction = wigner_web::state::WaveFunction;
using WignerFunction = wigner_web::state::WignerFunction;

TEST(wigner_function, norm){
    std::shared_ptr<Basis> basis = std::make_shared<ScaledBasis>(std::make_shared<OrthogonalLegendre>(100), -7., 7.);
    WaveFunction wf(basis, [](double x){ return exp(-x*x/2.)*std::pow(M_PI, -.25); }, 0);
    EXPECT_NEAR(wf.norm(), 1., 1.e-12);

    WignerFunction wigner1 {wf, 128};
    WignerFunction wigner2 {wf, 64};

    EXPECT_NEAR(wigner1.norm(), 1., 1.e-9);
    EXPECT_NEAR(wigner2.norm(), 1., 1.e-9);
}

TEST(wigner_function, gaussian){
    std::shared_ptr<Basis> basis = std::make_shared<ScaledBasis>(std::make_shared<OrthogonalLegendre>(100), -7., 7.);
    WaveFunction wf(basis, [](double x){ return exp(-x*x/2.)*std::pow(M_PI, -.25); }, 0);
    EXPECT_NEAR(wf.norm(), 1., 1.e-12);

    WignerFunction wigner1 {wf, 128};
    WignerFunction wigner2 {wf, 64};

    auto expected = [](int x, int size_x, double lower_x, double upper_x, int p, int size_p, double lower_p, double upper_p){
        double value_x = lower_x + (upper_x - lower_x)*x/size_x;
        double value_p = lower_p + (upper_p - lower_p)*p/size_p;

        return exp(-value_x*value_x -value_p*value_p)/M_PI;
    };

    for(int i=0; i<wigner1.matrix.rows(); i++){
        for(int j=0; j<wigner1.matrix.cols(); j++){
            EXPECT_NEAR(wigner1.matrix(i,j), expected(i, wigner1.matrix.rows(), wigner1.get_lower_x(), wigner1.get_upper_x(), j, wigner1.matrix.cols(), wigner1.get_lower_p(), wigner1.get_upper_p()), 1.e-1);
        }
    }
    
    for(int i=0; i<wigner2.matrix.rows(); i++){
        for(int j=0; j<wigner2.matrix.cols(); j++){
            EXPECT_NEAR(wigner2.matrix(i,j), expected(i, wigner2.matrix.rows(), wigner2.get_lower_x(), wigner2.get_upper_x(), j, wigner2.matrix.cols(), wigner2.get_lower_p(), wigner2.get_upper_p()), 1.e-1);
        }
    }
}
