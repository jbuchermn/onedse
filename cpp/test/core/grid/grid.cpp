#include <gtest/gtest.h>

#include <memory>
#include <cmath>

#include "core/grid/grid.h"

using namespace core::grid;
using core::state::WignerFunction;

namespace{
    const std::complex<double> I(0., 1.);

    std::shared_ptr<Schroedinger1D> harmonic(std::shared_ptr<Grid> g){
        auto sys = std::make_shared<Schroedinger1D>(g);
        sys->set_potential([](double x){ return .5*x*x; });
        return sys;
    }
}

TEST(grid_wavefunction, gaussian_moments){
    auto g = std::make_shared<Grid>(-20., 20., 512);
    auto psi = GridWaveFunction::gaussian(g, 1.5, -2., .8);

    EXPECT_NEAR(psi.norm(), 1., 1.e-12);
    EXPECT_NEAR(psi.expect_x(), 1.5, 1.e-10);
    EXPECT_NEAR(psi.expect_p(), -2., 1.e-10);
    EXPECT_NEAR(psi.sigma_x(), .8, 1.e-10);
    EXPECT_NEAR(psi.sigma_x()*psi.sigma_p(), .5, 1.e-10);
    EXPECT_NEAR(psi.probability(1.5, 20.), .5, .02);
}

TEST(grid_wavefunction, momentum_space){
    auto g = std::make_shared<Grid>(-20., 20., 512);
    double x0 = 1.5, p0 = 2., sigma = .8;
    auto psi = GridWaveFunction::gaussian(g, x0, p0, sigma);

    Eigen::ArrayXd p;
    Eigen::ArrayXcd phi;
    psi.momentum_space(p, phi);

    double dp = p(1) - p(0);
    EXPECT_NEAR(phi.abs2().sum()*dp, 1., 1.e-10);
    for(int i=0; i<p.size(); i++){
        // |phi(p)|^2 is a Gaussian of width 1/(2 sigma) around p0
        double sp = .5/sigma;
        double expected = std::exp(-std::pow(p(i)-p0, 2.)/(2.*sp*sp))/std::sqrt(2.*M_PI*sp*sp);
        EXPECT_NEAR(std::norm(phi(i)), expected, 1.e-9);
    }
}

TEST(grid_wavefunction, wigner){
    auto g = std::make_shared<Grid>(-16., 16., 512);
    double x0 = 2., p0 = 1.5, sigma = .7;
    auto psi = GridWaveFunction::gaussian(g, x0, p0, sigma);

    WignerFunction w = psi.wigner(128);
    ASSERT_EQ(w.matrix.rows(), 128);
    EXPECT_NEAR(w.norm(), 1., 1.e-9);

    // W(x, p) = 1/pi exp(-(x-x0)^2/(2 sigma^2) - 2 sigma^2 (p-p0)^2)
    double dx = (w.get_upper_x() - w.get_lower_x())/128;
    double dp = (w.get_upper_p() - w.get_lower_p())/128;
    for(int i=0; i<128; i+=3){
        for(int j=0; j<128; j+=3){
            double x = w.get_lower_x() + i*dx;
            double p = w.get_lower_p() + j*dp;
            double expected = 1./M_PI*std::exp(-std::pow(x-x0, 2.)/(2.*sigma*sigma) - 2.*sigma*sigma*std::pow(p-p0, 2.));
            EXPECT_NEAR(w.matrix(i, j), expected, 1.e-8);
        }
    }

    // A pure state as density matrix gives the same Wigner function
    WignerFunction w2 = GridDensityMatrix(psi).wigner(128);
    EXPECT_NEAR((w.matrix - w2.matrix).norm(), 0., 1.e-12);
}

TEST(grid_wavefunction, cat_state_wigner_is_negative){
    auto g = std::make_shared<Grid>(-16., 16., 512);
    GridWaveFunction psi(g, [](double x){ return std::exp(-std::pow(x-4., 2.)/2.) + std::exp(-std::pow(x+4., 2.)/2.); });

    WignerFunction w = psi.wigner(128);
    EXPECT_NEAR(w.norm(), 1., 1.e-9);
    EXPECT_LT(w.matrix.minCoeff(), -.25);
}

TEST(schroedinger_1d, free_spreading){
    auto g = std::make_shared<Grid>(-60., 60., 1024);
    Schroedinger1D sys(g);
    sys.set_mass(2.);

    double sigma = 1., p0 = 1.5, t = 6.;
    auto psi = GridWaveFunction::gaussian(g, -5., p0, sigma);
    sys.propagate(psi, 0., t, .5);  // exact for any time step without potential

    EXPECT_NEAR(psi.norm(), 1., 1.e-12);
    EXPECT_NEAR(psi.expect_x(), -5. + p0/2.*t, 1.e-9);
    EXPECT_NEAR(psi.sigma_x(), sigma*std::sqrt(1. + std::pow(t/(2.*2.*sigma*sigma), 2.)), 1.e-9);
}

