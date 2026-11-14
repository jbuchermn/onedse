#include <cmath>
#include <stdexcept>

#include "core/grid/grid2d.h"
#include "core/grid/fft.h"
#include "core/utility/interrupt.h"

namespace core::grid{
    namespace{
        const std::complex<double> I(0., 1.);
    }

    /*
     * GridWaveFunction2D
     */
    GridWaveFunction2D::GridWaveFunction2D(std::shared_ptr<const Grid2D> grid_):
        grid(grid_), psi(Eigen::ArrayXXcd::Zero(grid_->x->size, grid_->y->size)){}

    GridWaveFunction2D::GridWaveFunction2D(std::shared_ptr<const Grid2D> grid_, const Eigen::ArrayXXcd& psi_): grid(grid_), psi(psi_){}

    GridWaveFunction2D::GridWaveFunction2D(std::shared_ptr<const Grid2D> grid_, std::function<std::complex<double>(double, double)> f):
        grid(grid_), psi(grid_->x->size, grid_->y->size){
        for(int iy=0; iy<grid->y->size; iy++){
            core::utility::check_interrupt();
            for(int ix=0; ix<grid->x->size; ix++) psi(ix, iy) = f(grid->x->x(ix), grid->y->x(iy));
        }
        normalize();
    }

    GridWaveFunction2D::GridWaveFunction2D(std::shared_ptr<const Grid2D> grid_, const GridWaveFunction& a, const GridWaveFunction& b):
        grid(grid_), psi(grid_->x->size, grid_->y->size){
        if(a.grid->size != grid->x->size || b.grid->size != grid->y->size) throw std::invalid_argument("Product state: grids do not match");
        psi = (a.psi.matrix() * b.psi.matrix().transpose()).array();
        normalize();
    }

    double GridWaveFunction2D::norm() const{
        return std::sqrt(psi.abs2().sum() * grid->cell());
    }

    void GridWaveFunction2D::normalize(){
        double n = norm();
        if(n>1.e-300) psi /= n;
    }

    void GridWaveFunction2D::validate() const{
        if(psi.hasNaN()) throw std::out_of_range("Wavefunction has NaN values");
    }

    double GridWaveFunction2D::expect_x() const{
        Eigen::ArrayXd m = marginal(0);
        double res = 0.;
        for(int i=0; i<m.size(); i++) res += grid->x->x(i)*m(i);
        return res / m.sum();
    }

    double GridWaveFunction2D::expect_y() const{
        Eigen::ArrayXd m = marginal(1);
        double res = 0.;
        for(int i=0; i<m.size(); i++) res += grid->y->x(i)*m(i);
        return res / m.sum();
    }

    double GridWaveFunction2D::probability(double x0, double x1, double y0, double y1) const{
        double res = 0.;
        for(int iy=0; iy<grid->y->size; iy++){
            double y = grid->y->x(iy);
            if(y<y0 || y>y1) continue;
            for(int ix=0; ix<grid->x->size; ix++){
                double x = grid->x->x(ix);
                if(x>=x0 && x<=x1) res += std::norm(psi(ix, iy));
            }
        }
        return res * grid->cell();
    }

    void GridWaveFunction2D::symmetrize(int sign){
        if(grid->x->size != grid->y->size || grid->x->lower != grid->y->lower || grid->x->upper != grid->y->upper){
            throw std::invalid_argument("Symmetrisation requires identical grids for both particles");
        }
        Eigen::ArrayXXcd transposed = psi.transpose();
        psi += double(sign<0 ? -1 : 1)*transposed;
        normalize();
    }

    GridWaveFunction GridWaveFunction2D::slice_x(double x) const{
        int ix = int(std::lround((grid->x->wrap(x) - grid->x->lower)/grid->x->dx())) % grid->x->size;
        return GridWaveFunction(grid->y, psi.row(ix).transpose());
    }

    GridWaveFunction GridWaveFunction2D::slice_y(double y) const{
        int iy = int(std::lround((grid->y->wrap(y) - grid->y->lower)/grid->y->dx())) % grid->y->size;
        return GridWaveFunction(grid->x, psi.col(iy));
    }

