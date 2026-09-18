#pragma once

#include "GaussNewton.h"

#include <gui/Button.h>
#include <gui/GridLayout.h>
#include <gui/Label.h>
#include <gui/View.h>

#include <array>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>

class EvaluationView : public gui::View
{
    gui::Label _heading{ "Final model evaluation", gui::Font::ID::SystemLargerBold };
    gui::Label _description{
        "A compact report of convergence, predictive fit, and the estimated model.",
        gui::Font::ID::SystemNormal };
    std::array<gui::Label, 10> _names{
        gui::Label("Station"), gui::Label("Observations"), gui::Label("Accepted iterations"),
        gui::Label("Stopping condition"), gui::Label("Initial RSS"), gui::Label("Final RSS"),
        gui::Label("RSS reduction"), gui::Label("RMSE"), gui::Label("R²"),
        gui::Label("Final parameters")
    };
    std::array<gui::Label, 10> _values;
    gui::Button _exportButton{ "Export results to CSV" };
    gui::Label _exportStatus{ "No export has been created in this session." };
    gui::GridLayout _layout{ 13, 2 };
    std::function<void()> _onExport;

    static std::string fixed(double value, int precision)
    {
        std::ostringstream out;
        out << std::fixed << std::setprecision(precision) << value;
        return out.str();
    }

public:
    EvaluationView()
        : gui::View(30, 20, 30, 20)
    {
        _layout.setMargins(20, 18);
        _layout.setSpaceBetweenCells(10, 24);
        _layout.insert(0, 0, _heading, 2, td::HAlignment::Left);
        _layout.insert(1, 0, _description, 2, td::HAlignment::Left);
        for (std::size_t i = 0; i < _names.size(); ++i)
        {
            _values[i].setResizable(20);
            _layout.insert(static_cast<td::BYTE>(i + 2), 0, _names[i]);
            _layout.insert(static_cast<td::BYTE>(i + 2), 1, _values[i]);
        }
        _layout.insert(12, 0, _exportButton);
        _layout.insert(12, 1, _exportStatus);
        setLayout(&_layout);
        _exportStatus.setResizable(20);
        _exportButton.onClick([this]() { if (_onExport) _onExport(); });
    }

    void onExport(const std::function<void()>& callback) { _onExport = callback; }
    void setExportStatus(const std::string& value) { _exportStatus.setTitle(value.c_str()); }

    void update(const std::string& station, std::size_t observations,
        const OptimizationResult& result, double initialRss, double finalRss,
        double rmse, double rSquared)
    {
        const double reduction = initialRss > 0.0
            ? 100.0 * (initialRss - finalRss) / initialRss : 0.0;
        std::ostringstream parameters;
        parameters << "a₀=" << fixed(result.parameters[0], 5)
            << ", a₁=" << fixed(result.parameters[1], 7)
            << ", A₁=" << fixed(result.parameters[2], 5)
            << ", φ₁=" << fixed(result.parameters[3], 5)
            << ", A₂=" << fixed(result.parameters[4], 5)
            << ", φ₂=" << fixed(result.parameters[5], 5);

        const std::array<std::string, 10> values{
            station,
            std::to_string(observations),
            std::to_string(result.iterations),
            optimizationStatusText(result.status),
            fixed(initialRss, 4), fixed(finalRss, 4),
            fixed(reduction, 2) + "%", fixed(rmse, 4) + " °C",
            fixed(rSquared, 5), parameters.str()
        };
        for (std::size_t i = 0; i < values.size(); ++i)
            _values[i].setTitle(values[i].c_str());
    }
};
