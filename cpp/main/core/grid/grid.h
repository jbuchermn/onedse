#pragma once

#include <complex>
#include <functional>
#include <memory>
#include <random>
#include <vector>
#include <utility>
#include <Eigen/Dense>

#include "core/state/wigner_function.h"

/**
 * Fourier pseudo-spectral discretisation on a uniform periodic grid.
 *
 * Complements the Galerkin bases in core::discretization: less general (no
 * arbitrary operators), but fast enough for long propagations, nonlinear
 * equations and two-dimensional configuration spaces.
 *
 * Units: hbar = 1 by default. One-dimensional grids can carry a different value of
 * hbar, to study the classical limit of the same system.
 */
namespace core::grid{

    /**
     * Gaussian coarse-graining of a phase space distribution with resolution (sigma_x, sigma_p):
     * what an apparatus sees that cannot resolve finer structures. For
     * sigma_x sigma_p >= hbar/2 the result is non-negative (Husimi function).
     */
    core::state::WignerFunction smooth(const core::state::WignerFunction& wigner, double sigma_x, double sigma_p);

    class Grid{
    public:
        const double lower;
        const double upper;
        const int size;
        /// Planck's constant: p = hbar k
        const double hbar;

        Grid(double lower_, double upper_, int size_, double hbar_=1.);

        double length() const{ return upper - lower; }
        double dx() const{ return (upper - lower)/size; }
        double x(int i) const{ return lower + i*dx(); }
        /// Wave number of FFT bin i
        double k(int i) const;
        /// Largest representable wave number
        double k_max() const;
        /// Largest representable momentum hbar k_max
        double p_max() const{ return hbar*k_max(); }
        /// Map any coordinate back into [lower, upper)
        double wrap(double x) const;
    };

    class GridWaveFunction{
    public:
        std::shared_ptr<const Grid> grid;
        Eigen::ArrayXcd psi;

        GridWaveFunction(std::shared_ptr<const Grid> grid_);
        GridWaveFunction(std::shared_ptr<const Grid> grid_, const Eigen::ArrayXcd& psi_);
        /// Sampled from a function and normalised
        GridWaveFunction(std::shared_ptr<const Grid> grid_, std::function<std::complex<double>(double)> f);

        /// Minimal uncertainty wave packet centred at (x0, p0) with position uncertainty sigma
        static GridWaveFunction gaussian(std::shared_ptr<const Grid> grid, double x0, double p0, double sigma);

        double norm() const;
        void normalize();
        void validate() const;
        std::complex<double> dot(const GridWaveFunction& other) const;

        double expect_x() const;
        double expect_p() const;
        double sigma_x() const;
        double sigma_p() const;
        /// Probability to find the particle in [a, b]
        double probability(double a, double b) const;

        /// Momentum space representation phi(p) on ascending momenta, normalised to \int |phi|^2 dp = 1
        void momentum_space(Eigen::ArrayXd& p, Eigen::ArrayXcd& phi) const;

        /// Wigner function sampled on (approximately) points x points
        core::state::WignerFunction wigner(int points) const;
        /// \int |W| dx dp - 1: zero for classical-looking states, grows with interference
        double negativity(int points) const;
        /// Negativity that survives coarse-graining with resolution (sigma_x, sigma_p)
        double negativity_smoothed(int points, double sigma_x, double sigma_p) const;

        GridWaveFunction& operator/=(const std::complex<double>& scalar){ psi/=scalar;    return *this; }
        GridWaveFunction& operator*=(const std::complex<double>& scalar){ psi*=scalar;    return *this; }
        GridWaveFunction& operator+=(const GridWaveFunction& other)     { psi+=other.psi; return *this; }
        GridWaveFunction& operator-=(const GridWaveFunction& other)     { psi-=other.psi; return *this; }

        GridWaveFunction operator/(const std::complex<double>& scalar) const { return GridWaveFunction{grid, psi/scalar}; }
        GridWaveFunction operator*(const std::complex<double>& scalar) const { return GridWaveFunction{grid, psi*scalar}; }
        GridWaveFunction operator+(const GridWaveFunction& other) const      { return GridWaveFunction{grid, psi+other.psi}; }
        GridWaveFunction operator-(const GridWaveFunction& other) const      { return GridWaveFunction{grid, psi-other.psi}; }
    };

