#include <iostream>
#include <LuaContext.hpp>
#include <boost/variant.hpp>
#include <json.hpp>
#include <utility>

#include "core/lua/system.h"
#include "core/discretization/basis.h"
#include "core/discretization/scaled_basis.h"
#include "core/discretization/orthogonal_chebyshev.h"
#include "core/discretization/orthogonal_legendre.h"
#include "core/discretization/orthogonal_hermite.h"
#include "core/state/wave_function.h"
#include "core/state/density_operator.h"
#include "core/state/wigner_function.h"
#include "core/map/map_wavefunction.h"
#include "core/propagation/propagator.h"
#include "core/propagation/runge_kutta.h"
#include "core/propagation/diagonal_propagator.h"
#include "core/lua/expose.h"

using json = nlohmann::json;
using Basis = core::discretization::Basis;
using ScaledBasis = core::discretization::ScaledBasis;
using OrthogonalChebyshev = core::discretization::OrthogonalChebyshev;
using OrthogonalHermite = core::discretization::OrthogonalHermite;
using OrthogonalLegendre = core::discretization::OrthogonalLegendre;
using WaveFunction = core::state::WaveFunction;
using DensityOperator = core::state::DensityOperator;
using WignerFunction = core::state::WignerFunction;
using MapWaveFunction = core::map::MapWaveFunction;
using MapDensityOperator = core::map::MapDensityOperator;

template<class StateClass>
using Propagator = core::propagation::Propagator<StateClass>;

template<class StateClass>
using RungeKutta = core::propagation::RungeKutta<StateClass>;

template<class StateClass>
using DiagonalPropagator = core::propagation::DiagonalPropagator<StateClass>;
        
using DiagonalRepresentationLuaElement = std::vector<std::pair< int, boost::variant<double, std::shared_ptr<WaveFunction>>>>;
using DiagonalRepresentationLua = std::vector<std::pair<int, DiagonalRepresentationLuaElement>>;