    GridWaveFunction2D GridWaveFunction2D::multiply(const GridWaveFunction2D& other) const{
        if(psi.rows() != other.psi.rows() || psi.cols() != other.psi.cols()) throw std::invalid_argument("multiply: grids do not match");
        GridWaveFunction2D result(grid, psi*other.psi);
        result.normalize();
        return result;
    }

    Eigen::ArrayXd GridWaveFunction2D::marginal(int axis) const{
        if(axis==0) return psi.abs2().rowwise().sum() * grid->y->dx();
        return psi.abs2().colwise().sum().transpose() * grid->x->dx();
    }

    GridDensityMatrix GridWaveFunction2D::reduced(int axis) const{
        if(axis==0){
            // rho(x, x') = \int dy psi(x, y) psi^*(x', y)
            return GridDensityMatrix(grid->x, psi.matrix() * psi.matrix().adjoint() * grid->y->dx());
        }else{
            // rho(y, y') = \int dx psi(x, y) psi^*(x, y')
            return GridDensityMatrix(grid->y, psi.matrix().transpose() * psi.matrix().conjugate() * grid->x->dx());
        }
    }

    /*
     * Schroedinger2D
     */
    Schroedinger2D::Schroedinger2D(std::shared_ptr<const Grid2D> grid_):
        grid(grid_), V(Eigen::ArrayXXd::Zero(grid_->x->size, grid_->y->size)), absorber(0, 0){
        if(grid_->x->hbar!=1. || grid_->y->hbar!=1.) throw std::invalid_argument("Schroedinger2D supports hbar = 1 only");
    }

    void Schroedinger2D::set_mass(double mass_x_, double mass_y_){
        if(!(mass_x_>0.) || !(mass_y_>0.)) throw std::invalid_argument("Mass must be positive");
        mass_x = mass_x_;
        mass_y = mass_y_;
        kinetic_phase.resize(0, 0);
    }

    void Schroedinger2D::set_potential(std::function<double(double, double)> V_){
        time_dependent = false;
        for(int iy=0; iy<grid->y->size; iy++){
            core::utility::check_interrupt();
            for(int ix=0; ix<grid->x->size; ix++) V(ix, iy) = V_(grid->x->x(ix), grid->y->x(iy));
        }
    }

    void Schroedinger2D::set_potential_td(std::function<double(double, double, double)> V_){
        potential_td = V_;
        time_dependent = true;
        potential_time = std::nan("");
        set_time(0.);
    }

    void Schroedinger2D::set_time(double t){
        if(!time_dependent || t==potential_time) return;
        for(int iy=0; iy<grid->y->size; iy++){
            for(int ix=0; ix<grid->x->size; ix++) V(ix, iy) = potential_td(grid->x->x(ix), grid->y->x(iy), t);
        }
        potential_time = t;
    }

    void Schroedinger2D::set_absorber(double width, double strength){
        absorber = Eigen::ArrayXXd::Zero(grid->x->size, grid->y->size);
        if(width<=0.) return;
        auto profile = [&](const Grid& g, int i){
            double x = g.x(i);
            double d = std::min(x - g.lower, g.upper - x);
            return d<width ? strength * std::pow(std::cos(.5*M_PI*d/width), 2.) : 0.;
        };
        for(int iy=0; iy<grid->y->size; iy++){
            for(int ix=0; ix<grid->x->size; ix++) absorber(ix, iy) = std::max(profile(*grid->x, ix), profile(*grid->y, iy));
        }
    }

    const Eigen::ArrayXXcd& Schroedinger2D::get_kinetic_phase(std::complex<double> dt) const{
        int nx = grid->x->size, ny = grid->y->size;
        if(kinetic_phase.rows() != nx || kinetic_phase.cols() != ny || kinetic_phase_dt != dt){
            kinetic_phase.resize(nx, ny);
            for(int iy=0; iy<ny; iy++){
                double ky = grid->y->k(iy);
                for(int ix=0; ix<nx; ix++){
                    double kx = grid->x->k(ix);
                    kinetic_phase(ix, iy) = std::exp(-I*dt*(kx*kx/(2.*mass_x) + ky*ky/(2.*mass_y)));
                }
            }
            kinetic_phase_dt = dt;
        }
        return kinetic_phase;
    }

