#include <iostream>
#include <memory>
#include <utility>
#include <cmath>
#include <complex>

#include "wigner_web/discretization/basis.h"
#include "wigner_web/state/wigner_function.h"
#include "wigner_web/state/wave_function.h"
#include "wigner_web/state/density_operator.h"
#include "wigner_web/utility/fft.h"

using Basis = wigner_web::discretization::Basis;
using WaveFunction = wigner_web::state::WaveFunction;
using DensityOperator = wigner_web::state::DensityOperator;
using FFT = wigner_web::utility::FFT;

namespace wigner_web::state{

    WignerFunction::WignerFunction(double lower_x_, double upper_x_, double lower_p_, double upper_p_, Eigen::MatrixXd&& matrix_): 
        lower_x(lower_x_), upper_x(upper_x_), lower_p(lower_p_), upper_p(upper_p_), matrix(matrix_){}

    WignerFunction::WignerFunction(const WaveFunction& wavefunction, int points){
        set_from_wavefunction(wavefunction, points);
    }

    WignerFunction::WignerFunction(const DensityOperator& density_operator, int points){
        matrix = Eigen::MatrixXd(points, points);
        for(std::pair<double, std::shared_ptr<WaveFunction>> p: density_operator.diagonalize()){
            (*this) += p.first * WignerFunction{ *(p.second), points };
        }
    }
        
    void WignerFunction::set_from_wavefunction(const WaveFunction& wavefunction, int points){
        lower_x = wavefunction.basis->lower;
        upper_x = wavefunction.basis->upper;
        lower_p = -(upper_x-lower_x)/2.;
        upper_p = -lower_p;

        Eigen::VectorXd x_grid(points);
        for(int i=0; i<points; i++) x_grid(i) = lower_x + 1.*i/points*(upper_x-lower_x);
        Eigen::VectorXcd psi = wavefunction.grid(x_grid);

        matrix = Eigen::MatrixXd(points, points);
        FFT fft(2*points, true);

        for(int i=0; i<points; i++){
            Eigen::VectorXcd tmp(2*points);
            for(int k=-points; k<points; k++){
                if(wavefunction.basis->boundary_conditions == Basis::BoundaryConditions::Periodic){
                    tmp(k+points) = std::conj(psi((i+k+points)%points)) * psi((i-k+points)%points);
                }else{
                    // Default to Dirichlet
                    tmp(k+points) = (i+k>=points || i+k<0 || i-k>=points || i-k<0) ? 0 : std::conj(psi(i+k)) * psi(i-k);
                }
            }
            fft(tmp);
            for(int k=0; k<points; k++)
                matrix(i, k) = std::sqrt(2/M_PI)*tmp(2*k+1).real(); 
        }
    }

    void WignerFunction::plot_to_terminal() const{
        for(int i=0; i<matrix.rows(); i++){
            for(int j=0; j<matrix.cols(); j++){
                if(matrix(i,j)<-1.) std::cout<<"O";
                else if(matrix(i,j)<-1.e-4) std::cout<<"o";
                else if(matrix(i,j)<1.e-4) std::cout<<".";
                else if(matrix(i,j)<1.) std::cout<<"x";
                else std::cout<<"X";
            }
            std::cout<<std::endl;
        }
    }

    double WignerFunction::get_lower_x() const{ return lower_x; }
    double WignerFunction::get_upper_x() const{ return upper_x; }
    double WignerFunction::get_lower_p() const{ return lower_p; }
    double WignerFunction::get_upper_p() const{ return upper_p; }
}
