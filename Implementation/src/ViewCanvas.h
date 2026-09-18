#pragma once

#include "PlotData.h"

#include <gui/Canvas.h>
#include <gui/DrawableString.h>
#include <gui/Shape.h>

#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

class ViewCanvas : public gui::Canvas
{
    SeasonalFitPlotData _data;
    bool _ready{ false };

    static float w(const gui::Rect& r) { return static_cast<float>(r.width()); }
    static float h(const gui::Rect& r) { return static_cast<float>(r.height()); }

    static float px(double value, const PlotRange& range, const gui::Rect& r)
    {
        return static_cast<float>(r.left) +
            static_cast<float>((value - range.minimum) /
                std::max(range.maximum - range.minimum, 1e-12)) * w(r);
    }

    static float py(double value, const PlotRange& range, const gui::Rect& r)
    {
        return static_cast<float>(r.bottom) -
            static_cast<float>((value - range.minimum) /
                std::max(range.maximum - range.minimum, 1e-12)) * h(r);
    }

    static std::string format(double value, int precision = 2)
    {
        std::ostringstream out;
        out << std::fixed << std::setprecision(precision) << value;
        return out.str();
    }

    static void text(const std::string& value, const gui::Rect& r,
        gui::Font::ID font = gui::Font::ID::SystemSmaller,
        td::ColorID color = td::ColorID::SysText,
        td::TextAlignment alignment = td::TextAlignment::Left)
    {
        gui::DrawableString label(value.c_str());
        label.draw(r, font, color, alignment, td::VAlignment::Center);
    }

    static void line(float x1, float y1, float x2, float y2,
        td::ColorID color, float thickness = 1.0f)
    {
        const gui::Point points[] = { {x1, y1}, {x2, y2} };
        gui::Shape shape;
        shape.createLines(points, 2, thickness);
        shape.drawWire(color);
    }

    static gui::Rect plotRect(const gui::Rect& panel)
    {
        return { panel.left + 58, panel.top + 58, panel.right - 17, panel.bottom - 40 };
    }

    static void panel(const gui::Rect& r, const char* title)
    {
        gui::Shape border;
        border.createRect(r, 1.0f);
        border.drawWire(td::ColorID::LightGray);
        text(title, { r.left + 12, r.top + 5, r.right - 12, r.top + 31 },
            gui::Font::ID::SystemBold);
    }

    static void axes(const gui::Rect& area, const PlotRange& xr, const PlotRange& yr,
        const char* xLabel, const char* yLabel, bool integerXAxis = false)
    {
        constexpr int yTicks = 4;
        for (int i = 0; i <= yTicks; ++i)
        {
            const float f = static_cast<float>(i) / yTicks;
            const float y = static_cast<float>(area.bottom) - f * h(area);
            line(static_cast<float>(area.left), y, static_cast<float>(area.right), y, td::ColorID::LightGray);
            const double yv = yr.minimum + f * (yr.maximum - yr.minimum);
            text(format(yv, 1), { area.left - 54, y - 9, area.left - 5, y + 9 },
                gui::Font::ID::SystemSmallest, td::ColorID::Gray, td::TextAlignment::Right);
        }

        if (integerXAxis)
        {
            const int first = static_cast<int>(std::ceil(xr.minimum));
            const int last = static_cast<int>(std::floor(xr.maximum));
            const int step = std::max(1, static_cast<int>(std::ceil((last - first) / 4.0)));
            for (int value = first; value <= last; value += step)
            {
                const float x = px(static_cast<double>(value), xr, area);
                line(x, static_cast<float>(area.top), x, static_cast<float>(area.bottom), td::ColorID::LightGray);
                text(std::to_string(value), { x - 28, area.bottom + 3, x + 28, area.bottom + 21 },
                    gui::Font::ID::SystemSmallest, td::ColorID::Gray, td::TextAlignment::Center);
            }
            if ((last - first) % step != 0)
            {
                const float x = px(static_cast<double>(last), xr, area);
                line(x, static_cast<float>(area.top), x, static_cast<float>(area.bottom), td::ColorID::LightGray);
                text(std::to_string(last), { x - 28, area.bottom + 3, x + 28, area.bottom + 21 },
                    gui::Font::ID::SystemSmallest, td::ColorID::Gray, td::TextAlignment::Center);
            }
        }
        else
        {
            constexpr int xTicks = 4;
            for (int i = 0; i <= xTicks; ++i)
            {
                const float f = static_cast<float>(i) / xTicks;
                const float x = static_cast<float>(area.left) + f * w(area);
                const double xv = xr.minimum + f * (xr.maximum - xr.minimum);
                line(x, static_cast<float>(area.top), x, static_cast<float>(area.bottom), td::ColorID::LightGray);
                text(format(xv, 0), { x - 28, area.bottom + 3, x + 28, area.bottom + 21 },
                    gui::Font::ID::SystemSmallest, td::ColorID::Gray, td::TextAlignment::Center);
            }
        }
        line(static_cast<float>(area.left), static_cast<float>(area.top),
            static_cast<float>(area.left), static_cast<float>(area.bottom), td::ColorID::Gray);
        line(static_cast<float>(area.left), static_cast<float>(area.bottom),
            static_cast<float>(area.right), static_cast<float>(area.bottom), td::ColorID::Gray);
        text(xLabel, { area.left, area.bottom + 20, area.right, area.bottom + 39 },
            gui::Font::ID::SystemSmallestBold, td::ColorID::Gray, td::TextAlignment::Center);
        text(yLabel, { area.left - 54, area.top - 22, area.left + 110, area.top - 3 },
            gui::Font::ID::SystemSmallestBold, td::ColorID::Gray);
    }