    /**
     * Mixed state rho(x, x') on a grid, e.g. the reduced state of one out of two particles.
     * Normalisation: \int rho(x, x) dx = 1.
     */
    class GridDensityMatrix{
    public:
        std::shared_ptr<const Grid> grid;
        Eigen::MatrixXcd rho;

        GridDensityMatrix(std::shared_ptr<const Grid> grid_, const Eigen::MatrixXcd& rho_);
        GridDensityMatrix(const GridWaveFunction& psi);
        /// Zero operator, to be filled with add()
        GridDensityMatrix(std::shared_ptr<const Grid> grid_);

        /// rho += weight |psi><psi|, e.g. to average over an ensemble of pure states
        void add(double weight, const GridWaveFunction& psi);
        /// |<x_a| rho |x_b>| at the grid points closest to a and b
        double coherence(double a, double b) const;

        double trace() const;
        /// tr rho^2: 1 for pure states, 1/d for a maximally mixed state of dimension d
        double purity() const;
        /// von Neumann entropy -tr rho ln rho
        double entropy() const;
        Eigen::ArrayXd density() const;
        /// Probabilities of the eigen-decomposition (Schmidt coefficients squared for reduced states)
        Eigen::ArrayXd eigenvalues() const;

        core::state::WignerFunction wigner(int points) const;
    };

    /**
     * i d/dt psi = [ p^2/2m + V(x, t) + g |psi|^2 - i W(x) ] psi
     *
     * propagated by the symmetric (Strang) split-operator method. Covers the linear
     * Schroedinger equation (g = 0), the Gross-Pitaevskii / nonlinear Schroedinger equation
     * (g != 0) and scattering setups with absorbing boundaries (W != 0).
     */
    class Schroedinger1D{
        std::function<double(double, double)> potential_td;
        bool time_dependent = false;
        double potential_time = 0.;

        mutable Eigen::ArrayXcd kinetic_phase;
        mutable std::complex<double> kinetic_phase_dt = 0.;

        const Eigen::ArrayXcd& get_kinetic_phase(std::complex<double> dt) const;
        void apply_potential(GridWaveFunction& psi, std::complex<double> dt) const;
        void step_complex(GridWaveFunction& psi, double t, std::complex<double> dt);

    public:
        std::shared_ptr<const Grid> grid;
        double mass = 1.;
        double nonlinearity = 0.;
        Eigen::ArrayXd V;
        Eigen::ArrayXd absorber;

        Schroedinger1D(std::shared_ptr<const Grid> grid_);

        void set_mass(double mass_);
        void set_nonlinearity(double g){ nonlinearity = g; }
        void set_potential(std::function<double(double)> V_);
        void set_potential_td(std::function<double(double, double)> V_);
        /// Smooth complex absorbing layer of given width at both ends of the grid
        void set_absorber(double width, double strength);
        /// Evaluate V(x, t) on the grid (no-op for static potentials)
        void set_time(double t);

        /// One Strang step
        void step(GridWaveFunction& psi, double t, double dt);
        /// Potential part only, exp(-i V tau / hbar): a "kick" of strength tau
        void kick(GridWaveFunction& psi, double tau) const;
        /// Kinetic part only (exact for any tau)
        void drift(GridWaveFunction& psi, double tau) const;
        void propagate(GridWaveFunction& psi, double t_start, double t_final, double dt);
        /// Imaginary time propagation: converges towards the ground state
        void relax(GridWaveFunction& psi, double dt, int steps);

        double kinetic_energy(const GridWaveFunction& psi) const;
        double potential_energy(const GridWaveFunction& psi) const;
        double interaction_energy(const GridWaveFunction& psi) const;
        double energy(const GridWaveFunction& psi) const;

        /// Dense Hamiltonian matrix in the grid representation (linear part, real symmetric)
        Eigen::MatrixXd hamiltonian() const;
        /// Lowest n eigenstates of the linear Hamiltonian by full diagonalisation
        std::vector<std::pair<double, std::shared_ptr<GridWaveFunction>>> eigenstates(int n) const;

