#pragma once

#include "TemperatureData.h"
#include "ViewCanvas.h"
#include "SettingsView.h"
#include "EvaluationView.h"

#include <gui/Button.h>
#include <gui/GridLayout.h>
#include <gui/Label.h>
#include <gui/Slider.h>
#include <gui/StandardTabView.h>
#include <gui/Timer.h>
#include <gui/View.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

class MainView : public gui::View
{
    gui::Label _title{ "Seasonal Model Fitting - Monthly Temperature", gui::Font::ID::SystemLargerBold };
    gui::Label _formula{
        "f(t; θ) = a₀ + a₁t + A₁ sin(2πt/12 + φ₁) + A₂ sin(4πt/12 + φ₂)",
        gui::Font::ID::SystemNormal };
    gui::Label _iterationLabel{ "Iteration" };
    gui::Slider _iterationSlider{ gui::DataCtrl::Orientation::Horizontal, true };
    gui::Button _previousButton{ "Previous" };
    gui::Button _playButton{ "Play" };
    gui::Button _nextButton{ "Next" };
    gui::Button _resetButton{ "Reset" };
    gui::Label _speedLabel{ "Animation speed" };
    gui::Slider _speedSlider{ gui::DataCtrl::Orientation::Horizontal, true };
    gui::Label _statusLabel{ "Loading temperature data..." };
    ViewCanvas _canvas;
    SettingsView _settings;
    EvaluationView _evaluation;
    gui::StandardTabView _tabs;
    gui::GridLayout _layout{ 5, 8 };
    gui::Timer _animationTimer;

    std::vector<double> _time;
    std::vector<double> _temperature;
    OptimizationResult _result;
    int _displayedIteration{ 0 };
    bool _hasResult{ false };
    std::string _stationName{ "Sarajevo" };
    int _lastMaxIterations{ 100 };
    double _lastGradientTolerance{ 1e-8 };
    double _lastStepTolerance{ 1e-8 };
    double _lastObjectiveTolerance{ 1e-12 };

    int maximumIteration() const
    {
        return _result.parameterHistory.empty()
            ? 0
            : static_cast<int>(_result.parameterHistory.size()) - 1;
    }

    void updateStatus()
    {
        if (!_hasResult) return;
        const double rss = objective(_time, _temperature,
            _result.parameterHistory[static_cast<std::size_t>(_displayedIteration)]);
        double mean = 0.0;
        for (double value : _temperature) mean += value;
        mean /= static_cast<double>(_temperature.size());
        double totalSumSquares = 0.0;
        for (double value : _temperature)
        {
            const double centered = value - mean;
            totalSumSquares += centered * centered;
        }
        const double rmse = std::sqrt(rss / static_cast<double>(_temperature.size()));
        const double rSquared = 1.0 - rss / std::max(totalSumSquares, 1e-12);
        std::ostringstream status;
        status << _stationName << "    Iteration " << _displayedIteration << " / " << maximumIteration()
            << "    RSS: " << std::fixed << std::setprecision(2) << rss
            << "    RMSE: " << std::setprecision(3) << rmse
            << "    R^2: " << std::setprecision(4) << rSquared;
        _statusLabel.setTitle(status.str().c_str());
    }

    void qualityMetrics(double rss, double& rmse, double& rSquared) const
    {
        double mean = 0.0;
        for (double value : _temperature) mean += value;
        mean /= static_cast<double>(_temperature.size());
        double totalSumSquares = 0.0;
        for (double value : _temperature)
        {
            const double centered = value - mean;
            totalSumSquares += centered * centered;
        }
        rmse = std::sqrt(rss / static_cast<double>(_temperature.size()));
        rSquared = 1.0 - rss / std::max(totalSumSquares, 1e-12);
    }

    void showIteration(int iteration)
    {
        if (!_hasResult) return;
        _displayedIteration = std::max(0, std::min(iteration, maximumIteration()));
        _iterationSlider.setValue(static_cast<double>(_displayedIteration), false);
        _canvas.setPlotData(makePlotData(_time, _temperature, _result, _displayedIteration));
        updateStatus();
    }

    void stopAnimation()
    {
        _animationTimer.stop();
        _playButton.setTitle("Play");
    }

