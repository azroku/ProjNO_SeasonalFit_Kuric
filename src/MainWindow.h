#pragma once

#include "MainView.h"

#include <gui/Window.h>

class MainWindow : public gui::Window
{
private:
    MainView _mainView;
public:
    MainWindow()
        : gui::Window(gui::Geometry(80, 80, 1400, 900))
    {
        setTitle("Seasonal Model Fitting to Meteorological Data");
        // Fix only the minimum content size. The window remains fully
        // resizable and maximizable on Windows, macOS and Linux.
        setCentralView(&_mainView, gui::Frame::FixSizes::FixMin);
        setResizable(true);

    }

    bool shouldClose() override
    {
        return true;
    }

    void onClose() override
    {
        gui::Window::onClose();
    }
};
