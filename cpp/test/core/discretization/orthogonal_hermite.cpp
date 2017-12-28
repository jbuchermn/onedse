#include <gtest/gtest.h>
#include <Eigen/Dense>
#include <iostream>

#include "core/discretization/orthogonal_hermite.h"

using OrthogonalHermite = core::discretization::OrthogonalHermite;

TEST(orthogonal_hermite, value){
    OrthogonalHermite hermite(4);
    for(double x=-10.; x<=10.; x+=.5){
        Eigen::VectorXcd vals = hermite.evaluate(x);
        Eigen::VectorXcd check(4);
        check << (1.              )*std::exp(-x*x/2.)            ,
                 (2.*x            )*std::exp(-x*x/2.) /sqrt(2.)  ,
                 (4.*x*x - 2.     )*std::exp(-x*x/2.) /sqrt(8.)  ,
                 (8.*x*x*x - 12.*x)*std::exp(-x*x/2.) /sqrt(48.) ;

        EXPECT_TRUE((vals-check).norm()<1.e-12);
    }
}

TEST(orthogonal_hermite, first_derivative){
    OrthogonalHermite hermite(4);
    for(double x=-10.; x<=10.; x+=.5){
        Eigen::VectorXcd vals = hermite.evaluate(x, 1);
        Eigen::VectorXcd check(4);
        check <<( ((1.              )*(-x)*std::exp(-x*x/2.) + (0.           )*std::exp(-x*x/2.))           ),
                ( ((2.*x            )*(-x)*std::exp(-x*x/2.) + (2.           )*std::exp(-x*x/2.))/sqrt(2.)  ),
                ( ((4.*x*x - 2.     )*(-x)*std::exp(-x*x/2.) + (8.*x         )*std::exp(-x*x/2.))/sqrt(8.)  ),
                ( ((8.*x*x*x - 12.*x)*(-x)*std::exp(-x*x/2.) + (24.*x*x - 12.)*std::exp(-x*x/2.))/sqrt(48.) );

        EXPECT_TRUE((vals-check).norm()<1.e-12);
    }
}

TEST(orthogonal_hermite, second_derivative){
    OrthogonalHermite hermite(4);
    for(double x=-10.; x<=10.; x+=.5){
        Eigen::VectorXcd vals = hermite.evaluate(x, 2);
        Eigen::VectorXcd check(4);
        check <<( ((1.              )*(x*x - 1.)*std::exp(-x*x/2.) + 2.*(0.           )*(-x)*std::exp(-x*x/2.) + (0.   )*std::exp(-x*x/2.))           ),
                ( ((2.*x            )*(x*x - 1.)*std::exp(-x*x/2.) + 2.*(2.           )*(-x)*std::exp(-x*x/2.) + (0.   )*std::exp(-x*x/2.))/sqrt(2.)  ),
                ( ((4.*x*x - 2.     )*(x*x - 1.)*std::exp(-x*x/2.) + 2.*(8.*x         )*(-x)*std::exp(-x*x/2.) + (8.   )*std::exp(-x*x/2.))/sqrt(8.)  ),
                ( ((8.*x*x*x - 12.*x)*(x*x - 1.)*std::exp(-x*x/2.) + 2.*(24.*x*x - 12.)*(-x)*std::exp(-x*x/2.) + (48.*x)*std::exp(-x*x/2.))/sqrt(48.) );

        EXPECT_TRUE((vals-check).norm()<1.e-12);
    }
}

TEST(orthogonal_hermite, overlap){
    OrthogonalHermite hermite(20);
    Eigen::MatrixXcd overlap = hermite.get_metric_cov();
    Eigen::MatrixXcd check = Eigen::MatrixXcd::Zero(20, 20);

    for(int i=0; i<check.rows(); i++) check(i, i) = std::sqrt(M_PI);

    EXPECT_TRUE((overlap-check).norm()<1.e-12);
}
