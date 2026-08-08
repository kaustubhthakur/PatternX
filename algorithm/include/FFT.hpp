#ifndef FFT_HPP
#define FFT_HPP

#include <complex>
#include <vector>

std::vector<std::complex<double>>
computeFFT(const std::vector<double>& input);

std::vector<double>
computeMagnitude(const std::vector<std::complex<double>>& fft);

#endif