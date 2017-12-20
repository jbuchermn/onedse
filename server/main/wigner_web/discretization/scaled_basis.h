#pragma once

#include <memory>
#include <Eigen/Dense>

#include "wigner_web/discretization/basis.h"


namespace wigner_web::discretization{
    
    class ScaledBasis: public wigner_web::discretization::Basis{
        std::shared_ptr<wigner_web::discretization::Basis> unscaled;

        double unscaled_lower;
        double unscaled_upper;
        double scaled_lower;
        double scaled_upper;

        inline double scale(double x) const{
            return scaled_lower + (scaled_upper-scaled_lower)*(x-unscaled_lower)/(unscaled_upper-unscaled_lower);
        }

        inline double unscale(double x) const{
            return unscaled_lower + (unscaled_upper-unscaled_lower)*(x-scaled_lower)/(scaled_upper-scaled_lower);
        }

        inline double jacobian() const{
            return (scaled_upper-scaled_lower)/(unscaled_upper-unscaled_lower);
        }

    public:
        ScaledBasis(std::shared_ptr<wigner_web::discretization::Basis> unscaled_, double scaled_lower_, double scaled_upper_);
        ScaledBasis(std::shared_ptr<wigner_web::discretization::Basis> unscaled_, double unscaled_lower_, double unscaled_upper_, double scaled_lower_, double scaled_upper_);

        Eigen::VectorXcd evaluate(double x, int derivative=0) const override;
        void quadrature(int order, Eigen::VectorXd& points, Eigen::VectorXd& weights) const override;
    };

}
