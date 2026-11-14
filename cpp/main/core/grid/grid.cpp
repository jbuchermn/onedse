#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>
#include <Eigen/Eigenvalues>

#include "core/grid/grid.h"
#include "core/grid/fft.h"
#include "core/utility/interrupt.h"

using WignerFunction = core::state::WignerFunction;

namespace core::grid{
    namespace{
        const std::complex<double> I(0., 1.);

        /// Smallest stride >= size/points that divides size
        int stride_for(int size, int points){
            int stride = std::max(1, size/std::max(points, 1));
            while(size % stride != 0) stride++;
            return stride;
        }
    }

    /*
     * Grid
     */
    Grid::Grid(double lower_, double upper_, int size_): lower(lower_), upper(upper_), size(size_){
        if(size<2) throw std::invalid_argument("Grid: need at least two points");
        if(!(upper>lower)) throw std::invalid_argument("Grid: upper bound must be larger than lower bound");
    }

    double Grid::k(int i) const{ return wave_number(i, size, length()); }
    double Grid::k_max() const{ return M_PI/dx(); }

    double Grid::wrap(double x) const{
        double L = length();
        double y = std::fmod(x - lower, L);
        if(y<0) y += L;
        return lower + y;
    }

    /*
     * GridWaveFunction
     */
    GridWaveFunction::GridWaveFunction(std::shared_ptr<const Grid> grid_): grid(grid_), psi(Eigen::ArrayXcd::Zero(grid_->size)){}
    GridWaveFunction::GridWaveFunction(std::shared_ptr<const Grid> grid_, const Eigen::ArrayXcd& psi_): grid(grid_), psi(psi_){}

    GridWaveFunction::GridWaveFunction(std::shared_ptr<const Grid> grid_, std::function<std::complex<double>(double)> f): grid(grid_), psi(grid_->size){
        for(int i=0; i<grid->size; i++) psi(i) = f(grid->x(i));
        normalize();
    }

    GridWaveFunction GridWaveFunction::gaussian(std::shared_ptr<const Grid> grid, double x0, double p0, double sigma){
        return GridWaveFunction(grid, [=](double x){
            return std::exp(-(x-x0)*(x-x0)/(4.*sigma*sigma) + I*p0*(x-x0));
        });
    }

    double GridWaveFunction::norm() const{
        return std::sqrt(psi.abs2().sum() * grid->dx());
    }

    void GridWaveFunction::normalize(){
        double n = norm();
        if(n>1.e-300) psi /= n;
    }

    void GridWaveFunction::validate() const{
        if(psi.hasNaN()) throw std::out_of_range("Wavefunction has NaN values");
    }

    std::complex<double> GridWaveFunction::dot(const GridWaveFunction& other) const{
        return (psi.conjugate() * other.psi).sum() * grid->dx();
    }

    double GridWaveFunction::expect_x() const{
        double res = 0.;
        for(int i=0; i<grid->size; i++) res += grid->x(i) * std::norm(psi(i));
        return res / psi.abs2().sum();
    }

    double GridWaveFunction::sigma_x() const{
        double mean = expect_x();
        double res = 0.;
        for(int i=0; i<grid->size; i++) res += std::pow(grid->x(i) - mean, 2.) * std::norm(psi(i));
        return std::sqrt(res / psi.abs2().sum());
    }

    double GridWaveFunction::expect_p() const{
        Eigen::ArrayXcd phi = psi;
        fft_1d(phi.data(), grid->size, -1);
        double res = 0.;
        for(int i=0; i<grid->size; i++) res += grid->k(i) * std::norm(phi(i));
        return res / phi.abs2().sum();
    }

    double GridWaveFunction::sigma_p() const{
        Eigen::ArrayXcd phi = psi;
        fft_1d(phi.data(), grid->size, -1);
        double total = phi.abs2().sum();
        double mean = 0.;
        for(int i=0; i<grid->size; i++) mean += grid->k(i) * std::norm(phi(i));
        mean /= total;
        double res = 0.;
        for(int i=0; i<grid->size; i++) res += std::pow(grid->k(i) - mean, 2.) * std::norm(phi(i));
        return std::sqrt(res / total);
    }

