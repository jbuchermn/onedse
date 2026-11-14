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

using Grid = core::grid::Grid;
using Grid2D = core::grid::Grid2D;
using GridWaveFunction = core::grid::GridWaveFunction;
using GridWaveFunction2D = core::grid::GridWaveFunction2D;
using GridDensityMatrix = core::grid::GridDensityMatrix;
using GridSpinor = core::grid::GridSpinor;
using Schroedinger1D = core::grid::Schroedinger1D;
using Schroedinger2D = core::grid::Schroedinger2D;
using Dirac1D = core::grid::Dirac1D;
using ClassicalEnsemble = core::grid::ClassicalEnsemble;
using BohmianTrajectories = core::grid::BohmianTrajectories;
using SpaceTime = core::grid::SpaceTime;

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

using EigenstatesLuaElement = std::vector<std::pair< int, boost::variant<double, std::shared_ptr<GridWaveFunction>>>>;
using EigenstatesLua = std::vector<std::pair<int, EigenstatesLuaElement>>;

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

    /// Smallest stride such that size/stride <= max_points
    int stride_for(int size, int max_points){
        if(max_points<=0) return 1;
        return std::max(1, (size + max_points - 1)/max_points);
    }

    template<class Array>
    std::vector<float> strided(const Array& values, int stride){
        std::vector<float> res;
        for(int i=0; i<values.size(); i+=stride) res.push_back(float(values(i)));
        return res;
    }

    /// p_max > 0: only the rows with |p| <= p_max are sent
    void wigner_to_json(const WignerFunction& wigner, json& plot, double p_max=0.){
        int np = wigner.matrix.cols();
        double cell_p = (wigner.get_upper_p() - wigner.get_lower_p())/np;
        int j0 = 0, j1 = np;
        if(p_max>0.){
            j0 = std::max(0, int(std::floor((-p_max - wigner.get_lower_p())/cell_p)));
            j1 = std::min(np, int(std::ceil((p_max - wigner.get_lower_p())/cell_p)) + 1);
            if(j1<=j0){ j0 = 0; j1 = np; }
        }

        plot["lower_x"] = wigner.get_lower_x();
        plot["upper_x"] = wigner.get_upper_x();
        plot["lower_p"] = wigner.get_lower_p() + j0*cell_p;
        plot["upper_p"] = wigner.get_lower_p() + j1*cell_p;
        plot["nx"] = wigner.matrix.rows();
        plot["np"] = j1 - j0;

        size_t cells = wigner.matrix.rows()*(j1 - j0);
        if(cells <= 128*128){
            std::vector<float> data;
            data.reserve(cells);
            for(int i=0; i<wigner.matrix.rows(); i++){
                for(int j=j0; j<j1; j++) data.push_back(float(wigner.matrix(i, j)));
            }
            plot["data"] = encode_f32(data);
        }else{
            // Large frames are quantised to 8 bit (data = data_i8 * scale): a quarter of the size,
            // and more levels than a colour scale can show anyway
            double amax = wigner.matrix.cwiseAbs().maxCoeff();
            double scale = amax>0. ? amax/127. : 1.;
            std::vector<unsigned char> data;
            data.reserve(cells);
            for(int i=0; i<wigner.matrix.rows(); i++){
                for(int j=j0; j<j1; j++) data.push_back(static_cast<unsigned char>(static_cast<signed char>(std::lround(wigner.matrix(i, j)/scale))));
            }
            plot["data_i8"] = core::utility::base64(data.data(), data.size());
            plot["scale"] = scale;
        }
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
        expose_grid();
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

    /*
     * Fourier grids: split-operator propagation in 1D and 2D, Gross-Pitaevskii, Dirac,
     * classical ensembles and Bohmian trajectories
     */
    void System::expose_grid(){
        using GridPtr = std::shared_ptr<Grid>;
        using Grid2DPtr = std::shared_ptr<Grid2D>;

        /*
         * Grid
         */
        lua.writeFunction("grid", [](double lower, double upper, int size){ return std::make_shared<Grid>(lower, upper, size); });
        lua.writeFunction("grid_hbar", [](double lower, double upper, int size, double hbar){ return std::make_shared<Grid>(lower, upper, size, hbar); });
        lua.registerFunction<double (Grid::*)()>("hbar", [](Grid& self){ return self.hbar; });
        lua.registerFunction("p_max", &Grid::p_max);
        lua.registerFunction("dx", &Grid::dx);
        lua.registerFunction("length", &Grid::length);
        lua.registerFunction("k_max", &Grid::k_max);

        lua.registerFunction<std::shared_ptr<GridWaveFunction> (GridPtr::*)(std::function<complex(double)>)>("wavefunction",
                [](GridPtr& self, std::function<complex(double)> f){ return std::make_shared<GridWaveFunction>(self, f); });
        lua.registerFunction<std::shared_ptr<GridWaveFunction> (GridPtr::*)(double, double, double)>("gaussian",
                [](GridPtr& self, double x0, double p0, double sigma){
                    return std::make_shared<GridWaveFunction>(GridWaveFunction::gaussian(self, x0, p0, sigma));
                });
        lua.registerFunction<std::shared_ptr<Schroedinger1D> (GridPtr::*)()>("schroedinger",
                [](GridPtr& self){ return std::make_shared<Schroedinger1D>(self); });
        lua.registerFunction<std::shared_ptr<Dirac1D> (GridPtr::*)()>("dirac",
                [](GridPtr& self){ return std::make_shared<Dirac1D>(self); });

        /*
         * GridWaveFunction
         */
        expose_linear_space_operators<GridWaveFunction, complex>(lua);
        lua.registerFunction("norm", &GridWaveFunction::norm);
        lua.registerFunction("normalize", &GridWaveFunction::normalize);
        lua.registerFunction("validate", &GridWaveFunction::validate);
        lua.registerFunction("expect_x", &GridWaveFunction::expect_x);
        lua.registerFunction("expect_p", &GridWaveFunction::expect_p);
        lua.registerFunction("sigma_x", &GridWaveFunction::sigma_x);
        lua.registerFunction("sigma_p", &GridWaveFunction::sigma_p);
        lua.registerFunction("probability", &GridWaveFunction::probability);
        lua.registerFunction("negativity", &GridWaveFunction::negativity);
        lua.registerFunction("negativity_smoothed", &GridWaveFunction::negativity_smoothed);
        lua.registerFunction<std::shared_ptr<ClassicalEnsemble> (GridWaveFunction::*)(int, int, int)>("wigner_ensemble",
                [](GridWaveFunction& self, int n, int points, int seed){ return std::make_shared<ClassicalEnsemble>(self.wigner(points), n, seed); });
        lua.registerFunction<complex (GridWaveFunction::*)(std::shared_ptr<GridWaveFunction>)>("dot",
                [](GridWaveFunction& self, std::shared_ptr<GridWaveFunction> other){ return self.dot(*other); });
        lua.registerFunction<std::shared_ptr<GridWaveFunction> (GridWaveFunction::*)()>("copy",
                [](GridWaveFunction& self){ return std::make_shared<GridWaveFunction>(self); });
        lua.registerFunction<std::shared_ptr<GridDensityMatrix> (GridWaveFunction::*)()>("density_matrix",
                [](GridWaveFunction& self){ return std::make_shared<GridDensityMatrix>(self); });
        lua.registerFunction<std::shared_ptr<BohmianTrajectories> (GridWaveFunction::*)(int, double)>("bohmian",
                [](GridWaveFunction& self, int n, double t){ return std::make_shared<BohmianTrajectories>(self, n, t); });

        /*
         * GridDensityMatrix
         */
        lua.registerFunction<std::shared_ptr<GridDensityMatrix> (GridPtr::*)()>("density_matrix",
                [](GridPtr& self){ return std::make_shared<GridDensityMatrix>(self); });
        lua.registerFunction<void (GridDensityMatrix::*)(double, std::shared_ptr<GridWaveFunction>)>("add",
                [](GridDensityMatrix& self, double weight, std::shared_ptr<GridWaveFunction> psi){ self.add(weight, *psi); });
        lua.registerFunction("coherence", &GridDensityMatrix::coherence);

        using PositionMeasurement = core::grid::PositionMeasurement;
        lua.writeFunction("position_measurement", [](double kappa, int seed){ return std::make_shared<PositionMeasurement>(kappa, seed); });
        lua.registerFunction<void (PositionMeasurement::*)(std::shared_ptr<GridWaveFunction>, double)>("apply",
                [](PositionMeasurement& self, std::shared_ptr<GridWaveFunction> psi, double dt){ self.apply(*psi, dt); });

        lua.registerFunction("trace", &GridDensityMatrix::trace);
        lua.registerFunction("purity", &GridDensityMatrix::purity);
        lua.registerFunction("entropy", &GridDensityMatrix::entropy);
        lua.registerFunction<std::vector<std::pair<int, double>> (GridDensityMatrix::*)(int)>("eigenvalues",
                [](GridDensityMatrix& self, int n){
                    Eigen::ArrayXd ev = self.eigenvalues();
                    std::vector<std::pair<int, double>> result;
                    for(int i=0; i<std::min<int>(n, ev.size()); i++) result.push_back({ i+1, ev(i) });
                    return result;
                });

        /*
         * Schroedinger1D
         */
        lua.registerFunction("set_mass", &Schroedinger1D::set_mass);
        lua.registerFunction("set_nonlinearity", &Schroedinger1D::set_nonlinearity);
        lua.registerFunction("set_potential", &Schroedinger1D::set_potential);
        lua.registerFunction("set_potential_td", &Schroedinger1D::set_potential_td);
        lua.registerFunction("set_absorber", &Schroedinger1D::set_absorber);
        lua.registerFunction<void (Schroedinger1D::*)(std::shared_ptr<GridWaveFunction>, double, double)>("step",
                [](Schroedinger1D& self, std::shared_ptr<GridWaveFunction> psi, double t, double dt){ self.step(*psi, t, dt); });
        lua.registerFunction<void (Schroedinger1D::*)(std::shared_ptr<GridWaveFunction>, double, double, double)>("propagate",
                [](Schroedinger1D& self, std::shared_ptr<GridWaveFunction> psi, double t_start, double t_final, double dt){ self.propagate(*psi, t_start, t_final, dt); });
        lua.registerFunction<void (Schroedinger1D::*)(std::shared_ptr<GridWaveFunction>, double)>("kick",
                [](Schroedinger1D& self, std::shared_ptr<GridWaveFunction> psi, double tau){ self.kick(*psi, tau); });
        lua.registerFunction<void (Schroedinger1D::*)(std::shared_ptr<GridWaveFunction>, double)>("drift",
                [](Schroedinger1D& self, std::shared_ptr<GridWaveFunction> psi, double tau){ self.drift(*psi, tau); });
        lua.registerFunction<void (Schroedinger1D::*)(std::shared_ptr<GridWaveFunction>, double, int)>("relax",
                [](Schroedinger1D& self, std::shared_ptr<GridWaveFunction> psi, double dt, int steps){ self.relax(*psi, dt, steps); });
        lua.registerFunction<double (Schroedinger1D::*)(std::shared_ptr<GridWaveFunction>)>("energy",
                [](Schroedinger1D& self, std::shared_ptr<GridWaveFunction> psi){ return self.energy(*psi); });
        lua.registerFunction<double (Schroedinger1D::*)(std::shared_ptr<GridWaveFunction>)>("kinetic_energy",
                [](Schroedinger1D& self, std::shared_ptr<GridWaveFunction> psi){ return self.kinetic_energy(*psi); });
        lua.registerFunction<double (Schroedinger1D::*)(std::shared_ptr<GridWaveFunction>)>("potential_energy",
                [](Schroedinger1D& self, std::shared_ptr<GridWaveFunction> psi){ return self.potential_energy(*psi); });
        lua.registerFunction<EigenstatesLua (Schroedinger1D::*)(int)>("eigenstates",
                [](Schroedinger1D& self, int n){
                    EigenstatesLua result;
                    int i=1;
                    for(auto& v: self.eigenstates(n)){
                        result.push_back({i++, EigenstatesLuaElement{ {1, v.first}, {2, v.second} }});
                    }
                    return result;
                });

        /*
         * ClassicalEnsemble
         */
        lua.writeFunction("classical_ensemble", [](int n, double x0, double p0, double sigma_x, double sigma_p, int seed){
            return std::make_shared<ClassicalEnsemble>(n, x0, p0, sigma_x, sigma_p, seed);
        });
        lua.registerFunction<void (ClassicalEnsemble::*)(std::shared_ptr<Schroedinger1D>, double, double, double)>("propagate",
                [](ClassicalEnsemble& self, std::shared_ptr<Schroedinger1D> system, double t_start, double t_final, double dt){
                    self.propagate(*system, t_start, t_final, dt);
                });
        lua.registerFunction<void (ClassicalEnsemble::*)(std::shared_ptr<Schroedinger1D>, double)>("kick",
                [](ClassicalEnsemble& self, std::shared_ptr<Schroedinger1D> system, double tau){ self.kick(*system, tau); });
        lua.registerFunction<void (ClassicalEnsemble::*)(std::shared_ptr<Schroedinger1D>, double)>("drift",
                [](ClassicalEnsemble& self, std::shared_ptr<Schroedinger1D> system, double tau){ self.drift(*system, tau); });
        lua.registerFunction<double (ClassicalEnsemble::*)(std::shared_ptr<GridWaveFunction>, int)>("distance",
                [](ClassicalEnsemble& self, std::shared_ptr<GridWaveFunction> psi, int points){ return self.distance(psi->wigner(points)); });
        lua.registerFunction<double (ClassicalEnsemble::*)(std::shared_ptr<GridWaveFunction>, int, double, double)>("distance_smoothed",
                [](ClassicalEnsemble& self, std::shared_ptr<GridWaveFunction> psi, int points, double sigma_x, double sigma_p){
                    return self.distance_smoothed(psi->wigner(points), sigma_x, sigma_p);
                });
        lua.registerFunction("expect_p2", &ClassicalEnsemble::expect_p2);
        lua.registerFunction("expect_x", &ClassicalEnsemble::expect_x);
        lua.registerFunction("expect_p", &ClassicalEnsemble::expect_p);
        lua.registerFunction("probability", &ClassicalEnsemble::probability);
        lua.registerFunction<double (ClassicalEnsemble::*)(std::shared_ptr<Schroedinger1D>)>("energy",
                [](ClassicalEnsemble& self, std::shared_ptr<Schroedinger1D> system){ return self.energy(*system); });

        /*
         * BohmianTrajectories, SpaceTime
         */
        lua.registerFunction<void (BohmianTrajectories::*)(std::shared_ptr<GridWaveFunction>, double, double, double)>("advance",
                [](BohmianTrajectories& self, std::shared_ptr<GridWaveFunction> psi, double mass, double t, double dt){
                    self.advance(*psi, mass, t, dt);
                });

        lua.writeFunction("spacetime", [](int max_points){ return std::make_shared<SpaceTime>(max_points); });
        lua.registerFunction<void (SpaceTime::*)(std::shared_ptr<GridWaveFunction>, double)>("add",
                [](SpaceTime& self, std::shared_ptr<GridWaveFunction> psi, double t){ self.add(*psi, t); });

        /*
         * Two dimensions / two particles
         */
        lua.writeFunction("grid2d", [](GridPtr x, GridPtr y){ return std::make_shared<Grid2D>(x, y); });
        lua.registerFunction<std::shared_ptr<GridWaveFunction2D> (Grid2DPtr::*)(std::function<complex(double, double)>)>("wavefunction",
                [](Grid2DPtr& self, std::function<complex(double, double)> f){ return std::make_shared<GridWaveFunction2D>(self, f); });
        lua.registerFunction<std::shared_ptr<GridWaveFunction2D> (Grid2DPtr::*)(std::shared_ptr<GridWaveFunction>, std::shared_ptr<GridWaveFunction>)>("product",
                [](Grid2DPtr& self, std::shared_ptr<GridWaveFunction> a, std::shared_ptr<GridWaveFunction> b){
                    return std::make_shared<GridWaveFunction2D>(self, *a, *b);
                });
        lua.registerFunction<std::shared_ptr<Schroedinger2D> (Grid2DPtr::*)()>("schroedinger",
                [](Grid2DPtr& self){ return std::make_shared<Schroedinger2D>(self); });

        lua.registerFunction<std::shared_ptr<GridWaveFunction2D> (Grid2DPtr::*)(std::shared_ptr<GridWaveFunction2D>, std::shared_ptr<GridWaveFunction2D>)>("multiply",
                [](Grid2DPtr&, std::shared_ptr<GridWaveFunction2D> a, std::shared_ptr<GridWaveFunction2D> b){
                    return std::make_shared<GridWaveFunction2D>(a->multiply(*b));
                });

        expose_linear_space_operators<GridWaveFunction2D, complex>(lua);
        lua.registerFunction<std::shared_ptr<GridWaveFunction> (GridWaveFunction2D::*)(double)>("slice_x",
                [](GridWaveFunction2D& self, double x){ return std::make_shared<GridWaveFunction>(self.slice_x(x)); });
        lua.registerFunction<std::shared_ptr<GridWaveFunction> (GridWaveFunction2D::*)(double)>("slice_y",
                [](GridWaveFunction2D& self, double y){ return std::make_shared<GridWaveFunction>(self.slice_y(y)); });
        lua.registerFunction("norm", &GridWaveFunction2D::norm);
        lua.registerFunction("normalize", &GridWaveFunction2D::normalize);
        lua.registerFunction("validate", &GridWaveFunction2D::validate);
        lua.registerFunction("expect_x", &GridWaveFunction2D::expect_x);
        lua.registerFunction("expect_y", &GridWaveFunction2D::expect_y);
        lua.registerFunction("probability", &GridWaveFunction2D::probability);
        lua.registerFunction("symmetrize", &GridWaveFunction2D::symmetrize);
        lua.registerFunction<std::shared_ptr<GridWaveFunction2D> (GridWaveFunction2D::*)()>("copy",
                [](GridWaveFunction2D& self){ return std::make_shared<GridWaveFunction2D>(self); });
        lua.registerFunction<std::shared_ptr<GridDensityMatrix> (GridWaveFunction2D::*)(int)>("reduced",
                [](GridWaveFunction2D& self, int particle){
                    if(particle!=1 && particle!=2) throw std::invalid_argument("reduced: particle must be 1 or 2");
                    return std::make_shared<GridDensityMatrix>(self.reduced(particle-1));
                });

        lua.registerFunction("set_mass", &Schroedinger2D::set_mass);
        lua.registerFunction("set_nonlinearity", &Schroedinger2D::set_nonlinearity);
        lua.registerFunction("set_potential", &Schroedinger2D::set_potential);
        lua.registerFunction("set_potential_td", &Schroedinger2D::set_potential_td);
        lua.registerFunction("set_absorber", &Schroedinger2D::set_absorber);
        lua.registerFunction<void (Schroedinger2D::*)(std::shared_ptr<GridWaveFunction2D>, double, double)>("step",
                [](Schroedinger2D& self, std::shared_ptr<GridWaveFunction2D> psi, double t, double dt){ self.step(*psi, t, dt); });
        lua.registerFunction<void (Schroedinger2D::*)(std::shared_ptr<GridWaveFunction2D>, double, double, double)>("propagate",
                [](Schroedinger2D& self, std::shared_ptr<GridWaveFunction2D> psi, double t_start, double t_final, double dt){ self.propagate(*psi, t_start, t_final, dt); });
        lua.registerFunction<void (Schroedinger2D::*)(std::shared_ptr<GridWaveFunction2D>, double, int)>("relax",
                [](Schroedinger2D& self, std::shared_ptr<GridWaveFunction2D> psi, double dt, int steps){ self.relax(*psi, dt, steps); });
        lua.registerFunction<double (Schroedinger2D::*)(std::shared_ptr<GridWaveFunction2D>)>("energy",
                [](Schroedinger2D& self, std::shared_ptr<GridWaveFunction2D> psi){ return self.energy(*psi); });
        lua.registerFunction<double (Schroedinger2D::*)(std::shared_ptr<GridWaveFunction2D>)>("angular_momentum",
                [](Schroedinger2D& self, std::shared_ptr<GridWaveFunction2D> psi){ return self.angular_momentum(*psi); });

        /*
         * Dirac
         */
        lua.writeFunction("spinor", [](std::shared_ptr<GridWaveFunction> up, std::shared_ptr<GridWaveFunction> down){
            return std::make_shared<GridSpinor>(*up, *down);
        });
        lua.registerFunction("norm", &GridSpinor::norm);
        lua.registerFunction("normalize", &GridSpinor::normalize);
        lua.registerFunction("validate", &GridSpinor::validate);
        lua.registerFunction("expect_x", &GridSpinor::expect_x);
        lua.registerFunction("probability", &GridSpinor::probability);

        lua.registerFunction("set_mass", &Dirac1D::set_mass);
        lua.registerFunction("set_c", &Dirac1D::set_c);
        lua.registerFunction("set_potential", &Dirac1D::set_potential);
        lua.registerFunction("set_absorber", &Dirac1D::set_absorber);
        lua.registerFunction<void (Dirac1D::*)(std::shared_ptr<GridSpinor>, double, double, double)>("propagate",
                [](Dirac1D& self, std::shared_ptr<GridSpinor> psi, double t_start, double t_final, double dt){ self.propagate(*psi, t_start, t_final, dt); });
        lua.registerFunction<void (Dirac1D::*)(std::shared_ptr<GridSpinor>, int)>("project",
                [](Dirac1D& self, std::shared_ptr<GridSpinor> psi, int sign){ self.project(*psi, sign); });
        lua.registerFunction<double (Dirac1D::*)(std::shared_ptr<GridSpinor>)>("positive_energy_fraction",
                [](Dirac1D& self, std::shared_ptr<GridSpinor> psi){ return self.positive_energy_fraction(*psi); });
        lua.registerFunction<double (Dirac1D::*)(std::shared_ptr<GridSpinor>)>("velocity",
                [](Dirac1D& self, std::shared_ptr<GridSpinor> psi){ return self.velocity(*psi); });
        lua.registerFunction<double (Dirac1D::*)(std::shared_ptr<GridSpinor>)>("energy",
                [](Dirac1D& self, std::shared_ptr<GridSpinor> psi){ return self.energy(*psi); });
    }

    void System::expose_plot(){
        lua.registerFunction("wavefunction", &System::plot_wavefunction);
        lua.registerFunction("wave", &System::plot_wave);
        lua.registerFunction("wave2d", &System::plot_wave2d);
        lua.registerFunction("momentum", &System::plot_momentum);
        lua.registerFunction("wigner", &System::plot_wigner);
        lua.registerFunction("ensemble", &System::plot_ensemble);
        lua.registerFunction("wigner_p", &System::plot_wigner_p);
        lua.registerFunction("wigner_smooth", &System::plot_wigner_smooth);
        lua.registerFunction("ensemble_smooth", &System::plot_ensemble_smooth);
        lua.registerFunction("ensemble_p", &System::plot_ensemble_p);
        lua.registerFunction("momentum_compare", &System::plot_momentum_compare);
        lua.registerFunction("density_matrix", &System::plot_density_matrix);
        lua.registerFunction("scalar", &System::plot_scalar);
        lua.registerFunction("levels", &System::plot_levels);
        lua.registerFunction("trajectories", &System::plot_trajectories);
        lua.registerFunction("carpet", &System::plot_carpet);
        lua.registerFunction("field", &System::plot_field);
        lua.registerFunction("marginals", &System::plot_marginals);
        lua.registerFunction("spinor", &System::plot_spinor);
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
    void System::plot_wavefunction(AnyWaveFunction wavefunction, std::string title, int points){
        json plot = { {"type", "plot"}, {"kind", "wavefunction"}, {"title", title} };

        if(auto* spectral = boost::get<std::shared_ptr<WaveFunction>>(&wavefunction)){
            if(points<2) points = 200;
            auto [lower, upper] = (*spectral)->basis->plot_boundaries();
            Eigen::VectorXd x(points);
            for(int i=0; i<points; i++) x(i) = lower + 1.*i/points*(upper - lower);
            Eigen::VectorXcd vals = (*spectral)->grid(x);

            plot["lower"] = lower;
            plot["upper"] = upper;
            plot["re"] = encode_f32_from(vals.real());
            plot["im"] = encode_f32_from(vals.imag());
        }else{
            auto& psi = *boost::get<std::shared_ptr<GridWaveFunction>>(wavefunction);
            int stride = stride_for(psi.grid->size, points>0 ? points : 2048);

            plot["lower"] = psi.grid->lower;
            plot["upper"] = psi.grid->upper;
            plot["re"] = encode_f32(strided(psi.psi.real(), stride));
            plot["im"] = encode_f32(strided(psi.psi.imag(), stride));
        }

        emit(std::move(plot));
    }

    void System::plot_wave(std::shared_ptr<Schroedinger1D> system, std::shared_ptr<GridWaveFunction> wavefunction, std::string title){
        const GridWaveFunction& psi = *wavefunction;
        int stride = stride_for(psi.grid->size, 2048);

        emit({ {"type", "plot"}, {"kind", "wavefunction"}, {"title", title},
               {"lower", psi.grid->lower}, {"upper", psi.grid->upper},
               {"re", encode_f32(strided(psi.psi.real(), stride))},
               {"im", encode_f32(strided(psi.psi.imag(), stride))},
               {"V", encode_f32(strided(system->V, stride))} });
    }

    void System::plot_momentum(std::shared_ptr<GridWaveFunction> wavefunction, std::string title, double p_max){
        Eigen::ArrayXd p;
        Eigen::ArrayXcd phi;
        wavefunction->momentum_space(p, phi);

        if(!(p_max>0.)) p_max = wavefunction->grid->p_max();
        std::vector<float> re, im;
        double lower = 0., upper = 0.;
        bool first = true;
        for(int i=0; i<p.size(); i++){
            if(std::abs(p(i))>p_max) continue;
            if(first){ lower = p(i); first = false; }
            upper = p(i);
            re.push_back(float(phi(i).real()));
            im.push_back(float(phi(i).imag()));
        }
        // The frontend places sample i at lower + i*(upper-lower)/n
        if(re.size()>1) upper += (upper-lower)/(re.size()-1);

        emit({ {"type", "plot"}, {"kind", "wavefunction"}, {"title", title}, {"space", "p"},
               {"lower", lower}, {"upper", upper}, {"re", encode_f32(re)}, {"im", encode_f32(im)} });
    }

    void System::plot_wigner(AnyState state, std::string title, int points){
        plot_wigner_p(state, title, points, 0.);
    }

    void System::plot_wigner_p(AnyState state, std::string title, int points, double p_max){
        json plot = { {"type", "plot"}, {"kind", "wigner"}, {"title", title} };
        if(points<2) points = 128;

        if(auto* rho = boost::get<std::shared_ptr<DensityOperator>>(&state)){
            wigner_to_json(WignerFunction{ **rho, points }, plot, p_max);
        }else if(auto* psi = boost::get<std::shared_ptr<GridWaveFunction>>(&state)){
            wigner_to_json((*psi)->wigner(points), plot, p_max);
        }else{
            wigner_to_json(boost::get<std::shared_ptr<GridDensityMatrix>>(state)->wigner(points), plot, p_max);
        }

        emit(std::move(plot));
    }

    void System::plot_ensemble(std::shared_ptr<ClassicalEnsemble> ensemble, std::shared_ptr<Schroedinger1D> system, std::string title, int points){
        plot_ensemble_p(ensemble, system, title, points, 0.);
    }

    void System::plot_ensemble_p(std::shared_ptr<ClassicalEnsemble> ensemble, std::shared_ptr<Schroedinger1D> system, std::string title, int points, double p_max){
        json plot = { {"type", "plot"}, {"kind", "wigner"}, {"title", title} };
        // Ensembles sampled from a Wigner function carry negative weights: show them on the signed scale
        if(ensemble->weight.minCoeff()>=0.) plot["classical"] = true;
        if(points<2) points = 128;
        wigner_to_json(ensemble->histogram(*system->grid, points), plot, p_max);
        emit(std::move(plot));
    }

    void System::plot_wigner_smooth(std::shared_ptr<GridWaveFunction> wavefunction, std::string title, int points, double p_max, double sigma_x, double sigma_p){
        json plot = { {"type", "plot"}, {"kind", "wigner"}, {"title", title} };
        wigner_to_json(core::grid::smooth(wavefunction->wigner(points<2 ? 128 : points), sigma_x, sigma_p), plot, p_max);
        emit(std::move(plot));
    }

    void System::plot_ensemble_smooth(std::shared_ptr<ClassicalEnsemble> ensemble, std::shared_ptr<Schroedinger1D> system, std::string title, int points, double p_max, double sigma_x, double sigma_p){
        json plot = { {"type", "plot"}, {"kind", "wigner"}, {"title", title} };
        if(ensemble->weight.minCoeff()>=0.) plot["classical"] = true;
        wigner_to_json(core::grid::smooth(ensemble->histogram(*system->grid, points<2 ? 128 : points), sigma_x, sigma_p), plot, p_max);
        emit(std::move(plot));
    }

    void System::plot_momentum_compare(std::shared_ptr<GridWaveFunction> wavefunction, std::shared_ptr<ClassicalEnsemble> ensemble, std::string title, double p_max, int bins){
        if(!(p_max>0.)) p_max = wavefunction->grid->p_max();
        if(bins<2) bins = 200;
        double width = 2.*p_max/bins;

        Eigen::ArrayXd p;
        Eigen::ArrayXcd phi;
        wavefunction->momentum_space(p, phi);
        double dp = p(1) - p(0);

        std::vector<float> quantum(bins, 0.f), classical(bins, 0.f);
        for(int i=0; i<p.size(); i++){
            int b = int(std::floor((p(i) + p_max)/width));
            if(b>=0 && b<bins) quantum[b] += float(std::norm(phi(i))*dp/width);
        }
        for(int i=0; i<ensemble->p.size(); i++){
            int b = int(std::floor((ensemble->p(i) + p_max)/width));
            if(b>=0 && b<bins) classical[b] += float(ensemble->weight(i)/width);
        }

        json plot = { {"type", "plot"}, {"kind", "curves"}, {"title", title}, {"lower", -p_max}, {"upper", p_max}, {"xlabel", "p"} };
        plot["series"] = json::array({
            { {"name", "quantum"},   {"y", encode_f32(quantum)} },
            { {"name", "classical"}, {"y", encode_f32(classical)} },
        });
        emit(std::move(plot));
    }

    void System::plot_density_matrix(std::shared_ptr<GridDensityMatrix> rho, std::string title, int points){
        const Grid& g = *rho->grid;
        int stride = stride_for(g.size, points>0 ? points : 256);
        int n = (g.size + stride - 1)/stride;

        std::vector<float> data;
        data.reserve(n*n);
        for(int iy=0; iy<g.size; iy+=stride){
            for(int ix=0; ix<g.size; ix+=stride) data.push_back(float(std::abs(rho->rho(ix, iy))));
        }

        emit({ {"type", "plot"}, {"kind", "image"}, {"title", title},
               {"x0", g.lower}, {"x1", g.upper}, {"y0", g.lower}, {"y1", g.upper}, {"nx", n}, {"ny", n},
               {"xlabel", "x"}, {"ylabel", "x'"}, {"data", encode_f32(data)} });
    }

    void System::plot_scalar(std::string title, double t, std::string name, double value){
        emit({ {"type", "plot"}, {"kind", "scalar"}, {"title", title}, {"t", t}, {"name", name}, {"value", value} });
    }

    void System::plot_levels(std::shared_ptr<Schroedinger1D> system, int n, std::string title){
        const Grid& g = *system->grid;
        int stride = stride_for(g.size, 512);

        json plot = { {"type", "plot"}, {"kind", "levels"}, {"title", title},
                      {"lower", g.lower}, {"upper", g.upper}, {"V", encode_f32(strided(system->V, stride))} };
        plot["energies"] = json::array();
        plot["states"] = json::array();
        for(auto& state: system->eigenstates(n)){
            plot["energies"].push_back(state.first);
            plot["states"].push_back(encode_f32(strided(state.second->psi.real(), stride)));
        }

        emit(std::move(plot));
    }

    void System::plot_trajectories(std::shared_ptr<BohmianTrajectories> trajectories, std::string title){
        json plot = { {"type", "plot"}, {"kind", "paths"}, {"title", title}, {"xlabel", "t"}, {"ylabel", "x"} };

        std::vector<float> t(trajectories->times.begin(), trajectories->times.end());
        plot["t"] = encode_f32(t);
        plot["paths"] = json::array();
        for(int i=0; i<trajectories->x.size(); i++){
            std::vector<float> path;
            path.reserve(trajectories->history.size());
            for(auto& positions: trajectories->history) path.push_back(float(positions(i)));
            plot["paths"].push_back(encode_f32(path));
        }

        emit(std::move(plot));
    }

    void System::plot_carpet(std::shared_ptr<SpaceTime> carpet, std::string title){
        if(carpet->rows.empty()) return;

        std::vector<float> data;
        for(auto& row: carpet->rows){
            for(int i=0; i<row.size(); i++) data.push_back(float(row(i)));
        }

        emit({ {"type", "plot"}, {"kind", "image"}, {"title", title},
               {"x0", carpet->lower}, {"x1", carpet->upper}, {"y0", carpet->times.front()}, {"y1", carpet->times.back()},
               {"nx", carpet->rows[0].size()}, {"ny", carpet->rows.size()},
               {"xlabel", "x"}, {"ylabel", "t"}, {"data", encode_f32(data)} });
    }

    void System::emit_field(const GridWaveFunction2D& wavefunction, const Schroedinger2D* system, const std::string& title){
        const Grid& gx = *wavefunction.grid->x;
        const Grid& gy = *wavefunction.grid->y;
        int sx = stride_for(gx.size, 256);
        int sy = stride_for(gy.size, 256);

        std::vector<float> re, im, V;
        for(int iy=0; iy<gy.size; iy+=sy){
            for(int ix=0; ix<gx.size; ix+=sx){
                re.push_back(float(wavefunction.psi(ix, iy).real()));
                im.push_back(float(wavefunction.psi(ix, iy).imag()));
                if(system) V.push_back(float(system->V(ix, iy)));
            }
        }

        json plot = { {"type", "plot"}, {"kind", "field"}, {"title", title},
                      {"x0", gx.lower}, {"x1", gx.upper}, {"y0", gy.lower}, {"y1", gy.upper},
                      {"nx", (gx.size + sx - 1)/sx}, {"ny", (gy.size + sy - 1)/sy},
                      {"re", encode_f32(re)}, {"im", encode_f32(im)} };
        if(system) plot["V"] = encode_f32(V);

        emit(std::move(plot));
    }

    void System::plot_field(std::shared_ptr<GridWaveFunction2D> wavefunction, std::string title){
        emit_field(*wavefunction, nullptr, title);
    }

    void System::plot_wave2d(std::shared_ptr<Schroedinger2D> system, std::shared_ptr<GridWaveFunction2D> wavefunction, std::string title){
        emit_field(*wavefunction, system.get(), title);
    }

    void System::plot_marginals(std::shared_ptr<GridWaveFunction2D> wavefunction, std::string title){
        const Grid& gx = *wavefunction->grid->x;
        const Grid& gy = *wavefunction->grid->y;
        if(gx.size != gy.size || gx.lower != gy.lower || gx.upper != gy.upper){
            throw std::invalid_argument("marginals: both coordinates need the same grid");
        }
        int stride = stride_for(gx.size, 1024);

        json plot = { {"type", "plot"}, {"kind", "curves"}, {"title", title}, {"lower", gx.lower}, {"upper", gx.upper}, {"xlabel", "x"} };
        plot["series"] = json::array({
            { {"name", "particle 1"}, {"y", encode_f32(strided(wavefunction->marginal(0), stride))} },
            { {"name", "particle 2"}, {"y", encode_f32(strided(wavefunction->marginal(1), stride))} },
        });

        emit(std::move(plot));
    }

    void System::plot_spinor(std::shared_ptr<Dirac1D> system, std::shared_ptr<GridSpinor> spinor, std::string title){
        const Grid& g = *spinor->grid;
        int stride = stride_for(g.size, 2048);

        json plot = { {"type", "plot"}, {"kind", "curves"}, {"title", title}, {"lower", g.lower}, {"upper", g.upper}, {"xlabel", "x"} };
        plot["series"] = json::array({
            { {"name", "total density"},  {"y", encode_f32(strided(spinor->density(), stride))} },
            { {"name", "upper component"},   {"y", encode_f32(strided(spinor->up.abs2(), stride))} },
            { {"name", "lower component"}, {"y", encode_f32(strided(spinor->down.abs2(), stride))} },
        });
        plot["V"] = encode_f32(strided(system->V, stride));

        emit(std::move(plot));
    }
}
