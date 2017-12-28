#include "wigner_web/map/map_wavefunction.h"
#include "wigner_web/map/map.h"
#include "wigner_web/state/wave_function.h"

template<class VectorClass>
using Map = wigner_web::map::Map<VectorClass>;

using Basis = wigner_web::discretization::Basis;
using WaveFunction = wigner_web::state::WaveFunction;

namespace wigner_web::map{

    MapWaveFunction::MapWaveFunction(std::shared_ptr<const wigner_web::discretization::Basis> basis):
        Map<WaveFunction>(basis), matrix(Eigen::MatrixXcd::Zero(basis->size, basis->size)) {}
    
    MapWaveFunction::MapWaveFunction(std::shared_ptr<const Basis> basis, const Eigen::MatrixXcd& matrix_): 
        Map<WaveFunction>(basis), matrix(matrix_){}

    MapWaveFunction::MapWaveFunction(std::shared_ptr<const Basis> basis, int left_derivative, int right_derivative, std::function<std::complex<double>(double)> V, int order):
        Map<WaveFunction>(basis){

        Eigen::MatrixXcd components = basis->discretize_op_cov(left_derivative, right_derivative, V, order);
        set_from_components_cov(components);
    }

    const Eigen::MatrixXcd& MapWaveFunction::matrix_representation() const{
        return matrix;
    }
        
    void MapWaveFunction::add(int left_derivative, int right_derivative, std::function<std::complex<double>(double)> V, int order){
        *this += MapWaveFunction { basis, left_derivative, right_derivative, V, order };
    }
        
    void MapWaveFunction::set_from_components_cov(const Eigen::MatrixXcd& components){
        matrix = basis->get_metric_contrav()*components;
    }
        

    void MapWaveFunction::apply(WaveFunction& wavefunction) const{
        wavefunction.vector = matrix*wavefunction.vector;
    }
}
