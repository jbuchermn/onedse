#include <gtest/gtest.h>
#include <Eigen/Dense>

#include "core/utility/fft.h"

using FFT = core::utility::FFT;

TEST(fft, identity){
    FFT forward(1024, true);
    FFT backward(1024, false);

    Eigen::VectorXcd vec = Eigen::VectorXcd::Random(1024);
    Eigen::VectorXcd check = vec;
    forward(vec);
    backward(vec);

    EXPECT_NEAR((vec-check).norm(), 0., 1.e-12);
}

TEST(fft, plancherel){
    FFT forward(1024, true);

    Eigen::VectorXcd vec = Eigen::VectorXcd::Random(1024);
    Eigen::VectorXcd check = vec;
    forward(vec);

    EXPECT_NEAR(vec.norm(), check.norm(), 1.e-12);
}
