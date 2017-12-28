#include "wigner_web/map/map_density_operator.h"
#include "wigner_web/discretization/basis.h"
#include "wigner_web/map/map.h"
#include "wigner_web/state/density_operator.h"

template<class StateClass>
using Map = wigner_web::map::Map<StateClass>;

using Basis = wigner_web::discretization::Basis;
using DensityOperator = wigner_web::state::DensityOperator;

namespace wigner_web::map{

    MapDensityOperator::Term::Term(std::shared_ptr<const Basis> basis_): basis(basis_), left_matrix(0, 0), right_matrix(0, 0){}

    void MapDensityOperator::Term::set_left_from_components_cov(const Eigen::MatrixXcd& components){
        left_matrix = basis->get_metric_contrav()*components;
    }

    void MapDensityOperator::Term::set_right_from_components_cov(const Eigen::MatrixXcd& components){
        right_matrix = components*basis->get_metric_contrav();
    }
            
    void MapDensityOperator::Term::setup_matrix(Eigen::MatrixXcd& matrix) const{

        if(left_matrix.rows()==0 && right_matrix.rows()!=0){
            int N = right_matrix.rows();

            for(int i=0; i<N; i++){
                for(int j=0; j<N; j++){
                    for(int l=0; l<N; l++) matrix(l + N*i, l + N*j) += right_matrix(j, i);
                }
            }

        }else if(left_matrix.rows()!=0 && right_matrix.rows()==0){
            int N = left_matrix.rows();

            for(int i=0; i<N; i++){
                for(int j=0; j<N; j++){
                    for(int l=0; l<N; l++) matrix(i + N*l, j + N*l) += left_matrix(i, j);
                }
            }

        }else if(left_matrix.rows()!=0 && right_matrix.rows()!=0){
            int N = left_matrix.rows();

            for(int il=0; il<N; il++){
                for(int jl=0; jl<N; jl++){
                    for(int ir=0; ir<N; ir++){
                        for(int jr=0; jr<N; jr++){
                            matrix(il + N*jr, jl + N*ir) = left_matrix(il, jl)*right_matrix(ir, jr);
                        }
                    }
                }
            }
        }
    }
            
    bool MapDensityOperator::Term::try_add(const Term& other){
        if(left_matrix.rows()==0 && other.left_matrix.rows()==0){
            right_matrix += other.right_matrix;
            return true;
        }else if(right_matrix.rows()==0 && other.right_matrix.rows()==0){
            left_matrix += other.left_matrix;
            return true;
        }else{
            return false;
        }
    }
            
    Eigen::MatrixXcd MapDensityOperator::Term::apply(const Eigen::MatrixXcd& density_operator) const{
        if(left_matrix.rows()==0 && right_matrix.rows()!=0){
            return density_operator * right_matrix;
        }else if(left_matrix.rows()!=0 && right_matrix.rows()==0){
            return left_matrix * density_operator;
        }else if(left_matrix.rows()!=0 && right_matrix.rows()!=0){
            return left_matrix * density_operator * right_matrix;
        }else{
            return Eigen::MatrixXcd::Zero(density_operator.rows(), density_operator.cols());
        }
    }
            
    MapDensityOperator::Term& MapDensityOperator::Term::operator*=(const std::complex<double>& scalar){
        if(left_matrix.rows()!=0){
            left_matrix*=scalar;
        }else if(right_matrix.rows()!=0){
            right_matrix*=scalar;
        }    
    }
        
    MapDensityOperator::MapDensityOperator(std::shared_ptr<const Basis> basis): Map<DensityOperator>(basis) {}
    MapDensityOperator::MapDensityOperator(std::shared_ptr<const Basis> basis, std::vector<Term> terms_): Map<DensityOperator>(basis), terms(terms_) {}


    void MapDensityOperator::setup_matrix() const{
        matrix = Eigen::MatrixXcd::Zero(basis->size*basis->size, basis->size*basis->size);
        for(auto& term: terms){
            term.setup_matrix(matrix);
        }
    }
        
    const Eigen::MatrixXcd& MapDensityOperator::matrix_representation() const{
        if(matrix.rows()==0) setup_matrix();
        return matrix;
    }
    
    void MapDensityOperator::apply(DensityOperator& density_operator) const{
        if(matrix.rows()!=0){
            density_operator.vector = matrix*density_operator.vector;
        }else{

            Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(density_operator.matrix.rows(), density_operator.matrix.cols());
            for(auto& term: terms){
                result += term.apply(density_operator.matrix);
            }
            density_operator.matrix = result;
        }
    }
    
    MapDensityOperator& MapDensityOperator::operator*=(const std::complex<double>& scalar){
        for(auto& term: terms){
            term *= scalar;
        }
    }
        
    void MapDensityOperator::add_from_components_left(const Eigen::MatrixXcd& components_cov){
        Term term(basis);
        term.set_left_from_components_cov(components_cov);
        (*this) += MapDensityOperator{basis, {term}};
    }

    void MapDensityOperator::add_from_components_right(const Eigen::MatrixXcd& components_cov){
        Term term(basis);
        term.set_right_from_components_cov(components_cov);
        (*this) += MapDensityOperator{basis, {term}};
    }

    void MapDensityOperator::add_from_components(const Eigen::MatrixXcd& left_components_cov, const Eigen::MatrixXcd& right_components_cov){
        Term term(basis);
        term.set_left_from_components_cov(left_components_cov);
        term.set_right_from_components_cov(right_components_cov);
        (*this) += MapDensityOperator{basis, {term}};
    }
        
    void MapDensityOperator::add_left(int left_derivative, int right_derivative, std::function<std::complex<double>(double)> V, int order){
        add_from_components_left(basis->discretize_op_cov(left_derivative, right_derivative, V, order));
    }

    void MapDensityOperator::add_right(int left_derivative, int right_derivative, std::function<std::complex<double>(double)> V, int order){
        add_from_components_right(basis->discretize_op_cov(left_derivative, right_derivative, V, order));
    }

    void MapDensityOperator::add(int left_left_derivative,  int left_right_derivative,  std::function<std::complex<double>(double)> left_V,  int left_order,
                                 int right_left_derivative, int right_right_derivative, std::function<std::complex<double>(double)> right_V, int right_order){

        add_from_components(basis->discretize_op_cov(left_left_derivative,  left_right_derivative,  left_V,  left_order), 
                            basis->discretize_op_cov(right_left_derivative, right_right_derivative, right_V, right_order));
    }

    MapDensityOperator& MapDensityOperator::operator+=(const MapDensityOperator& other){
        for(auto& other_term: other.terms){
            for(auto& term: terms){
                if(term.try_add(other_term)) goto DoneWithOtherTerm;
            }
            terms.push_back(other_term);
DoneWithOtherTerm:;
        }
    }
}
