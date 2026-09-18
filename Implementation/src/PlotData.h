#pragma once

#include "GaussNewton.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

struct PlotPoint
{
    double x{};
    double y{};
};

struct PlotRange
{
    double minimum{};
    double maximum{};
};

struct SeasonalFitPlotData
{
    std::vector<PlotPoint> observed;
    std::vector<PlotPoint> fitted;
    std::vector<PlotPoint> finalFitted;
    std::vector<PlotPoint> residuals;
    std::vector<PlotPoint> convergence;

    Parameters displayedParameters{};
    int displayedIteration{ 0 };
    int totalIterations{ 0 };

    PlotRange timeRange;
    PlotRange temperatureRange;
    PlotRange residualRange;
    PlotRange convergenceRange;
};

inline PlotRange makePlotRange(
    double minimum,
    double maximum,
    double relativeMargin = 0.08)
{
    if (minimum > maximum)
        std::swap(minimum, maximum);

    const double span = maximum - minimum;

    if (span <= 1e-12)
    {
        const double margin =
            std::max(1.0, std::abs(minimum) * relativeMargin);

        return { minimum - margin, maximum + margin };
    }

    const double margin = span * relativeMargin;

    return { minimum - margin, maximum + margin };
}

inline PlotRange makeSymmetricPlotRange(
    double maximumAbsoluteValue,
    double relativeMargin = 0.10)
{
    const double limit =
        std::max(1.0, maximumAbsoluteValue * (1.0 + relativeMargin));

    return { -limit, limit };
}

inline std::size_t clampIterationIndex(
    int requestedIteration,
    std::size_t historySize)
{
    if (historySize == 0)
        throw std::invalid_argument(
            "Cannot display an iteration because parameter history is empty.");

    if (requestedIteration <= 0)
        return 0;

    const std::size_t requested =
        static_cast<std::size_t>(requestedIteration);

    return std::min(requested, historySize - 1);
}

inline SeasonalFitPlotData makePlotData(
    const std::vector<double>& time,
    const std::vector<double>& temperature,
    const OptimizationResult& result,
    int requestedIteration = -1)
{
    validateDataVectors(time, temperature);

    if (result.parameterHistory.empty())
    {
        throw std::invalid_argument(
            "Optimization result does not contain parameter history.");
    }

    const int lastAvailableIteration =
        static_cast<int>(result.parameterHistory.size()) - 1;

    const int iterationToDisplay =
        requestedIteration < 0
            ? lastAvailableIteration
            : requestedIteration;

    const std::size_t historyIndex =
        clampIterationIndex(
            iterationToDisplay,
            result.parameterHistory.size());

    SeasonalFitPlotData data;

    data.displayedParameters = result.parameterHistory[historyIndex];
    data.displayedIteration = static_cast<int>(historyIndex);
    data.totalIterations = result.iterations;

    data.observed.reserve(time.size());
    data.fitted.reserve(time.size());
    data.finalFitted.reserve(time.size());
    data.residuals.reserve(time.size());
    const std::size_t visibleObjectiveCount = std::min(
        result.objectiveHistory.size(), historyIndex + 1);
    data.convergence.reserve(visibleObjectiveCount);

    const std::vector<double> currentFittedValues =
        fittedValues(time, data.displayedParameters);
    const std::vector<double> finalFittedValues =
        fittedValues(time, result.parameterHistory.back());

    double minimumTemperature =
        *std::min_element(temperature.begin(), temperature.end());

    double maximumTemperature =
        *std::max_element(temperature.begin(), temperature.end());

    double maximumAbsoluteResidual = 0.0;

    for (std::size_t i = 0; i < time.size(); ++i)
    {
        const double residual =
            temperature[i] - currentFittedValues[i];

        data.observed.push_back({ time[i], temperature[i] });
        data.fitted.push_back({ time[i], currentFittedValues[i] });
        data.finalFitted.push_back({ time[i], finalFittedValues[i] });
        data.residuals.push_back({ time[i], residual });

        minimumTemperature =
            std::min(minimumTemperature, currentFittedValues[i]);

        maximumTemperature =
            std::max(maximumTemperature, currentFittedValues[i]);

        maximumAbsoluteResidual =
            std::max(maximumAbsoluteResidual, std::abs(residual));
    }

    for (std::size_t i = 0; i < visibleObjectiveCount; ++i)
    {
        const double objectiveValue =
            std::max(result.objectiveHistory[i], 1e-12);

        data.convergence.push_back(
            {
                static_cast<double>(i),
                std::log10(objectiveValue)
            });
    }

    const auto [minimumTimeIt, maximumTimeIt] =
        std::minmax_element(time.begin(), time.end());

    data.timeRange =
        makePlotRange(*minimumTimeIt, *maximumTimeIt);

    data.temperatureRange =
        makePlotRange(minimumTemperature, maximumTemperature);

    data.residualRange =
        makeSymmetricPlotRange(maximumAbsoluteResidual);

    if (data.convergence.empty())
    {
        data.convergenceRange = { -1.0, 1.0 };
    }
    else
    {
        auto [minimumConvergenceIt, maximumConvergenceIt] =
            std::minmax_element(
                data.convergence.begin(),
                data.convergence.end(),
                [](const PlotPoint& first, const PlotPoint& second)
                {
                    return first.y < second.y;
                });

        data.convergenceRange =
            makePlotRange(
                minimumConvergenceIt->y,
                maximumConvergenceIt->y);
    }

    return data;
}