    double GridWaveFunction::probability(double a, double b) const{
        double res = 0.;
        for(int i=0; i<grid->size; i++){
            double x = grid->x(i);
            if(x>=a && x<=b) res += std::norm(psi(i));
        }
        return res * grid->dx();
    }

    void GridWaveFunction::momentum_space(Eigen::ArrayXd& p, Eigen::ArrayXcd& phi) const{
        int n = grid->size;
        Eigen::ArrayXcd tmp = psi;
        fft_1d(tmp.data(), n, -1);

        p = Eigen::ArrayXd(n);
        phi = Eigen::ArrayXcd(n);
        int first_negative = (n+1)/2;
        for(int j=0; j<n; j++){
            int i = (first_negative + j) % n;
            p(j) = grid->k(i);
            phi(j) = tmp(i) * std::exp(-I*p(j)*grid->lower) * grid->dx() / std::sqrt(2.*M_PI);
        }
    }

    WignerFunction GridWaveFunction::wigner(int points) const{
        int stride = stride_for(grid->size, points);
        return WignerFunction::from_correlation(grid->size/stride, grid->lower, grid->upper, false,
                [this, stride](int a, int b){ return psi(a*stride)*std::conj(psi(b*stride)); });
    }

    /*
     * GridDensityMatrix
     */
    GridDensityMatrix::GridDensityMatrix(std::shared_ptr<const Grid> grid_, const Eigen::MatrixXcd& rho_): grid(grid_), rho(rho_){}
    GridDensityMatrix::GridDensityMatrix(const GridWaveFunction& psi): grid(psi.grid), rho(psi.psi.matrix() * psi.psi.matrix().adjoint()){}

    GridDensityMatrix::GridDensityMatrix(std::shared_ptr<const Grid> grid_): grid(grid_), rho(Eigen::MatrixXcd::Zero(grid_->size, grid_->size)){}

    void GridDensityMatrix::add(double weight, const GridWaveFunction& psi){
        if(psi.grid->size != grid->size) throw std::invalid_argument("Density matrix: grids do not match");
        rho += weight * psi.psi.matrix() * psi.psi.matrix().adjoint();
    }

    double GridDensityMatrix::coherence(double a, double b) const{
        auto index = [this](double x){
            return int(std::lround((grid->wrap(x) - grid->lower)/grid->dx())) % grid->size;
        };
        return std::abs(rho(index(a), index(b)));
    }

    double GridDensityMatrix::trace() const{
        return rho.trace().real() * grid->dx();
    }

    double GridDensityMatrix::purity() const{
        return rho.squaredNorm() * grid->dx() * grid->dx();
    }

    Eigen::ArrayXd GridDensityMatrix::eigenvalues() const{
        Eigen::SelfAdjointEigenSolver<Eigen::MatrixXcd> solver(rho * grid->dx(), Eigen::EigenvaluesOnly);
        return solver.eigenvalues().array().reverse();
    }

    double GridDensityMatrix::entropy() const{
        Eigen::ArrayXd ev = eigenvalues();
        double res = 0.;
        for(int i=0; i<ev.size(); i++){
            if(ev(i)>1.e-14) res -= ev(i)*std::log(ev(i));
        }
        return res;
    }

    Eigen::ArrayXd GridDensityMatrix::density() const{
        return rho.diagonal().real().array();
    }

    WignerFunction GridDensityMatrix::wigner(int points) const{
        int stride = stride_for(grid->size, points);
        return WignerFunction::from_correlation(grid->size/stride, grid->lower, grid->upper, false,
                [this, stride](int a, int b){ return rho(a*stride, b*stride); });
    }

    /*
     * Schroedinger1D
     */
    Schroedinger1D::Schroedinger1D(std::shared_ptr<const Grid> grid_): grid(grid_), V(Eigen::ArrayXd::Zero(grid_->size)), absorber(0){}

    void Schroedinger1D::set_mass(double mass_){
        if(!(mass_>0.)) throw std::invalid_argument("Mass must be positive");
        mass = mass_;
        kinetic_phase.resize(0);
    }

    void Schroedinger1D::set_potential(std::function<double(double)> V_){
        time_dependent = false;
        for(int i=0; i<grid->size; i++) V(i) = V_(grid->x(i));
    }

    void Schroedinger1D::set_potential_td(std::function<double(double, double)> V_){
        potential_td = V_;
        time_dependent = true;
        potential_time = std::nan("");
        set_time(0.);
    }

