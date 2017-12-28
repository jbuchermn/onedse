#pragma once

#include <memory>
#include <functional>
#include <Eigen/Dense>

#include "wigner_web/discretization/basis.h"
#include "wigner_web/map/map.h"
#include "wigner_web/state/density_operator.h"

namespace wigner_web::map{

    class MapDensityOperator: public wigner_web::map::Map<wigner_web::state::DensityOperator>{

        class Term{
        public:
            std::shared_ptr<const wigner_web::discretization::Basis> basis;

            /**
             * Left matrix has indices M^î_j, right matrix M_i^j.
             * Matrices of size 0x0 are interpreted as 0.
             */
            Eigen::MatrixXcd left_matrix;
            Eigen::MatrixXcd right_matrix;

            Term(std::shared_ptr<const wigner_web::discretization::Basis> basis_);

            void setup_matrix(Eigen::MatrixXcd& matrix) const;

            void set_left_from_components_cov(const Eigen::MatrixXcd& components);
            void set_right_from_components_cov(const Eigen::MatrixXcd& components);

            /**
             * Returns true if other term has been added
             */
            bool try_add(const Term& other);
            Eigen::MatrixXcd apply(const Eigen::MatrixXcd& density_operator) const;

            Term& operator*=(const std::complex<double>& scalar);
        };

        std::vector<Term> terms;
        MapDensityOperator(std::shared_ptr<const wigner_web::discretization::Basis> basis, std::vector<Term> terms_);
        
    public:
        /**
         * Full representation of the operator with mixed indices rho'^{i + N*j} = M^{i + N*j}_{k + N*l} rho^{k + N*l}
         *
         * Not set up by default, as this can become a serious memory penalty
         */
        mutable Eigen::MatrixXcd matrix;
        
        MapDensityOperator(std::shared_ptr<const wigner_web::discretization::Basis> basis);

        void setup_matrix() const;
        const Eigen::MatrixXcd& matrix_representation() const override;

        void add_from_components_left(const Eigen::MatrixXcd& components_cov);
        void add_from_components_right(const Eigen::MatrixXcd& components_cov);
        void add_from_components(const Eigen::MatrixXcd& left_components_cov, const Eigen::MatrixXcd& right_components_cov);

        void add_left(int left_derivative, int right_derivative, std::function<std::complex<double>(double)> V, int order);
        void add_right(int left_derivative, int right_derivative, std::function<std::complex<double>(double)> V, int order);
        void add(int left_left_derivative,  int left_right_derivative,  std::function<std::complex<double>(double)> left_V,  int left_order,
                 int right_left_derivative, int right_right_derivative, std::function<std::complex<double>(double)> right_V, int right_order);

        void apply(wigner_web::state::DensityOperator& density_operator) const override;
        
        MapDensityOperator& operator*=(const std::complex<double>& scalar);
        MapDensityOperator& operator+=(const MapDensityOperator& other); 
        MapDensityOperator& operator/=(const std::complex<double>& scalar){ (*this)*(1./scalar); return *this; }
        MapDensityOperator& operator-=(const MapDensityOperator& other)   { (*this)+(-1.*other); return *this; }

        MapDensityOperator operator/(const std::complex<double>& scalar) const { MapDensityOperator result(*this); result/=scalar; return result; }
        MapDensityOperator operator*(const std::complex<double>& scalar) const { MapDensityOperator result(*this); result*=scalar; return result; }
        MapDensityOperator operator+(const MapDensityOperator& other) const    { MapDensityOperator result(*this); result+=other; return result; }
        MapDensityOperator operator-(const MapDensityOperator& other) const    { MapDensityOperator result(*this); result-=other; return result; }

        friend MapDensityOperator operator*(const std::complex<double>& scalar, const MapDensityOperator& op){
            return op*scalar;
        }
    };
}
