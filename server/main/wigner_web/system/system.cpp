#include <iostream>
#include <LuaContext.hpp>
#include <json.hpp>
#include <iomanip>

#include "wigner_web/system/system.h"
#include "wigner_web/discretization/basis.h"
#include "wigner_web/discretization/scaled_basis.h"
#include "wigner_web/discretization/orthogonal_chebyshev.h"
#include "wigner_web/discretization/orthogonal_legendre.h"
#include "wigner_web/state/wave_function.h"
#include "wigner_web/state/density_operator.h"
#include "wigner_web/state/wigner_function.h"
#include "wigner_web/map/operator_wavefunction.h"
#include "wigner_web/propagation/propagator.h"
#include "wigner_web/propagation/runge_kutta.h"
#include "wigner_web/propagation/diagonal_propagator.h"
#include "wigner_web/utility/lua_expose.h"

using json = nlohmann::json;
using Basis = wigner_web::discretization::Basis;
using ScaledBasis = wigner_web::discretization::ScaledBasis;
using OrthogonalChebyshev = wigner_web::discretization::OrthogonalChebyshev;
using OrthogonalLegendre = wigner_web::discretization::OrthogonalLegendre;
using WaveFunction = wigner_web::state::WaveFunction;
using DensityOperator = wigner_web::state::DensityOperator;
using WignerFunction = wigner_web::state::WignerFunction;
using OperatorWaveFunction = wigner_web::map::OperatorWaveFunction;

template<class StateClass>
using Propagator = wigner_web::propagation::Propagator<StateClass>;

template<class StateClass>
using RungeKutta = wigner_web::propagation::RungeKutta<StateClass>;

using DiagonalPropagator = wigner_web::propagation::DiagonalPropagator;

namespace wigner_web::system{
    System::System(){
        wigner_web::utility::lua_expose_complex<double>(lua);

        wigner_web::utility::lua_expose_linear_space_operators<WaveFunction,         std::complex<double>>(lua);
        wigner_web::utility::lua_expose_linear_space_operators<OperatorWaveFunction, std::complex<double>>(lua);
        wigner_web::utility::lua_expose_linear_space_operators<DensityOperator,      std::complex<double>>(lua);
        wigner_web::utility::lua_expose_linear_space_operators<WignerFunction,       double              >(lua);
        
        lua.registerFunction("norm", &DensityOperator::norm);
        lua.registerFunction("norm", &WaveFunction::norm);
        lua.registerFunction("norm", &WignerFunction::norm);

        lua.registerFunction("create_basis", &System::create_basis);
        lua.registerFunction("create_wavefunction", &System::create_wavefunction);
        lua.registerFunction("create_density_operator", &System::create_density_operator);
        lua.registerFunction("create_operator_wavefunction", &System::create_operator_wavefunction);
        lua.registerFunction("create_wavefunction_propagator", &System::create_wavefunction_propagator);

        lua.registerFunction("plot_wavefunction", &System::plot_wavefunction);
        lua.registerFunction("plot_wigner", &System::plot_wigner);

        lua.registerFunction("add_wavefunction", &DensityOperator::add_wavefunction);
        lua.registerFunction<void (DiagonalPropagator::*)(std::shared_ptr<WaveFunction>, double, double)>("step", [](DiagonalPropagator& prop, std::shared_ptr<WaveFunction> wf, double t_start, double t_step){
                prop.step(*wf, t_start, t_step);
        });


        lua.writeVariable("root", this);
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


    std::shared_ptr<Basis> System::create_basis(std::string name, double lower, double upper, int order) const{
        return std::make_shared<ScaledBasis>(std::make_shared<OrthogonalLegendre>(order), lower, upper);
    }

    std::shared_ptr<WaveFunction> System::create_wavefunction(std::shared_ptr<Basis> basis, std::function<std::complex<double>(double)> psi, int order) const{
        return std::make_shared<WaveFunction>(basis, psi, order);
    }


    std::shared_ptr<DensityOperator> System::create_density_operator(std::shared_ptr<Basis> basis) const{
        return std::make_shared<DensityOperator>(basis);
    }
        
    std::shared_ptr<OperatorWaveFunction> System::create_operator_wavefunction(std::shared_ptr<Basis> basis, int left_derivative, int right_derivative, std::function<std::complex<double>(double)> V, int order) const{
        return std::make_shared<OperatorWaveFunction>(basis, left_derivative, right_derivative, V, order);
    }
        
    std::shared_ptr<DiagonalPropagator> System::create_wavefunction_propagator(std::string name, std::shared_ptr<OperatorWaveFunction> map) const{
        return std::make_shared<DiagonalPropagator>(map);
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
