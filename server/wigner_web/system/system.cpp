#include <iostream>
#include <LuaContext.hpp>
#include <json.hpp>
#include <iomanip>

#include "wigner_web/system/system.h"
#include "wigner_web/discretization/basis.h"
#include "wigner_web/discretization/scaled_orthogonal_chebyshev.h"
#include "wigner_web/state/wave_function.h"

using json = nlohmann::json;
using Basis = wigner_web::discretization::Basis;
using ScaledOrthogonalChebyshev = wigner_web::discretization::ScaledOrthogonalChebyshev;
using WaveFunction = wigner_web::state::WaveFunction;

namespace wigner_web::system{
    System::System(){
        lua.registerFunction("create_basis", &System::create_basis);
        lua.registerFunction("create_wavefunction", &System::create_wavefunction);
        lua.registerFunction("get_wavefunction", &System::get_wavefunction);

        lua.writeVariable("root", this);
    }

    void System::execute(std::string code, json& result){
        this->result = json();
        lua.executeCode(code);
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

    void System::get_wavefunction(std::shared_ptr<WaveFunction> wavefunction, std::string name, int points){

    }
}