        /// Classical force -dV/dx at arbitrary position (linear interpolation)
        double force(double x) const;
    };

    /**
     * Ensemble of classical point particles: the classical Liouville counterpart of the
     * Wigner function. Initialised from a Gaussian phase space distribution, which is
     * exactly the Wigner function of a Gaussian wave packet.
     */
    class ClassicalEnsemble{
    public:
        Eigen::ArrayXd x;
        Eigen::ArrayXd p;
        /// Weights summing to one. Negative for particles sampled from negative regions of a Wigner function.
        Eigen::ArrayXd weight;

        ClassicalEnsemble(int n, double x0, double p0, double sigma_x, double sigma_p, int seed);

        /**
         * Sample a Wigner function, including its negative parts (as particles with negative
         * weight). Propagating this ensemble shows what classical mechanics would do to
         * the very same initial state ("truncated Wigner approximation").
         */
        ClassicalEnsemble(const core::state::WignerFunction& wigner, int n, int seed);

        /// Momentum change by the force of the system's potential acting for a time tau
        void kick(const Schroedinger1D& system, double tau);
        /// Free motion for a time tau
        void drift(const Schroedinger1D& system, double tau);
        double expect_p2() const{ return (weight*p*p).sum(); }
        /// 1/2 \int |W_ensemble - W| dx dp: 0 for identical, 1 for non-overlapping distributions
        double distance(const core::state::WignerFunction& wigner) const;
        /// The same after coarse-graining both with resolution (sigma_x, sigma_p)
        double distance_smoothed(const core::state::WignerFunction& wigner, double sigma_x, double sigma_p) const;
        /// Histogram on the phase space grid of a given Wigner function
        core::state::WignerFunction histogram_like(const core::state::WignerFunction& wigner) const;

        /// Velocity-Verlet in the potential of the given system
        void propagate(Schroedinger1D& system, double t_start, double t_final, double dt);

        double expect_x() const{ return (weight*x).sum(); }
        double expect_p() const{ return (weight*p).sum(); }
        double probability(double a, double b) const;
        double energy(const Schroedinger1D& system) const;

        /// Histogram on the same phase space grid as GridWaveFunction::wigner(points)
        core::state::WignerFunction histogram(const Grid& grid, int points) const;
    };

    /**
     * De Broglie-Bohm trajectories: particles guided by the velocity field j/|psi|^2.
     * Initial positions are the quantiles of |psi|^2, such that the trajectories
     * follow the probability flow.
     */
    class BohmianTrajectories{
    public:
        Eigen::ArrayXd x;
        std::vector<double> times;
        std::vector<Eigen::ArrayXd> history;

        BohmianTrajectories(const GridWaveFunction& psi, int n, double t);

        /// Move particles by dt in the velocity field of psi and record their positions
        void advance(const GridWaveFunction& psi, double mass, double t, double dt);
    };

    /**
     * Continuous measurement of position: one "quantum trajectory" of the master equation
     *
     *   d rho/dt = -i [H, rho] - kappa/2 [x, [x, rho]]
     *
     * The environment monitors x; conditioned on what it has recorded, the system stays in
     * a pure state that follows the stochastic Schroedinger equation
     *
     *   d psi = -i H psi dt - kappa/2 (x - <x>)^2 psi dt + sqrt(kappa) (x - <x>) psi dW .
     *
     * Averaging |psi><psi| over many realisations of the noise gives back rho.
     */
    class PositionMeasurement{
        std::mt19937_64 rng;
        std::normal_distribution<double> normal;

    public:
        double kappa;

        PositionMeasurement(double kappa_, int seed);

        /// Apply the measurement part of the equation for a time dt (interleave with Schroedinger1D::step)
        void apply(GridWaveFunction& psi, double dt);
    };

    /// Records |psi(x, t)|^2 over time ("quantum carpet")
    class SpaceTime{
    public:
        double lower = 0.;
        double upper = 0.;
        int max_points;
        std::vector<double> times;
        std::vector<Eigen::ArrayXd> rows;

        SpaceTime(int max_points_): max_points(max_points_){}
        void add(const GridWaveFunction& psi, double t);
    };
}
