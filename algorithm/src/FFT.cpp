#include "../include/FFT.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <vector>

namespace
{
    const double PI = std::acos(-1.0);

    std::size_t nextPowerOfTwo(std::size_t n)
    {
        std::size_t power = 1;

        while (power < n)
        {
            power <<= 1;
        }

        return power;
    }

    void fft(std::vector<std::complex<double>>& a)
    {
        const std::size_t n = a.size();

    
        for (std::size_t i = 1, j = 0; i < n; ++i)
        {
            std::size_t bit = n >> 1;

            while (j & bit)
            {
                j ^= bit;
                bit >>= 1;
            }

            j ^= bit;

            if (i < j)
            {
                std::swap(a[i], a[j]);
            }
        }

        // Cooley-Tukey FFT
        for (std::size_t len = 2; len <= n; len <<= 1)
        {
            const double angle =
                -2.0 * PI / static_cast<double>(len);

            const std::complex<double> wLen(
                std::cos(angle),
                std::sin(angle)
            );

            for (std::size_t i = 0; i < n; i += len)
            {
                std::complex<double> w(1.0, 0.0);

                for (std::size_t j = 0; j < len / 2; ++j)
                {
                    const std::complex<double> u =
                        a[i + j];

                    const std::complex<double> v =
                        a[i + j + len / 2] * w;

                    a[i + j] = u + v;

                    a[i + j + len / 2] =
                        u - v;

                    w *= wLen;
                }
            }
        }
    }
}



std::vector<std::complex<double>>
computeFFT(const std::vector<double>& input)
{
    if (input.empty())
    {
        return {};
    }

    
    const std::size_t n =
        nextPowerOfTwo(input.size());

    std::vector<std::complex<double>> result(n);

    
    for (std::size_t i = 0;
         i < input.size();
         ++i)
    {
        result[i] =
            std::complex<double>(input[i], 0.0);
    }

  
    for (std::size_t i = input.size();
         i < n;
         ++i)
    {
        result[i] =
            std::complex<double>(0.0, 0.0);
    }

  
    fft(result);

    return result;
}



std::vector<double>
computeMagnitude(
    const std::vector<std::complex<double>>& fft)
{
    std::vector<double> magnitude;

    magnitude.reserve(fft.size());

    for (const auto& value : fft)
    {
        magnitude.push_back(std::abs(value));
    }

    return magnitude;
}