#include "PlotFigure.hpp"
#include <algorithm>
#include <cmath>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wingdi.h>

namespace cnlab {

// UTF-8 to UTF-16 conversion helper
static std::wstring utf8ToUtf16(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring utf16(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &utf16[0], size);
    return utf16;
}

// SubplotData is defined in PlotFigure.hpp

class PlotFigure::Impl {
public:
    HWND hwnd = nullptr;
    int width = 900, height = 700;
    std::string windowTitle;
    
    // Subplot support
    int subplotRows = 1;
    int subplotCols = 1;
    int currentSubplot = 1;
    std::vector<SubplotData> subplots;
    bool useSubplots = false;

    // Margins for each subplot
    int marginLeft = 60, marginRight = 40;
    int marginTop = 50, marginBottom = 60;

    Impl(const std::string& t, int w, int h) : windowTitle(t), width(w), height(h) {}

    ~Impl() {
        if (hwnd) DestroyWindow(hwnd);
    }

    SubplotData& current() { 
        if (subplots.empty()) {
            subplots.resize(subplotRows * subplotCols);
        }
        return subplots[currentSubplot - 1]; 
    }

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (uMsg == WM_DESTROY || uMsg == WM_CLOSE) {
            PostQuitMessage(0);
            return 0;
        }
        if (uMsg == WM_PAINT) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            PlotFigure::Impl* self = reinterpret_cast<PlotFigure::Impl*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (self) self->draw(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    void createWindow() {
        // Set process to use UTF-8 code page
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        static bool classRegistered = false;
        static const wchar_t* CLASS_NAME = L"MiniPlotWindow";

        if (!classRegistered) {
            WNDCLASSW wc = {};
            wc.lpfnWndProc = WindowProc;
            wc.hInstance = GetModuleHandle(nullptr);
            wc.lpszClassName = CLASS_NAME;
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            RegisterClassW(&wc);
            classRegistered = true;
        }

        // Window title uses ASCII only (no UTF-8 to avoid encoding issues)
        // Chart title (set via setTitle) supports UTF-8 and displays inside the chart
        std::string asciiTitle = windowTitle.empty() ? "Figure" : windowTitle;
        std::wstring wtitle(asciiTitle.begin(), asciiTitle.end());

        // Calculate window size to ensure client area is at least width x height
        DWORD style = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
        RECT rect = {0, 0, width, height};
        AdjustWindowRect(&rect, style, FALSE);
        int winWidth = rect.right - rect.left;
        int winHeight = rect.bottom - rect.top;

        hwnd = CreateWindowExW(
            0, CLASS_NAME, wtitle.c_str(),
            style,
            CW_USEDEFAULT, CW_USEDEFAULT, winWidth, winHeight,
            nullptr, nullptr, GetModuleHandle(nullptr), nullptr
        );

        if (hwnd) {
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
            // Set minimum window size to ensure title is visible
            SetWindowPos(hwnd, nullptr, 0, 0, winWidth, winHeight,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
            ShowWindow(hwnd, SW_SHOW);
            UpdateWindow(hwnd);
        }
    }

    void updateLimits(SubplotData& subplot, const std::vector<double>& x, const std::vector<double>& y) {
        if (x.empty() || y.empty()) return;
        double xmin = *std::min_element(x.begin(), x.end());
        double xmax = *std::max_element(x.begin(), x.end());
        double ymin = *std::min_element(y.begin(), y.end());
        double ymax = *std::max_element(y.begin(), y.end());
        double xpad = (xmax - xmin) * 0.05;
        double ypad = (ymax - ymin) * 0.05;
        if (xpad == 0) xpad = 1;
        if (ypad == 0) ypad = 1;
        if (subplot.limits.autoX) {
            subplot.limits.xmin = xmin - xpad;
            subplot.limits.xmax = xmax + xpad;
        }
        if (subplot.limits.autoY) {
            subplot.limits.ymin = ymin - ypad;
            subplot.limits.ymax = ymax + ypad;
        }
    }

    int mapX(double x, double xmin, double xmax, int plotLeft, int plotRight) {
        double range = xmax - xmin;
        if (range == 0) range = 1;
        return plotLeft + static_cast<int>((x - xmin) / range * (plotRight - plotLeft));
    }

    int mapY(double y, double ymin, double ymax, int plotTop, int plotBottom) {
        double range = ymax - ymin;
        if (range == 0) range = 1;
        return plotBottom - static_cast<int>((y - ymin) / range * (plotBottom - plotTop));
    }

    // Cohen-Sutherland line clipping algorithm
    // Returns true if line is visible (at least partially inside rect)
    bool clipLine(int& x1, int& y1, int& x2, int& y2, int xmin, int ymin, int xmax, int ymax) {
        // Region codes
        const int INSIDE = 0;
        const int LEFT = 1;
        const int RIGHT = 2;
        const int BOTTOM = 4;
        const int TOP = 8;

        auto computeCode = [&](int x, int y) -> int {
            int code = INSIDE;
            if (x < xmin) code |= LEFT;
            else if (x > xmax) code |= RIGHT;
            if (y < ymin) code |= TOP;  // Y increases downward in screen coords
            else if (y > ymax) code |= BOTTOM;
            return code;
        };

        int code1 = computeCode(x1, y1);
        int code2 = computeCode(x2, y2);

        while (true) {
            if ((code1 | code2) == 0) {
                // Both endpoints inside
                return true;
            } else if ((code1 & code2) != 0) {
                // Both endpoints outside on same side
                return false;
            } else {
                // Clip the line
                int code = code1 != 0 ? code1 : code2;
                int x = 0, y = 0;

                if (code & TOP) {
                    x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1);
                    y = ymin;
                } else if (code & BOTTOM) {
                    x = x1 + (x2 - x1) * (ymax - y1) / (y2 - y1);
                    y = ymax;
                } else if (code & RIGHT) {
                    y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1);
                    x = xmax;
                } else if (code & LEFT) {
                    y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1);
                    x = xmin;
                }

                if (code == code1) {
                    x1 = x;
                    y1 = y;
                    code1 = computeCode(x1, y1);
                } else {
                    x2 = x;
                    y2 = y;
                    code2 = computeCode(x2, y2);
                }
            }
        }
    }

