#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

inline constexpr std::size_t ParameterCount = 6;

using Parameters = std::array<double, ParameterCount>;
using MatrixRow = std::array<double, ParameterCount>;

inline constexpr double PI = 3.14159265358979323846;
inline constexpr double MonthsPerYear = 12.0;
inline constexpr double OMEGA = 2.0 * PI / MonthsPerYear;

inline void validateDataVectors(
    const std::vector<double>& time,
    const std::vector<double>& temperature)
{
    if (time.empty())
        throw std::invalid_argument("Time vector must not be empty.");

    if (time.size() != temperature.size())
        throw std::invalid_argument("Time and temperature vectors must have the same size.");
}

inline double modelValue(double time, const Parameters& parameters)
{
    const double annualPhase = OMEGA * time + parameters[3];
    const double semiAnnualPhase = 2.0 * OMEGA * time + parameters[5];

    return parameters[0]
        + parameters[1] * time
        + parameters[2] * std::sin(annualPhase)
        + parameters[4] * std::sin(semiAnnualPhase);
}

inline std::vector<double> fittedValues(
    const std::vector<double>& time,
    const Parameters& parameters)
{
    std::vector<double> fitted;
    fitted.reserve(time.size());

    for (double value : time)
        fitted.push_back(modelValue(value, parameters));

    return fitted;
}

inline std::vector<double> residualValues(
    const std::vector<double>& time,
    const std::vector<double>& temperature,
    const Parameters& parameters)
{
    validateDataVectors(time, temperature);

    std::vector<double> residuals(time.size());

    for (std::size_t i = 0; i < time.size(); ++i)
        residuals[i] = temperature[i] - modelValue(time[i], parameters);

    return residuals;
}

inline double objective(
    const std::vector<double>& time,
    const std::vector<double>& temperature,
    const Parameters& parameters)
{
    validateDataVectors(time, temperature);

    double rss = 0.0;

    for (std::size_t i = 0; i < time.size(); ++i)
    {
        const double residual = temperature[i] - modelValue(time[i], parameters);
        rss += residual * residual;
    }

    return rss;
}

inline void residualsAndJacobian(
    const std::vector<double>& time,
    const std::vector<double>& temperature,
    const Parameters& parameters,
    std::vector<double>& residuals,
    std::vector<MatrixRow>& jacobian)
{
    validateDataVectors(time, temperature);

    residuals.resize(time.size());
    jacobian.resize(time.size());

    for (std::size_t i = 0; i < time.size(); ++i)
    {
        const double annualPhase = OMEGA * time[i] + parameters[3];
        const double semiAnnualPhase = 2.0 * OMEGA * time[i] + parameters[5];

        residuals[i] = temperature[i] - modelValue(time[i], parameters);

        jacobian[i] = {
            -1.0,
            -time[i],
            -std::sin(annualPhase),
            -parameters[2] * std::cos(annualPhase),
            -std::sin(semiAnnualPhase),
            -parameters[4] * std::cos(semiAnnualPhase)
        };
    }
}

inline double vectorNorm(const Parameters& values)
{
    double squaredNorm = 0.0;

    for (double value : values)
        squaredNorm += value * value;

    return std::sqrt(squaredNorm);
}

inline Parameters addScaledStep(
    const Parameters& parameters,
    const Parameters& step,
    double alpha)
{
    Parameters result{};

    for (std::size_t i = 0; i < ParameterCount; ++i)
        result[i] = parameters[i] + alpha * step[i];

    return result;
}

inline void normalizePhases(Parameters& parameters)
{
    constexpr double TwoPi = 2.0 * PI;

    auto normalize = [](double phase)
    {
        phase = std::fmod(phase + PI, TwoPi);

        if (phase < 0.0)
            phase += TwoPi;

        return phase - PI;
    };

    parameters[3] = normalize(parameters[3]);
    parameters[5] = normalize(parameters[5]);
}