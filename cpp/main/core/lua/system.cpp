#include <iostream>
#include <utility>
#include <vector>
#include <cmath>
#include <LuaContext.hpp>
#include <boost/variant.hpp>
#include <nlohmann/json.hpp>

#include "core/lua/system.h"
#include "core/lua/expose.h"
#include "core/discretization/basis.h"
#include "core/discretization/scaled_basis.h"
#include "core/discretization/orthogonal_chebyshev.h"
#include "core/discretization/orthogonal_legendre.h"
#include "core/discretization/orthogonal_hermite.h"
#include "core/state/wave_function.h"
#include "core/state/density_operator.h"
#include "core/state/wigner_function.h"
#include "core/map/map_wavefunction.h"
#include "core/map/map_density_operator.h"
#include "core/propagation/propagator.h"
#include "core/propagation/runge_kutta.h"
#include "core/propagation/diagonal_propagator.h"
#include "core/utility/encode.h"
#include "core/utility/interrupt.h"

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

using core::utility::encode_f32;
using core::utility::encode_f32_from;

template<class StateClass>
using Propagator = core::propagation::Propagator<StateClass>;

template<class StateClass>
using RungeKutta = core::propagation::RungeKutta<StateClass>;

template<class StateClass>
using DiagonalPropagator = core::propagation::DiagonalPropagator<StateClass>;

using DiagonalRepresentationLuaElement = std::vector<std::pair< int, boost::variant<double, std::shared_ptr<WaveFunction>>>>;
using DiagonalRepresentationLua = std::vector<std::pair<int, DiagonalRepresentationLuaElement>>;

using complex = std::complex<double>;

namespace{
    /*
     * LuaContext keeps its lua_State private, but we need it to install an interrupt
     * hook. Explicit template instantiation may legally name private members.
     */
    template<typename Tag, typename Tag::type Member>
    struct Rob{
        friend typename Tag::type get(Tag){ return Member; }
    };
    struct LuaStateTag{
        typedef lua_State* LuaContext::*type;
        friend type get(LuaStateTag);
    };
    template struct Rob<LuaStateTag, &LuaContext::mState>;

    void interrupt_hook(lua_State* L, lua_Debug*){
        if(core::utility::interrupt_flag.load(std::memory_order_relaxed)){
            luaL_error(L, "Interrupted");
        }
    }

    void wigner_to_json(const WignerFunction& wigner, json& plot){
        plot["lower_x"] = wigner.get_lower_x();
        plot["upper_x"] = wigner.get_upper_x();
        plot["lower_p"] = wigner.get_lower_p();
        plot["upper_p"] = wigner.get_upper_p();
        plot["nx"] = wigner.matrix.rows();
        plot["np"] = wigner.matrix.cols();

        std::vector<float> data;
        data.reserve(wigner.matrix.size());
        for(int i=0; i<wigner.matrix.rows(); i++){
            for(int j=0; j<wigner.matrix.cols(); j++) data.push_back(float(wigner.matrix(i, j)));
        }
        plot["data"] = encode_f32(data);
    }
}

namespace core::lua{
    System::System(Sink sink_, bool sandbox): sink(sink_){
        expose_complex<double>(lua);

        /*
         * Print
         */
        lua.registerFunction("stdout", &System::print);
        lua.writeVariable("plot", this);
        lua.executeCode(R"(
            print = function(...)
                local parts = {}
                for i = 1, select("#", ...) do parts[i] = tostring((select(i, ...))) end
                plot:stdout(table.concat(parts, "\t") .. "\n")
            end
        )");

        // Removed from the standard library in Lua 5.3, but too useful for physics to miss
        lua.executeCode(R"(
            math.sinh = math.sinh or function(x) return (math.exp(x) - math.exp(-x))/2 end
            math.cosh = math.cosh or function(x) return (math.exp(x) + math.exp(-x))/2 end
            math.tanh = math.tanh or function(x)
                if x > 20 then return 1 elseif x < -20 then return -1 end
                local a, b = math.exp(x), math.exp(-x)
                return (a - b)/(a + b)
            end
        )");

        if(sandbox){
            lua.executeCode(R"(
                os = { time = os.time, clock = os.clock, date = os.date, difftime = os.difftime }
                io = nil; package = nil
                debug = { traceback = debug.traceback }   -- needed for error reporting
                require = nil; dofile = nil; loadfile = nil; load = nil; loadstring = nil
                string.dump = nil; collectgarbage = nil
            )");
        }

        expose_spectral();
        expose_plot();

        lua_State* L = lua.*get(LuaStateTag());
        lua_sethook(L, interrupt_hook, LUA_MASKCOUNT, 10000);
    }

    /*
     * Galerkin (spectral) discretisation: arbitrary operators, density operators, Lindblad equations
     */
    void System::expose_spectral(){
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
        lua.registerFunction("purity", &DensityOperator::purity);
        lua.registerFunction("entropy", &DensityOperator::entropy);

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
    }

    void System::expose_plot(){
        lua.registerFunction("wavefunction", &System::plot_wavefunction);
        lua.registerFunction("wigner", &System::plot_wigner);
        lua.registerFunction("scalar", &System::plot_scalar);
    }

    bool System::execute(const std::string& code){
        try{
            lua.executeCode(code);
        }catch(const std::exception& ex){
            print_exception(ex);
            return false;
        }
        return true;
    }

    void System::emit(json&& message){
        core::utility::check_interrupt();
        if(sink) sink(message);
    }

    void System::print(std::string data){
        emit({ {"type", "stdout"}, {"text", data} });
    }

    void System::print_exception(const std::exception& e, int level){
        if(sink) sink({ {"type", "stderr"}, {"text", std::string(2*level, ' ') + e.what() + "\n"} });
        try {
            std::rethrow_if_nested(e);
        } catch(const std::exception& e) {
            print_exception(e, level+1);
        } catch(...) {}
    }

    /*
     * Plots
     */
    void System::plot_wavefunction(std::shared_ptr<WaveFunction> wavefunction, std::string title, int points){
        if(points<2) points = 200;
        auto [lower, upper] = wavefunction->basis->plot_boundaries();
        Eigen::VectorXd x(points);
        for(int i=0; i<points; i++) x(i) = lower + 1.*i/points*(upper - lower);
        Eigen::VectorXcd vals = wavefunction->grid(x);

        emit({ {"type", "plot"}, {"kind", "wavefunction"}, {"title", title},
               {"lower", lower}, {"upper", upper},
               {"re", encode_f32_from(vals.real())}, {"im", encode_f32_from(vals.imag())} });
    }

    void System::plot_wigner(std::shared_ptr<DensityOperator> density_operator, std::string title, int points){
        json plot = { {"type", "plot"}, {"kind", "wigner"}, {"title", title} };
        if(points<2) points = 128;
        wigner_to_json(WignerFunction{ *density_operator, points }, plot);
        emit(std::move(plot));
    }

    void System::plot_scalar(std::string title, double t, std::string name, double value){
        emit({ {"type", "plot"}, {"kind", "scalar"}, {"title", title}, {"t", t}, {"name", name}, {"value", value} });
    }
}
