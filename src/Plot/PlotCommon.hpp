#pragma once

#include <vector>
#include <string>

namespace cnlab {

enum class LineStyle { Solid, Dashed, Dotted, DashDot, None };
enum class MarkerStyle { None, Circle, Square, Diamond, Triangle, Cross, Plus, Star, Point };

struct Color {
    unsigned char r, g, b;
    Color(unsigned char r_ = 0, unsigned char g_ = 0, unsigned char b_ = 0) : r(r_), g(g_), b(b_) {}
    static Color Red()     { return Color(255, 0, 0); }
    static Color Green()   { return Color(0, 255, 0); }
    static Color Blue()    { return Color(0, 0, 255); }
    static Color Black()   { return Color(0, 0, 0); }
    static Color White()   { return Color(255, 255, 255); }
    static Color Gray()    { return Color(128, 128, 128); }
    static Color Yellow()  { return Color(255, 255, 0); }
    static Color Cyan()    { return Color(0, 255, 255); }
    static Color Magenta() { return Color(255, 0, 255); }
    static Color Orange()  { return Color(255, 165, 0); }
    static Color Purple()  { return Color(128, 0, 128); }
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

struct DataSeries {
    std::vector<double> x, y;
    PlotStyle style;
    std::string type;
};

struct AxisLimits {
    double xmin = 0, xmax = 1;
    double ymin = 0, ymax = 1;
    bool autoX = true, autoY = true;
};

}
