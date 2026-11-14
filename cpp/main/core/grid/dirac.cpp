#include <cmath>
#include <stdexcept>

#include "core/grid/dirac.h"
#include "core/grid/fft.h"
#include "core/utility/interrupt.h"

namespace core::grid{
    namespace{
        const std::complex<double> I(0., 1.);
    }

    GridSpinor::GridSpinor(std::shared_ptr<const Grid> grid_): grid(grid_), up(Eigen::ArrayXcd::Zero(grid_->size)), down(Eigen::ArrayXcd::Zero(grid_->size)){}

    GridSpinor::GridSpinor(const GridWaveFunction& up_, const GridWaveFunction& down_): grid(up_.grid), up(up_.psi), down(down_.psi){
        if(up.size() != down.size()) throw std::invalid_argument("Spinor components live on different grids");
        normalize();
    }

    double GridSpinor::norm() const{
        return std::sqrt((up.abs2().sum() + down.abs2().sum()) * grid->dx());
    }

    void GridSpinor::normalize(){
        double n = norm();
        if(n>1.e-300){ up /= n; down /= n; }
    }

    void GridSpinor::validate() const{
        if(up.hasNaN() || down.hasNaN()) throw std::out_of_range("Spinor has NaN values");
    }

    Eigen::ArrayXd GridSpinor::density() const{
        return up.abs2() + down.abs2();
    }

    double GridSpinor::expect_x() const{
        Eigen::ArrayXd rho = density();
        double res = 0.;
        for(int i=0; i<grid->size; i++) res += grid->x(i)*rho(i);
        return res / rho.sum();
    }

    double GridSpinor::probability(double a, double b) const{
        Eigen::ArrayXd rho = density();
        double res = 0.;
        for(int i=0; i<grid->size; i++){
            double x = grid->x(i);
            if(x>=a && x<=b) res += rho(i);
        }
        return res * grid->dx();
    }


    Dirac1D::Dirac1D(std::shared_ptr<const Grid> grid_): grid(grid_), V(Eigen::ArrayXd::Zero(grid_->size)), absorber(0){
        if(grid_->hbar!=1.) throw std::invalid_argument("Dirac1D supports hbar = 1 only");
    }

    void Dirac1D::set_potential(std::function<double(double)> V_){
        for(int i=0; i<grid->size; i++) V(i) = V_(grid->x(i));
    }

    void Dirac1D::set_absorber(double width, double strength){
        absorber = Eigen::ArrayXd::Zero(grid->size);
        if(width<=0.) return;
        for(int i=0; i<grid->size; i++){
            double x = grid->x(i);
            double d = std::min(x - grid->lower, grid->upper - x);
            if(d<width) absorber(i) = strength * std::pow(std::cos(.5*M_PI*d/width), 2.);
        }
    }

    void Dirac1D::apply_potential(GridSpinor& psi, double dt) const{
        Eigen::ArrayXcd phase = (-I*dt*V).exp();
        if(absorber.size()>0) phase *= (-dt*absorber).exp();
        psi.up *= phase;
        psi.down *= phase;
    }

    void Dirac1D::step(GridSpinor& psi, double dt){
        int n = grid->size;

        apply_potential(psi, .5*dt);

        fft_1d(psi.up.data(), n, -1);
        fft_1d(psi.down.data(), n, -1);
        for(int i=0; i<n; i++){
            // exp(-i dt H_k) with H_k = c k sigma_x + m c^2 sigma_z, H_k^2 = E^2
            double hx = c*grid->k(i);
            double hz = mass*c*c;
            double E = std::sqrt(hx*hx + hz*hz);
            double cos_ = std::cos(E*dt);
            double sinc = E>0. ? std::sin(E*dt)/E : dt;

            std::complex<double> u = psi.up(i), d = psi.down(i);
            psi.up(i)   = (cos_ - I*sinc*hz)*u - I*sinc*hx*d;
            psi.down(i) = (cos_ + I*sinc*hz)*d - I*sinc*hx*u;
        }
        fft_1d(psi.up.data(), n, +1);
        fft_1d(psi.down.data(), n, +1);
        psi.up /= double(n);
        psi.down /= double(n);

        apply_potential(psi, .5*dt);
    }

    void Dirac1D::propagate(GridSpinor& psi, double t_start, double t_final, double dt){
        if(!(dt>0.)) throw std::invalid_argument("Time step must be positive");
        int steps = std::max(1, int(std::ceil((t_final-t_start)/dt - 1.e-9)));
        double h = (t_final-t_start)/steps;
        for(int i=0; i<steps; i++){
            core::utility::check_interrupt();
            step(psi, h);
        }
    }

    void Dirac1D::project(GridSpinor& psi, int sign) const{
        int n = grid->size;
        double s = sign<0 ? -1. : 1.;

        fft_1d(psi.up.data(), n, -1);
        fft_1d(psi.down.data(), n, -1);
        for(int i=0; i<n; i++){
            double hx = c*grid->k(i);
            double hz = mass*c*c;
            double E = std::sqrt(hx*hx + hz*hz);
            if(E==0.) continue;

            // P = (1 + s H_k/E)/2
            std::complex<double> u = psi.up(i), d = psi.down(i);
            psi.up(i)   = .5*((1. + s*hz/E)*u + s*hx/E*d);
            psi.down(i) = .5*((1. - s*hz/E)*d + s*hx/E*u);
        }
        fft_1d(psi.up.data(), n, +1);
        fft_1d(psi.down.data(), n, +1);
        psi.up /= double(n);
        psi.down /= double(n);
        psi.normalize();
    }

    double Dirac1D::positive_energy_fraction(const GridSpinor& psi) const{
        GridSpinor tmp = psi;
        int n = grid->size;
        fft_1d(tmp.up.data(), n, -1);
        fft_1d(tmp.down.data(), n, -1);
        double positive = 0., total = 0.;
        for(int i=0; i<n; i++){
            double hx = c*grid->k(i);
            double hz = mass*c*c;
            double E = std::sqrt(hx*hx + hz*hz);
            std::complex<double> u = tmp.up(i), d = tmp.down(i);
            total += std::norm(u) + std::norm(d);
            if(E==0.){ positive += .5*(std::norm(u) + std::norm(d)); continue; }
            positive += std::norm(.5*((1. + hz/E)*u + hx/E*d)) + std::norm(.5*((1. - hz/E)*d + hx/E*u));
        }
        return positive/total;
    }

    double Dirac1D::velocity(const GridSpinor& psi) const{
        return 2.*c*(psi.up.conjugate()*psi.down).sum().real() * grid->dx();
    }

    double Dirac1D::energy(const GridSpinor& psi) const{
        GridSpinor tmp = psi;
        int n = grid->size;
        fft_1d(tmp.up.data(), n, -1);
        fft_1d(tmp.down.data(), n, -1);
        double res = 0.;
        for(int i=0; i<n; i++){
            double hx = c*grid->k(i);
            double hz = mass*c*c;
            std::complex<double> u = tmp.up(i), d = tmp.down(i);
            res += hz*(std::norm(u) - std::norm(d)) + 2.*hx*(std::conj(u)*d).real();
        }
        res *= grid->dx()/n;
        return res + (V*psi.density()).sum()*grid->dx();
    }
}
