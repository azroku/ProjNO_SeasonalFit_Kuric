#pragma once

#include "SeasonalModel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using NormalMatrix = std::array<std::array<double, ParameterCount>, ParameterCount>;

enum class OptimizationStatus
{
    NotStarted,
    ConvergedGradient,
    ConvergedStep,
    ConvergedObjective,
    MaximumIterationsReached,
    SingularSystem,
    LineSearchFailed
};

struct OptimizationResult
{
    Parameters parameters{};

    std::vector<double> objectiveHistory;
    std::vector<Parameters> parameterHistory;

    std::vector<double> fittedValues;
    std::vector<double> residuals;

    int iterations{ 0 };
    bool converged{ false };

    OptimizationStatus status{ OptimizationStatus::NotStarted };
    double finalGradientNorm{ std::numeric_limits<double>::infinity() };
    double finalStepNorm{ std::numeric_limits<double>::infinity() };
};

inline const char* optimizationStatusText(OptimizationStatus status)
{
    switch (status)
    {
    case OptimizationStatus::ConvergedGradient:
        return "Converged: gradient norm is below tolerance.";

    case OptimizationStatus::ConvergedStep:
        return "Converged: step norm is below tolerance.";

    case OptimizationStatus::ConvergedObjective:
        return "Converged: relative objective change is below tolerance.";

    case OptimizationStatus::MaximumIterationsReached:
        return "Stopped: maximum iteration count reached.";

    case OptimizationStatus::SingularSystem:
        return "Stopped: normal-equation system is singular or ill-conditioned.";

    case OptimizationStatus::LineSearchFailed:
        return "Stopped: Armijo line search could not find an acceptable step.";

    case OptimizationStatus::NotStarted:
    default:
        return "Optimization has not started.";
    }
}

inline bool solveLinearSystem(
    NormalMatrix matrix,
    Parameters rhs,
    Parameters& solution)
{
    constexpr double PivotTolerance = 1e-12;

    solution.fill(0.0);

    for (std::size_t column = 0; column < ParameterCount; ++column)
    {
        std::size_t pivot = column;

        for (std::size_t row = column + 1; row < ParameterCount; ++row)
        {
            if (std::abs(matrix[row][column]) >
                std::abs(matrix[pivot][column]))
            {
                pivot = row;
            }
        }

        if (std::abs(matrix[pivot][column]) < PivotTolerance)
            return false;

        if (pivot != column)
        {
            std::swap(matrix[column], matrix[pivot]);
            std::swap(rhs[column], rhs[pivot]);
        }

        for (std::size_t row = column + 1; row < ParameterCount; ++row)
        {
            const double factor =
                matrix[row][column] / matrix[column][column];

            matrix[row][column] = 0.0;

            for (std::size_t j = column + 1; j < ParameterCount; ++j)
                matrix[row][j] -= factor * matrix[column][j];

            rhs[row] -= factor * rhs[column];
        }
    }

    for (int row = static_cast<int>(ParameterCount) - 1; row >= 0; --row)
    {
        double value = rhs[static_cast<std::size_t>(row)];

        for (std::size_t column =
            static_cast<std::size_t>(row) + 1;
            column < ParameterCount;
            ++column)
        {
            value -=
                matrix[static_cast<std::size_t>(row)][column] *
                solution[column];
        }

        const double diagonal =
            matrix[static_cast<std::size_t>(row)]
            [static_cast<std::size_t>(row)];

        if (std::abs(diagonal) < PivotTolerance)
            return false;

        solution[static_cast<std::size_t>(row)] = value / diagonal;
    }

    return true;
}

inline Parameters multiplyMatrixVector(
    const NormalMatrix& matrix,
    const Parameters& vector)
{
    Parameters result{};

    for (std::size_t row = 0; row < ParameterCount; ++row)
    {
        for (std::size_t column = 0; column < ParameterCount; ++column)
            result[row] += matrix[row][column] * vector[column];
    }

    return result;
}

inline double dotProduct(
    const Parameters& first,
    const Parameters& second)
{
    double value = 0.0;

    for (std::size_t i = 0; i < ParameterCount; ++i)
        value += first[i] * second[i];

    return value;
}

inline void fillFinalValues(
    const std::vector<double>& time,
    const std::vector<double>& temperature,
    OptimizationResult& result)
{
    result.fittedValues = fittedValues(time, result.parameters);
    result.residuals = residualValues(time, temperature, result.parameters);
}

