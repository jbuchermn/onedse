#include <gtest/gtest.h>
#include <memory>
#include <functional>
#include <complex>
#include <Eigen/Dense>

#include "wigner_web/discretization/basis.h"
#include "wigner_web/discretization/scaled_basis.h"
#include "wigner_web/discretization/orthogonal_hermite.h"
#include "wigner_web/discretization/orthogonal_chebyshev.h"
#include "wigner_web/discretization/orthogonal_legendre.h"

using Basis = wigner_web::discretization::Basis;
using ScaledBasis = wigner_web::discretization::ScaledBasis;
using OrthogonalHermite = wigner_web::discretization::OrthogonalHermite;
using OrthogonalChebyshev = wigner_web::discretization::OrthogonalChebyshev;
using OrthogonalLegendre = wigner_web::discretization::OrthogonalLegendre;

TEST(basis, discretize_polynomial){
    std::shared_ptr<Basis> basis = std::make_shared<OrthogonalHermite>(10);
    Eigen::VectorXcd cov = basis->discretize_function_cov([](double x){ return std::complex<double>{ x*x, .73*x*x*x }; }, 3);

    for(double x=-1.; x<=1.; x+=.05){
        std::complex<double> val = cov.conjugate().dot(basis->get_metric_contrav()*basis->evaluate(x));
        EXPECT_NEAR(val.real(), x*x, 1.e-12);
        EXPECT_NEAR(val.imag(), .73*x*x*x, 1.e-12);
    }
}

/**
 * We see, that a Gaussian can be approximated very well by polynomials (without adding quadrature order).
 * But our handling of the Hermite polynomials is numerically very unstable. Going higher than 20 in this
 * test results in even higher errors.
 */
TEST(basis, discretize_gaussian){
    std::shared_ptr<Basis> basis = std::make_shared<OrthogonalHermite>(20);
    Eigen::VectorXcd cov = basis->discretize_function_cov([](double x){ return std::exp(-x*x); }, 0);

    for(double x=-1.; x<=1.; x+=.05){
        std::complex<double> val = cov.conjugate().dot(basis->get_metric_contrav()*basis->evaluate(x));
        EXPECT_NEAR(val.real(), std::exp(-x*x), 1.e-2);
        EXPECT_NEAR(val.imag(), 0, 1.e-2);
    }
    
    basis = std::make_shared<OrthogonalChebyshev>(100);
    cov = basis->discretize_function_cov([](double x){ return std::exp(-25.*x*x); }, 0);

    for(double x=-1.; x<=1.; x+=.05){
        std::complex<double> val = cov.conjugate().dot(basis->get_metric_contrav()*basis->evaluate(x));
        EXPECT_NEAR(val.real(), std::exp(-25.*x*x), 1.e-12);
        EXPECT_NEAR(val.imag(), 0, 1.e-12);
    }
}

/**
 * Check the eigenvalue equation (-d^2/dx^2 + 2a(2ax^2 - 1))exp(-ax^2)=0
 */
TEST(basis, eigenvalue_equation){
    constexpr double a = 25.;

    std::shared_ptr<Basis> basis = std::make_shared<OrthogonalLegendre>(50);
    Eigen::VectorXcd vec_cov = basis->discretize_function_cov([](double x){ return std::exp(-a*x*x); }, 0);
    Eigen::MatrixXcd mat_cov = basis->discretize_op_cov(1, 1, [](double x){ return 1.; }, 0) // -d^2/dx^2
                                + basis->discretize_op_cov(0, 0, [](double x){ return 2*a*(2*a*x*x - 1.); }, 2); // + 2a(2ax^2 - 1)

    for(double x=-1.; x<=1.; x+=0.05){
        std::cout<<x<<"\t"<<basis->evaluate(x, 2).dot(basis->get_metric_contrav()*vec_cov)<<std::endl;
    }

    Eigen::VectorXcd check_cov = mat_cov*basis->get_metric_contrav()*vec_cov;
    
    for(double x=-1.; x<=1.; x+=0.05){
        std::cout<<x<<"\t"<<basis->evaluate(x).dot(basis->get_metric_contrav()*check_cov)<<std::endl;
    }

    double error = check_cov.dot(basis->get_metric_contrav()*check_cov).real();
    EXPECT_NEAR(error, 0, 1.e-9);
}
