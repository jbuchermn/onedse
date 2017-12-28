#pragma once

#include <memory>
#include <vector>
#include <string>
#include <exception>

#include "wigner_web/map/map.h"
#include "wigner_web/propagation/propagator.h"

namespace wigner_web::propagation{

    template<class StateClass>
    class RungeKutta: public Propagator<StateClass>{
        using wigner_web::propagation::Propagator<StateClass>::map;

        std::vector<std::vector<double>> a;
        std::vector<double> b;
        std::vector<double> c;

        std::vector<StateClass> vec_k;
    public:
        RungeKutta(std::shared_ptr<wigner_web::map::Map<StateClass>> map, std::string method): wigner_web::propagation::Propagator<StateClass>(map){

            if(method == "RK4"){
                a = {
                    {0. , 0. , 0.,  0.},
                    {0.5, 0. , 0.,  0.},
                    {0. , 0.5, 0.,  0.},        
                    {0. , 0. , 1.0, 0.}       
                };
                b = {1./6., 1./3., 1./3., 1./6};
                c = {0., 0.5, 0.5, 1.};

                this->consistency_order = 4;
            }else{
                throw std::runtime_error("Unknown Runge-Kutta method '"+method+"'");
            }

        }

        void step(StateClass& state, double t_start, double t_step) override{
            while(vec_k.size() < b.size()) vec_k.push_back(state);

            vec_k.back() = state;
            for(int i=0; i<a.size(); i++){
                vec_k[i] = vec_k.back();
                for(int j=0; j<i; j++) vec_k[i] += t_step*a[i][j]*vec_k[j];

                map->set_time(t_start + t_step*c[i]);
                map->apply(vec_k[i]);

                state += t_step*b[i]*vec_k[i];
            }
        }
    };
}
