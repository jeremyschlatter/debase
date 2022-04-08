#include "Window.h"

namespace UI {

bool View::_winErased(const Window& win) const {
    return win.erased();
}

WINDOW* View::_drawWin() const {
    assert(_drawState.win);
    return *_drawState.win;
}

Point View::_drawOff() const {
    assert(_drawState.win);
    return _drawState.off;
}

} // namespace UI
