#include <gtest/gtest.h>
#include <Eigen/Dense>
#include <cmath>

#include "wigner_web/discretization/orthogonal_chebyshev.h"

using OrthogonalChebyshev = wigner_web::discretization::OrthogonalChebyshev;

TEST(orthogonal_chebyshev, value){
    OrthogonalChebyshev chebyshev(4);
    for(double x=-10.; x<=10.; x+=.5){
        Eigen::VectorXcd vals = chebyshev.evaluate(x);
        Eigen::VectorXcd check(4);
        check << 1.,
                 x,
                 2.*x*x - 1.,
                 4.*x*x*x - 3.*x;

        EXPECT_TRUE((vals-check).norm()<1.e-12);
    }
}

TEST(orthogonal_chebyshev, first_derivative){
    OrthogonalChebyshev chebyshev(4);
    for(double x=-10.; x<=10.; x+=.5){
        Eigen::VectorXcd vals = chebyshev.evaluate(x, 1);
        Eigen::VectorXcd check(4);
        check << 0.,
                 1.,
                 4.*x,
                 12.*x*x - 3.;

        EXPECT_TRUE((vals-check).norm()<1.e-12);
    }
}

TEST(orthogonal_chebyshev, second_derivative){
    OrthogonalChebyshev chebyshev(4);
    for(double x=-10.; x<=10.; x+=.5){
        Eigen::VectorXcd vals = chebyshev.evaluate(x,2);
        Eigen::VectorXcd check(4);
        check << 0.,
                 0.,
                 4.,
                 24.*x;

        EXPECT_TRUE((vals-check).norm()<1.e-12);
    }
}

TEST(orthogonal_chebyshev, overlap){
    OrthogonalChebyshev chebyshev(10);
    Eigen::MatrixXcd overlap = chebyshev.get_metric_cov();
    Eigen::MatrixXcd check = Eigen::MatrixXcd::Zero(10, 10);

    check(0, 0) = M_PI;
    for(int i=1; i<check.rows(); i++) check(i, i) = M_PI/2.;

    EXPECT_TRUE((overlap-check).norm()<1.e-12);
}



