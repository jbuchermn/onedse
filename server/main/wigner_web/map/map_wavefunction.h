#pragma once

#include <memory>
#include <functional>
#include <Eigen/Dense>

#include "wigner_web/discretization/basis.h"
#include "wigner_web/map/map.h"
#include "wigner_web/state/wave_function.h"

namespace wigner_web::map{

    class MapWaveFunction: public wigner_web::map::Map<wigner_web::state::WaveFunction>{
    public:
        /// Matrix with mixed indices M^î_j
        Eigen::MatrixXcd matrix;
        
        MapWaveFunction(std::shared_ptr<const wigner_web::discretization::Basis> basis);
        MapWaveFunction(std::shared_ptr<const wigner_web::discretization::Basis> basis, int left_derivative, int right_derivative, std::function<std::complex<double>(double)> V, int order);
        MapWaveFunction(std::shared_ptr<const wigner_web::discretization::Basis> basis, const Eigen::MatrixXcd& matrix);

        void set_from_components_cov(const Eigen::MatrixXcd& components);

        void apply(wigner_web::state::WaveFunction& wavefunction) const override;
        
        MapWaveFunction& operator/=(const std::complex<double>& scalar){ matrix/=scalar;       return *this; }
        MapWaveFunction& operator*=(const std::complex<double>& scalar){ matrix*=scalar;       return *this; }
        MapWaveFunction& operator+=(const MapWaveFunction& other) { matrix+=other.matrix; return *this; }
        MapWaveFunction& operator-=(const MapWaveFunction& other) { matrix-=other.matrix; return *this; }

        MapWaveFunction operator/(const std::complex<double>& scalar) const { return MapWaveFunction{basis, matrix/scalar}; }
        MapWaveFunction operator*(const std::complex<double>& scalar) const { return MapWaveFunction{basis, matrix*scalar}; }
        MapWaveFunction operator+(const MapWaveFunction& other) const  { return MapWaveFunction{basis, matrix+other.matrix}; }
        MapWaveFunction operator-(const MapWaveFunction& other) const  { return MapWaveFunction{basis, matrix-other.matrix}; }

        friend MapWaveFunction operator*(const std::complex<double>& scalar, const MapWaveFunction& op){
            return op*scalar;
        }
    };
}