TEST(schroedinger_1d, coherent_state){
    auto g = std::make_shared<Grid>(-12., 12., 256);
    auto sys = harmonic(g);

    auto psi0 = GridWaveFunction::gaussian(g, 3., 0., 1./std::sqrt(2.));
    auto psi = psi0;
    double e0 = sys->energy(psi);
    EXPECT_NEAR(e0, .5 + 4.5, 1.e-9);

    // Quarter period: all potential energy has become kinetic
    sys->propagate(psi, 0., M_PI/2., 1.e-3);
    EXPECT_NEAR(psi.expect_x(), 0., 1.e-5);
    EXPECT_NEAR(psi.expect_p(), -3., 1.e-5);
    EXPECT_NEAR(psi.sigma_x(), 1./std::sqrt(2.), 1.e-5);

    // Full period: back at the start up to the phase exp(-i pi)
    sys->propagate(psi, M_PI/2., 2.*M_PI, 1.e-3);
    EXPECT_NEAR(std::abs(psi0.dot(psi) + 1.), 0., 1.e-4);
    EXPECT_NEAR(sys->energy(psi), e0, 1.e-6);
    EXPECT_NEAR(psi.norm(), 1., 1.e-12);
}

TEST(schroedinger_1d, second_order_convergence){
    auto g = std::make_shared<Grid>(-12., 12., 256);
    auto sys = harmonic(g);
    auto psi0 = GridWaveFunction::gaussian(g, 3., 0., .5);

    auto error = [&](double dt){
        auto reference = psi0, psi = psi0;
        sys->propagate(reference, 0., 1., dt/16.);
        sys->propagate(psi, 0., 1., dt);
        return (psi - reference).norm();
    };
    double ratio = error(.02)/error(.01);
    EXPECT_NEAR(ratio, 4., .3);
}

TEST(schroedinger_1d, eigenstates){
    auto g = std::make_shared<Grid>(-12., 12., 256);
    auto sys = harmonic(g);

    auto states = sys->eigenstates(10);
    ASSERT_EQ(states.size(), 10u);
    for(int n=0; n<10; n++){
        EXPECT_NEAR(states[n].first, n + .5, 1.e-9);
        EXPECT_NEAR(states[n].second->norm(), 1., 1.e-12);
        EXPECT_NEAR(sys->energy(*states[n].second), n + .5, 1.e-9);
    }
    EXPECT_NEAR(std::abs(states[2].second->dot(*states[4].second)), 0., 1.e-12);

    // Eigenstates are stationary
    auto psi = *states[3].second;
    sys->propagate(psi, 0., 1., .01);
    EXPECT_NEAR(std::abs(states[3].second->dot(psi)), 1., 1.e-8);
}

TEST(schroedinger_1d, imaginary_time){
    auto g = std::make_shared<Grid>(-12., 12., 256);
    auto sys = harmonic(g);

    auto psi = GridWaveFunction::gaussian(g, 1., 0., 2.);
    sys->relax(psi, .01, 2000);
    EXPECT_NEAR(sys->energy(psi), .5, 1.e-5);
    EXPECT_NEAR(psi.expect_x(), 0., 1.e-6);
}

TEST(schroedinger_1d, time_dependent_potential){
    // A uniformly accelerated frame: V = -F(t) x with F = cos(t) => <p>(t) = sin(t), <x>(t) = 1 - cos(t)
    auto g = std::make_shared<Grid>(-30., 30., 512);
    Schroedinger1D sys(g);
    sys.set_potential_td([](double x, double t){ return -std::cos(t)*x; });

    auto psi = GridWaveFunction::gaussian(g, 0., 0., 1.5);
    sys.propagate(psi, 0., 2., 1.e-3);
    EXPECT_NEAR(psi.expect_p(), std::sin(2.), 1.e-4);
    EXPECT_NEAR(psi.expect_x(), 1. - std::cos(2.), 1.e-4);
}

TEST(schroedinger_1d, absorber){
    auto g = std::make_shared<Grid>(-40., 40., 512);
    Schroedinger1D sys(g);
    sys.set_absorber(10., 2.);

    auto psi = GridWaveFunction::gaussian(g, 0., 3., 2.);
    sys.propagate(psi, 0., 25., .01);
    // Nothing comes back, neither reflected nor through the periodic boundary
    EXPECT_LT(psi.norm(), 1.e-2);
}

TEST(gross_pitaevskii, bright_soliton){
    // psi = sqrt(kappa/2) sech(kappa x) exp(i v x), kappa = |g|/2 is a stationary shape moving with v
    auto g = std::make_shared<Grid>(-30., 30., 1024);
    Schroedinger1D sys(g);
    double gint = -4., kappa = 2., v = .5;
    sys.set_nonlinearity(gint);

    GridWaveFunction psi(g, [&](double x){ return std::exp(I*v*x)/std::cosh(kappa*x); });
    double e0 = sys.energy(psi);
    // E = v^2/2 - g^2/24
    EXPECT_NEAR(e0, v*v/2. - gint*gint/24., 1.e-8);

    double sigma0 = psi.sigma_x();
    sys.propagate(psi, 0., 8., 1.e-3);
    EXPECT_NEAR(psi.expect_x(), 4., 1.e-4);
    EXPECT_NEAR(psi.sigma_x(), sigma0, 1.e-4);
    EXPECT_NEAR(sys.energy(psi), e0, 1.e-5);

    // The same packet without interaction spreads
    Schroedinger1D free(g);
    GridWaveFunction psi_free(g, [&](double x){ return std::exp(I*v*x)/std::cosh(kappa*x); });
    free.propagate(psi_free, 0., 8., 1.e-2);
    EXPECT_GT(psi_free.sigma_x(), 10.*sigma0);
}

