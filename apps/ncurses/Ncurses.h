#pragma once

#include <string>
// #include <functional>

#include <ncurses.h>

namespace ncurses {

namespace color {
    const int kBlack = COLOR_BLACK;
    const int kRed = COLOR_RED;
    const int kGreen = COLOR_GREEN;
    const int kYellow = COLOR_YELLOW;
    const int kBlue = COLOR_BLUE;
    const int kMagenta = COLOR_MAGENTA;
    const int kCyan = COLOR_CYAN;
    const int kWhite = COLOR_WHITE;
}

namespace attribute {
    const int kNormal = A_NORMAL;
    const int kStandout = A_STANDOUT;
    const int kUnderline = A_UNDERLINE;
    const int kReverse = A_REVERSE;
    const int kBlink = A_BLINK;
    const int kDim = A_DIM;
    const int kBold = A_BOLD;
}

enum class HorizontalAlignment {
    kLeft,
    kCenter,
    kRight
};

enum class VerticalAlignment {
    kTop,
    kCenter,
    kBottom
};

class Text {
public:
    Text() = default;
    Text(const Text&) = default;
    Text(Text&&) = default;
    Text(const std::string& str): str_(str) {}
    Text(std::string&& str): str_(std::move(str)) {}

    // TODO: non-ascii characters
    const size_t Width() const {
        return str_.size();
    }

    const std::string& String() const {
        return str_;
    }

    int Attributes() const {
        return attrs_;
    }

    int ForegroundColor() const {
        return fc_;
    }

    int BackgroundColor() const {
        return bc_;
    }

    Text& SetString(const std::string& str) {
        str_ = str;
        return *this;
    }

    Text& SetString(std::string&& str) {
        str_ = std::move(str);
        return *this;
    }

    Text& SetAttributes(int attrs) {
        attrs_ = attrs;
        return *this;
    }

    Text& EnableAttributes(int attr) {
        attrs_ |= attr;
        return *this;
    }

    Text& SetForegroundColor(int fc) {
        fc_ = fc;
        return *this;
    }

    Text& SetBackgroundColor(int bc) {
        bc_ = bc;
        return *this;
    }

    Text& SetColor(int fc, int bc) {
        fc_ = fc;
        bc_ = bc;
        return *this;
    }

private:
    std::string str_;
    int attrs_ = A_NORMAL;
    int fc_ = COLOR_WHITE;
    int bc_ = COLOR_BLACK;
};

class Window {
public:
    Window() = default;

    ~Window() {
        SetVisible(false);
    }

    int X() const {
        return x_;
    }

    int Y() const {
        return y_;
    }

    int Width() const {
        return width_;
    }

    int Height() const {
        return height_;
    }

    void EnableKeypad(bool enable) {
        if (win_) {
            keypad(win_, enable ? TRUE : FALSE);
        }
    }

    void Refresh() {
        if (win_) {
            wrefresh(win_);
        }
    }

    void EnableEcho(bool enable) {
        if (enable) {
            echo();
        } else {
            noecho();
        }
    }

    int Input(int x, int y, bool showCursor = true) {
        curs_set(showCursor ? 1 : 0);
        return mvwgetch(win_, y, x);
    }

    auto Draw(HorizontalAlignment ha, int y, const Text& text) {
        const auto x = ha2x(ha, text);
        return Draw(x, y, text);
    }

    auto Draw(int x, VerticalAlignment va, const Text& text) {
        const auto y = va2y(va);
        return Draw(x, y, text);
    }

    auto Draw(HorizontalAlignment ha, VerticalAlignment va, const Text& text) {
        const auto x = ha2x(ha, text);
        const auto y = va2y(va);
        return Draw(x, y, text);
    }

    auto Draw(HorizontalAlignment ha, int y, const std::string& str) {
        Text text{str};
        const auto x = ha2x(ha, text);
        return Draw(x, y, text);
    }

    auto Draw(int x, VerticalAlignment va, const std::string& str) {
        Text text{str};
        const auto y = va2y(va);
        return Draw(x, y, text);
    }

    auto Draw(HorizontalAlignment ha, VerticalAlignment va, const std::string& str) {
        Text text{str};
        const auto x = ha2x(ha, text);
        const auto y = va2y(va);
        return Draw(x, y, text);
    }