    void draw(HDC hdc) {
        RECT rect = {0, 0, width, height};
        FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

        if (useSubplots && !subplots.empty()) {
            // Draw multiple subplots
            int subplotWidth = width / subplotCols;
            int subplotHeight = height / subplotRows;
            
            for (size_t idx = 0; idx < subplots.size(); idx++) {
                int row = static_cast<int>(idx) / subplotCols;
                int col = static_cast<int>(idx) % subplotCols;
                
                // Calculate subplot position
                int subLeft = col * subplotWidth;
                int subTop = row * subplotHeight;
                int subRight = subLeft + subplotWidth;
                int subBottom = subTop + subplotHeight;
                
                // Add some padding between subplots
                int padX = 10;
                int padY = 10;
                drawSubplot(hdc, subplots[idx], 
                           subLeft + padX, subTop + padY, 
                           subRight - padX, subBottom - padY);
            }
        } else {
            // Draw single plot
            if (subplots.empty()) {
                subplots.resize(1);
            }
            drawSubplot(hdc, subplots[0], 0, 0, width, height);
        }
    }

    void drawSubplot(HDC hdc, SubplotData& subplot, int left, int top, int right, int bottom) {
        int plotLeft = left + marginLeft;
        int plotRight = right - marginRight;
        int plotTop = top + marginTop;
        int plotBottom = bottom - marginBottom;

        // Ensure valid plot area
        if (plotRight <= plotLeft || plotBottom <= plotTop) return;

        HPEN axisPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
        HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        HFONT font = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                VARIABLE_PITCH, L"Arial");
        HFONT titleFont = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                     OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                     VARIABLE_PITCH, L"Arial");
        HFONT oldFont = (HFONT)SelectObject(hdc, font);

        // Create clipping region for this subplot
        HRGN clipRgn = CreateRectRgn(left, top, right, bottom);
        SelectClipRgn(hdc, clipRgn);

        // Grid
        if (subplot.gridOn) {
            SelectObject(hdc, gridPen);
            for (int i = 0; i <= 5; i++) {
                int x = plotLeft + i * (plotRight - plotLeft) / 5;
                MoveToEx(hdc, x, plotTop, nullptr); LineTo(hdc, x, plotBottom);
                int y = plotTop + i * (plotBottom - plotTop) / 5;
                MoveToEx(hdc, plotLeft, y, nullptr); LineTo(hdc, plotRight, y);
            }
        }

        // Axes
        SelectObject(hdc, axisPen);
        MoveToEx(hdc, plotLeft, plotTop, nullptr);
        LineTo(hdc, plotLeft, plotBottom);
        LineTo(hdc, plotRight, plotBottom);
        LineTo(hdc, plotRight, plotTop);
        LineTo(hdc, plotLeft, plotTop);

        // Ticks and labels
        SetTextAlign(hdc, TA_CENTER | TA_TOP);
        for (int i = 0; i <= 5; i++) {
            double val = subplot.limits.xmin + i * (subplot.limits.xmax - subplot.limits.xmin) / 5;
            int x = plotLeft + i * (plotRight - plotLeft) / 5;
            wchar_t buf[32];
            swprintf(buf, 32, L"%.2g", val);
            TextOutW(hdc, x, plotBottom + 3, buf, (int)wcslen(buf));
        }

        SetTextAlign(hdc, TA_RIGHT | TA_BASELINE);
        for (int i = 0; i <= 5; i++) {
            double val = subplot.limits.ymin + i * (subplot.limits.ymax - subplot.limits.ymin) / 5;
            int y = plotBottom - i * (plotBottom - plotTop) / 5;
            wchar_t buf[32];
            swprintf(buf, 32, L"%.2g", val);
            TextOutW(hdc, plotLeft - 3, y, buf, (int)wcslen(buf));
        }

        // Title
        if (!subplot.title.empty()) {
            SelectObject(hdc, titleFont);
            SetTextAlign(hdc, TA_CENTER | TA_TOP);
            std::wstring wtitle = utf8ToUtf16(subplot.title);
            TextOutW(hdc, (plotLeft + plotRight) / 2, top + 5, wtitle.c_str(), (int)wtitle.length());
        }

        // Labels
        if (!subplot.xLabel.empty()) {
            SelectObject(hdc, font);
            SetTextAlign(hdc, TA_CENTER | TA_TOP);
            std::wstring wxlabel = utf8ToUtf16(subplot.xLabel);
            TextOutW(hdc, (plotLeft + plotRight) / 2, plotBottom + 15, wxlabel.c_str(), (int)wxlabel.length());
        }

        if (!subplot.yLabel.empty()) {
            SelectObject(hdc, font);
            SetTextAlign(hdc, TA_CENTER | TA_BASELINE);
            std::wstring wylabel = utf8ToUtf16(subplot.yLabel);
            // Draw Y label vertically on the left side
            int yPos = (plotTop + plotBottom) / 2;
            int xPos = left + 12;
            for (size_t i = 0; i < wylabel.length(); i++) {
                TextOutW(hdc, xPos, yPos + static_cast<int>(i) * 12, &wylabel[i], 1);
            }
        }

        // Plot series
        for (const auto& s : subplot.series) {
            if (s.x.empty() || s.y.empty()) continue;

            // Determine pen style based on line style
            int penStyle = PS_SOLID;
            switch (s.style.lineStyle) {
                case LineStyle::Dashed: penStyle = PS_DASH; break;
                case LineStyle::Dotted: penStyle = PS_DOT; break;
                case LineStyle::DashDot: penStyle = PS_DASHDOT; break;
                default: penStyle = PS_SOLID; break;
            }

            HPEN linePen = CreatePen(penStyle, s.style.lineWidth,
                RGB(s.style.color.r, s.style.color.g, s.style.color.b));
            SelectObject(hdc, linePen);

            // Draw line segments with clipping
            for (size_t i = 1; i < s.x.size() && i < s.y.size(); i++) {
                int x1 = mapX(s.x[i-1], subplot.limits.xmin, subplot.limits.xmax, plotLeft, plotRight);
                int y1 = mapY(s.y[i-1], subplot.limits.ymin, subplot.limits.ymax, plotTop, plotBottom);
                int x2 = mapX(s.x[i], subplot.limits.xmin, subplot.limits.xmax, plotLeft, plotRight);
                int y2 = mapY(s.y[i], subplot.limits.ymin, subplot.limits.ymax, plotTop, plotBottom);
                
                // Clip line segment to plot area using Cohen-Sutherland algorithm
                if (clipLine(x1, y1, x2, y2, plotLeft, plotTop, plotRight, plotBottom)) {
                    MoveToEx(hdc, x1, y1, nullptr);
                    LineTo(hdc, x2, y2);
                }
            }
            DeleteObject(linePen);
        }

        SelectClipRgn(hdc, nullptr);
        DeleteObject(clipRgn);
        SelectObject(hdc, oldFont);
        DeleteObject(font);
        DeleteObject(titleFont);
        DeleteObject(axisPen);
        DeleteObject(gridPen);
    }
};

