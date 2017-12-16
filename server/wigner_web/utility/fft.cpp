#include <Eigen/Dense>
#include <fftw3.h>

#include "wigner_web/utility/fft.h"

namespace wigner_web::utility{
    FFT::FFT(int size, bool forward_): forward(forward_){
        in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * size);
        out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * size);

        if(forward) plan = fftw_plan_dft_1d(size, in, out, FFTW_FORWARD, FFTW_MEASURE);
        else plan = fftw_plan_dft_1d(size, in, out, FFTW_BACKWARD,FFTW_MEASURE);
    }

    FFT::~FFT(){
        fftw_destroy_plan(plan);
        fftw_free(in);
        fftw_free(out);
    }

    Eigen::VectorXcd& FFT::operator()(Eigen::VectorXcd& vector){
        for(int k=0; k<vector.rows(); k++){
            // Momenta are sorted p_0,...p_(N/2),p_(-N/2),...p_(-1)
            int idx = !forward ? (k+vector.rows()/2)%vector.rows() : k;
            in[idx][0] = vector(k).real();
            in[idx][1] = vector(k).imag();
        }

        fftw_execute(plan);

        for(int k=0; k<vector.rows(); k++){
            // Momenta are sorted p_0,...p_(N/2),p_(-N/2),...p_(-1)
            int idx = forward ? (k+vector.rows()/2)%vector.rows() : k;
            vector(idx) = std::complex<double>(out[k][0], out[k][1]);
        }

        // include a normalization for conservation of sum_k |F[f](k)|^2=sum_l |f(l)|^2
        vector /= std::sqrt(double(vector.rows()));

        return vector;

    }
}

