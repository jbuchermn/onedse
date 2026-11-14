#pragma once

#include <complex>
#include <functional>
#include <memory>
#include <Eigen/Dense>

#include "core/grid/grid.h"

namespace core::grid{

    /// Two-component spinor of the Dirac equation in 1+1 dimensions
    class GridSpinor{
    public:
        std::shared_ptr<const Grid> grid;
        Eigen::ArrayXcd up;
        Eigen::ArrayXcd down;

        GridSpinor(std::shared_ptr<const Grid> grid_);
        GridSpinor(const GridWaveFunction& up_, const GridWaveFunction& down_);

        double norm() const;
        void normalize();
        void validate() const;
        Eigen::ArrayXd density() const;
        double expect_x() const;
        double probability(double a, double b) const;
    };

    /**
     * i d/dt psi = [ c sigma_x p + m c^2 sigma_z + V(x) ] psi
     *
     * The free part is diagonalised exactly in momentum space, so the split-operator
     * scheme resolves the Zitterbewegung frequency 2mc^2 without a time step restriction.
     */
    class Dirac1D{
        void apply_potential(GridSpinor& psi, double dt) const;

    public:
        std::shared_ptr<const Grid> grid;
        double mass = 1.;
        double c = 1.;
        Eigen::ArrayXd V;
        Eigen::ArrayXd absorber;

        Dirac1D(std::shared_ptr<const Grid> grid_);

        void set_mass(double mass_){ mass = mass_; }
        void set_c(double c_){ c = c_; }
        void set_potential(std::function<double(double)> V_);
        void set_absorber(double width, double strength);

        void step(GridSpinor& psi, double dt);
        void propagate(GridSpinor& psi, double t_start, double t_final, double dt);

        /// Project onto positive (sign > 0) or negative (sign < 0) energy solutions of the free equation
        void project(GridSpinor& psi, int sign) const;
        /// Weight of the positive energy solutions of the free equation
        double positive_energy_fraction(const GridSpinor& psi) const;
        /// <c sigma_x>
        double velocity(const GridSpinor& psi) const;
        double energy(const GridSpinor& psi) const;
    };
}
