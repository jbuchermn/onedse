#ifndef SYSTEM_H
#define SYSTEM_H

#include <memory>
#include <map>
#include <utility>
#include <vector>
#include <string>
#include <boost/variant.hpp>
#include <LuaContext.hpp>
#include <json.hpp>

#include "wigner_web/discretization/basis.h"
#include "wigner_web/state/wave_function.h"
#include "wigner_web/state/density_operator.h"

namespace wigner_web::system{
    class System{
        LuaContext lua;

        mutable nlohmann::json result;
    public:
        System();

        /**
         * Lua API
         */
        std::shared_ptr<wigner_web::discretization::Basis> create_basis(std::string name, double lower, double upper, int order) const;
        std::shared_ptr<wigner_web::state::WaveFunction> create_wavefunction(std::shared_ptr<wigner_web::discretization::Basis> basis, std::function<std::map<std::string, double>(double)> psi, int order) const;
        std::shared_ptr<wigner_web::state::DensityOperator> create_density_operator(std::shared_ptr<wigner_web::discretization::Basis> basis) const;

        void plot_wavefunction(std::shared_ptr<wigner_web::state::WaveFunction> wavefunction, std::string name, int points);
        void plot_wigner(std::shared_ptr<wigner_web::state::DensityOperator> density_operator, std::string name, int points);

        void execute(std::string code, nlohmann::json& result);
    };
}

#endif
