#include "PlotManager.hpp"

namespace cnlab {

class PlotManager::Impl {
public:
    std::map<int, std::shared_ptr<PlotFigure>> figures;
    int currentFig = 1;
    int nextFigNum = 1;
};

PlotManager::PlotManager() : pImpl(std::make_unique<Impl>()) {}
PlotManager::~PlotManager() = default;

std::shared_ptr<PlotFigure> PlotManager::figure(int figNum) {
    if (figNum < 0) {
        // Use current figure or create new one
        if (pImpl->figures.empty()) {
            figNum = pImpl->nextFigNum++;
        } else {
            figNum = pImpl->currentFig;
        }
    }

    auto it = pImpl->figures.find(figNum);
    if (it == pImpl->figures.end()) {
        // Create figure with default title, actual title set later via setTitle()
        auto fig = std::make_shared<PlotFigure>("Fig " + std::to_string(figNum));
        pImpl->figures[figNum] = fig;
    }
    pImpl->currentFig = figNum;
    return pImpl->figures[figNum];
}

std::shared_ptr<PlotFigure> PlotManager::gcf() {
    return figure(pImpl->currentFig);
}

void PlotManager::close(int figNum) {
    if (figNum < 0) {
        for (auto& [num, fig] : pImpl->figures) {
            fig->close();
        }
        pImpl->figures.clear();
    } else {
        auto it = pImpl->figures.find(figNum);
        if (it != pImpl->figures.end()) {
            it->second->close();
            pImpl->figures.erase(it);
        }
    }
}

void PlotManager::closeAll() {
    close(-1);
}

void PlotManager::plot(const std::vector<double>& x, const std::vector<double>& y, const PlotStyle& style) {
    figure()->plot(x, y, style);
}

void PlotManager::scatter(const std::vector<double>& x, const std::vector<double>& y, const PlotStyle& style) {
    figure()->scatter(x, y, style);
}

void PlotManager::bar(const std::vector<double>& x, const std::vector<double>& y, const PlotStyle& style) {
    figure()->bar(x, y, style);
}

void PlotManager::hist(const std::vector<double>& data, int bins, const PlotStyle& style) {
    figure()->hist(data, bins, style);
}

void PlotManager::title(const std::string& t) {
    figure()->setTitle(t);
}

void PlotManager::xlabel(const std::string& l) {
    figure()->setXLabel(l);
}

void PlotManager::ylabel(const std::string& l) {
    figure()->setYLabel(l);
}

void PlotManager::legend(const std::vector<std::string>& labels) {
    figure()->setLegend(labels);
}

void PlotManager::grid(bool on) {
    figure()->setGrid(on);
}

void PlotManager::hold(bool on) {
    figure()->setHold(on);
}

void PlotManager::xlim(double min, double max) {
    figure()->setXLim(min, max);
}

void PlotManager::ylim(double min, double max) {
    figure()->setYLim(min, max);
}

void PlotManager::axis(const std::vector<double>& limits) {
    figure()->setAxis(limits);
}

void PlotManager::clf() {
    figure()->clear();
}

void PlotManager::show() {
    figure()->show();
}

void PlotManager::subplot(int rows, int cols, int index) {
    // Use a special figure number for subplot mode
    // All subplots share the same figure window
    int subplotFigNum = 999999; // Special figure number for subplot mode
    
    // Get or create the subplot figure
    auto it = pImpl->figures.find(subplotFigNum);
    if (it == pImpl->figures.end()) {
        auto fig = std::make_shared<PlotFigure>("Figure with Subplots", 900, 700);
        pImpl->figures[subplotFigNum] = fig;
    }
    pImpl->currentFig = subplotFigNum;
    
    // Set the subplot grid on the current figure
    figure()->setSubplotGrid(rows, cols, index);
}

}
