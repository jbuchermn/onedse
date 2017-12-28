#include <gtest/gtest.h>
#include <Eigen/Dense>
#include <cmath>

#include "wigner_web/discretization/orthogonal_chebyshev.h"

using OrthogonalChebyshev = wigner_web::discretization::OrthogonalChebyshev;

TEST(orthogonal_chebyshev, value){
    OrthogonalChebyshev chebyshev(4);
    for(double x=-.95; x<=.95; x+=.05){
        Eigen::VectorXcd vals = chebyshev.evaluate(x);
        Eigen::VectorXcd check(4);
        check << (1.             )*std::pow(1-x*x, -.25),
                 (x              )*std::pow(1-x*x, -.25),
                 (2.*x*x - 1.    )*std::pow(1-x*x, -.25),
                 (4.*x*x*x - 3.*x)*std::pow(1-x*x, -.25);

        EXPECT_TRUE((vals-check).norm()<1.e-12);
    }
}

TEST(orthogonal_chebyshev, first_derivative){
    OrthogonalChebyshev chebyshev(4);
    for(double x=-.95; x<=.95; x+=.05){
        Eigen::VectorXcd vals = chebyshev.evaluate(x, 1);
        Eigen::VectorXcd check(4);
        check << ( (0.          )*std::pow(1-x*x, -.25) + (1.             )*(.5*x)*std::pow(1-x*x, -1.25) ),
                 ( (1           )*std::pow(1-x*x, -.25) + (x              )*(.5*x)*std::pow(1-x*x, -1.25) ),
                 ( (4.*x        )*std::pow(1-x*x, -.25) + (2.*x*x - 1.    )*(.5*x)*std::pow(1-x*x, -1.25) ),
                 ( (12.*x*x - 3.)*std::pow(1-x*x, -.25) + (4.*x*x*x - 3.*x)*(.5*x)*std::pow(1-x*x, -1.25) );

        EXPECT_TRUE((vals-check).norm()<1.e-12);
    }
}

TEST(orthogonal_chebyshev, second_derivative){
    OrthogonalChebyshev chebyshev(4);
    for(double x=-.95; x<=.95; x+=.05){
        Eigen::VectorXcd vals = chebyshev.evaluate(x, 2);
        Eigen::VectorXcd check(4);
        check << ( (0.   )*std::pow(1-x*x, -.25) + 2.*(0.          )*(.5*x)*std::pow(1-x*x, -1.25) + (1.             )*(.5*std::pow(1-x*x, -1.25) + 1.25*x*x*std::pow(1-x*x, -2.25)) ),
                 ( (0    )*std::pow(1-x*x, -.25) + 2.*(1.          )*(.5*x)*std::pow(1-x*x, -1.25) + (x              )*(.5*std::pow(1-x*x, -1.25) + 1.25*x*x*std::pow(1-x*x, -2.25)) ),
                 ( (4.   )*std::pow(1-x*x, -.25) + 2.*(4.*x        )*(.5*x)*std::pow(1-x*x, -1.25) + (2.*x*x - 1.    )*(.5*std::pow(1-x*x, -1.25) + 1.25*x*x*std::pow(1-x*x, -2.25)) ),
                 ( (24.*x)*std::pow(1-x*x, -.25) + 2.*(12.*x*x - 3.)*(.5*x)*std::pow(1-x*x, -1.25) + (4.*x*x*x - 3.*x)*(.5*std::pow(1-x*x, -1.25) + 1.25*x*x*std::pow(1-x*x, -2.25)) );

        EXPECT_TRUE((vals-check).norm()<1.e-12);
    }
}

TEST(orthogonal_chebyshev, overlap){
    OrthogonalChebyshev chebyshev(20);
    Eigen::MatrixXcd overlap = chebyshev.get_metric_cov();
    Eigen::MatrixXcd check = Eigen::MatrixXcd::Zero(20, 20);

    check(0, 0) = M_PI;
    for(int i=1; i<check.rows(); i++) check(i, i) = M_PI/2.;

    EXPECT_TRUE((overlap-check).norm()<1.e-12);
}