PlotFigure::PlotFigure(const std::string& t, int w, int h) : pImpl(std::make_unique<Impl>(t, w, h)) {}

PlotFigure::~PlotFigure() = default;

void PlotFigure::setSubplotGrid(int rows, int cols, int index) {
    pImpl->useSubplots = true;
    pImpl->subplotRows = rows;
    pImpl->subplotCols = cols;
    pImpl->currentSubplot = index;
    
    // Resize subplots vector if needed
    size_t totalSubplots = static_cast<size_t>(rows) * cols;
    if (pImpl->subplots.size() != totalSubplots) {
        pImpl->subplots.resize(totalSubplots);
    }
    
    // Clear the current subplot data
    pImpl->current().clear();
}

void PlotFigure::plot(const std::vector<double>& x, const std::vector<double>& y, const PlotStyle& style) {
    if (!pImpl->hwnd) pImpl->createWindow();
    auto& curr = pImpl->current();
    if (!curr.holdOn) curr.series.clear();
    pImpl->updateLimits(curr, x, y);
    curr.series.push_back({x, y, style, "line"});
    InvalidateRect(pImpl->hwnd, nullptr, FALSE);
}

void PlotFigure::scatter(const std::vector<double>& x, const std::vector<double>& y, const PlotStyle& style) {
    if (!pImpl->hwnd) pImpl->createWindow();
    auto& curr = pImpl->current();
    if (!curr.holdOn) curr.series.clear();
    pImpl->updateLimits(curr, x, y);
    PlotStyle s = style;
    s.lineStyle = LineStyle::None;
    if (s.marker == MarkerStyle::None) s.marker = MarkerStyle::Circle;
    curr.series.push_back({x, y, s, "scatter"});
    InvalidateRect(pImpl->hwnd, nullptr, FALSE);
}

