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
#include "core/grid/grid.h"
#include "core/grid/grid2d.h"
#include "core/grid/dirac.h"

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

        void emit_field(const core::grid::GridWaveFunction2D& wavefunction, const core::grid::Schroedinger2D* system, const std::string& title);

        void emit(nlohmann::json&& message);
        void expose_spectral();
        void expose_grid();
        void expose_plot();

    public:
        /// sandbox: remove everything from Lua that touches the file system or the OS
        System(Sink sink_, bool sandbox=false);

        /// Returns false if the script raised an error (which is reported to the sink)
        bool execute(const std::string& code);

        /*
         * Lua API (plot:...)
         */
        using AnyWaveFunction = boost::variant<std::shared_ptr<core::state::WaveFunction>, std::shared_ptr<core::grid::GridWaveFunction>>;
        using AnyState = boost::variant<
            std::shared_ptr<core::state::DensityOperator>,
            std::shared_ptr<core::grid::GridWaveFunction>,
            std::shared_ptr<core::grid::GridDensityMatrix>>;

        void print(std::string data);
        void print_exception(const std::exception& e, int level=0);

        void plot_wavefunction(AnyWaveFunction wavefunction, std::string title, int points);
        void plot_wave(std::shared_ptr<core::grid::Schroedinger1D> system, std::shared_ptr<core::grid::GridWaveFunction> wavefunction, std::string title);
        void plot_momentum(std::shared_ptr<core::grid::GridWaveFunction> wavefunction, std::string title, double p_max);
        void plot_wigner(AnyState state, std::string title, int points);
        void plot_wigner_p(AnyState state, std::string title, int points, double p_max);
        void plot_ensemble_p(std::shared_ptr<core::grid::ClassicalEnsemble> ensemble, std::shared_ptr<core::grid::Schroedinger1D> system, std::string title, int points, double p_max);
        void plot_momentum_compare(std::shared_ptr<core::grid::GridWaveFunction> wavefunction, std::shared_ptr<core::grid::ClassicalEnsemble> ensemble, std::string title, double p_max, int bins);
        void plot_wigner_smooth(std::shared_ptr<core::grid::GridWaveFunction> wavefunction, std::string title, int points, double p_max, double sigma_x, double sigma_p);
        void plot_ensemble_smooth(std::shared_ptr<core::grid::ClassicalEnsemble> ensemble, std::shared_ptr<core::grid::Schroedinger1D> system, std::string title, int points, double p_max, double sigma_x, double sigma_p);
        void plot_ensemble(std::shared_ptr<core::grid::ClassicalEnsemble> ensemble, std::shared_ptr<core::grid::Schroedinger1D> system, std::string title, int points);
        void plot_density_matrix(std::shared_ptr<core::grid::GridDensityMatrix> rho, std::string title, int points);
        void plot_scalar(std::string title, double t, std::string name, double value);
        void plot_levels(std::shared_ptr<core::grid::Schroedinger1D> system, int n, std::string title);
        void plot_trajectories(std::shared_ptr<core::grid::BohmianTrajectories> trajectories, std::string title);
        void plot_carpet(std::shared_ptr<core::grid::SpaceTime> carpet, std::string title);
        void plot_field(std::shared_ptr<core::grid::GridWaveFunction2D> wavefunction, std::string title);
        void plot_wave2d(std::shared_ptr<core::grid::Schroedinger2D> system, std::shared_ptr<core::grid::GridWaveFunction2D> wavefunction, std::string title);
        void plot_marginals(std::shared_ptr<core::grid::GridWaveFunction2D> wavefunction, std::string title);
        void plot_spinor(std::shared_ptr<core::grid::Dirac1D> system, std::shared_ptr<core::grid::GridSpinor> spinor, std::string title);
    };
}
