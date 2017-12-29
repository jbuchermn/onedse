#include <iostream>
#include <memory>
#include <utility>
#include <cmath>
#include <stdexcept>
#include <math.h>
#include <json.hpp>


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
        upper_x = density_operator.basis->upper;
        lower_x = density_operator.basis->lower;

        upper_p =  .5*M_PI/(upper_x-lower_x) * points;
        lower_p = -.5*M_PI/(upper_x-lower_x) * points;
        
        matrix = Eigen::MatrixXd::Zero(points, points);

        for(std::pair<double, std::shared_ptr<WaveFunction>> p: density_operator.diagonalize()){
            *this += p.first*WignerFunction{*(p.second), points};
        }
    }
        
    void WignerFunction::set_from_wavefunction(const WaveFunction& wavefunction, int points){
        upper_x = wavefunction.basis->upper;
        lower_x = wavefunction.basis->lower;

        upper_p =  .5*M_PI/(upper_x-lower_x) * points;
        lower_p = -.5*M_PI/(upper_x-lower_x) * points;
        
        matrix = Eigen::MatrixXd::Zero(points, points);

        Eigen::VectorXd x_grid(points);
        for(int i=0; i<points; i++) x_grid(i) = lower_x + 1.*i/points*(upper_x-lower_x);
        Eigen::VectorXcd psi = wavefunction.grid(x_grid);

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
            for(int k=0; k<points; k++) matrix(i, k) = tmp(2*points-2-2*k).real(); 
        }

        // Normalization
        matrix *= std::sqrt(2)/M_PI * std::sqrt(points) * (upper_x-lower_x)/points;
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