inline OptimizationResult optimizeGaussNewton(
    const std::vector<double>& time,
    const std::vector<double>& temperature,
    Parameters initial,
    int maxIterations = 100,
    double gradientTolerance = 1e-8,
    double stepTolerance = 1e-8,
    double relativeObjectiveTolerance = 1e-12)
{
    validateDataVectors(time, temperature);

    if (maxIterations <= 0)
        throw std::invalid_argument("maxIterations must be greater than zero.");

    if (gradientTolerance <= 0.0 ||
        stepTolerance <= 0.0 ||
        relativeObjectiveTolerance <= 0.0)
    {
        throw std::invalid_argument(
            "All optimization tolerances must be greater than zero.");
    }

    OptimizationResult result;
    result.parameters = initial;
    normalizePhases(result.parameters);

    result.parameterHistory.push_back(result.parameters);

    constexpr double Damping = 1e-6;
    constexpr double ArmijoConstant = 1e-4;
    constexpr double BacktrackingReduction = 0.5;
    constexpr double MinimumStepLength = 1e-10;

    for (int iteration = 0; iteration < maxIterations; ++iteration)
    {
        std::vector<double> residuals;
        std::vector<MatrixRow> jacobian;

        residualsAndJacobian(
            time,
            temperature,
            result.parameters,
            residuals,
            jacobian);

        const double currentObjective =
            objective(time, temperature, result.parameters);

        result.objectiveHistory.push_back(currentObjective);

        NormalMatrix normalMatrix{};
        Parameters negativeJtResidual{};

        for (std::size_t i = 0; i < jacobian.size(); ++i)
        {
            for (std::size_t row = 0; row < ParameterCount; ++row)
            {
                negativeJtResidual[row] -=
                    jacobian[i][row] * residuals[i];

                for (std::size_t column = 0;
                    column < ParameterCount;
                    ++column)
                {
                    normalMatrix[row][column] +=
                        jacobian[i][row] * jacobian[i][column];
                }
            }
        }

        result.finalGradientNorm = vectorNorm(negativeJtResidual);

        if (result.finalGradientNorm < gradientTolerance)
        {
            result.converged = true;
            result.status = OptimizationStatus::ConvergedGradient;
            result.iterations = iteration;
            fillFinalValues(time, temperature, result);
            return result;
        }

        for (std::size_t i = 0; i < ParameterCount; ++i)
        {
            const double diagonalScale =
                std::max(1.0, std::abs(normalMatrix[i][i]));

            normalMatrix[i][i] += Damping * diagonalScale;
        }

        Parameters step{};

        if (!solveLinearSystem(normalMatrix, negativeJtResidual, step))
        {
            result.status = OptimizationStatus::SingularSystem;
            result.iterations = iteration;
            fillFinalValues(time, temperature, result);
            return result;
        }

        result.finalStepNorm = vectorNorm(step);

        if (result.finalStepNorm < stepTolerance)
        {
            result.converged = true;
            result.status = OptimizationStatus::ConvergedStep;
            result.iterations = iteration;
            fillFinalValues(time, temperature, result);
            return result;
        }

        /*
         * For r = y - f, grad F(theta) = 2 J^T r.
         * Since negativeJtResidual = -J^T r, the directional derivative is
         * -2 * negativeJtResidual^T d and must be negative for descent.
         */
        const double directionalDerivative =
            -2.0 * dotProduct(negativeJtResidual, step);

        if (directionalDerivative >= 0.0)
        {
            result.status = OptimizationStatus::LineSearchFailed;
            result.iterations = iteration;
            fillFinalValues(time, temperature, result);
            return result;
        }

        double alpha = 1.0;
        Parameters trialParameters{};
        double trialObjective = currentObjective;
        bool stepAccepted = false;

        while (alpha >= MinimumStepLength)
        {
            trialParameters =
                addScaledStep(result.parameters, step, alpha);

            normalizePhases(trialParameters);

            trialObjective =
                objective(time, temperature, trialParameters);

            const double armijoBound =
                currentObjective +
                ArmijoConstant * alpha * directionalDerivative;

            if (trialObjective <= armijoBound)
            {
                stepAccepted = true;
                break;
            }

            alpha *= BacktrackingReduction;
        }

        if (!stepAccepted)
        {
            result.status = OptimizationStatus::LineSearchFailed;
            result.iterations = iteration;
            fillFinalValues(time, temperature, result);
            return result;
        }

        const double relativeObjectiveChange =
            std::abs(currentObjective - trialObjective) /
            std::max(1.0, std::abs(currentObjective));

        result.parameters = trialParameters;
        result.parameterHistory.push_back(result.parameters);
        result.iterations = iteration + 1;

        if (relativeObjectiveChange < relativeObjectiveTolerance)
        {
            result.objectiveHistory.push_back(trialObjective);
            result.converged = true;
            result.status = OptimizationStatus::ConvergedObjective;
            fillFinalValues(time, temperature, result);
            return result;
        }
    }

    result.status = OptimizationStatus::MaximumIterationsReached;
    // Keep objectiveHistory aligned with parameterHistory, including the
    // parameters produced by the final accepted step.
    if (result.objectiveHistory.size() < result.parameterHistory.size())
    {
        result.objectiveHistory.push_back(
            objective(time, temperature, result.parameters));
    }
    fillFinalValues(time, temperature, result);

    return result;
}
