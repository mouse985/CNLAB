#include "PlotCommon.hpp"

namespace cnlab {

void PlotStyle::parseSpec(const std::string& spec) {
    for (size_t i = 0; i < spec.length(); i++) {
        char c = spec[i];
        char nextC = (i + 1 < spec.length()) ? spec[i + 1] : '\0';

        switch (c) {
            // Colors
            case 'r': color = Color::Red(); break;
            case 'g': color = Color::Green(); break;
            case 'b': color = Color::Blue(); break;
            case 'c': color = Color::Cyan(); break;
            case 'm': color = Color::Magenta(); break;
            case 'y': color = Color::Yellow(); break;
            case 'k': color = Color::Black(); break;
            case 'w': color = Color::White(); break;

            // Line styles
            case '-':
                if (nextC == '-') {
                    lineStyle = LineStyle::Dashed;
                    i++; // Skip next '-'
                } else if (nextC == '.') {
                    lineStyle = LineStyle::DashDot;
                    i++; // Skip next '.'
                } else {
                    lineStyle = LineStyle::Solid;
                }
                break;
            case ':': lineStyle = LineStyle::Dotted; break;

            // Markers
            case 'o': marker = MarkerStyle::Circle; break;
            case 's': marker = MarkerStyle::Square; break;
            case 'd': marker = MarkerStyle::Diamond; break;
            case '^': marker = MarkerStyle::Triangle; break;
            case 'x': marker = MarkerStyle::Cross; break;
            case '+': marker = MarkerStyle::Plus; break;
            case '*': marker = MarkerStyle::Star; break;
            case '.': marker = MarkerStyle::Point; break;
        }
    }
}

}
