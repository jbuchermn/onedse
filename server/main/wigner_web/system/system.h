#pragma once

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
#include "wigner_web/map/map_wavefunction.h"
#include "wigner_web/map/map_density_operator.h"
#include "wigner_web/propagation/propagator.h"
#include "wigner_web/propagation/runge_kutta.h"
#include "wigner_web/propagation/diagonal_propagator.h"

namespace wigner_web::system{
    class System{
        LuaContext lua;

        mutable nlohmann::json result;
    public:
        System();

        /**
         * Lua API
         */
        void plot_wavefunction(std::shared_ptr<wigner_web::state::WaveFunction> wavefunction, std::string name, int points);
        void plot_wigner(std::shared_ptr<wigner_web::state::DensityOperator> density_operator, std::string name, int points);

        void execute(std::string code, nlohmann::json& result);
    };
}