    auto Draw(int x, int y, const std::string& str) {
        // TODO: support more style
        Text text;
        std::string striped;
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '^' && i + 1 < str.size()) {
                switch (str[i+1]) {
                    case 'b': {
                        text.EnableAttributes(attribute::kBold);
                        ++i;
                        break;
                    }
                    default: {
                        striped += str[i+1];
                    }
                }
            } else {
                striped += str[i];
            }
        }
        text.SetString(striped);
        return Draw(x, y, text);
    }

    void Draw(int x, int y, const Text& text) {
        if (!win_) {
            return;
        }
        // attr_t lastAttrs = 0;
        // short lastPair = 0;
        // wattr_get(win_, &lastAttrs, &lastPair, nullptr);
        const auto attrs = text.Attributes();
        const auto fc = text.ForegroundColor();
        const auto bc = text.BackgroundColor();
        const short colorId = fc * 8 + bc + 1;
        init_pair(colorId, fc, bc);
        wattron(win_, attrs | COLOR_PAIR(colorId));
        mvwaddstr(win_, y, x, text.String().c_str());
        wattroff(win_, attrs | COLOR_PAIR(colorId));
        // wattr_set(win_, lastAttrs, lastPair, nullptr);
    }

    void Clear() {
        if (!win_) {
            return;
        }
        werase(win_);
        if (boxed_) {
            box(win_, 0, 0);
        }
    }

    void MoveTo(int x, int y) {
        x_ = x;
        y_ = y;
        if (win_) {
            mvwin(win_, y_, x_);
        }
    }

    void Resize(int width, int height) {
        width_ = width;
        height_ = height;
        if (win_) {
            wresize(win_, height_, width_);
            mvwin(win_, y_, x_);
        }
    }

    void SetVisible(bool visible) {
        if (visible) {
            if (!win_) {
                win_ = newwin(height_, width_, y_, x_);
                if (boxed_) {
                    box(win_, 0, 0);
                }
            }
        } else {
            if (win_) {
                delwin(win_);
                win_ = nullptr;
            }
        }
    }

    bool IsVisible() const {
        return !!win_;
    }

    void AttrOn(int attrs) {
        if (win_) {
            wattron(win_, attrs);
        }
    }

    void AttrSet(int attrs) {
        if (win_) {
            wattrset(win_, attrs);
        }
    }

    void AttrOff(int attrs) {
        if (win_) {
            wattroff(win_, attrs);
        }
    }

    void ResetAttrColor() {
        if (!win_) {
            return;
        }
        init_pair(0, color::kWhite, color::kBlack);
        wattrset(win_, attribute::kNormal | COLOR_PAIR(0));
    }

    void ColorOn(int fg, int bg) {
        if (!win_) {
            return;
        }
        const short colorId = fg * 8 + bg + 1;
        init_pair(colorId, fg, bg);
        wattron(win_, COLOR_PAIR(colorId));
    }

    void ColorOff(short colorId) {
        if (!win_) {
            return;
        }
        wattroff(win_, COLOR_PAIR(colorId));
        wattroff(win_, COLOR_PAIR(0));
    }

// public:
//     void SafelyDo(const std::function<void (void)>& fn)
//     {
//         use_window(win_, &WindowCallback, const_cast<void*>(static_cast<const void*>(&fn)));
//     }

// private:
//     static int WindowCallback(WINDOW* _w, void* p)
//     {
//         using namespace std;
//         auto func = static_cast<std::function<void (void)>*>(p);
//         (*func)();
//         return 0;
//     }

private:
    int ha2x(HorizontalAlignment ha, const Text& text) const {
        int x = 0;
        switch (ha) {
            case HorizontalAlignment::kLeft: {
                x = 0;
                break;
            }
            case HorizontalAlignment::kCenter: {
                x = (width_ - text.Width()) / 2;
                break;
            }
            case HorizontalAlignment::kRight: {
                x = width_ - text.Width();
                break;
            }
        }
        return x;
    }

    int va2y(VerticalAlignment va) const {
        int y = 0;
        switch (va) {
            case VerticalAlignment::kTop: {
                y = 0;
                break;
            }
            case VerticalAlignment::kCenter: {
                y = height_ / 2;
                break;
            }
            case VerticalAlignment::kBottom: {
                y = height_;
                break;
            }
        }
        return y;
    }

    WINDOW* win_ = nullptr;
    bool boxed_ = true;
    int x_ = 0;
    int y_ = 0;
    int width_ = 0;
    int height_ = 0;
};

}