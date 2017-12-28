#pragma once

#include <Eigen/Dense>
#include <fftw3.h>

namespace core::utility{
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
