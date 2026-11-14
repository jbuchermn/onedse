#pragma once

#include <complex>

namespace core::grid{
    /**
     * Unnormalised in-place FFTs with cached plans (thread-safe planning).
     * sign = -1: forward (x -> k), sign = +1: backward (k -> x). A round trip multiplies by n.
     */
    void fft_1d(std::complex<double>* data, int n, int sign);

    /// data is laid out as data[ix + nx*iy]
    void fft_2d(std::complex<double>* data, int nx, int ny, int sign);

    /// Angular wave number of FFT bin i on a periodic domain of given length
    inline double wave_number(int i, int n, double length){
        const double two_pi = 6.283185307179586476925286766559;
        return two_pi/length * (i < (n+1)/2 ? i : i - n);
    }
}
