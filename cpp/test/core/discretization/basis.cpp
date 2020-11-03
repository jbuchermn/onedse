#include <gtest/gtest.h>
#include <memory>
#include <functional>
#include <complex>
#include <Eigen/Dense>

#include "core/discretization/basis.h"
#include "core/discretization/scaled_basis.h"
#include "core/discretization/orthogonal_hermite.h"
#include "core/discretization/orthogonal_chebyshev.h"
#include "core/discretization/orthogonal_legendre.h"

using Basis = core::discretization::Basis;
using ScaledBasis = core::discretization::ScaledBasis;
using OrthogonalHermite = core::discretization::OrthogonalHermite;
using OrthogonalChebyshev = core::discretization::OrthogonalChebyshev;
using OrthogonalLegendre = core::discretization::OrthogonalLegendre;

TEST(basis, discretize_polynomial){
    std::shared_ptr<Basis> basis = std::make_shared<OrthogonalLegendre>(10);
    Eigen::VectorXcd cov = basis->discretize_function_cov([](double x){ return std::complex<double>{ x*x, .73*x*x*x }; }, 3);

    for(double x=-1.; x<=1.; x+=.05){
        std::complex<double> val = cov.conjugate().dot(basis->get_metric_contrav()*basis->evaluate(x));
        EXPECT_NEAR(val.real(), x*x, 1.e-12);
        EXPECT_NEAR(val.imag(), .73*x*x*x, 1.e-12);
    }
}

TEST(basis, discretize_gaussian){

    /*
     * Hermites
     */
    {
        double a = 1.;
        std::shared_ptr<Basis> basis = std::make_shared<OrthogonalHermite>(100);
        Eigen::VectorXcd cov = basis->discretize_function_cov([a](double x){ return std::exp(-a*x*x); }, 10);

        for(double x=-10.; x<=10.; x+=.5){
            std::complex<double> val = cov.conjugate().dot(basis->get_metric_contrav()*basis->evaluate(x));
            EXPECT_NEAR(val.real(), std::exp(-a*x*x), 1.e-12);
            EXPECT_NEAR(val.imag(), 0, 1.e-12);
        }
    }
    
    /*
     * Legendres
     */
    {
        double a = 25.;
        std::shared_ptr<Basis> basis = std::make_shared<OrthogonalLegendre>(100);
        Eigen::VectorXcd cov = basis->discretize_function_cov([a](double x){ return std::exp(-a*x*x); }, 0);

        for(double x=-1.; x<=1.; x+=.05){
            std::complex<double> val = cov.conjugate().dot(basis->get_metric_contrav()*basis->evaluate(x));
            EXPECT_NEAR(val.real(), std::exp(-a*x*x), 1.e-12);
            EXPECT_NEAR(val.imag(), 0, 1.e-12);
        }
    }
    
    /*
     * Chebyshevs
     */
    {
        double a = 25.;
        std::shared_ptr<Basis> basis = std::make_shared<OrthogonalChebyshev>(100);
        Eigen::VectorXcd cov = basis->discretize_function_cov([a](double x){ return std::exp(-a*x*x); }, 0);

        for(double x=-0.95; x<=1.; x+=.05){
            std::complex<double> val = cov.conjugate().dot(basis->get_metric_contrav()*basis->evaluate(x));
            EXPECT_NEAR(val.real(), std::exp(-a*x*x), 1.e-12);
            EXPECT_NEAR(val.imag(), 0, 1.e-12);
        }

    }
}

/**
 * Check the eigenvalue equation (-d^2/dx^2 + 2a(2ax^2 - 1))exp(-ax^2)=0
 */
TEST(basis, eigenvalue_equation){

    /*
     * Hermites
     */
    {
        double a = 1.;
        std::shared_ptr<Basis> basis = std::make_shared<OrthogonalHermite>(100);
        Eigen::VectorXcd vec_cov = basis->discretize_function_cov([a](double x){ return std::exp(-a*x*x); }, 0);
        Eigen::MatrixXcd mat_cov = basis->discretize_op_cov(1, 1, [](double x){ return 1.; }, 0) // -d^2/dx^2
            + basis->discretize_op_cov(0, 0, [a](double x){ return 2*a*(2*a*x*x - 1.); }, 2); // + 2a(2ax^2 - 1)

        Eigen::VectorXcd check_cov = mat_cov*basis->get_metric_contrav()*vec_cov;

        double error = check_cov.dot(basis->get_metric_contrav()*check_cov).real();
        EXPECT_NEAR(error, 0, 1.e-9);
    }

    /*
     * Legendres
     */
    {
        double a = 25.;
        std::shared_ptr<Basis> basis = std::make_shared<OrthogonalLegendre>(100);
        Eigen::VectorXcd vec_cov = basis->discretize_function_cov([a](double x){ return std::exp(-a*x*x); }, 0);
        Eigen::MatrixXcd mat_cov = basis->discretize_op_cov(1, 1, [](double x){ return 1.; }, 0) // -d^2/dx^2
            + basis->discretize_op_cov(0, 0, [a](double x){ return 2*a*(2*a*x*x - 1.); }, 2); // + 2a(2ax^2 - 1)

        Eigen::VectorXcd check_cov = mat_cov*basis->get_metric_contrav()*vec_cov;

        double error = check_cov.dot(basis->get_metric_contrav()*check_cov).real();
        EXPECT_NEAR(error, 0, 1.e-9);
    }

    /*
     * Chebyshevs
     */
    {
        double a = 25.;
        std::shared_ptr<Basis> basis = std::make_shared<OrthogonalChebyshev>(100);
        Eigen::VectorXcd vec_cov = basis->discretize_function_cov([a](double x){ return std::exp(-a*x*x); }, 0);
        Eigen::MatrixXcd mat_cov = basis->discretize_op_cov(1, 1, [](double x){ return 1.; }, 0) // -d^2/dx^2
            + basis->discretize_op_cov(0, 0, [a](double x){ return 2*a*(2*a*x*x - 1.); }, 2); // + 2a(2ax^2 - 1)

        Eigen::VectorXcd check_cov = mat_cov*basis->get_metric_contrav()*vec_cov;

        double error = check_cov.dot(basis->get_metric_contrav()*check_cov).real();
        EXPECT_NEAR(error, 0, 1.e-9);

    }
}