    static void polyline(const std::vector<PlotPoint>& points, const PlotRange& xr,
        const PlotRange& yr, const gui::Rect& area, td::ColorID color, float thickness)
    {
        if (points.size() < 2) return;
        std::vector<gui::Point> mapped;
        mapped.reserve(points.size());
        for (const auto& p : points) mapped.emplace_back(px(p.x, xr, area), py(p.y, yr, area));
        gui::Shape shape;
        shape.createPolyLine(mapped.data(), mapped.size(), thickness);
        shape.drawWire(color);
    }

    void temperature(const gui::Rect& r) const
    {
        panel(r, "Observed data and seasonal fit");
        const auto area = plotRect(r);
        axes(area, _data.timeRange, _data.temperatureRange, "Month index", "Temperature (C)");
        polyline(_data.finalFitted, _data.timeRange, _data.temperatureRange, area, td::ColorID::Gray, 2.0f);
        polyline(_data.fitted, _data.timeRange, _data.temperatureRange, area, td::ColorID::Red, 2.2f);

        const float legendY = static_cast<float>(r.top) + 41.0f;
        line(static_cast<float>(r.right) - 224, legendY, static_cast<float>(r.right) - 204,
            legendY, td::ColorID::Red, 2.2f);
        text("Current", { r.right - 199, legendY - 9, r.right - 145, legendY + 9 },
            gui::Font::ID::SystemSmallest);
        line(static_cast<float>(r.right) - 136, legendY, static_cast<float>(r.right) - 116,
            legendY, td::ColorID::Gray, 2.0f);
        text("Final", { r.right - 111, legendY - 9, r.right - 64, legendY + 9 },
            gui::Font::ID::SystemSmallest);

        std::vector<gui::Circle> circles;
        circles.reserve(_data.observed.size());
        for (const auto& p : _data.observed)
            circles.emplace_back(px(p.x, _data.timeRange, area), py(p.y, _data.temperatureRange, area), 2.1f);
        if (!circles.empty())
        {
            gui::Shape points;
            points.createCircles(circles.data(), circles.size(), 1.0f);
            points.drawFill(td::ColorID::Blue);
        }
    }