void PlotFigure::bar(const std::vector<double>& x, const std::vector<double>& y, const PlotStyle& style) {
    if (!pImpl->hwnd) pImpl->createWindow();
    auto& curr = pImpl->current();
    if (!curr.holdOn) curr.series.clear();
    pImpl->updateLimits(curr, x, y);
    curr.series.push_back({x, y, style, "bar"});
    InvalidateRect(pImpl->hwnd, nullptr, FALSE);
}

void PlotFigure::hist(const std::vector<double>& data, int bins, const PlotStyle& style) {
    if (data.empty()) return;
    double minVal = *std::min_element(data.begin(), data.end());
    double maxVal = *std::max_element(data.begin(), data.end());
    double binWidth = (maxVal - minVal) / bins;
    std::vector<double> x(bins), y(bins, 0);
    for (int i = 0; i < bins; i++) x[i] = minVal + i * binWidth + binWidth / 2;
    for (double v : data) {
        int bin = static_cast<int>((v - minVal) / binWidth);
        if (bin >= 0 && bin < bins) y[bin]++;
    }
    bar(x, y, style);
}

void PlotFigure::setTitle(const std::string& t) {
    auto& curr = pImpl->current();
    curr.title = t;
    if (pImpl->hwnd) {
        InvalidateRect(pImpl->hwnd, nullptr, FALSE);
    }
}

