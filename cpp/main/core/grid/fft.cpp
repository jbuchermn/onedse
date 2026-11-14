#include <map>
#include <mutex>
#include <tuple>
#include <fftw3.h>

#include "core/grid/fft.h"

namespace core::grid{
    namespace{
        std::mutex plan_mutex;
        std::map<std::tuple<int, int, int>, fftw_plan> plans;

        fftw_plan get_plan(int nx, int ny, int sign){
            std::lock_guard<std::mutex> lock(plan_mutex);
            auto key = std::make_tuple(nx, ny, sign);
            auto it = plans.find(key);
            if(it != plans.end()) return it->second;

            // Plans are executed on arbitrary arrays (new-array interface), hence FFTW_UNALIGNED.
            fftw_complex* tmp = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * nx * (ny>0 ? ny : 1));
            fftw_plan plan = ny>0
                ? fftw_plan_dft_2d(ny, nx, tmp, tmp, sign<0 ? FFTW_FORWARD : FFTW_BACKWARD, FFTW_ESTIMATE | FFTW_UNALIGNED)
                : fftw_plan_dft_1d(nx,     tmp, tmp, sign<0 ? FFTW_FORWARD : FFTW_BACKWARD, FFTW_ESTIMATE | FFTW_UNALIGNED);
            fftw_free(tmp);

            plans[key] = plan;
            return plan;
        }
    }

    void fft_1d(std::complex<double>* data, int n, int sign){
        fftw_complex* d = reinterpret_cast<fftw_complex*>(data);
        fftw_execute_dft(get_plan(n, 0, sign), d, d);
    }

    void fft_2d(std::complex<double>* data, int nx, int ny, int sign){
        fftw_complex* d = reinterpret_cast<fftw_complex*>(data);
        fftw_execute_dft(get_plan(nx, ny, sign), d, d);
    }
}
