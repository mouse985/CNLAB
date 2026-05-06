#pragma once

#include "PlotCommon.hpp"
#include <memory>
#include <string>
#include <vector>

namespace cnlab {

// Subplot data for a single subplot
struct SubplotData {
    std::string title;
    std::string xLabel, yLabel;
    std::vector<std::string> legendLabels;
    bool gridOn = true;
    bool holdOn = false;
    AxisLimits limits;
    std::vector<DataSeries> series;
    
    void clear() {
        series.clear();
        legendLabels.clear();
        limits = AxisLimits();
    }
};

class PlotFigure {
public:
    PlotFigure(const std::string& title = "Figure", int width = 800, int height = 600);
    ~PlotFigure();

    void plot(const std::vector<double>& x, const std::vector<double>& y, const PlotStyle& style = PlotStyle());
    void scatter(const std::vector<double>& x, const std::vector<double>& y, const PlotStyle& style = PlotStyle());
    void bar(const std::vector<double>& x, const std::vector<double>& y, const PlotStyle& style = PlotStyle());
    void hist(const std::vector<double>& data, int bins = 10, const PlotStyle& style = PlotStyle());

    void setTitle(const std::string& t);
    void setXLabel(const std::string& l);
    void setYLabel(const std::string& l);
    void setLegend(const std::vector<std::string>& labels);
    void setGrid(bool on);
    void setHold(bool on);
    void setXLim(double min, double max);
    void setYLim(double min, double max);
    void setAxis(const std::vector<double>& limits);
    void clear();

    void show();
    void close();
    bool isOpen() const;

    void redraw();

    // Subplot support
    void setSubplotGrid(int rows, int cols, int index);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}
