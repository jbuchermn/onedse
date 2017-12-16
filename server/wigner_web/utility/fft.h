#ifndef FFT_H
#define FFT_H

#include <Eigen/Dense>
#include <fftw3.h>

namespace wigner_web::utility{
    class FFT{
        fftw_complex* in;
        fftw_complex* out;

        bool forward;
        fftw_plan plan;
    public:
        FFT(int size, bool forward_);
        ~FFT();

        Eigen::VectorXcd& operator()(Eigen::VectorXcd& vector);
    };
}

#endif
