#pragma once

#include <Eigen/Dense>
#include <json.hpp>

#include "wigner_web/state/wave_function.h"
#include "wigner_web/state/density_operator.h"

namespace wigner_web::state{

    class WignerFunction{
        double lower_x;
        double upper_x;
        double lower_p;
        double upper_p;

    public:
        /// Grid representation: matrix(i, j) = W(x_i, p_j)
        Eigen::MatrixXd matrix;

        WignerFunction(const wigner_web::state::WaveFunction& wavefunction, int points);
        WignerFunction(const wigner_web::state::DensityOperator& density_operator, int points);

        void set_from_wavefunction(const wigner_web::state::WaveFunction& wavefunction, int points);
        void plot_to_terminal() const;

        double get_lower_x() const;
        double get_upper_x() const;
        double get_lower_p() const;
        double get_upper_p() const;
        
        void to_json(nlohmann::json& json) const;
    };
}
