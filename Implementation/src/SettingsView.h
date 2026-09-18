#pragma once

#include "SeasonalModel.h"

#include <gui/Button.h>
#include <gui/ComboBox.h>
#include <gui/GridLayout.h>
#include <gui/Label.h>
#include <gui/NumericEdit.h>
#include <gui/View.h>

#include <functional>

class SettingsView : public gui::View
{
    gui::Label _heading{ "Optimization configuration", gui::Font::ID::SystemLargerBold };
    gui::Label _description{
        "Choose a station and initial point, then run the damped Gauss-Newton method.",
        gui::Font::ID::SystemNormal };
    gui::Label _stationLabel{ "Meteorological station" };
    gui::ComboBox _station;

    gui::Label _a0Label{ "a₀  Intercept" };
    gui::Label _a1Label{ "a₁  Linear trend" };
    gui::Label _A1Label{ "A₁  Annual amplitude" };
    gui::Label _phi1Label{ "φ₁  Annual phase" };
    gui::Label _A2Label{ "A₂  Semi-annual amplitude" };
    gui::Label _phi2Label{ "φ₂  Semi-annual phase" };
    gui::NumericEdit _a0{ td::real8, gui::LineEdit::Messages::DoNotSend, false, "Initial intercept", 4 };
    gui::NumericEdit _a1{ td::real8, gui::LineEdit::Messages::DoNotSend, false, "Initial linear trend", 6 };
    gui::NumericEdit _A1{ td::real8, gui::LineEdit::Messages::DoNotSend, false, "Initial annual amplitude", 4 };
    gui::NumericEdit _phi1{ td::real8, gui::LineEdit::Messages::DoNotSend, false, "Initial annual phase", 4 };
    gui::NumericEdit _A2{ td::real8, gui::LineEdit::Messages::DoNotSend, false, "Initial semi-annual amplitude", 4 };
    gui::NumericEdit _phi2{ td::real8, gui::LineEdit::Messages::DoNotSend, false, "Initial semi-annual phase", 4 };

    gui::Label _maxIterationsLabel{ "Maximum iterations" };
    gui::Label _gradientToleranceLabel{ "Gradient tolerance" };
    gui::Label _stepToleranceLabel{ "Step tolerance" };
    gui::Label _objectiveToleranceLabel{ "Relative objective tolerance" };
    gui::NumericEdit _maxIterations{ td::int4, gui::LineEdit::Messages::DoNotSend, false, "Maximum iterations", 0 };
    gui::NumericEdit _gradientTolerance{ td::real8, gui::LineEdit::Messages::DoNotSend, false, "Gradient tolerance", 10 };
    gui::NumericEdit _stepTolerance{ td::real8, gui::LineEdit::Messages::DoNotSend, false, "Step tolerance", 10 };
    gui::NumericEdit _objectiveTolerance{ td::real8, gui::LineEdit::Messages::DoNotSend, false, "Relative objective tolerance", 12 };

    gui::Button _runButton{ "Run optimization" };
    gui::Button _defaultsButton{ "Restore defaults" };
    gui::Label _hint{
        "Tip: different initial values demonstrate the sensitivity of nonlinear least squares.",
        gui::Font::ID::SystemSmaller };
    gui::GridLayout _layout{ 9, 4 };
    std::function<void()> _onRun;

    static void set(gui::NumericEdit& edit, double value)
    {
        edit.setValue(td::Variant(value), false);
    }

public:
    SettingsView()
        : gui::View(24, 18, 24, 18)
    {
        _station.addItem("Sarajevo");
        _station.addItem("Bjelasnica");
        _station.selectIndex(0, false);

        _layout.setMargins(18, 18);
        _layout.setSpaceBetweenCells(12, 18);
        _layout.insert(0, 0, _heading, 4, td::HAlignment::Left);
        _layout.insert(1, 0, _description, 4, td::HAlignment::Left);
        _layout.insert(2, 0, _stationLabel);
        _layout.insert(2, 1, _station);

        _layout.insert(3, 0, _a0Label); _layout.insert(3, 1, _a0);
        _layout.insert(3, 2, _a1Label); _layout.insert(3, 3, _a1);
        _layout.insert(4, 0, _A1Label); _layout.insert(4, 1, _A1);
        _layout.insert(4, 2, _phi1Label); _layout.insert(4, 3, _phi1);
        _layout.insert(5, 0, _A2Label); _layout.insert(5, 1, _A2);
        _layout.insert(5, 2, _phi2Label); _layout.insert(5, 3, _phi2);

        _layout.insert(6, 0, _maxIterationsLabel); _layout.insert(6, 1, _maxIterations);
        _layout.insert(6, 2, _gradientToleranceLabel); _layout.insert(6, 3, _gradientTolerance);
        _layout.insert(7, 0, _stepToleranceLabel); _layout.insert(7, 1, _stepTolerance);
        _layout.insert(7, 2, _objectiveToleranceLabel); _layout.insert(7, 3, _objectiveTolerance);
        _layout.insert(8, 0, _runButton);
        _layout.insert(8, 1, _defaultsButton);
        _layout.insert(8, 2, _hint, 2, td::HAlignment::Left);
        setLayout(&_layout);

        _runButton.setAsDefault();
        _maxIterations.setMinValue(1.0);
        _maxIterations.setMaxValue(1000.0);
        _gradientTolerance.setMinValue(1e-14);
        _gradientTolerance.setMaxValue(1.0);
        _stepTolerance.setMinValue(1e-14);
        _stepTolerance.setMaxValue(1.0);
        _objectiveTolerance.setMinValue(1e-14);
        _objectiveTolerance.setMaxValue(1.0);
        _runButton.onClick([this]() { if (_onRun) _onRun(); });
        _defaultsButton.onClick([this]() { restoreDefaults(); });
        restoreDefaults();
    }

    void restoreDefaults()
    {
        set(_a0, 10.0); set(_a1, 0.0); set(_A1, 5.0);
        set(_phi1, -3.0); set(_A2, 2.0); set(_phi2, 2.0);
        _maxIterations.setValue(td::Variant(static_cast<td::INT4>(100)), false);
        set(_gradientTolerance, 1e-8);
        set(_stepTolerance, 1e-8);
        set(_objectiveTolerance, 1e-12);
    }

    void onRun(const std::function<void()>& callback) { _onRun = callback; }
    int stationIndex() const { return _station.getSelectedIndex(); }

    Parameters initialParameters() const
    {
        Parameters values{};
        _a0.getValue(values[0]); _a1.getValue(values[1]);
        _A1.getValue(values[2]); _phi1.getValue(values[3]);
        _A2.getValue(values[4]); _phi2.getValue(values[5]);
        return values;
    }

    int maxIterations() const
    {
        td::INT4 value = 100;
        _maxIterations.getValue(value);
        return static_cast<int>(value);
    }

    double gradientTolerance() const { double v{}; _gradientTolerance.getValue(v); return v; }
    double stepTolerance() const { double v{}; _stepTolerance.getValue(v); return v; }
    double objectiveTolerance() const { double v{}; _objectiveTolerance.getValue(v); return v; }
};
