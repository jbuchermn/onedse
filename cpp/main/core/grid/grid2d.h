#pragma once

#include <complex>
#include <functional>
#include <memory>
#include <Eigen/Dense>

#include "core/grid/grid.h"

namespace core::grid{

    /**
     * Product of two periodic grids. Describes either one particle in two dimensions
     * or the configuration space (x1, x2) of two particles in one dimension.
     */
    class Grid2D{
    public:
        std::shared_ptr<const Grid> x;
        std::shared_ptr<const Grid> y;

        Grid2D(std::shared_ptr<const Grid> x_, std::shared_ptr<const Grid> y_): x(x_), y(y_){}
        double cell() const{ return x->dx()*y->dx(); }
    };

    class GridWaveFunction2D{
    public:
        std::shared_ptr<const Grid2D> grid;
        /// psi(ix, iy)
        Eigen::ArrayXXcd psi;

        GridWaveFunction2D(std::shared_ptr<const Grid2D> grid_);
        GridWaveFunction2D(std::shared_ptr<const Grid2D> grid_, const Eigen::ArrayXXcd& psi_);
        GridWaveFunction2D(std::shared_ptr<const Grid2D> grid_, std::function<std::complex<double>(double, double)> f);
        /// Product state psi(x, y) = a(x) b(y)
        GridWaveFunction2D(std::shared_ptr<const Grid2D> grid_, const GridWaveFunction& a, const GridWaveFunction& b);

        double norm() const;
        void normalize();
        void validate() const;

        double expect_x() const;
        double expect_y() const;
        /// Probability inside the rectangle [x0, x1] x [y0, y1]
        double probability(double x0, double x1, double y0, double y1) const;

        /// psi(x, y) -> psi(x, y) + sign * psi(y, x): bosons (+1) or fermions (-1) in 1D
        void symmetrize(int sign);

        /// Cut psi(x = const, y), not normalised
        GridWaveFunction slice_x(double x) const;
        /// Cut psi(x, y = const), not normalised
        GridWaveFunction slice_y(double y) const;
        /// Pointwise product, normalised (e.g. to imprint a phase pattern)
        GridWaveFunction2D multiply(const GridWaveFunction2D& other) const;

        /// Integrate out one coordinate; axis = 0 keeps x, axis = 1 keeps y
        Eigen::ArrayXd marginal(int axis) const;
        /// Reduced density matrix of one coordinate; axis = 0 keeps x, axis = 1 keeps y
        GridDensityMatrix reduced(int axis) const;

        GridWaveFunction2D& operator/=(const std::complex<double>& scalar){ psi/=scalar;    return *this; }
        GridWaveFunction2D& operator*=(const std::complex<double>& scalar){ psi*=scalar;    return *this; }
        GridWaveFunction2D& operator+=(const GridWaveFunction2D& other)   { psi+=other.psi; return *this; }
        GridWaveFunction2D& operator-=(const GridWaveFunction2D& other)   { psi-=other.psi; return *this; }

        GridWaveFunction2D operator/(const std::complex<double>& scalar) const { return GridWaveFunction2D{grid, psi/scalar}; }
        GridWaveFunction2D operator*(const std::complex<double>& scalar) const { return GridWaveFunction2D{grid, psi*scalar}; }
        GridWaveFunction2D operator+(const GridWaveFunction2D& other) const    { return GridWaveFunction2D{grid, psi+other.psi}; }
        GridWaveFunction2D operator-(const GridWaveFunction2D& other) const    { return GridWaveFunction2D{grid, psi-other.psi}; }
    };

    /**
     * i d/dt psi = [ px^2/2mx + py^2/2my + V(x, y, t) + g |psi|^2 - i W(x, y) ] psi
     */
    class Schroedinger2D{
        std::function<double(double, double, double)> potential_td;
        bool time_dependent = false;
        double potential_time = 0.;

        mutable Eigen::ArrayXXcd kinetic_phase;
        mutable std::complex<double> kinetic_phase_dt = 0.;

        const Eigen::ArrayXXcd& get_kinetic_phase(std::complex<double> dt) const;
        void apply_potential(GridWaveFunction2D& psi, std::complex<double> dt) const;
        void step_complex(GridWaveFunction2D& psi, double t, std::complex<double> dt);

    public:
        std::shared_ptr<const Grid2D> grid;
        double mass_x = 1.;
        double mass_y = 1.;
        double nonlinearity = 0.;
        Eigen::ArrayXXd V;
        Eigen::ArrayXXd absorber;

        Schroedinger2D(std::shared_ptr<const Grid2D> grid_);

        void set_mass(double mass_x_, double mass_y_);
        void set_nonlinearity(double g){ nonlinearity = g; }
        void set_potential(std::function<double(double, double)> V_);
        void set_potential_td(std::function<double(double, double, double)> V_);
        void set_absorber(double width, double strength);
        void set_time(double t);

        void step(GridWaveFunction2D& psi, double t, double dt);
        void propagate(GridWaveFunction2D& psi, double t_start, double t_final, double dt);
        void relax(GridWaveFunction2D& psi, double dt, int steps);

        double energy(const GridWaveFunction2D& psi) const;
        /// <Lz> = <x py - y px>
        double angular_momentum(const GridWaveFunction2D& psi) const;
    };
}
