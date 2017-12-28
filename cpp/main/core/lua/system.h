#pragma once

#include <memory>
#include <map>
#include <utility>
#include <vector>
#include <string>
#include <boost/variant.hpp>
#include <LuaContext.hpp>
#include <json.hpp>

#include "core/discretization/basis.h"
#include "core/state/wave_function.h"
#include "core/state/density_operator.h"
#include "core/map/map_wavefunction.h"
#include "core/map/map_density_operator.h"
#include "core/propagation/propagator.h"
#include "core/propagation/runge_kutta.h"
#include "core/propagation/diagonal_propagator.h"

namespace core::lua{
    class System{
        LuaContext lua;

        mutable nlohmann::json result;
    public:
        System();

        /**
         * Lua API
         */
        void plot_wavefunction(std::shared_ptr<core::state::WaveFunction> wavefunction, std::string name, int points);
        void plot_wigner(std::shared_ptr<core::state::DensityOperator> density_operator, std::string name, int points);

        void execute(std::string code, nlohmann::json& result);
    };
}