    void Schroedinger1D::set_time(double t){
        if(!time_dependent || t==potential_time) return;
        for(int i=0; i<grid->size; i++) V(i) = potential_td(grid->x(i), t);
        potential_time = t;
    }

    void Schroedinger1D::set_absorber(double width, double strength){
        absorber = Eigen::ArrayXd::Zero(grid->size);
        if(width<=0.) return;
        for(int i=0; i<grid->size; i++){
            double x = grid->x(i);
            double d = std::min(x - grid->lower, grid->upper - x);
            if(d<width) absorber(i) = strength * std::pow(std::cos(.5*M_PI*d/width), 2.);
        }
    }

    const Eigen::ArrayXcd& Schroedinger1D::get_kinetic_phase(std::complex<double> dt) const{
        if(kinetic_phase.size() != grid->size || kinetic_phase_dt != dt){
            kinetic_phase.resize(grid->size);
            for(int i=0; i<grid->size; i++){
                double k = grid->k(i);
                kinetic_phase(i) = std::exp(-I*dt*k*k/(2.*mass));
            }
            kinetic_phase_dt = dt;
        }
        return kinetic_phase;
    }

    void Schroedinger1D::apply_potential(GridWaveFunction& psi, std::complex<double> dt) const{
        if(nonlinearity != 0.){
            psi.psi *= (-I*dt*(V + nonlinearity*psi.psi.abs2())).exp();
        }else{
            psi.psi *= (-I*dt*V).exp();
        }
        if(absorber.size()>0 && dt.imag()==0.) psi.psi *= (-dt.real()*absorber).exp();
    }

    void Schroedinger1D::step_complex(GridWaveFunction& psi, double t, std::complex<double> dt){
        set_time(t + .5*dt.real());

        apply_potential(psi, .5*dt);
        fft_1d(psi.psi.data(), grid->size, -1);
        psi.psi *= get_kinetic_phase(dt);
        fft_1d(psi.psi.data(), grid->size, +1);
        psi.psi /= double(grid->size);
        apply_potential(psi, .5*dt);
    }

    void Schroedinger1D::step(GridWaveFunction& psi, double t, double dt){
        step_complex(psi, t, dt);
    }

    void Schroedinger1D::propagate(GridWaveFunction& psi, double t_start, double t_final, double dt){
        if(!(dt>0.)) throw std::invalid_argument("Time step must be positive");
        int steps = std::max(1, int(std::ceil((t_final-t_start)/dt - 1.e-9)));
        double h = (t_final-t_start)/steps;
        for(int i=0; i<steps; i++){
            core::utility::check_interrupt();
            step_complex(psi, t_start + i*h, h);
        }
    }

    void Schroedinger1D::relax(GridWaveFunction& psi, double dt, int steps){
        for(int i=0; i<steps; i++){
            core::utility::check_interrupt();
            step_complex(psi, potential_time==potential_time ? potential_time : 0., -I*dt);
            psi.normalize();
        }
    }

    double Schroedinger1D::kinetic_energy(const GridWaveFunction& psi) const{
        Eigen::ArrayXcd phi = psi.psi;
        fft_1d(phi.data(), grid->size, -1);
        double res = 0.;
        for(int i=0; i<grid->size; i++){
            double k = grid->k(i);
            res += k*k/(2.*mass) * std::norm(phi(i));
        }
        return res * grid->dx() / grid->size;
    }

    double Schroedinger1D::potential_energy(const GridWaveFunction& psi) const{
        return (V * psi.psi.abs2()).sum() * grid->dx();
    }

    double Schroedinger1D::interaction_energy(const GridWaveFunction& psi) const{
        return .5 * nonlinearity * psi.psi.abs2().square().sum() * grid->dx();
    }

    double Schroedinger1D::energy(const GridWaveFunction& psi) const{
        return kinetic_energy(psi) + potential_energy(psi) + interaction_energy(psi);
    }

