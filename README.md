# Seasonal Model Fitting

A natID/C++ application that fits a nonlinear seasonal model to Sarajevo monthly temperature data with the damped Gauss-Newton method and Armijo backtracking.

## Features

- observed temperatures and fitted seasonal curve;
- residual chart for the selected iteration;
- log-scale RSS convergence chart;
- live table of all six model parameters;
- iteration slider, previous/next navigation, and Play/Pause animation;
- adjustable animation speed;
- selectable Sarajevo or Bjelasnica datasets;
- editable initial parameters and convergence tolerances;
- a dedicated final-evaluation tab with the stopping condition and RSS reduction;
- CSV export containing metrics, fitted values, and residuals;
- current RSS, RMSE, and R-squared quality indicators;
- final-fit reference curve for visually tracking optimization progress;
- the complete fitted model formula and an explained residual-sign legend;
- analytical Jacobian and robust stopping criteria.

## Requirements

- CMake 3.18 or newer;
- a C++20 compiler;
- the natID SDK and its platform binaries.

## Build

1. Install natID according to the official instructions: https://github.com/idzafic/natID
2. Place the `natID.SDK` directory directly in your home directory. On Windows, the expected path is `%USERPROFILE%/natID.SDK`.
3. Extract this project, open its root directory in Visual Studio 2022 as a CMake project, and select an x64 configuration.
4. Build and run the `SeasonalFit` target.

Command-line CMake may also be used:

```text
cmake -S . -B build
cmake --build build --config Release
```

### macOS

The project includes `src/Info.plist`, which describes the native `.app`
bundle to Finder and supplies the executable name, bundle identifier, version,
and minimum supported macOS release. Xcode handles bundle expansion and signing
itself. Makefiles and Ninja builds run a post-build step that finalizes the
plist and applies an ad-hoc signature, which is sufficient for launching a
locally built application on Intel and Apple Silicon Macs.

Example terminal build:

```text
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
open build/SeasonalFit.app
```

The main window starts at 1400 x 900 and remains freely resizable. A
1280 x 760 minimum content size prevents the four dashboard panels, legends,
and parameter table from overlapping when the window is made smaller.

The dataset path is embedded by CMake, so the application can be launched from the IDE or build directory without copying CSV files.

## Project structure

- `src/` - optimization, data loading, visualization, and natID GUI code;
- `data/sarajevo/sarajevo_temperature.csv` - cleaned monthly observations;
- `Docs/` - IEEE project report and its complete LaTeX source package;
- `res/` - natID resources;
- `CMakeLists.txt` and `SeasonalFit.cmake` - build configuration.

## Model

`f(t; θ) = a₀ + a₁t + A₁ sin(2πt/12 + φ₁) + A₂ sin(4πt/12 + φ₂)`

The displayed objective is the residual sum of squares (RSS).

## Export

The `Final Evaluation` tab creates a `SeasonalFit_exports` folder in the application's working directory. Every export receives a unique station-and-time filename, for example `Sarajevo_20260918_172530_125.csv`, so previous experiments are never overwritten. Each file contains the convergence settings, initial and final parameters, summary metrics, observations, fitted values, and residuals.

## Data quality

Both cleaned station files contain finite temperatures and unique, strictly increasing month indices. Missing calendar months are intentionally retained as gaps: the optimizer evaluates the model at the recorded month index and does not require contiguous observations.