    void toggleAnimation()
    {
        if (!_hasResult) return;
        if (_animationTimer.isRunning())
        {
            stopAnimation();
            return;
        }
        if (_displayedIteration >= maximumIteration()) showIteration(0);
        _playButton.setTitle("Pause");
        _animationTimer.start();
    }

    void loadAndOptimize()
    {
        const bool useBjelasnica = _settings.stationIndex() == 1;
        _stationName = useBjelasnica ? "Bjelasnica" : "Sarajevo";
        const std::string fileName = std::string(PROJECT_DATA_DIR) +
            (useBjelasnica
                ? "/sarajevo/bjelasnica_temperature.csv"
                : "/sarajevo/sarajevo_temperature.csv");
        const auto records = loadTemperatureCsv(fileName);
        recordsToVectors(records, _time, _temperature);
        const Parameters initial = _settings.initialParameters();
        _lastMaxIterations = _settings.maxIterations();
        _lastGradientTolerance = _settings.gradientTolerance();
        _lastStepTolerance = _settings.stepTolerance();
        _lastObjectiveTolerance = _settings.objectiveTolerance();
        _result = optimizeGaussNewton(
            _time, _temperature, initial,
            _lastMaxIterations,
            _lastGradientTolerance,
            _lastStepTolerance,
            _lastObjectiveTolerance);
        _hasResult = true;
        _iterationSlider.setRange(0.0, static_cast<double>(maximumIteration()),
            std::max(2, maximumIteration() + 1));
        showIteration(0);

        const double initialRss = objective(_time, _temperature, initial);
        const double finalRss = objective(_time, _temperature, _result.parameters);
        double rmse = 0.0;
        double rSquared = 0.0;
        qualityMetrics(finalRss, rmse, rSquared);
        _evaluation.update(_stationName, _temperature.size(), _result,
            initialRss, finalRss, rmse, rSquared);
        _evaluation.setExportStatus("Ready to export the current run.");
    }

    void runFromSettings()
    {
        stopAnimation();
        try
        {
            loadAndOptimize();
            _tabs.setCurrentViewPos(0);
        }
        catch (const std::exception& error)
        {
            _statusLabel.setTitle((std::string("Error: ") + error.what()).c_str());
            showAlert("Optimization could not be completed", error.what());
        }
    }

    void exportResults()
    {
        if (!_hasResult) return;

        const std::filesystem::path exportDirectory = "SeasonalFit_exports";
        std::error_code directoryError;
        std::filesystem::create_directories(exportDirectory, directoryError);
        if (directoryError)
        {
            _evaluation.setExportStatus("Export failed: the SeasonalFit_exports folder could not be created.");
            return;
        }

        const auto now = std::chrono::system_clock::now();
        const std::time_t clockTime = std::chrono::system_clock::to_time_t(now);
        std::tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &clockTime);
#else
        localtime_r(&clockTime, &localTime);
#endif
        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count() % 1000;
        std::ostringstream fileName;
        fileName << _stationName << '_' << std::put_time(&localTime, "%Y%m%d_%H%M%S")
            << '_' << std::setw(3) << std::setfill('0') << milliseconds << ".csv";
        const std::filesystem::path outputPath = exportDirectory / fileName.str();

        std::ofstream output(outputPath);
        if (!output)
        {
            _evaluation.setExportStatus("Export failed: the output file could not be opened.");
            return;
        }

        const double initialRss = objective(
            _time, _temperature, _result.parameterHistory.front());
        const double finalRss = objective(_time, _temperature, _result.parameters);
        double rmse = 0.0;
        double rSquared = 0.0;
        qualityMetrics(finalRss, rmse, rSquared);
        output << std::setprecision(12)
            << "metric,value\n"
            << "station," << _stationName << '\n'
            << "observations," << _temperature.size() << '\n'
            << "iterations," << _result.iterations << '\n'
            << "maximum_iterations," << _lastMaxIterations << '\n'
            << "gradient_tolerance," << _lastGradientTolerance << '\n'
            << "step_tolerance," << _lastStepTolerance << '\n'
            << "relative_objective_tolerance," << _lastObjectiveTolerance << '\n'
            << "status,\"" << optimizationStatusText(_result.status) << "\"\n"
            << "initial_rss," << initialRss << '\n'
            << "final_rss," << finalRss << '\n'
            << "rmse," << rmse << '\n'
            << "r_squared," << rSquared << '\n';
        const char* names[] = { "a0", "a1", "A1", "phi1", "A2", "phi2" };
        for (std::size_t i = 0; i < ParameterCount; ++i)
        {
            output << "initial_" << names[i] << ','
                << _result.parameterHistory.front()[i] << '\n';
            output << "final_" << names[i] << ',' << _result.parameters[i] << '\n';
        }

