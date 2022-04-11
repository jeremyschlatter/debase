#pragma once
#define NCURSES_WIDECHAR 1
#include <vector>
#include "lib/ncurses/include/curses.h"
#include "lib/ncurses/include/panel.h"
#include "Bitfield.h"
#include "UI.h"
#include "CursorState.h"
#include "UTF8.h"
#include "View.h"
#include <os/log.h>

namespace UI {

class Window : public View {
public:
    struct UninitType {};
    static constexpr UninitType Uninit;
    Window(UninitType) {}
    
    Window(WINDOW* win=nullptr) : _s{.win = win} {
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
    
    virtual ~Window() {
        if (_s.win) {
            ::delwin(_s.win);
        }
    }
    
    Point origin() const override { return View::origin(); }
    
    void origin(const Point& x) override {
        const bool drawNeededPrev = drawNeeded();
        View::origin(x);
        // Suppress View's behavior of dirtying ourself when changing the origin
        if (!drawNeededPrev) drawNeeded(false);
    }
    
    Size size() const override { return View::size(); }
    void size(const Size& x) override {
        if (x == size()) return;
        View::size(x);
        eraseNeeded(true);
    }
    
    virtual Point windowOrigin() const { return { getbegx(_s.win), getbegy(_s.win) }; }
    virtual void windowOrigin(const Point& p) {
        if (p == windowOrigin()) return;
        ::wmove(*this, p.y, p.x);
    }
    
    virtual Size windowSize() const { return { getmaxx(_s.win), getmaxy(_s.win) }; }
    virtual void windowSize(const Size& s) {
        if (s == windowSize()) return;
//        const Size ss = {std::max(1,s.x), std::max(1,s.y)};
//        if (ss == windowSize()) return;
        ::wresize(*this, s.y, s.x);
    }
    
//    Point origin() const override { return { getbegx(_s.win), getbegy(_s.win) }; }
//    
//    Size size() const override { return { getmaxx(_s.win), getmaxy(_s.win) }; }
//    void size(const Size& s) override {
//        Size ss = {std::max(1,s.x), std::max(1,s.y)};
//        // Short-circuit if the size hasn't changed
//        if (ss == size()) return;
//        ::wresize(*this, ss.y, ss.x);
//    }
    
//    Point mousePosition(const Event& ev) const {
//        return ev.mouse.origin-origin();
//    }
    
//    // convert(): convert a point from the coorindate system of the parent window to the coordinate system of `this`
//    virtual Point convert(const Point& p) const {
//        return p-origin();
//    }
//    
//    // convert(): convert an event from the coorindate system of the parent window to the coordinate system of `this`
//    virtual Event convert(const Event& p) const {
//        Event r = p;
//        if (r.type == Event::Type::Mouse) {
//            r.mouse.point = convert(r.mouse.point);
//        }
//        return r;
//    }
    
    virtual void drawRect() const override {
        // For the case where we're drawing the window's border, we attempt
        // to be more efficent than View's drawRect() by using ::box().
        ::box(*this, 0, 0);
    }
    
    virtual void drawRect(const Rect& rect) const override {
        View::drawRect(rect);
    }
    
//    bool layoutNeeded() const override { return View::layoutNeeded() || _s.sizePrev!=size(); }
//    void layoutNeeded(bool x) override { View::layoutNeeded(x); }
    
//    void layout(const Window& win, const Point& orig) override {
//        windowSize(size());
//        windowOrigin(orig);
//        
////        if (_s.originPrev != orig) {
////            
////            _s.originPrev = orig;
////        }
////        
////        // Detect size changes
////        // ncurses can change our size out from under us (eg by the
////        // terminal size changing), so we handle all size changes
////        // here, instead of in the size() setter
////        if (_s.sizePrev != size()) {
////            // We need to erase+redraw after resizing
////            // (eraseNeeded=true implicity sets drawNeeded=true)
////            eraseNeeded(true);
////            _s.sizePrev = size();
////        }
//        
//        View::layout(*this, {});
//    }
//    
//    void draw(const Window& win, const Point& orig) override {
//        // Remember whether we erased ourself during this draw cycle
//        // This is used by View instances (Button and TextField)
//        // to know whether they need to be drawn again
//        _s.erased = _s.eraseNeeded;
//        
//        // Erase ourself if needed, and remember that we did so
//        if (_s.eraseNeeded) {
//            ::werase(*this);
//            _s.eraseNeeded = false;
//        }
//        
//        View::draw(*this, {});
//    }
    
    
    virtual GraphicsState convert(GraphicsState x) override {
        x.window = this;
        x.originWindow = {};
        x.originScreen += origin();
        x.erased = false;
        return x;
    }
    
    virtual void layout(GraphicsState gstate) override {
        if (!visible()) return;
        
//        const Size sizePrev = size();
        windowSize(size());
        windowOrigin(gstate.originScreen);
        
//        os_log(OS_LOG_DEFAULT, "SIZE CHANGED %d", windowSize().y);
//        
////        // Detect size changes
////        // ncurses can change our size out from under us (eg by the
////        // terminal size changing), so we handle all size changes
////        // here, instead of in the size() setter
////        if (_s.sizePrev != size()) {
////            // We need to erase+redraw after resizing
////            // (eraseNeeded=true implicity sets drawNeeded=true)
////            eraseNeeded(true);
////            _s.sizePrev = size();
////        }
////        
//        // Update our size/origin based on ncurses' adjusted size
//        const Size sizeActual = windowSize();
//        const Size originScreenActual = windowOrigin();
//        const Size originScreenDelta = originScreenActual-gstate.originScreen;
//        if (originScreenDelta.x || originScreenDelta.y) {
////            layoutNeeded(true);
//            
//            os_log(OS_LOG_DEFAULT, "ORIGIN CHANGED");
//        }
//        
////        size(sizeActual);
////        origin(origin()+originScreenDelta);
//        
//        if (sizeActual != sizePrev) {
//            os_log(OS_LOG_DEFAULT, "SIZE CHANGED");
//            
////            abort();
////            // We need to erase+redraw after resizing
////            // (eraseNeeded=true implicity triggers a redraw)
////            eraseNeeded(true);
//        }
        
//        
//        if (originScreenDelta.x || originScreenDelta.y) {
//            os_log(OS_LOG_DEFAULT, "origin AFTER: %d %d", origin().x, origin().y);
//        }
//        
//        // Update our gstate based on ncurses' adjusted size
//        gstate.originWindow += originScreenDelta;
//        gstate.originScreen += originScreenDelta;
//        
//        layoutNeeded(true);
        
        View::layout(gstate);
    }
    
    virtual void draw(GraphicsState gstate) override {
        if (!visible()) return;
        if (eraseNeeded()) {
            gstate.erased = true;
            ::werase(*this);
            eraseNeeded(false);
        }
        
        View::draw(gstate);
    }
    
    // eraseNeeded(): whether the window should be erased the next time it's drawn
    virtual bool eraseNeeded() { return _s.eraseNeeded; }
    virtual void eraseNeeded(bool x) { _s.eraseNeeded = x; }
    
    virtual Window& operator =(Window&& x) { std::swap(_s, x._s); return *this; }
    
    virtual operator WINDOW*() const { return _s.win; }
    
protected:
    template <typename X, typename Y>
    void _setForce(X& x, const Y& y) {
        x = y;
        drawNeeded(true);
    }
    
    template <typename X, typename Y>
    void _set(X& x, const Y& y) {
        if (x == y) return;
        x = y;
        drawNeeded(true);
    }
    
private:
    struct {
        WINDOW* win = nullptr;
//        Point originPrev;
//        Size sizePrev;
        // eraseNeeded: tracks whether the window needs to be erased the next time it's drawn
        bool eraseNeeded = true;
    } _s;
};

using WindowPtr = std::shared_ptr<Window>;

} // namespace UI
