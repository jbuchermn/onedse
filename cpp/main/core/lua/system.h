#pragma once

#include <memory>
#include <functional>
#include <exception>
#include <string>
#include <boost/variant.hpp>
#include <LuaContext.hpp>
#include <nlohmann/json.hpp>

#include "core/state/wave_function.h"
#include "core/state/density_operator.h"

namespace core::lua{
    /**
     * A Lua interpreter with the whole library exposed. Everything a script
     * produces (output, plots, errors) is passed as a JSON message to the sink
     * as soon as it is available, such that frontends can show results live.
     *
     * Messages:
     *   { type: "stdout", text }
     *   { type: "stderr", text }
     *   { type: "plot", kind, title: "Panel/Frame", ... }   see docs/api.md
     */
    class System{
    public:
        using Sink = std::function<void(const nlohmann::json&)>;

    private:
        LuaContext lua;
        Sink sink;

        void emit(nlohmann::json&& message);
        void expose_spectral();
        void expose_plot();

    public:
        /// sandbox: remove everything from Lua that touches the file system or the OS
        System(Sink sink_, bool sandbox=false);

        /// Returns false if the script raised an error (which is reported to the sink)
        bool execute(const std::string& code);

        /*
         * Lua API (plot:...)
         */
        void print(std::string data);
        void print_exception(const std::exception& e, int level=0);

        void plot_wavefunction(std::shared_ptr<core::state::WaveFunction> wavefunction, std::string title, int points);
        void plot_wigner(std::shared_ptr<core::state::DensityOperator> density_operator, std::string title, int points);
        void plot_scalar(std::string title, double t, std::string name, double value);
    };
}