    Eigen::MatrixXd Schroedinger1D::hamiltonian() const{
        int n = grid->size;
        if(n>4096) throw std::invalid_argument("Grid too large for dense diagonalisation (max. 4096 points)");

        // Kinetic energy is a circulant matrix: T(a, b) = t((a-b) mod n)
        Eigen::ArrayXcd t(n);
        for(int i=0; i<n; i++){
            double k = grid->k(i);
            // The Nyquist mode of even grids has no well defined sign of k, but k^2 is fine
            t(i) = k*k/(2.*mass);
        }
        fft_1d(t.data(), n, +1);
        t /= double(n);

        Eigen::MatrixXd H(n, n);
        for(int a=0; a<n; a++){
            for(int b=0; b<n; b++) H(a, b) = t(((a-b)%n + n)%n).real();
            H(a, a) += V(a);
        }
        return H;
    }

    std::vector<std::pair<double, std::shared_ptr<GridWaveFunction>>> Schroedinger1D::eigenstates(int n) const{
        Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(hamiltonian());
        if(solver.info() != Eigen::Success) throw std::runtime_error("Diagonalisation of the Hamiltonian failed");

        std::vector<std::pair<double, std::shared_ptr<GridWaveFunction>>> result;
        n = std::min(n, grid->size);
        for(int i=0; i<n; i++){
            Eigen::ArrayXcd psi = solver.eigenvectors().col(i).cast<std::complex<double>>().array() / std::sqrt(grid->dx());

            // Fix the arbitrary sign: largest lobe positive
            Eigen::Index idx;
            psi.abs().maxCoeff(&idx);
            if(psi(idx).real()<0.) psi = -psi;

            result.push_back({ solver.eigenvalues()(i), std::make_shared<GridWaveFunction>(grid, psi) });
        }
        return result;
    }

    double Schroedinger1D::force(double x) const{
        int n = grid->size;
        double dx = grid->dx();
        double pos = (grid->wrap(x) - grid->lower)/dx;
        int i = int(std::floor(pos)) % n;
        double frac = pos - std::floor(pos);

        auto gradient = [&](int j){
            return (V((j+1)%n) - V((j-1+n)%n)) / (2.*dx);
        };
        return -((1.-frac)*gradient(i) + frac*gradient((i+1)%n));
    }

    /*
     * ClassicalEnsemble
     */
    ClassicalEnsemble::ClassicalEnsemble(int n, double x0, double p0, double sigma_x, double sigma_p, int seed): x(n), p(n){
        std::mt19937_64 rng(seed);
        std::normal_distribution<double> normal(0., 1.);
        for(int i=0; i<n; i++){
            x(i) = x0 + sigma_x*normal(rng);
            p(i) = p0 + sigma_p*normal(rng);
        }
    }

    void ClassicalEnsemble::propagate(Schroedinger1D& system, double t_start, double t_final, double dt){
        if(!(dt>0.)) throw std::invalid_argument("Time step must be positive");
        int steps = std::max(1, int(std::ceil((t_final-t_start)/dt - 1.e-9)));
        double h = (t_final-t_start)/steps;

        for(int s=0; s<steps; s++){
            core::utility::check_interrupt();

            system.set_time(t_start + s*h);
            for(int i=0; i<x.size(); i++) p(i) += .5*h*system.force(x(i));
            for(int i=0; i<x.size(); i++) x(i) = system.grid->wrap(x(i) + h*p(i)/system.mass);
            system.set_time(t_start + (s+1)*h);
            for(int i=0; i<x.size(); i++) p(i) += .5*h*system.force(x(i));
        }
    }

    double ClassicalEnsemble::probability(double a, double b) const{
        int count = 0;
        for(int i=0; i<x.size(); i++) if(x(i)>=a && x(i)<=b) count++;
        return double(count)/x.size();
    }

    double ClassicalEnsemble::energy(const Schroedinger1D& system) const{
        const Grid& g = *system.grid;
        double res = 0.;
        for(int i=0; i<x.size(); i++){
            double pos = (g.wrap(x(i)) - g.lower)/g.dx();
            int j = int(std::floor(pos)) % g.size;
            double frac = pos - std::floor(pos);
            res += p(i)*p(i)/(2.*system.mass) + (1.-frac)*system.V(j) + frac*system.V((j+1)%g.size);
        }
        return res/x.size();
    }

