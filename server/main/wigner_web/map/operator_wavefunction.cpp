#include "wigner_web/map/operator_wavefunction.h"
#include "wigner_web/map/operator.h"
#include "wigner_web/state/wave_function.h"

template<class VectorClass>
using Operator = wigner_web::map::Operator<VectorClass>;

using Basis = wigner_web::discretization::Basis;
using WaveFunction = wigner_web::state::WaveFunction;

namespace wigner_web::map{

    OperatorWaveFunction::OperatorWaveFunction(std::shared_ptr<const wigner_web::discretization::Basis> basis):
        Operator<WaveFunction>(basis), matrix(Eigen::MatrixXcd::Zero(basis->size, basis->size)) {}
    
    OperatorWaveFunction::OperatorWaveFunction(std::shared_ptr<const Basis> basis, const Eigen::MatrixXcd& matrix_): 
        Operator<WaveFunction>(basis), matrix(matrix_){}

    OperatorWaveFunction::OperatorWaveFunction(std::shared_ptr<const Basis> basis, int left_derivative, int right_derivative, std::function<std::complex<double>(double)> V, int order):
        Operator<WaveFunction>(basis){

        Eigen::MatrixXcd components = basis->discretize_op_cov(left_derivative, right_derivative, V, order);
        set_from_components_cov(components);
    }
        
    void OperatorWaveFunction::set_from_components_cov(const Eigen::MatrixXcd& components){
        matrix = basis->get_metric_contrav()*components;
    }
        

    void OperatorWaveFunction::apply(WaveFunction& wavefunction) const{
        wavefunction.vector = matrix*wavefunction.vector;
    }
}
