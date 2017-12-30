#pragma once

#include <iostream>
#include <string>
#include <exception>
#include <numeric>
#include <boost/type_traits.hpp>

#include "core/map/map.h"

namespace core::propagation{
    template <class StateClass>
    class Propagator{
        BOOST_STATIC_ASSERT(boost::is_base_of<core::state::State, StateClass>::value);

    protected:
        int consistency_order;
        double safety_factor;

        std::shared_ptr<core::map::Map<StateClass>> map;

    public:
        class propagation_error: public std::runtime_error{
        public:
            propagation_error(std::string msg): runtime_error(msg){}
        };

        class StepSizeHistogram{
            std::vector<double> steps;

        public:
            void add(double step){ steps.push_back(step); }
            double avg(){ return std::accumulate(steps.begin(), steps.end(), 0.) / steps.size(); }
            double std_dev(){ 
                double mean = avg();
                std::vector<double> diff(steps.size());
                std::transform(steps.begin(), steps.end(), diff.begin(), [mean](double x) { return x - mean; });
                double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
                return std::sqrt(sq_sum / steps.size());
            }
        };


        Propagator(std::shared_ptr<core::map::Map<StateClass>> map_): map(map_), consistency_order(1), safety_factor(.6) {}

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
            return t_step * safety_factor * std::pow(epsilon/error, 1./(consistency_order+1) );
        }

        virtual StepSizeHistogram propagate(StateClass& state, double t_start, double t_final, double epsilon=1.e-9){
            double t_step = 1.e-7;
            double t = t_start;
            StateClass previous = state;

            StepSizeHistogram result;

            while(t<t_final){
                t_step = std::min(t_step, t_final-t);
                result.add(t_step);

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

            return result;
        }

        virtual void propagate_const_step(StateClass& state, double t_start, double t_final, double t_step){
            double t=t_start;
            for(; t<t_final; t+=t_step){
                step(state, t, t_step);
            }
            step(state, t, t_final - t);
        }

        int get_consistency_order() const{ return consistency_order; }

    };
}
