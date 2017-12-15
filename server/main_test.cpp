#include <iostream>
#include <memory>
#include <exception>
#include <Eigen/Dense>

#include "wigner_web/discretization/basis.h"
#include "wigner_web/discretization/orthogonal_hermite.h"
#include "wigner_web/state/wave_function.h"
#include "wigner_web/state/density_operator.h"

using Basis = wigner_web::discretization::Basis;
using OrthogonalHermite = wigner_web::discretization::OrthogonalHermite;
using WaveFunction = wigner_web::state::WaveFunction;
using DensityOperator = wigner_web::state::DensityOperator;

int main(int argc, char* argv[]){
    auto basis = std::make_shared<OrthogonalHermite>(-5., 5., 100);

    for(double x=-5.; x<=5.; x+=0.5){
        Eigen::VectorXcd vals, derivs;
        basis->evaluate(x, vals, derivs);

        if(std::abs( vals(0) - 1.               ) >1.e-12 ) throw std::runtime_error("H_0("+std::to_string(x)+")");
        if(std::abs( vals(1) - 2.*x             ) >1.e-12 ) throw std::runtime_error("H_1("+std::to_string(x)+")");
        if(std::abs( vals(2) - 4.*x*x + 2.      ) >1.e-12 ) throw std::runtime_error("H_2("+std::to_string(x)+")");
        if(std::abs( vals(3) - 8.*x*x*x + 12.*x ) >1.e-12 ) throw std::runtime_error("H_3("+std::to_string(x)+")");
    }

    Eigen::VectorXd grid(10);
    for(int i=0; i<10; i++) grid(i)=i-5.;

    auto wf1 = std::make_shared<WaveFunction>(basis);
    Eigen::VectorXcd components = basis->discretize_function_cov([](double x){ return std::exp(-x*x); });
    wf1->set_from_components_cov(components);
    (*wf1)/=wf1->norm();

    auto wf2 = std::make_shared<WaveFunction>(basis);
    components = basis->discretize_function_cov([](double x){ return std::exp(-(x-1.)*(x-1.)); });
    wf2->set_from_components_cov(components);
    (*wf2)/=wf2->norm();

    auto wf3 = std::make_shared<WaveFunction>(((*wf1) + (*wf2))/std::sqrt(2.));

    std::cout<<"WF1"<<std::endl<<wf1->grid(grid)<<std::endl<<"----"<<std::endl
             <<"WF2"<<std::endl<<wf2->grid(grid)<<std::endl<<"----"<<std::endl
             <<"WF3"<<std::endl<<wf3->grid(grid)<<std::endl<<"----"<<std::endl;

    DensityOperator rho(basis);
    rho.set_from_wavefunctions({{.5, wf1}, {.5, wf2}});

    auto diag = rho.diagonalize();
    for(auto wf: diag){
        std::cout<<wf.first<<std::endl<<"---"<<std::endl<<wf.second->grid(grid)<<std::endl<<"-----"<<std::endl;
    }
}
