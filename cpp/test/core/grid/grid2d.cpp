#include <gtest/gtest.h>

#include <memory>
#include <cmath>

#include "core/grid/grid2d.h"

using namespace core::grid;

namespace{
    std::shared_ptr<Grid2D> square(double l, int n){
        auto g = std::make_shared<Grid>(-l, l, n);
        return std::make_shared<Grid2D>(g, g);
    }
}

TEST(schroedinger_2d, separable_harmonic_oscillator){
    auto g = square(10., 64);
    Schroedinger2D sys(g);
    sys.set_potential([](double x, double y){ return .5*x*x + 2.*y*y; });   // omega_x = 1, omega_y = 2

    double s = 1./std::sqrt(2.);
    GridWaveFunction2D psi(g, GridWaveFunction::gaussian(g->x, 2., 0., s), GridWaveFunction::gaussian(g->y, 1., 0., s/std::sqrt(2.)));
    EXPECT_NEAR(psi.norm(), 1., 1.e-12);
    double e0 = sys.energy(psi);
    EXPECT_NEAR(e0, .5 + 1. + .5*4. + 2.*1., 1.e-8);

    double t = 1.3;
    sys.propagate(psi, 0., t, 1.e-3);
    EXPECT_NEAR(psi.expect_x(), 2.*std::cos(t), 1.e-5);
    EXPECT_NEAR(psi.expect_y(), 1.*std::cos(2.*t), 1.e-5);
    EXPECT_NEAR(sys.energy(psi), e0, 1.e-5);
    EXPECT_NEAR(psi.norm(), 1., 1.e-12);

    // A product state stays a product state under a separable Hamiltonian
    EXPECT_NEAR(psi.reduced(0).purity(), 1., 1.e-9);
    EXPECT_NEAR(psi.reduced(1).purity(), 1., 1.e-9);
}

TEST(wavefunction_2d, entanglement){
    auto g = square(12., 64);
    auto left  = GridWaveFunction::gaussian(g->x, -4., 0., .7);
    auto right = GridWaveFunction::gaussian(g->x,  4., 0., .7);

    GridWaveFunction2D product(g, left, right);
    EXPECT_NEAR(product.reduced(0).trace(), 1., 1.e-12);
    EXPECT_NEAR(product.reduced(0).purity(), 1., 1.e-10);
    EXPECT_NEAR(product.reduced(0).entropy(), 0., 1.e-8);

    // (|L R> + |R L>)/sqrt(2): maximally entangled in a two-dimensional subspace
    GridWaveFunction2D bosons = product;
    bosons.symmetrize(+1);
    EXPECT_NEAR(bosons.norm(), 1., 1.e-12);
    for(int axis=0; axis<2; axis++){
        GridDensityMatrix rho = bosons.reduced(axis);
        EXPECT_NEAR(rho.trace(), 1., 1.e-10);
        EXPECT_NEAR(rho.purity(), .5, 1.e-8);
        EXPECT_NEAR(rho.entropy(), std::log(2.), 1.e-7);
        EXPECT_NEAR(rho.eigenvalues()(0), .5, 1.e-6);  // the two packets overlap by ~1e-7
        EXPECT_NEAR(rho.eigenvalues()(1), .5, 1.e-6);
    }

    // Fermions never sit at the same place
    GridWaveFunction2D fermions = product;
    fermions.symmetrize(-1);
    for(int i=0; i<64; i++) EXPECT_NEAR(std::abs(fermions.psi(i, i)), 0., 1.e-14);

    // Marginals are consistent with the reduced density matrices
    Eigen::ArrayXd marginal = bosons.marginal(0);
    Eigen::ArrayXd diagonal = bosons.reduced(0).density();
    EXPECT_NEAR((marginal - diagonal).abs().maxCoeff(), 0., 1.e-12);
}

TEST(schroedinger_2d, angular_momentum){
    auto g = square(10., 64);
    Schroedinger2D sys(g);
    sys.set_potential([](double x, double y){ return .5*(x*x + y*y); });

    // (x + i y) exp(-r^2/2): Lz = 1, E = 2
    GridWaveFunction2D psi(g, [](double x, double y){ return std::complex<double>(x, y)*std::exp(-(x*x + y*y)/2.); });
    EXPECT_NEAR(sys.angular_momentum(psi), 1., 1.e-9);
    EXPECT_NEAR(sys.energy(psi), 2., 1.e-9);

    GridWaveFunction2D slice_source = psi;
    GridWaveFunction cut = slice_source.slice_y(0.);
    EXPECT_NEAR(std::abs(cut.psi(32)), 0., 1.e-12);    // node at the origin
}

TEST(schroedinger_2d, imaginary_time){
    auto g = square(10., 64);
    Schroedinger2D sys(g);
    sys.set_potential([](double x, double y){ return .5*(x*x + y*y); });

    GridWaveFunction2D psi(g, [](double x, double y){ return std::exp(-((x-1.)*(x-1.) + y*y)/6.); });
    sys.relax(psi, .01, 1500);
    EXPECT_NEAR(sys.energy(psi), 1., 1.e-4);
}
