#include <gtest/gtest.h>
#include <Eigen/Dense>

#include "wigner_web/discretization/orthogonal_hermite.h"

using OrthogonalHermite = wigner_web::discretization::OrthogonalHermite;

TEST(orthogonal_hermite, value){
    OrthogonalHermite hermite(4);
    for(double x=-10.; x<=10.; x+=.5){
        Eigen::VectorXcd vals = hermite.evaluate(x);
        Eigen::VectorXcd check(4);
        check << 1.,
                 2.*x,
                 4.*x*x - 2.,
                 8.*x*x*x - 12.*x;

        EXPECT_TRUE((vals-check).norm()<1.e-12);
    }
}

TEST(orthogonal_hermite, first_derivative){
    OrthogonalHermite hermite(4);
    for(double x=-10.; x<=10.; x+=.5){
        Eigen::VectorXcd vals = hermite.evaluate(x, 1);
        Eigen::VectorXcd check(4);
        check << 0.,
                 2.,
                 8.*x,
                 24.*x*x - 12.;

        EXPECT_TRUE((vals-check).norm()<1.e-12);
    }
}

TEST(orthogonal_hermite, second_derivative){
    OrthogonalHermite hermite(4);
    for(double x=-10.; x<=10.; x+=.5){
        Eigen::VectorXcd vals = hermite.evaluate(x, 2);
        Eigen::VectorXcd check(4);
        check << 0.,
                 0.,
                 8.,
                 48.*x;

        EXPECT_TRUE((vals-check).norm()<1.e-12);
    }
}

TEST(orthogonal_hermite, overlap){
    OrthogonalHermite hermite(5);
    Eigen::MatrixXcd overlap = hermite.get_metric_cov();
    Eigen::MatrixXcd check = Eigen::MatrixXcd::Zero(5, 5);

    for(int i=0; i<check.rows(); i++) check(i, i) = std::sqrt(M_PI)*std::pow(2., i)*std::tgamma(i+1);

    EXPECT_TRUE((overlap-check).norm()<1.e-12);
}
