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

using json = nlohmann::json;
using Basis = wigner_web::discretization::Basis;
using ScaledOrthogonalChebyshev = wigner_web::discretization::ScaledOrthogonalChebyshev;
using WaveFunction = wigner_web::state::WaveFunction;
using DensityOperator = wigner_web::state::DensityOperator;
using WignerFunction = wigner_web::state::WignerFunction;

namespace wigner_web::system{
    System::System(){
        lua.registerFunction("create_basis", &System::create_basis);
        lua.registerFunction("create_wavefunction", &System::create_wavefunction);
        lua.registerFunction("create_density_operator", &System::create_density_operator);

        lua.registerFunction("plot_wavefunction", &System::plot_wavefunction);
        lua.registerFunction("plot_wigner", &System::plot_wigner);

        lua.registerFunction("diagonalize", &DensityOperator::diagonalize);
        lua.registerFunction("add_wavefunction", &DensityOperator::add_wavefunction);
        lua.registerFunction("evaluate", &WaveFunction::operator());
        lua.registerFunction("norm", &WaveFunction::norm);

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

    std::shared_ptr<WaveFunction> System::create_wavefunction(std::shared_ptr<Basis> basis, std::function<std::map<std::string, double>(double)> psi, int order) const{
        auto psi_complex = [&psi](double x){
            auto v = psi(x);
            return std::complex<double>{ v["re"], v["im"] };
        };
        
        return std::make_shared<WaveFunction>(basis, psi_complex, order);
    }


    std::shared_ptr<wigner_web::state::DensityOperator> System::create_density_operator(std::shared_ptr<wigner_web::discretization::Basis> basis) const{
        return std::make_shared<DensityOperator>(basis);
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