    void Schroedinger2D::apply_potential(GridWaveFunction2D& psi, std::complex<double> dt) const{
        if(nonlinearity != 0.){
            psi.psi *= (-I*dt*(V + nonlinearity*psi.psi.abs2())).exp();
        }else{
            psi.psi *= (-I*dt*V).exp();
        }
        if(absorber.size()>0 && dt.imag()==0.) psi.psi *= (-dt.real()*absorber).exp();
    }

    void Schroedinger2D::step_complex(GridWaveFunction2D& psi, double t, std::complex<double> dt){
        set_time(t + .5*dt.real());

        int nx = grid->x->size, ny = grid->y->size;
        apply_potential(psi, .5*dt);
        fft_2d(psi.psi.data(), nx, ny, -1);
        psi.psi *= get_kinetic_phase(dt);
        fft_2d(psi.psi.data(), nx, ny, +1);
        psi.psi /= double(nx*ny);
        apply_potential(psi, .5*dt);
    }

    void Schroedinger2D::step(GridWaveFunction2D& psi, double t, double dt){
        step_complex(psi, t, dt);
    }

    void Schroedinger2D::propagate(GridWaveFunction2D& psi, double t_start, double t_final, double dt){
        if(!(dt>0.)) throw std::invalid_argument("Time step must be positive");
        int steps = std::max(1, int(std::ceil((t_final-t_start)/dt - 1.e-9)));
        double h = (t_final-t_start)/steps;
        for(int i=0; i<steps; i++){
            core::utility::check_interrupt();
            step_complex(psi, t_start + i*h, h);
        }
    }

    void Schroedinger2D::relax(GridWaveFunction2D& psi, double dt, int steps){
        for(int i=0; i<steps; i++){
            core::utility::check_interrupt();
            step_complex(psi, time_dependent ? potential_time : 0., -I*dt);
            psi.normalize();
        }
    }

    double Schroedinger2D::energy(const GridWaveFunction2D& psi) const{
        int nx = grid->x->size, ny = grid->y->size;

        Eigen::ArrayXXcd phi = psi.psi;
        fft_2d(phi.data(), nx, ny, -1);
        double kinetic = 0.;
        for(int iy=0; iy<ny; iy++){
            double ky = grid->y->k(iy);
            for(int ix=0; ix<nx; ix++){
                double kx = grid->x->k(ix);
                kinetic += (kx*kx/(2.*mass_x) + ky*ky/(2.*mass_y)) * std::norm(phi(ix, iy));
            }
        }
        kinetic *= grid->cell()/(nx*ny);

        double potential = (V * psi.psi.abs2()).sum() * grid->cell();
        double interaction = .5 * nonlinearity * psi.psi.abs2().square().sum() * grid->cell();
        return kinetic + potential + interaction;
    }

    double Schroedinger2D::angular_momentum(const GridWaveFunction2D& psi) const{
        int nx = grid->x->size, ny = grid->y->size;

        // d/dx and d/dy by one-dimensional transforms along the respective axis
        Eigen::ArrayXXcd dx = psi.psi, dy = psi.psi;
        for(int iy=0; iy<ny; iy++){
            Eigen::ArrayXcd col = dx.col(iy);
            fft_1d(col.data(), nx, -1);
            for(int ix=0; ix<nx; ix++) col(ix) *= I*grid->x->k(ix)/double(nx);
            fft_1d(col.data(), nx, +1);
            dx.col(iy) = col;
        }
        for(int ix=0; ix<nx; ix++){
            Eigen::ArrayXcd row = dy.row(ix).transpose();
            fft_1d(row.data(), ny, -1);
            for(int iy=0; iy<ny; iy++) row(iy) *= I*grid->y->k(iy)/double(ny);
            fft_1d(row.data(), ny, +1);
            dy.row(ix) = row.transpose();
        }

        std::complex<double> res = 0.;
        for(int iy=0; iy<ny; iy++){
            double y = grid->y->x(iy);
            for(int ix=0; ix<nx; ix++){
                double x = grid->x->x(ix);
                res += std::conj(psi.psi(ix, iy)) * (-I) * (x*dy(ix, iy) - y*dx(ix, iy));
            }
        }
        return res.real() * grid->cell();
    }
}
