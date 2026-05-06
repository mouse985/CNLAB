#ifndef MINIPLOT_HPP
#define MINIPLOT_HPP

#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace cnlab {

enum class LineStyle { Solid, Dashed, Dotted, DashDot, None };
enum class MarkerStyle { None, Circle, Square, Diamond, Triangle, Cross, Plus, Star, Point };

struct Color {
    unsigned char r, g, b;
    Color(unsigned char r_=0, unsigned char g_=0, unsigned char b_=0) : r(r_), g(g_), b(b_) {}
    static Color Red()    { return Color(255, 0, 0); }
    static Color Green()  { return Color(0, 255, 0); }
    static Color Blue()   { return Color(0, 0, 255); }
    static Color Black()  { return Color(0, 0, 0); }
    static Color White()  { return Color(255, 255, 255); }
    static Color Gray()   { return Color(128, 128, 128); }
    static Color Yellow() { return Color(255, 255, 0); }
    static Color Cyan()   { return Color(0, 255, 255); }
    static Color Magenta(){ return Color(255, 0, 255); }
    static Color Orange() { return Color(255, 165, 0); }
    static Color Purple() { return Color(128, 0, 128); }
};

struct PlotStyle {
    Color color = Color::Blue();
    LineStyle lineStyle = LineStyle::Solid;
    MarkerStyle marker = MarkerStyle::None;
    int lineWidth = 2;
    int markerSize = 6;
    std::string label;
    
    void parseSpec(const std::string& spec);
};

struct AxisLimits {
    double xmin = 0, xmax = 1;
    double ymin = 0, ymax = 1;
    bool autoX = true, autoY = true;
};

class PlotFigure {
public:
    PlotFigure(const std::string& title = "Figure", int width = 800, int height = 600);
    ~PlotFigure();
    
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
    void showNonBlocking();
    void close();
    bool isOpen() const;
    
    void save(const std::string& filename);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

class MiniPlot {
public:
    MiniPlot();
    ~MiniPlot();
    
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
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}

#endif
