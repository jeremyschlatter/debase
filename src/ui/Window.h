#pragma once
#define NCURSES_WIDECHAR 1
#include "lib/ncurses/include/curses.h"
#include "lib/ncurses/include/panel.h"
#include "Bitfield.h"
#include "UI.h"
#include "CursorState.h"

namespace UI {

class Window {
public:
    class Attr {
    public:
        Attr() {}
        Attr(const Window& win, int attr) : _s({.win=&win, .attr=attr}) {
//            if (rand() % 2) {
//                wattron(*_s.win, A_REVERSE);
//            } else {
//                wattroff(*_s.win, A_REVERSE);
//            }
            
            wattron(*_s.win, _s.attr);
        }
        
        Attr(const Attr& x) = delete;
        Attr(Attr&& x) { std::swap(_s, x._s); }
        Attr& operator =(Attr&& x) { std::swap(_s, x._s); return *this; }
        
        ~Attr() {
            if (_s.win) {
                wattroff(*_s.win, _s.attr);
            }
        }
    
    private:
        struct {
            const Window* win = nullptr;
            int attr = 0;
        } _s;
    };
    
    Window() {}
    
    Window(WINDOW* win) : _s{.win = win} {
        if (!_s.win) {
            _s.win = ::newwin(0, 0, 0, 0);
            assert(_s.win);
        }
        
        ::keypad(_s.win, true);
        ::meta(_s.win, true);
    }
    
    Window(const Window& x) = delete;
    Window(Window&& x) {
        std::swap(_s, x._s);
    }
    
    ~Window() {
        if (_s.win) {
            ::delwin(_s.win);
        }
    }
    
    void drawBorder() const {
        ::box(*this, 0, 0);
    }
    
    void drawLineHoriz(const Point& p, int len, chtype ch=0) const {
        mvwhline(*this, p.y, p.x, ch, len);
    }
    
    void drawLineVert(const Point& p, int len, chtype ch=0) const {
        mvwvline(*this, p.y, p.x, ch, len);
    }
    
    void drawRect(const Rect& rect) const {
        const int x1 = rect.point.x;
        const int y1 = rect.point.y;
        const int x2 = rect.point.x+rect.size.x-1;
        const int y2 = rect.point.y+rect.size.y-1;
        mvwhline(*this, y1, x1, 0, rect.size.x);
        mvwhline(*this, y2, x1, 0, rect.size.x);
        mvwvline(*this, y1, x1, 0, rect.size.y);
        mvwvline(*this, y1, x2, 0, rect.size.y);
        mvwaddch(*this, y1, x1, ACS_ULCORNER);
        mvwaddch(*this, y2, x1, ACS_LLCORNER);
        mvwaddch(*this, y1, x2, ACS_URCORNER);
        mvwaddch(*this, y2, x2, ACS_LRCORNER);
    }
    
    template <typename ...T_Args>
    void drawText(const Point& p, const char* fmt, T_Args&&... args) const {
        mvwprintw(*this, p.y, p.x, fmt, std::forward<T_Args>(args)...);
    }
    
    Point position() const { return { getbegx(_s.win), getbegy(_s.win) }; }
    
    Size size() const { return { getmaxx(_s.win), getmaxy(_s.win) }; }
    void size(const Size& s) {
        // Short-circuit if the size hasn't changed
        if (s == size()) return;
        ::wresize(*this, std::max(1, s.y), std::max(1, s.x));
        layoutNeeded(true);
        drawNeeded(true);
    }
    
    virtual Size sizeIntrinsic() { return size(); }
    
    Rect bounds() const {
        return Rect{ .size = size() };
    }
    
    Rect frame() const {
        return Rect{
            .point = position(),
            .size  = size(),
        };
    }
    
    bool hitTest(const Point& p) const {
        return HitTest(frame(), p);
    }
    
    // convert(): convert a point from the coorindate system of the parent window to the coordinate system of `this`
    Point convert(const Point& p) const {
        Point r = p;
        r -= frame().point;
        return r;
    }
    