        output << "\nmonth_index,observed_temperature,fitted_temperature,residual\n";
        for (std::size_t i = 0; i < _time.size(); ++i)
        {
            const double fitted = modelValue(_time[i], _result.parameters);
            output << _time[i] << ',' << _temperature[i] << ',' << fitted
                << ',' << (_temperature[i] - fitted) << '\n';
        }
        _evaluation.setExportStatus(
            std::string("Saved: ") + outputPath.generic_string());
    }

public:
    void getMinSize(gui::Size& minSize) const override
    {
        // The dashboard contains four information-dense panels. Allow the
        // window to grow freely, but prevent a resize below the point where
        // labels, legends and the parameter table would be clipped.
        minSize = gui::Size(1280, 760);
    }

    MainView()
        : gui::View(10, 10, 10, 10)
        , _animationTimer(this, 0.80f, false)
    {
        _tabs.addView(&_canvas, "Dashboard");
        _tabs.addView(&_settings, "Model Setup");
        _tabs.addView(&_evaluation, "Final Evaluation");
        _tabs.setCurrentViewPos(0);

        _layout.setMargins(6, 6);
        _layout.setSpaceBetweenCells(8, 8);
        _layout.insert(0, 0, _title, 8, td::HAlignment::Left);
        _layout.insert(1, 0, _formula, 8, td::HAlignment::Center);

        _layout.insert(2, 0, _iterationLabel);
        _layout.insert(2, 1, _iterationSlider, 3, td::HAlignment::Left);
        _layout.insert(2, 4, _previousButton);
        _layout.insert(2, 5, _playButton);
        _layout.insert(2, 6, _nextButton);
        _layout.insert(2, 7, _resetButton);

        _layout.insert(3, 0, _speedLabel);
        _layout.insert(3, 1, _speedSlider, 2, td::HAlignment::Left);
        _layout.insert(3, 3, _statusLabel, 5, td::HAlignment::Left);
        _layout.insert(4, 0, _tabs, 8, td::HAlignment::Left);
        setLayout(&_layout);

        _statusLabel.setResizable(20);
        _iterationSlider.setRange(0.0, 0.0, 1);
        _speedSlider.setRange(0.30, 1.50, 9);
        _speedSlider.setValue(0.80, false);
        _playButton.setAsDefault();

        _iterationSlider.onChangedValue([this]()
        {
            stopAnimation();
            const int requested = static_cast<int>(std::lround(_iterationSlider.getValue()));
            if (requested != _displayedIteration)
                showIteration(requested);
        });
        _speedSlider.onChangedValue([this]()
        {
            _animationTimer.setInterval(static_cast<float>(_speedSlider.getValue()));
        });
        _previousButton.onClick([this]() { stopAnimation(); showIteration(_displayedIteration - 1); });
        _nextButton.onClick([this]() { stopAnimation(); showIteration(_displayedIteration + 1); });
        _resetButton.onClick([this]() { stopAnimation(); showIteration(0); });
        _playButton.onClick([this]() { toggleAnimation(); });
        _animationTimer.onTimer([this]()
        {
            if (_displayedIteration >= maximumIteration()) stopAnimation();
            else showIteration(_displayedIteration + 1);
        });
        _settings.onRun([this]() { runFromSettings(); });
        _evaluation.onExport([this]() { exportResults(); });
        _tabs.onChangedSelection([this](int position)
        {
            stopAnimation();
            const bool dashboardSelected = position == 0;
            _iterationSlider.enable(dashboardSelected);
            _speedSlider.enable(dashboardSelected);
            _previousButton.enable(dashboardSelected);
            _playButton.enable(dashboardSelected);
            _nextButton.enable(dashboardSelected);
            _resetButton.enable(dashboardSelected);
        });

        try
        {
            loadAndOptimize();
        }
        catch (const std::exception& error)
        {
            _statusLabel.setTitle((std::string("Error: ") + error.what()).c_str());
            mu::dbgLog("Seasonal fitting failed: %s", error.what());
        }
    }
};
