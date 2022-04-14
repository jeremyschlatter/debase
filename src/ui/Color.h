#pragma once
#include <atomic>
#include <vector>
#include "lib/toastbox/RuntimeError.h"

namespace UI {

struct ColorIdx {
    NCURSES_COLOR_T idx = COLOR_BLACK;
    
    ColorIdx() {}
    ColorIdx(NCURSES_COLOR_T idx) : idx(idx) {}
    operator NCURSES_COLOR_T() const { return idx; }
};

struct ColorPair {
    NCURSES_PAIRS_T idx = WhiteOnBlackIdx;
    
    ColorPair() {}
    ColorPair(NCURSES_PAIRS_T idx) : idx(idx) {}
    
    // WhiteOnBlackIdx: color-pair 0 is ncurses' immutable white-on-black color pair
    static constexpr NCURSES_PAIRS_T WhiteOnBlackIdx = 0;
    operator NCURSES_PAIRS_T() const { return COLOR_PAIR(idx); }
};

class ColorSwapper {
public:
    ColorSwapper() {}
    ColorSwapper(const ColorIdx& idx, NCURSES_COLOR_T r, NCURSES_COLOR_T g, NCURSES_COLOR_T b) : _s{.idx=idx} {
        // Remember the previous RGB values for color index `idx`
        int ir = ::color_content(*_s.idx, &_s.r, &_s.g, &_s.b);
        if (ir != OK) throw std::runtime_error("color_content() failed");
        
        // Set the new RGB values for color index `idx`
        ir = ::init_color(*_s.idx, r, g, b);
        if (ir != OK) throw std::runtime_error("init_color() failed");
    }
    
    ColorSwapper(const ColorSwapper& x) = delete;
    ColorSwapper(ColorSwapper&& x) {
        std::swap(_s, x._s);
    }
    
    ColorSwapper& operator =(ColorSwapper&& x) {
        std::swap(_s, x._s);
        return *this;
    }
    
    ~ColorSwapper() {
        if (_s.idx) {
            ::init_color(*_s.idx, _s.r, _s.g, _s.b);
        }
    }
    
private:
    struct {
        std::optional<ColorIdx> idx;
        NCURSES_COLOR_T r = -1;
        NCURSES_COLOR_T g = -1;
        NCURSES_COLOR_T b = -1;
    } _s;
};


class ColorPairSwapper {
public:
    ColorPairSwapper() {}
    ColorPairSwapper(const ColorPair& pair, NCURSES_COLOR_T fg, NCURSES_COLOR_T bg) : _s{.pair=pair} {
        // Remember the previous fg/bg colors for the color pair c.idx
        int ir = ::pair_content(_s.pair->idx, &_s.fg, &_s.bg);
        if (ir != OK) throw std::runtime_error("pair_content() failed");
        
        // Set the new RGB values for the color `c.idx`
        ir = ::init_pair(_s.pair->idx, fg, bg);
        if (ir != OK) throw std::runtime_error("init_pair() failed");
    }
    
    ColorPairSwapper(const ColorPairSwapper& x) = delete;
    
    ColorPairSwapper(ColorPairSwapper&& x) {
        std::swap(_s, x._s);
    }
    
    ColorPairSwapper& operator =(ColorPairSwapper&& x) {
        std::swap(_s, x._s);
        return *this;
    }
    
    ~ColorPairSwapper() {
        if (_s.pair) {
            ::init_pair(_s.pair->idx, _s.fg, _s.bg);
        }
    }
    
private:
    struct {
        std::optional<ColorPair> pair;
        NCURSES_COLOR_T fg = -1;
        NCURSES_COLOR_T bg = -1;
    } _s;
};

class ColorPalette {
public:
    ColorPair normal;
    ColorPair dimmed;
    ColorPair selection;
    ColorPair selectionSimilar;
    ColorPair selectionCopy;
    ColorPair menu;
    ColorPair error;
    
    // add(): create a new color pair with a given color index as the foreground color
    ColorPair add(NCURSES_COLOR_T colorIdx) {
        _colorPairSwappers.emplace_back(_colorPairIdx, colorIdx, -1);
        return _colorPairIdx++;
    }
    
    // add(): create a new color with the given RGB components, and assign it to a new color pair as the foreground color
    ColorPair add(uint8_t r, uint8_t g, uint8_t b) {
        _colorSwappers.emplace_back(_colorIdx,
            ((NCURSES_COLOR_T)r*1000)/255,
            ((NCURSES_COLOR_T)g*1000)/255,
            ((NCURSES_COLOR_T)b*1000)/255
        );
        return add(_colorIdx++);
    }
    
private:
    // _ColorIdxInit: start outside the standard 0-7 range because we don't want
    // to clobber the standard terminal colors. This is because reading the current
    // terminal colors isn't reliable (via color_content), therefore when we
    // restore colors on exit, we won't necessarily be restoring the original
    // color. So if we're going to clobber colors, clobber the colors that are less
    // likely to be used.
    static constexpr NCURSES_COLOR_T _ColorIdxInit = 16;
    
    NCURSES_COLOR_T _colorIdx = _ColorIdxInit;
    NCURSES_PAIRS_T _colorPairIdx = 1;
    
    std::vector<ColorSwapper> _colorSwappers;
    std::vector<ColorPairSwapper> _colorPairSwappers;
};

} // namespace UI
