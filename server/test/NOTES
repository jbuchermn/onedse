#include <iostream>
#include <memory>
#include <exception>
#include <Eigen/Dense>

#include "wigner_web/discretization/basis.h"
#include "wigner_web/discretization/orthogonal_hermite.h"
#include "wigner_web/discretization/scaled_orthogonal_chebyshev.h"
#include "wigner_web/state/wave_function.h"
#include "wigner_web/state/density_operator.h"
#include "wigner_web/state/wigner_function.h"

using Basis = wigner_web::discretization::Basis;
using OrthogonalHermite = wigner_web::discretization::OrthogonalHermite;
using ScaledOrthogonalChebyshev = wigner_web::discretization::ScaledOrthogonalChebyshev;
using WaveFunction = wigner_web::state::WaveFunction;
using DensityOperator = wigner_web::state::DensityOperator;
using WignerFunction = wigner_web::state::WignerFunction;

int main(int argc, char* argv[]){
    auto basis = std::make_shared<ScaledOrthogonalChebyshev>(-10., 10., 100);
    

    // for(double x=-5.; x<=5.; x+=0.5){
    //     Eigen::VectorXcd vals = basis->evaluate(x);
    //
    //     if(std::abs( vals(0) - 1.               ) >1.e-12 ) throw std::runtime_error("H_0("+std::to_string(x)+")");
    //     if(std::abs( vals(1) - 2.*x             ) >1.e-12 ) throw std::runtime_error("H_1("+std::to_string(x)+")");
    //     if(std::abs( vals(2) - 4.*x*x + 2.      ) >1.e-12 ) throw std::runtime_error("H_2("+std::to_string(x)+")");
    //     if(std::abs( vals(3) - 8.*x*x*x + 12.*x ) >1.e-12 ) throw std::runtime_error("H_3("+std::to_string(x)+")");
    // }
    // for(double x=-5.; x<=5.; x+=0.5){
    //     Eigen::VectorXcd vals = basis->evaluate(x, 2);
    //
    //     if(std::abs( vals(0) - 0.    ) >1.e-12 ) throw std::runtime_error("H''_0("+std::to_string(x)+")");
    //     if(std::abs( vals(1) - 0.    ) >1.e-12 ) throw std::runtime_error("H''_1("+std::to_string(x)+")");
    //     if(std::abs( vals(2) - 8.    ) >1.e-12 ) throw std::runtime_error("H''_2("+std::to_string(x)+")");
    //     if(std::abs( vals(3) - 48.*x ) >1.e-12 ) throw std::runtime_error("H''_3("+std::to_string(x)+")");
    // }


    // auto wf1 = std::make_shared<WaveFunction>(basis, [](double x){ return 1.; }, 10);
    auto wf1 = std::make_shared<WaveFunction>(basis, [](double x){ return std::exp(-(x+2.)*(x+2.)*1.41); }, 10);
    (*wf1)/=wf1->norm();

    auto wf2 = std::make_shared<WaveFunction>(basis, [](double x){ return std::exp(-(x-2.)*(x-2.)*1.41); }, 10);
    (*wf2)/=wf2->norm();

    auto wf3 = std::make_shared<WaveFunction>(((*wf1) + (*wf2))/std::sqrt(2.));

    std::cout<<"WF1"<<std::endl;
    wf1->plot_to_terminal();
    std::cout<<std::endl<<"WF2"<<std::endl;
    wf2->plot_to_terminal();
    std::cout<<std::endl<<"WF3"<<std::endl;
    wf3->plot_to_terminal();

    DensityOperator rho({{.5, wf1}, {.5, wf2}});

    for(auto wf: rho.diagonalize()){
        std::cout<<wf.first<<std::endl<<"---"<<std::endl;
        wf.second->plot_to_terminal();
        std::cout<<"-----"<<std::endl;
    }

    WignerFunction wigner{ rho, 200 };
    wigner.plot_to_terminal();

    WignerFunction wigner2{ *wf3, 200 };
    wigner2.plot_to_terminal();
}
