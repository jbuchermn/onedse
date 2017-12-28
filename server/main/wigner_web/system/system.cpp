#include <iostream>
#include <LuaContext.hpp>
#include <boost/variant.hpp>
#include <json.hpp>
#include <utility>

#include "wigner_web/system/system.h"
#include "wigner_web/discretization/basis.h"
#include "wigner_web/discretization/scaled_basis.h"
#include "wigner_web/discretization/orthogonal_chebyshev.h"
#include "wigner_web/discretization/orthogonal_legendre.h"
#include "wigner_web/discretization/orthogonal_hermite.h"
#include "wigner_web/state/wave_function.h"
#include "wigner_web/state/density_operator.h"
#include "wigner_web/state/wigner_function.h"
#include "wigner_web/map/map_wavefunction.h"
#include "wigner_web/propagation/propagator.h"
#include "wigner_web/propagation/runge_kutta.h"
#include "wigner_web/propagation/diagonal_propagator.h"
#include "wigner_web/utility/lua_expose.h"

using json = nlohmann::json;
using Basis = wigner_web::discretization::Basis;
using ScaledBasis = wigner_web::discretization::ScaledBasis;
using OrthogonalChebyshev = wigner_web::discretization::OrthogonalChebyshev;
using OrthogonalHermite = wigner_web::discretization::OrthogonalHermite;
using OrthogonalLegendre = wigner_web::discretization::OrthogonalLegendre;
using WaveFunction = wigner_web::state::WaveFunction;
using DensityOperator = wigner_web::state::DensityOperator;
using WignerFunction = wigner_web::state::WignerFunction;
using MapWaveFunction = wigner_web::map::MapWaveFunction;
using MapDensityOperator = wigner_web::map::MapDensityOperator;

template<class StateClass>
using Propagator = wigner_web::propagation::Propagator<StateClass>;

template<class StateClass>
using RungeKutta = wigner_web::propagation::RungeKutta<StateClass>;

template<class StateClass>
using DiagonalPropagator = wigner_web::propagation::DiagonalPropagator<StateClass>;
        
using DiagonalRepresentationLuaElement = std::vector<std::pair< int, boost::variant<double, std::shared_ptr<WaveFunction>>>>;
using DiagonalRepresentationLua = std::vector<std::pair<int, DiagonalRepresentationLuaElement>>;

using namespace wigner_web::utility;

namespace wigner_web::system{
    System::System(){
        lua_expose_complex<double>(lua);

        /*
         * Constructors
         */
        lua_expose_constructor<OrthogonalLegendre,  Basis, int>(lua, "orthogonal_legendre");
        lua_expose_constructor<OrthogonalHermite,   Basis, int>(lua, "orthogonal_hermite");
        lua_expose_constructor<OrthogonalChebyshev, Basis, int>(lua, "orthogonal_chebyshev");
        lua_expose_constructor<ScaledBasis,         Basis, std::shared_ptr<Basis>, double, double>(lua, "scaled_basis");

        lua_expose_constructor_as_member<WaveFunction,       WaveFunction,       std::shared_ptr<Basis>, std::function<std::complex<double>(double)>, int>(lua, "wavefunction");
        lua_expose_constructor_as_member<MapWaveFunction,    MapWaveFunction,    std::shared_ptr<Basis>, int, int, std::function<std::complex<double>(double)>, int>(lua, "map_wavefunction");
        lua_expose_constructor_as_member<DensityOperator,    DensityOperator,    std::shared_ptr<Basis>>(lua, "density_operator");
        lua_expose_constructor_as_member<DensityOperator,    DensityOperator,    std::shared_ptr<WaveFunction>>(lua, "density_operator");
        lua_expose_constructor_as_member<MapDensityOperator, MapDensityOperator, std::shared_ptr<Basis>>(lua, "map_density_operator");

        lua_expose_constructor_as_member<DiagonalPropagator<WaveFunction>,     Propagator<WaveFunction>,    std::shared_ptr<MapWaveFunction>>(lua, "diagonal_propagator");
        lua_expose_constructor_as_member<DiagonalPropagator<DensityOperator>,  Propagator<DensityOperator>, std::shared_ptr<MapDensityOperator>>(lua, "diagonal_propagator");
        lua_expose_constructor_as_member<RungeKutta<WaveFunction>,             Propagator<WaveFunction>,    std::shared_ptr<MapWaveFunction>,    std::string>(lua, "rk_propagator");
        lua_expose_constructor_as_member<RungeKutta<DensityOperator>,          Propagator<DensityOperator>, std::shared_ptr<MapDensityOperator>, std::string>(lua, "rk_propagator");

        /*
         * States as vectors
         */
        lua_expose_linear_space_operators<WaveFunction,        std::complex<double>>(lua);
        lua_expose_linear_space_operators<MapWaveFunction,     std::complex<double>>(lua);
        lua_expose_linear_space_operators<MapDensityOperator,  std::complex<double>>(lua);
        lua_expose_linear_space_operators<DensityOperator,     std::complex<double>>(lua);
        lua_expose_linear_space_operators<WignerFunction,      double              >(lua);
        
        lua.registerFunction("norm", &DensityOperator::norm);
        lua.registerFunction("norm", &WaveFunction::norm);
        lua.registerFunction("norm", &WignerFunction::norm);

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
        lua.registerFunction("add", &MapWaveFunction::add);

        /*
         * MapDensityOperator
         */
        lua.registerFunction("add_left", &MapDensityOperator::add_left);
        lua.registerFunction("add_right", &MapDensityOperator::add_right);
        lua.registerFunction("add", &MapDensityOperator::add);


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

        /*
         * Plot
         */
        lua.registerFunction("wavefunction", &System::plot_wavefunction);
        lua.registerFunction("wigner", &System::plot_wigner);

        lua.writeVariable("plot", this);
    }

    void System::execute(std::string code, json& result){
        this->result = json();

        lua.executeCode(R"(
            __stdout__ = ""
            print = function(...)
                for arg, val in ipairs({...}) do
                    __stdout__ = __stdout__ .. tostring(val) .. " " 
                end
                __stdout__ = __stdout__ .. "\n"
            end
        )");
        lua.executeCode(code);
        this->result["stdout"] = lua.readVariable<std::string>("__stdout__");

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
}
