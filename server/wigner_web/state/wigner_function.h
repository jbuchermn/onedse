#ifndef WIGNER_FUNCTION_H
#define WIGNER_FUNCTION_H

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

        WignerFunction(double lower_x_, double upper_x_, double lower_p_, double upper_p_, Eigen::MatrixXd&& matrix_);
        WignerFunction(const wigner_web::state::WaveFunction& wavefunction, int points);
        WignerFunction(const wigner_web::state::DensityOperator& density_operator, int points);

        void set_from_wavefunction(const wigner_web::state::WaveFunction& wavefunction, int points);
        void plot_to_terminal() const;

        double get_lower_x() const;
        double get_upper_x() const;
        double get_lower_p() const;
        double get_upper_p() const;
        
        WignerFunction& operator/=(const double& scalar){ matrix/=scalar; return *this; }
        WignerFunction& operator*=(const double& scalar){ matrix*=scalar; return *this; }
        WignerFunction& operator+=(const WignerFunction& other){ matrix+=other.matrix; return *this; }
        WignerFunction& operator-=(const WignerFunction& other){ matrix-=other.matrix; return *this; }

        WignerFunction operator/(const double& scalar) const { return WignerFunction{lower_x, upper_x, lower_p, upper_p, matrix/scalar}; }
        WignerFunction operator*(const double& scalar) const { return WignerFunction{lower_x, upper_x, lower_p, upper_p, matrix*scalar}; }
        WignerFunction operator+(const WignerFunction& other) const { return WignerFunction{lower_x, upper_x, lower_p, upper_p, matrix+other.matrix}; }
        WignerFunction operator-(const WignerFunction& other) const { return WignerFunction{lower_x, upper_x, lower_p, upper_p, matrix-other.matrix}; }

        friend WignerFunction operator*(const double& scalar, const WignerFunction& wf){
            return wf*scalar;
        }

        void to_json(nlohmann::json& json) const;
    };
}

#endif