namespace core::lua{
    System::System(){
        expose_complex<double>(lua);

        /*
         * Constructors
         */
        expose_constructor<OrthogonalLegendre,  Basis, int>(lua, "orthogonal_legendre");
        expose_constructor<OrthogonalHermite,   Basis, int>(lua, "orthogonal_hermite");
        expose_constructor<OrthogonalChebyshev, Basis, int>(lua, "orthogonal_chebyshev");
        expose_constructor<ScaledBasis,         Basis, std::shared_ptr<Basis>, double, double>(lua, "scaled_basis");

        expose_constructor_as_member<WaveFunction,       WaveFunction,       std::shared_ptr<Basis>, std::function<std::complex<double>(double)>, int>(lua, "wavefunction");
        expose_constructor_as_member<MapWaveFunction,    MapWaveFunction,    std::shared_ptr<Basis>>(lua, "map_wavefunction");
        expose_constructor_as_member<DensityOperator,    DensityOperator,    std::shared_ptr<Basis>>(lua, "density_operator");
        expose_constructor_as_member<DensityOperator,    DensityOperator,    std::shared_ptr<WaveFunction>>(lua, "density_operator");
        expose_constructor_as_member<MapDensityOperator, MapDensityOperator, std::shared_ptr<Basis>>(lua, "map_density_operator");

        expose_constructor_as_member<DiagonalPropagator<WaveFunction>,     Propagator<WaveFunction>,    std::shared_ptr<MapWaveFunction>>(lua, "diagonal_propagator");
        expose_constructor_as_member<DiagonalPropagator<DensityOperator>,  Propagator<DensityOperator>, std::shared_ptr<MapDensityOperator>>(lua, "diagonal_propagator");
        expose_constructor_as_member<RungeKutta<WaveFunction>,             Propagator<WaveFunction>,    std::shared_ptr<MapWaveFunction>,    std::string>(lua, "rk_propagator");
        expose_constructor_as_member<RungeKutta<DensityOperator>,          Propagator<DensityOperator>, std::shared_ptr<MapDensityOperator>, std::string>(lua, "rk_propagator");

        /*
         * States as vectors
         */
        expose_linear_space_operators<WaveFunction,        std::complex<double>>(lua);
        expose_linear_space_operators<MapWaveFunction,     std::complex<double>>(lua);
        expose_linear_space_operators<MapDensityOperator,  std::complex<double>>(lua);
        expose_linear_space_operators<DensityOperator,     std::complex<double>>(lua);
        expose_linear_space_operators<WignerFunction,      double              >(lua);
        
        lua.registerFunction("norm", &DensityOperator::norm);
        lua.registerFunction("norm", &WaveFunction::norm);
        lua.registerFunction("norm", &WignerFunction::norm);
        lua.registerFunction("validate", &DensityOperator::validate);
        lua.registerFunction("validate", &WaveFunction::validate);
        lua.registerFunction("validate", &WignerFunction::validate);

        lua.registerFunction("trace", &DensityOperator::trace);

        /*
         * DensityOperator
         */
        lua.registerFunction("add", &DensityOperator::add_wavefunction);
        lua.registerFunction<DiagonalRepresentationLua (DensityOperator::*)()>("diagonalize", 
                [](DensityOperator& self){
                    DensityOperator::DiagonalRepresentation rep = self.diagonalize();
                    DiagonalRepresentationLua result;
                    int i=1;
                    for(auto& v: rep){
                        result.push_back({i++, DiagonalRepresentationLuaElement{ {1, v.first}, {2, v.second} }});
                    }
                    return result;
                }
        );

        /*
         * MapWaveFunction
         */
        lua.registerFunction("add_term", &MapWaveFunction::add_term);

        /*
         * MapDensityOperator
         */
        lua.registerFunction("add_left", &MapDensityOperator::add_left);
        lua.registerFunction("add_right", &MapDensityOperator::add_right);
        lua.registerFunction("add_both", &MapDensityOperator::add_both);

        lua.registerFunction<void (MapDensityOperator::*)(std::shared_ptr<MapWaveFunction>)>("add_from_wavefunction_left", 
                [](MapDensityOperator& self, std::shared_ptr<MapWaveFunction> wf){ self.add_from_wavefunction_left(*wf); });

        lua.registerFunction<void (MapDensityOperator::*)(std::shared_ptr<MapWaveFunction>)>("add_from_wavefunction_right", 
                [](MapDensityOperator& self, std::shared_ptr<MapWaveFunction> wf){ self.add_from_wavefunction_right(*wf); });

        lua.registerFunction<void (MapDensityOperator::*)(std::shared_ptr<MapWaveFunction>, std::shared_ptr<MapWaveFunction>)>("add_from_wavefunction_both", 
                [](MapDensityOperator& self, std::shared_ptr<MapWaveFunction> left, std::shared_ptr<MapWaveFunction> right){ self.add_from_wavefunction_both(*left, *right); });


        /*
         * Propagator
         */
        lua.registerFunction<void (Propagator<WaveFunction>::*)(std::shared_ptr<WaveFunction>, double, double)>("step", 
                [](Propagator<WaveFunction>& self, std::shared_ptr<WaveFunction> state, double t_start, double t_step){
                    self.step(*state, t_start, t_step);
                }
        );
        lua.registerFunction<void (Propagator<DensityOperator>::*)(std::shared_ptr<DensityOperator>, double, double)>("step", 
                [](Propagator<DensityOperator>& self, std::shared_ptr<DensityOperator> state, double t_start, double t_step){
                    self.step(*state, t_start, t_step);
                }
        );

        lua.registerFunction<void (Propagator<WaveFunction>::*)(std::shared_ptr<WaveFunction>, double, double, double)>("propagate", 
                [](Propagator<WaveFunction>& self, std::shared_ptr<WaveFunction> state, double t_start, double t_final, double epsilon){
                    if(epsilon>0.)
                        self.propagate(*state, t_start, t_final, epsilon);
                    else
                        self.propagate(*state, t_start, t_final);
                }
        );
        lua.registerFunction<void (Propagator<DensityOperator>::*)(std::shared_ptr<DensityOperator>, double, double, double)>("propagate", 
                [](Propagator<DensityOperator>& self, std::shared_ptr<DensityOperator> state, double t_start, double t_final, double epsilon){
                    if(epsilon>0.)
                        self.propagate(*state, t_start, t_final, epsilon);
                    else
                        self.propagate(*state, t_start, t_final);
                }
        );
        
        lua.registerFunction<void (Propagator<WaveFunction>::*)(std::shared_ptr<WaveFunction>, double, double, double)>("propagate_const_step", 
                [](Propagator<WaveFunction>& self, std::shared_ptr<WaveFunction> state, double t_start, double t_final, double t_step){
                    self.propagate_const_step(*state, t_start, t_final, t_step);
                }
        );
        lua.registerFunction<void (Propagator<DensityOperator>::*)(std::shared_ptr<DensityOperator>, double, double, double)>("propagate_const_step", 
                [](Propagator<DensityOperator>& self, std::shared_ptr<DensityOperator> state, double t_start, double t_final, double t_step){
                    self.propagate_const_step(*state, t_start, t_final, t_step);
                }
        );

        /*
         * Plot
         */
        lua.registerFunction("wavefunction", &System::plot_wavefunction);
        lua.registerFunction("wigner", &System::plot_wigner);

        lua.writeVariable("plot", this);

        /*
         * Print
         */
        lua.registerFunction("stdout", &System::print);
        lua.executeCode(R"(
            print = function(...)
                for _, val in ipairs({...}) do
                    plot:stdout(tostring(val))
                end
                plot:stdout("\n")
            end
        )");
    }

    void System::execute(std::string code, json& result){
        this->result = json();
        this->print_stdout = "";
        this->print_stderr = "";

        try{
            lua.executeCode(code);
        }catch(const std::exception& ex){
            print_exception(ex);
        }

        this->result["stdout"] = this->print_stdout;
        this->result["stderr"] = this->print_stderr;
        result = std::move(this->result);
    }

    void System::plot_wavefunction(std::shared_ptr<WaveFunction> wavefunction, std::string name, int points){
        json plot;
        plot["title"] = name;

        wavefunction->to_json(plot["wavefunction"], points);

        result["plots"].push_back(plot);
    }

    void System::plot_wigner(std::shared_ptr<DensityOperator> density_operator, std::string name, int points){
        json plot;
        plot["title"] = name;

        WignerFunction wigner { *density_operator, points };
        wigner.to_json(plot["wigner"]);

        result["plots"].push_back(plot);
    }

    void System::print(std::string data){
        std::cout<<data;
        print_stdout += data;
    }

    void System::print_exception(const std::exception& e, int level){
        std::cerr << std::string(2*level, ' ') << "exception: " << e.what() << std::endl;
        print_stderr += std::string(2*level, ' ') + "exception: " + e.what() + "\n";
        try {
            std::rethrow_if_nested(e);
        } catch(const std::exception& e) {
            print_exception(e, level+1);
        } catch(...) {}
    }
}