TEST(classical_ensemble, harmonic_oscillator){
    auto g = std::make_shared<Grid>(-12., 12., 512);
    auto sys = harmonic(g);

    ClassicalEnsemble ensemble(20000, 3., 0., .7, .7, 42);
    double x0 = ensemble.expect_x(), p0 = ensemble.expect_p();
    double e0 = ensemble.energy(*sys);
    EXPECT_NEAR(x0, 3., .02);
    EXPECT_NEAR(e0, 4.5 + .49/2. + .49/2., .05);

    ensemble.propagate(*sys, 0., M_PI/2., .005);
    EXPECT_NEAR(ensemble.expect_x(), p0, 1.e-3);
    EXPECT_NEAR(ensemble.expect_p(), -x0, 1.e-3);
    EXPECT_NEAR(ensemble.energy(*sys), e0, 1.e-3);

    WignerFunction histogram = ensemble.histogram(*g, 128);
    EXPECT_NEAR(histogram.norm(), 1., 1.e-9);
}

TEST(bohmian_trajectories, free_gaussian){
    // For a spreading Gaussian at rest, trajectories scale with the width: x(t) = x(0) sigma(t)/sigma(0)
    auto g = std::make_shared<Grid>(-40., 40., 1024);
    Schroedinger1D sys(g);
    double sigma = 1.;
    auto psi = GridWaveFunction::gaussian(g, 0., 0., sigma);

    BohmianTrajectories bohm(psi, 9, 0.);
    Eigen::ArrayXd initial = bohm.x;
    EXPECT_NEAR(initial(4), 0., 1.e-3);
    EXPECT_NEAR(initial(0), -initial(8), 1.e-6);

    double dt = .002, t = 0.;
    for(int i=0; i<2000; i++){
        sys.step(psi, t, dt);
        bohm.advance(psi, 1., t, dt);
        t += dt;
    }

    double scale = std::sqrt(1. + std::pow(t/(2.*sigma*sigma), 2.));
    for(int i=0; i<9; i++) EXPECT_NEAR(bohm.x(i), initial(i)*scale, 5.e-3);
    EXPECT_EQ(bohm.history.size(), 2001u);
}

TEST(position_measurement, unravels_the_master_equation){
    // Static cat state (no Hamiltonian): the coherence between the lobes decays as
    // exp(-kappa d^2 t / 2), each single run ends in one lobe, with Born frequencies.
    auto g = std::make_shared<Grid>(-12., 12., 128);
    double d = 6., kappa = .05, t = .5, dt = .01, weight_left = .7;

    GridWaveFunction cat(g, [&](double x){
        return std::sqrt(weight_left)*std::exp(-std::pow(x+d/2., 2.)/2.) + std::sqrt(1.-weight_left)*std::exp(-std::pow(x-d/2., 2.)/2.);
    });
    double coherence0 = GridDensityMatrix(cat).coherence(-d/2., d/2.);

    int runs = 3000, left = 0, undecided = 0;
    GridDensityMatrix average(g);
    for(int r=0; r<runs; r++){
        PositionMeasurement measurement(kappa, r+1);
        GridWaveFunction psi = cat;
        for(int i=0; i<int(t/dt + .5); i++) measurement.apply(psi, dt);
        EXPECT_NEAR(psi.norm(), 1., 1.e-12);
        average.add(1./runs, psi);

        // Run on until the outcome is unambiguous
        for(int i=0; i<600; i++) measurement.apply(psi, dt);
        double p_left = psi.probability(-12., 0.);
        if(p_left>.1 && p_left<.9) undecided++;
        if(p_left>.5) left++;
    }
    // The measurement has a finite resolution (1/sqrt(4 kappa t)): a few runs stay ambiguous
    EXPECT_LE(undecided, runs/50);

    EXPECT_NEAR(average.trace(), 1., 1.e-10);
    EXPECT_NEAR(average.coherence(-d/2., d/2.)/coherence0, std::exp(-kappa*d*d*t/2.), .03);
    // Populations are untouched, only the coherence goes
    EXPECT_NEAR(average.coherence(-d/2., -d/2.), GridDensityMatrix(cat).coherence(-d/2., -d/2.), .03);
    // Born rule: 3 sigma of the binomial distribution is 0.025
    EXPECT_NEAR(double(left)/runs, weight_left, .03);
}