void PlotFigure::setXLabel(const std::string& l) {
    auto& curr = pImpl->current();
    curr.xLabel = l;
    if (pImpl->hwnd) InvalidateRect(pImpl->hwnd, nullptr, FALSE);
}

void PlotFigure::setYLabel(const std::string& l) {
    auto& curr = pImpl->current();
    curr.yLabel = l;
    if (pImpl->hwnd) InvalidateRect(pImpl->hwnd, nullptr, FALSE);
}

void PlotFigure::setLegend(const std::vector<std::string>& labels) { 
    auto& curr = pImpl->current();
    curr.legendLabels = labels; 
}

void PlotFigure::setGrid(bool on) { 
    auto& curr = pImpl->current();
    curr.gridOn = on; 
}

void PlotFigure::setHold(bool on) { 
    auto& curr = pImpl->current();
    curr.holdOn = on; 
}

void PlotFigure::setXLim(double min, double max) {
    auto& curr = pImpl->current();
    curr.limits.xmin = min; curr.limits.xmax = max; curr.limits.autoX = false;
}

void PlotFigure::setYLim(double min, double max) {
    auto& curr = pImpl->current();
    curr.limits.ymin = min; curr.limits.ymax = max; curr.limits.autoY = false;
}

void PlotFigure::setAxis(const std::vector<double>& limits) {
    auto& curr = pImpl->current();
    if (limits.size() >= 2) { curr.limits.xmin = limits[0]; curr.limits.xmax = limits[1]; curr.limits.autoX = false; }
    if (limits.size() >= 4) { curr.limits.ymin = limits[2]; curr.limits.ymax = limits[3]; curr.limits.autoY = false; }
}

void PlotFigure::clear() {
    auto& curr = pImpl->current();
    curr.clear();
}

void PlotFigure::show() {
    if (!pImpl->hwnd) pImpl->createWindow();
    ShowWindow(pImpl->hwnd, SW_SHOW);
    UpdateWindow(pImpl->hwnd);
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_QUIT) break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void PlotFigure::close() {
    if (pImpl->hwnd) { DestroyWindow(pImpl->hwnd); pImpl->hwnd = nullptr; }
}

bool PlotFigure::isOpen() const { return pImpl->hwnd != nullptr; }
void PlotFigure::redraw() { if (pImpl->hwnd) InvalidateRect(pImpl->hwnd, nullptr, FALSE); }

}

#else

namespace cnlab {
PlotFigure::PlotFigure(const std::string&, int, int) {}
PlotFigure::~PlotFigure() = default;
void PlotFigure::setSubplotGrid(int, int, int) {}
void PlotFigure::plot(const std::vector<double>&, const std::vector<double>&, const PlotStyle&) {}
void PlotFigure::scatter(const std::vector<double>&, const std::vector<double>&, const PlotStyle&) {}
void PlotFigure::bar(const std::vector<double>&, const std::vector<double>&, const PlotStyle&) {}
void PlotFigure::hist(const std::vector<double>&, int, const PlotStyle&) {}
void PlotFigure::setTitle(const std::string&) {}
void PlotFigure::setXLabel(const std::string&) {}
void PlotFigure::setYLabel(const std::string&) {}
void PlotFigure::setLegend(const std::vector<std::string>&) {}
void PlotFigure::setGrid(bool) {}
void PlotFigure::setHold(bool) {}
void PlotFigure::setXLim(double, double) {}
void PlotFigure::setYLim(double, double) {}
void PlotFigure::setAxis(const std::vector<double>&) {}
void PlotFigure::clear() {}
void PlotFigure::show() {}
void PlotFigure::close() {}
bool PlotFigure::isOpen() const { return false; }
void PlotFigure::redraw() {}
}

#endif