    WignerFunction ClassicalEnsemble::histogram(const Grid& grid, int points) const{
        int m = grid.size/stride_for(grid.size, points);

        double lower_x = grid.lower;
        double upper_x = grid.upper;
        double upper_p =  .5*M_PI/(upper_x-lower_x) * m;
        double lower_p = -upper_p;
        double bin_x = (upper_x-lower_x)/m;
        double bin_p = (upper_p-lower_p)/m;

        Eigen::MatrixXd matrix = Eigen::MatrixXd::Zero(m, m);
        double weight = 1./(x.size()*bin_x*bin_p);
        for(int i=0; i<x.size(); i++){
            int ix = int(std::lround((x(i)-lower_x)/bin_x));
            int ip = int(std::lround((p(i)-lower_p)/bin_p));
            if(ix>=0 && ix<m && ip>=0 && ip<m) matrix(ix, ip) += weight;
        }

        return WignerFunction{lower_x, upper_x, lower_p, upper_p, matrix};
    }

    /*
     * BohmianTrajectories
     */
    BohmianTrajectories::BohmianTrajectories(const GridWaveFunction& psi, int n, double t): x(n){
        const Grid& g = *psi.grid;
        Eigen::ArrayXd cdf(g.size+1);
        cdf(0) = 0.;
        for(int i=0; i<g.size; i++) cdf(i+1) = cdf(i) + std::norm(psi.psi(i));
        cdf /= cdf(g.size);

        int j = 0;
        for(int i=0; i<n; i++){
            double q = (i + .5)/n;
            while(j<g.size-1 && cdf(j+1)<q) j++;
            double frac = cdf(j+1)>cdf(j) ? (q - cdf(j))/(cdf(j+1) - cdf(j)) : 0.;
            x(i) = g.lower + (j - .5 + frac)*g.dx();
        }

        times.push_back(t);
        history.push_back(x);
    }

    void BohmianTrajectories::advance(const GridWaveFunction& psi, double mass, double t, double dt){
        const Grid& g = *psi.grid;
        int n = g.size;

        // Velocity field v = Im(psi^* dpsi/dx) / (m |psi|^2)
        Eigen::ArrayXcd dpsi = psi.psi;
        fft_1d(dpsi.data(), n, -1);
        for(int i=0; i<n; i++) dpsi(i) *= I*g.k(i)/double(n);
        if(n%2==0) dpsi(n/2) = 0.;
        fft_1d(dpsi.data(), n, +1);

        double v_max = g.k_max()/mass;
        Eigen::ArrayXd v(n);
        for(int i=0; i<n; i++){
            double density = std::norm(psi.psi(i));
            v(i) = density>1.e-280 ? (std::conj(psi.psi(i))*dpsi(i)).imag()/density/mass : 0.;
            v(i) = std::max(-v_max, std::min(v_max, v(i)));
        }

        for(int i=0; i<x.size(); i++){
            double pos = (g.wrap(x(i)) - g.lower)/g.dx();
            int j = int(std::floor(pos)) % n;
            double frac = pos - std::floor(pos);
            x(i) += dt*((1.-frac)*v(j) + frac*v((j+1)%n));
        }

        times.push_back(t + dt);
        history.push_back(x);
    }

    /*
     * PositionMeasurement
     */
    PositionMeasurement::PositionMeasurement(double kappa_, int seed): rng(seed), normal(0., 1.), kappa(kappa_){
        if(kappa<0.) throw std::invalid_argument("Measurement strength must not be negative");
    }

    void PositionMeasurement::apply(GridWaveFunction& psi, double dt){
        const Grid& g = *psi.grid;
        double mean = psi.expect_x();
        double dW = std::sqrt(dt)*normal(rng);

        // exp(a dW - a^2 dt) with a = sqrt(kappa)(x - <x>) solves d psi = (-a^2/2 dt + a dW) psi (Ito)
        for(int i=0; i<g.size; i++){
            double a = std::sqrt(kappa)*(g.x(i) - mean);
            psi.psi(i) *= std::exp(a*dW - a*a*dt);
        }
        psi.normalize();
    }

    /*
     * SpaceTime
     */
    void SpaceTime::add(const GridWaveFunction& psi, double t){
        const Grid& g = *psi.grid;
        int stride = stride_for(g.size, max_points);
        int m = g.size/stride;

        lower = g.lower;
        upper = g.upper;

        Eigen::ArrayXd row(m);
        for(int i=0; i<m; i++) row(i) = std::norm(psi.psi(i*stride));
        times.push_back(t);
        rows.push_back(row);
    }
}
