#include <gtest/gtest.h>

#include <memory>
#include <cmath>

#include "core/grid/dirac.h"

using namespace core::grid;

TEST(dirac, positive_energy_packet){
    auto g = std::make_shared<Grid>(-60., 60., 1024);
    Dirac1D dirac(g);

    double p0 = 1.5;
    auto packet = GridWaveFunction::gaussian(g, -10., p0, 3.);
    GridSpinor psi(packet, packet*0.);
    EXPECT_NEAR(psi.norm(), 1., 1.e-12);

    double fraction = dirac.positive_energy_fraction(psi);
    EXPECT_GT(fraction, .5);
    EXPECT_LT(fraction, .99);

    dirac.project(psi, 1);
    EXPECT_NEAR(dirac.positive_energy_fraction(psi), 1., 1.e-12);
    EXPECT_NEAR(psi.norm(), 1., 1.e-12);

    double E = std::sqrt(1. + p0*p0);
    EXPECT_NEAR(dirac.energy(psi), E, .02);
    EXPECT_NEAR(dirac.velocity(psi), p0/E, .02);

    double x0 = psi.expect_x(), e0 = dirac.energy(psi), v0 = dirac.velocity(psi);
    dirac.propagate(psi, 0., 10., .5);    // free propagation is exact
    EXPECT_NEAR(psi.norm(), 1., 1.e-12);
    EXPECT_NEAR(dirac.energy(psi), e0, 1.e-10);
    // No Zitterbewegung for positive energies: constant velocity below c
    EXPECT_NEAR(dirac.velocity(psi), v0, 1.e-10);
    EXPECT_NEAR(psi.expect_x(), x0 + v0*10., 1.e-8);
}

TEST(dirac, zitterbewegung){
    auto g = std::make_shared<Grid>(-40., 40., 1024);
    Dirac1D dirac(g);

    auto packet = GridWaveFunction::gaussian(g, 0., 1., 1.);
    GridSpinor psi(packet, packet*0.);

    // <sigma_x> vanishes initially and then oscillates
    EXPECT_NEAR(dirac.velocity(psi), 0., 1.e-12);
    double min = 0., max = 0.;
    for(int i=0; i<100; i++){
        dirac.propagate(psi, 0., .05, .05);
        min = std::min(min, dirac.velocity(psi));
        max = std::max(max, dirac.velocity(psi));
    }
    EXPECT_GT(max - min, .3);
    EXPECT_NEAR(psi.norm(), 1., 1.e-12);
}

TEST(dirac, nonrelativistic_limit){
    // For c -> infinity the upper component follows the Schroedinger equation
    auto g = std::make_shared<Grid>(-20., 20., 512);
    Dirac1D dirac(g);
    dirac.set_c(40.);
    dirac.set_potential([](double x){ return .5*x*x; });

    Schroedinger1D schroedinger(g);
    schroedinger.set_potential([](double x){ return .5*x*x; });

    auto packet = GridWaveFunction::gaussian(g, 2., 0., 1./std::sqrt(2.));
    GridSpinor psi(packet, packet*0.);
    dirac.project(psi, 1);

    dirac.propagate(psi, 0., 2., 2.e-4);
    schroedinger.propagate(packet, 0., 2., 1.e-3);
    EXPECT_NEAR(psi.expect_x(), packet.expect_x(), 5.e-3);
    EXPECT_NEAR(psi.expect_x(), 2.*std::cos(2.), 5.e-3);
}