    // convert(): convert an event from the coorindate system of the parent window to the coordinate system of `this`
    Event convert(const Event& p) const {
        Event r = p;
        if (r.type == Event::Type::Mouse) {
            r.mouse.point = convert(r.mouse.point);
        }
        return r;
    }
    
    Attr attr(int attr) const {
        return Attr(*this, attr);
    }
    
    Event nextEvent() const {
        // Wait for another mouse event
        for (;;) {
            int ch = ::wgetch(*this);
            if (ch == ERR) continue;
            
            Event ev = {
                .type = (Event::Type)ch,
                .time = std::chrono::steady_clock::now(),
            };
            switch (ev.type) {
            case Event::Type::Mouse: {
                MEVENT mouse = {};
                int ir = ::getmouse(&mouse);
                if (ir != OK) continue;
                ev.mouse = {
                    .point = {mouse.x, mouse.y},
                    .bstate = mouse.bstate,
                };
                break;
            }
            
            case Event::Type::WindowResize: {
                throw WindowResize();
            }
            
            case Event::Type::KeyCtrlC:
            case Event::Type::KeyCtrlD: {
                throw ExitRequest();
            }
            
            default: {
                break;
            }}
            
            ev = convert(ev);
            return ev;
        }
    }
    
    virtual bool handleEvent(const Event& ev) { return false; }
    
    void refresh() {
        layout();
        draw();
        CursorState::Draw();
        ::update_panels();
        ::refresh();
    }
    
    virtual void track() {
        for (;;) {
            refresh();
            
            _s.eventCurrent = nextEvent();
            bool handled = handleEvent(_s.eventCurrent);
            _s.eventCurrent = {};
            
            // Continue until an event isn't handled
            if (!handled) break;
        }
    }
    
    virtual bool layoutNeeded() const { return _s.layoutNeeded; }
    virtual void layoutNeeded(bool x) { _s.layoutNeeded = x; }
    virtual bool layout() {
        // Detect size changes that can occurs from underneath us
        // by ncurses (eg by the terminal size changing)
        const bool sizeChanged = _s.sizePrev!=size();
        if (layoutNeeded() || sizeChanged) {
            if (sizeChanged) {
                // We need to erase (and redraw) after resizing
                eraseNeeded(true);
            }
            _s.sizePrev = size();
            layoutNeeded(false);
            return true;
        }
        return false;
    }
    
    virtual bool drawNeeded() const { return _s.drawNeeded; }
    virtual void drawNeeded(bool x) { _s.drawNeeded = x; }
    virtual bool draw() {
        if (drawNeeded()) {
            // Remember whether we erased ourself during this draw cycle
            // This is used by Control instances (Button and TextField)
            // to know whether they need to be drawn again
            _s.erased = _s.eraseNeeded;
            
            // Erase ourself if needed, and remember that we did so
            if (_s.eraseNeeded) {
                ::werase(*this);
                _s.eraseNeeded = false;
            }
            
            drawNeeded(false);
            return true;
        }
        return false;
    }
    
    // eraseNeeded(): sets whether the window should be erased the next time it's drawn
    void eraseNeeded(bool x) {
        _s.eraseNeeded = x;
        if (_s.eraseNeeded) {
            drawNeeded(true);
        }
    }
    
    // erased(): whether the window was erased during this draw cycle
    bool erased() const { return _s.erased; }
    
    Window& operator =(Window&& x) { std::swap(_s, x._s); return *this; }
    
    const Event& eventCurrent() const { return _s.eventCurrent; }
    operator WINDOW*() const { return _s.win; }
    
private:
    struct {
        WINDOW* win = nullptr;
        Size sizePrev;
        Event eventCurrent;
        bool layoutNeeded = true;
        bool drawNeeded = true;
        // eraseNeeded: tracks whether the window needs to be erased the next time it's drawn
        bool eraseNeeded = true;
        // erased: tracks whether the window was erased in this draw cycle
        bool erased = false;
    } _s;
};

using WindowPtr = std::shared_ptr<Window>;

} // namespace UI