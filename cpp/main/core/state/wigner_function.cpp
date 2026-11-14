#include <iostream>
#include <memory>
#include <utility>
#include <cmath>
#include <stdexcept>
#include <math.h>
#include <nlohmann/json.hpp>


#include "core/discretization/basis.h"
#include "core/state/wigner_function.h"
#include "core/state/wave_function.h"
#include "core/state/density_operator.h"
#include "core/utility/fft.h"

using json = nlohmann::json;
using Basis = core::discretization::Basis;
using WaveFunction = core::state::WaveFunction;
using DensityOperator = core::state::DensityOperator;
using FFT = core::utility::FFT;

namespace core::state{

    WignerFunction::WignerFunction(double lower_x_, double upper_x_, double lower_p_, double upper_p_, const Eigen::MatrixXd& matrix_): lower_x(lower_x_), upper_x(upper_x_), lower_p(lower_p_), upper_p(upper_p_), matrix(matrix_){}
    
    WignerFunction::WignerFunction(const WaveFunction& wavefunction, int points){
        set_from_wavefunction(wavefunction, points);
    }

    WignerFunction::WignerFunction(const DensityOperator& density_operator, int points){
        auto b = density_operator.basis->plot_boundaries();

        Eigen::VectorXd x_grid(points);
        for(int i=0; i<points; i++) x_grid(i) = b.first + 1.*i/points*(b.second-b.first);

        // rho(x_a, x_b) = sum_ij e_i(x_a)^* rho^{ij} e_j(x_b), in line with psi(x) = sum_i e_i(x)^* psi^i
        Eigen::MatrixXcd basis_vals = density_operator.basis->evaluate(x_grid);
        Eigen::MatrixXcd rho = basis_vals.conjugate() * density_operator.matrix * basis_vals.transpose();

        *this = from_correlation(points, b.first, b.second,
                density_operator.basis->boundary_conditions == Basis::BoundaryConditions::Periodic,
                [&rho](int a, int b){ return rho(a, b); });
    }

    void WignerFunction::set_from_wavefunction(const WaveFunction& wavefunction, int points){
        auto b = wavefunction.basis->plot_boundaries();

        Eigen::VectorXd x_grid(points);
        for(int i=0; i<points; i++) x_grid(i) = b.first + 1.*i/points*(b.second-b.first);
        Eigen::VectorXcd psi = wavefunction.grid(x_grid);

        *this = from_correlation(points, b.first, b.second,
                wavefunction.basis->boundary_conditions == Basis::BoundaryConditions::Periodic,
                [&psi](int a, int b){ return psi(a)*std::conj(psi(b)); });
    }

    WignerFunction WignerFunction::from_correlation(int points, double lower_x, double upper_x, bool periodic,
            const std::function<std::complex<double>(int, int)>& rho){

        double upper_p =  .5*M_PI/(upper_x-lower_x) * points;
        double lower_p = -.5*M_PI/(upper_x-lower_x) * points;

        Eigen::MatrixXd matrix = Eigen::MatrixXd::Zero(points, points);

        FFT fft(2*points, true);

        Eigen::VectorXcd tmp(2*points);
        for(int i=0; i<points; i++){
            for(int k=-points; k<points; k++){
                if(periodic){
                    tmp(k+points) = rho((i-k+points)%points, (i+k+points)%points);
                }else{
                    // Default to Dirichlet
                    tmp(k+points) = (i+k>=points || i+k<0 || i-k>=points || i-k<0) ? 0 : rho(i-k, i+k);
                }
            }
            fft(tmp);
            for(int k=0; k<points; k++) matrix(i, k) = tmp( (2*points-2*k) % (2*points) ).real(); 
        }

        // Normalization
        matrix *= std::sqrt(2)/M_PI * std::sqrt(points) * (upper_x-lower_x)/points;

        return WignerFunction{lower_x, upper_x, lower_p, upper_p, matrix};
    }


    void WignerFunction::validate() const{
        if(matrix.hasNaN()) throw std::out_of_range("Wigner functions contains NaN");
    }

    void WignerFunction::plot_to_terminal() const{
        std::cout<<"x\\in["<<lower_x<<","<<upper_x<<"], p\\in["<<lower_p<<","<<upper_p<<"]"<<std::endl;
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
        
    double WignerFunction::norm() const{
        double result = 0.;
        for(int i=0; i<matrix.rows(); i++){
            for(int j=0; j<matrix.cols(); j++){
                result += matrix(i,j);
            }
        }

        return result*(upper_x-lower_x)*(upper_p-lower_p)/matrix.rows()/matrix.cols();
    }

    double WignerFunction::get_lower_x() const{ return lower_x; }
    double WignerFunction::get_upper_x() const{ return upper_x; }
    double WignerFunction::get_lower_p() const{ return lower_p; }
    double WignerFunction::get_upper_p() const{ return upper_p; }
        
    void WignerFunction::to_json(json& json) const{
        json["lower_x"] = lower_x;
        json["upper_x"] = upper_x;
        json["lower_p"] = lower_p;
        json["upper_p"] = upper_p;

        std::vector<std::vector<double>> data;
        for(int i=0; i<matrix.rows(); i++){
            data.push_back(std::vector<double>());
            for(int j=0; j<matrix.cols(); j++){
                data.back().push_back(matrix(i,j));
            }
        }
        json["data"] = data;
    }
}








