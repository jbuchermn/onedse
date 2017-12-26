#pragma once

#include <string>
#include <exception>
#include <boost/type_traits.hpp>

#include "wigner_web/map/map.h"

namespace wigner_web::propagation{
    template <class StateClass>
    class Propagator{
        BOOST_STATIC_ASSERT(boost::is_base_of<wigner_web::state::State, StateClass>::value);

    protected:
        int consistency_order;
        double safety_factor;

        std::shared_ptr<wigner_web::map::Map<StateClass>> map;

    public:
        class propagation_error: public std::runtime_error{
        public:
            propagation_error(std::string msg): runtime_error(msg){}
        };


        Propagator(std::shared_ptr<wigner_web::map::Map<StateClass>> map_): map(map_), consistency_order(1), safety_factor(.6) {}

        virtual void step(StateClass& state, double t_start, double t_step)=0;

        virtual double step_measure_error(StateClass& state, double t_start, double t_step){
            StateClass error = state;
            step(error, t_start, t_step);
            step(state, t_start, t_step/2.);
            step(state, t_start + t_step/2., t_step/2.);

            return (state-error).norm();
        }

        virtual double next_step(double t_step, double error, double epsilon=1.e-9){
            if(error == 0.) return 2.*t_step;
            return t_step * std::pow(safety_factor*epsilon/error, 1./(consistency_order+1) );
        }

        void propagate(StateClass& state, double t_start, double t_final){
            double t_step = t_final - t_start;
            propagate(state, t_start, t_final, t_step, 1.e-9);
        }

        virtual void propagate(StateClass& state, double t_start, double t_final, double& t_step, double epsilon=1.e-9){
            double t = t_start;
            StateClass previous = state;

            while(t<t_final){
                t_step = std::min(t_step, t_final-t);

                previous = state;
                double error = step_measure_error(state, t, t_step);
                if(error>epsilon){
                    state = previous;
                }else{
                    t += t_step;
                }

                t_step = next_step(t_step, error, epsilon);

                if(t_step<1.e-14) throw propagation_error("Step size too small: "+std::to_string(t_step));
            }
        }

        int get_consistency_order() const{ return consistency_order; }

    };
}
