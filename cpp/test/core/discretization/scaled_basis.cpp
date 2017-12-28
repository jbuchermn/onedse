#include <gtest/gtest.h>

#include "core/discretization/basis.h"
#include "core/discretization/orthogonal_chebyshev.h"
#include "core/discretization/scaled_basis.h"

using Basis = core::discretization::Basis;
using ScaledBasis = core::discretization::ScaledBasis;
using OrthogonalChebyshev = core::discretization::OrthogonalChebyshev;

TEST(scaled_basis, overlap){
    std::shared_ptr<Basis> basis = std::make_shared<OrthogonalChebyshev>(10);
    Eigen::MatrixXcd overlap = basis->get_metric_cov();

    std::shared_ptr<Basis> basis_scaled = std::make_shared<ScaledBasis>(basis, -12.34, 17.73);
    Eigen::MatrixXcd overlap_scaled = basis_scaled->get_metric_cov();

    EXPECT_NEAR((overlap - overlap_scaled).norm(), 0., 1.e-12);
}

TEST(scaled_basis, first_derivative){
    constexpr double jacobian = 12.43;

    std::shared_ptr<Basis> basis = std::make_shared<OrthogonalChebyshev>(10);
    Eigen::MatrixXcd overlap = basis->discretize_op_cov(1, 1, [](double x){ return 1.; }, 0);

    std::shared_ptr<Basis> basis_scaled = std::make_shared<ScaledBasis>(basis, jacobian*basis->lower, jacobian*basis->upper);
    Eigen::MatrixXcd overlap_scaled = basis_scaled->discretize_op_cov(1, 1, [](double x){ return 1.; }, 0);

    EXPECT_NEAR((overlap*std::pow(jacobian, -2) - overlap_scaled).norm(), 0., 1.e-8);

}

TEST(scaled_basis, second_derivative){
    constexpr double jacobian = 12.43;

    std::shared_ptr<Basis> basis = std::make_shared<OrthogonalChebyshev>(10);
    Eigen::MatrixXcd overlap = basis->discretize_op_cov(2, 2, [](double x){ return 1.; }, 0);

    std::shared_ptr<Basis> basis_scaled = std::make_shared<ScaledBasis>(basis, jacobian*basis->lower, jacobian*basis->upper);
    Eigen::MatrixXcd overlap_scaled = basis_scaled->discretize_op_cov(2, 2, [](double x){ return 1.; }, 0);

    EXPECT_NEAR((overlap*std::pow(jacobian, -4) - overlap_scaled).norm(), 0., 1.e-2);
}

TEST(scaled_basis, integrate_weight){
    constexpr double jacobian = 123.1;

    std::shared_ptr<Basis> basis = std::make_shared<OrthogonalChebyshev>(10);
    Eigen::VectorXd points, weights;
    basis->quadrature(0, points, weights);

    double integral = 0.;
    for(int i=0; i<weights.size(); i++) integral+=weights(i);

    std::shared_ptr<Basis> basis_scaled = std::make_shared<ScaledBasis>(basis, basis->lower*jacobian, basis->upper*jacobian);
    Eigen::VectorXd points_scaled, weights_scaled;
    basis_scaled->quadrature(0, points_scaled, weights_scaled);
    
    double integral_scaled = 0.;
    for(int i=0; i<weights_scaled.size(); i++) integral_scaled+=weights_scaled(i);

    EXPECT_NEAR((integral*jacobian - integral_scaled), 0., 1.e-12);
}
