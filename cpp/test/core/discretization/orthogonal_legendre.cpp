#include <gtest/gtest.h>
#include <Eigen/Dense>

#include "core/discretization/orthogonal_legendre.h"

using OrthogonalLegendre = core::discretization::OrthogonalLegendre;

TEST(orthogonal_legendre, value){
    OrthogonalLegendre legendre(4);
    for(double x=-1.; x<=1.02; x+=.05){
        Eigen::VectorXcd vals = legendre.evaluate(x);
        Eigen::VectorXcd check(4);
        check << 1.,
                 x,
                 1.5*x*x - .5,
                 2.5*x*x*x - 1.5*x;

        EXPECT_TRUE((vals-check).norm()<1.e-12);
    }
}

TEST(orthogonal_legendre, first_derivative){
    OrthogonalLegendre legendre(4);
    for(double x=-1.; x<=1.; x+=.05){
        Eigen::VectorXcd vals = legendre.evaluate(x, 1);
        Eigen::VectorXcd check(4);
        check << 0.,
                 1.,
                 3.*x,
                 7.5*x*x - 1.5;

        EXPECT_TRUE((vals-check).norm()<1.e-12);
    }
}

TEST(orthogonal_legendre, second_derivative){
    OrthogonalLegendre legendre(4);
    for(double x=-1.; x<=1.; x+=.05){
        Eigen::VectorXcd vals = legendre.evaluate(x, 2);
        Eigen::VectorXcd check(4);
        check << 0.,
                 0.,
                 3.,
                 15.*x;

        EXPECT_TRUE((vals-check).norm()<1.e-12);
    }
}

TEST(orthogonal_legendre, overlap){
    OrthogonalLegendre legendre(20);
    Eigen::MatrixXcd overlap = legendre.get_metric_cov();
    Eigen::MatrixXcd check = Eigen::MatrixXcd::Zero(20, 20);

    for(int i=0; i<check.rows(); i++) check(i, i) = 2./(2.*i+1);

    EXPECT_TRUE((overlap-check).norm()<1.e-12);
}
