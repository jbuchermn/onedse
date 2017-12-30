#pragma once

#include <memory>
#include <functional>
#include <Eigen/Dense>

#include "core/discretization/basis.h"
#include "core/map/map.h"
#include "core/state/wave_function.h"

namespace core::map{

    class MapWaveFunction: public core::map::Map<core::state::WaveFunction>{
    public:
        /// Matrix with mixed indices M^î_j
        Eigen::MatrixXcd matrix;
        
        MapWaveFunction(std::shared_ptr<const core::discretization::Basis> basis);
        MapWaveFunction(std::shared_ptr<const core::discretization::Basis> basis, int left_derivative, int right_derivative, std::function<std::complex<double>(double)> V, int order);
        MapWaveFunction(std::shared_ptr<const core::discretization::Basis> basis, const Eigen::MatrixXcd& matrix);

        void set_from_components_cov(const Eigen::MatrixXcd& components);
        void add_term(int left_derivative, int right_derivative, std::function<std::complex<double>(double)> V, int order);

        const Eigen::MatrixXcd& matrix_representation() const override;
        void apply(core::state::WaveFunction& wavefunction) const override;
        
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
