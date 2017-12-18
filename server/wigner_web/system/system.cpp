#include <iostream>
#include <LuaContext.hpp>
#include <json.hpp>
#include <iomanip>

#include "wigner_web/system/system.h"
#include "wigner_web/discretization/basis.h"
#include "wigner_web/discretization/scaled_orthogonal_chebyshev.h"
#include "wigner_web/state/wave_function.h"
#include "wigner_web/state/density_operator.h"
#include "wigner_web/state/wigner_function.h"
#include "wigner_web/map/operator_wavefunction.h"
#include "wigner_web/propagation/propagator.h"
#include "wigner_web/propagation/runge_kutta.h"

using json = nlohmann::json;
using Basis = wigner_web::discretization::Basis;
using ScaledOrthogonalChebyshev = wigner_web::discretization::ScaledOrthogonalChebyshev;
using WaveFunction = wigner_web::state::WaveFunction;
using DensityOperator = wigner_web::state::DensityOperator;
using WignerFunction = wigner_web::state::WignerFunction;
using OperatorWaveFunction = wigner_web::map::OperatorWaveFunction;

template<class StateClass>
using Propagator = wigner_web::propagation::Propagator<StateClass>;

template<class StateClass>
using RungeKutta = wigner_web::propagation::RungeKutta<StateClass>;

namespace wigner_web::system{
    System::System(){
        lua.registerFunction("create_basis", &System::create_basis);
        lua.registerFunction("create_wavefunction", &System::create_wavefunction);
        lua.registerFunction("create_density_operator", &System::create_density_operator);
        lua.registerFunction("create_operator_wavefunction", &System::create_operator_wavefunction);
        lua.registerFunction("create_wavefunction_propagator", &System::create_wavefunction_propagator);

        lua.registerFunction("plot_wavefunction", &System::plot_wavefunction);
        lua.registerFunction("plot_wigner", &System::plot_wigner);

        lua.registerFunction("add_wavefunction", &DensityOperator::add_wavefunction);
        lua.registerFunction("norm", &WaveFunction::norm);

        // Polymorphic functions need to be registered for every implementation
        // Template functions need to be registered for every instantiation
        lua.registerFunction("step", &RungeKutta<WaveFunction>::step);
        lua.registerFunction("step", &RungeKutta<DensityOperator>::step);

        lua.registerFunction("add", &WaveFunction::operator+=);
        lua.registerFunction("add", &DensityOperator::operator+=);
        lua.registerFunction("add", &OperatorWaveFunction::operator+=);
        lua.registerFunction("subtract", &WaveFunction::operator-=);
        lua.registerFunction("subtract", &DensityOperator::operator-=);
        lua.registerFunction("subtract", &OperatorWaveFunction::operator-=);
        lua.registerFunction("multiply", &WaveFunction::operator*=);
        lua.registerFunction("multiply", &DensityOperator::operator*=);
        lua.registerFunction("multiply", &OperatorWaveFunction::operator*=);
        lua.registerFunction("divide", &WaveFunction::operator/=);
        lua.registerFunction("divide", &DensityOperator::operator/=);
        lua.registerFunction("divide", &OperatorWaveFunction::operator/=);

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
        return std::make_shared<ScaledOrthogonalChebyshev>(lower, upper, order);
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
        
    std::shared_ptr<Propagator<WaveFunction>> System::create_wavefunction_propagator(std::string name, std::shared_ptr<OperatorWaveFunction> map) const{
        return std::make_shared<RungeKutta<WaveFunction>>("RK4", map);
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
