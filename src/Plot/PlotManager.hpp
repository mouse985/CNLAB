#pragma once

#include "PlotCommon.hpp"
#include "PlotFigure.hpp"
#include <memory>
#include <map>

namespace cnlab {

class PlotManager {
public:
    PlotManager();
    ~PlotManager();

    std::shared_ptr<PlotFigure> figure(int figNum = -1);
    std::shared_ptr<PlotFigure> gcf();
    void close(int figNum = -1);
    void closeAll();

    void plot(const std::vector<double>& x, const std::vector<double>& y, const PlotStyle& style = PlotStyle());
    void scatter(const std::vector<double>& x, const std::vector<double>& y, const PlotStyle& style = PlotStyle());
    void bar(const std::vector<double>& x, const std::vector<double>& y, const PlotStyle& style = PlotStyle());
    void hist(const std::vector<double>& data, int bins = 10, const PlotStyle& style = PlotStyle());

    void title(const std::string& t);
    void xlabel(const std::string& l);
    void ylabel(const std::string& l);
    void legend(const std::vector<std::string>& labels);
    void grid(bool on);
    void hold(bool on);
    void xlim(double min, double max);
    void ylim(double min, double max);
    void axis(const std::vector<double>& limits);
    void clf();
    void show();

    // Subplot support
    void subplot(int rows, int cols, int index);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}