    void residuals(const gui::Rect& r) const
    {
        panel(r, "Residuals at the selected iteration");
        const auto area = plotRect(r);
        axes(area, _data.timeRange, _data.residualRange, "Month index", "Residual rᵢ (C)");
        text("rᵢ = observed − fitted", { area.left + 125, r.top + 33, area.left + 275, r.top + 52 },
            gui::Font::ID::SystemSmallestBold, td::ColorID::Gray);
        line(static_cast<float>(area.left) + 285, static_cast<float>(r.top) + 43,
            static_cast<float>(area.left) + 303, static_cast<float>(r.top) + 43,
            td::ColorID::Red, 2.0f);
        text("Warmer", { area.left + 308, r.top + 33, area.left + 363, r.top + 52 },
            gui::Font::ID::SystemSmallest);
        line(static_cast<float>(area.left) + 368, static_cast<float>(r.top) + 43,
            static_cast<float>(area.left) + 386, static_cast<float>(r.top) + 43,
            td::ColorID::Blue, 2.0f);
        text("Cooler", { area.left + 391, r.top + 33, area.right, r.top + 52 },
            gui::Font::ID::SystemSmallest);
        const float zero = py(0.0, _data.residualRange, area);
        line(static_cast<float>(area.left), zero, static_cast<float>(area.right), zero, td::ColorID::Gray, 1.5f);
        std::vector<gui::Point> positive;
        std::vector<gui::Point> negative;
        positive.reserve(_data.residuals.size() * 2);
        negative.reserve(_data.residuals.size() * 2);
        for (const auto& p : _data.residuals)
        {
            const float x = px(p.x, _data.timeRange, area);
            auto& points = p.y >= 0.0 ? positive : negative;
            points.emplace_back(x, zero);
            points.emplace_back(x, py(p.y, _data.residualRange, area));
        }
        if (!positive.empty())
        {
            gui::Shape bars;
            bars.createLines(positive.data(), positive.size(), 1.2f);
            bars.drawWire(td::ColorID::Red);
        }
        if (!negative.empty())
        {
            gui::Shape bars;
            bars.createLines(negative.data(), negative.size(), 1.2f);
            bars.drawWire(td::ColorID::Blue);
        }
    }

    void convergence(const gui::Rect& r) const
    {
        panel(r, "Gauss-Newton convergence");
        const auto area = plotRect(r);
        const PlotRange xr{ 0.0, _data.convergence.empty() ? 1.0 :
            _data.convergence.back().x };
        axes(area, xr, _data.convergenceRange, "Iteration", "log10(RSS)", true);
        polyline(_data.convergence, xr, _data.convergenceRange, area, td::ColorID::Blue, 2.2f);
    }

    void parameters(const gui::Rect& r) const
    {
        panel(r, "Current parameter estimates");
        const std::array<const char*, ParameterCount> names{
            "a₀   Intercept", "a₁   Linear trend", "A₁   Annual amplitude",
            "φ₁   Annual phase", "A₂   Semi-annual amplitude", "φ₂   Semi-annual phase"
        };
        const float top = static_cast<float>(r.top) + 39.0f;
        const float rh = std::max(24.0f, (h(r) - 60.0f) / 7.0f);
        text("Parameter", { r.left + 22, top, r.left + 250, top + rh }, gui::Font::ID::SystemBold);
        text("Value", { r.left + 255, top, r.right - 22, top + rh },
            gui::Font::ID::SystemBold, td::ColorID::SysText, td::TextAlignment::Right);
        for (std::size_t i = 0; i < ParameterCount; ++i)
        {
            const float y = top + rh * static_cast<float>(i + 1);
            line(static_cast<float>(r.left) + 20, y, static_cast<float>(r.right) - 20, y, td::ColorID::LightGray);
            text(names[i], { r.left + 22, y, r.left + 270, y + rh }, gui::Font::ID::SystemNormal);
            text(format(_data.displayedParameters[i], 6), { r.left + 275, y, r.right - 22, y + rh },
                gui::Font::ID::SystemBold, td::ColorID::Blue, td::TextAlignment::Right);
        }
    }

public:
    void setPlotData(const SeasonalFitPlotData& data)
    {
        _data = data;
        _ready = true;
        reDraw();
    }

protected:
    void onDraw(const gui::Rect& r) override
    {
        gui::Shape::drawRect(r, td::ColorID::White);
        if (!_ready) return;

        const float gap = 12.0f;
        const float middleX = static_cast<float>(r.left) + w(r) * 0.58f;
        const float middleY = static_cast<float>(r.top) + h(r) * 0.55f;
        temperature({ r.left + gap, r.top + gap, middleX - gap / 2, middleY - gap / 2 });
        parameters({ middleX + gap / 2, r.top + gap, r.right - gap, middleY - gap / 2 });
        residuals({ r.left + gap, middleY + gap / 2, middleX - gap / 2, r.bottom - gap });
        convergence({ middleX + gap / 2, middleY + gap / 2, r.right - gap, r.bottom - gap });
    }
};
