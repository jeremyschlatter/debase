#pragma once
#include <algorithm>
#include "Panel.h"

namespace UI {

struct MenuButton {
    std::string name;
    std::string key;
    bool enabled = false;
};

class _Menu : public _Panel, public std::enable_shared_from_this<_Menu> {
public:
    _Menu(const ColorPalette& colors, std::vector<MenuButton> buttons) : _colors(colors), _buttons(buttons) {
        _buttons = buttons;
        
        // Find the longest button to set our width
        int width = 0;
        for (const MenuButton& button : _buttons) {
            int w = (int)button.name.size() + (int)button.key.size();
            width = std::max(width, w);
        }
        
        int w = width + KeySpacing + 2*(BorderSize+InsetX);
        int h = (RowHeight*(int)buttons.size())-1 + 2;
        setSize({w, h});
        _drawNeeded = true;
    }
    
    
//    _Menu(const std::vector<ButtonPtr>& buttons) : _buttons(buttons) {
//        // Require at least 1 button
//        assert(!buttons.empty());
//        
//        // Find the longest button to set our width
//        int width = 0;
//        for (ButtonPtr button : _buttons) {
//            int w = (int)button->name.size() + (int)button->key.size();
//            width = std::max(width, w);
//        }
//        
//        int w = width + KeySpacing + 2*InsetX;
//        int h = (RowHeight*(int)_buttons.size())-1 + 2;
//        setSize({w, h});
//        _drawNeeded = true;
//    }
    
//    std::optional<Button> hitTest() {
//        
//    }
    
//    ButtonPtr updateMousePosition(std::optional<Point> p) {
//        ButtonPtr mouseOverButton = nullptr;
//        
//        if (p) {
//            Rect frame = rect();
//            Rect bounds = {{}, frame.size};
//            Point off = *p-frame.point;
//            
//            if (!Empty(Intersection(bounds, {off, {1,1}}))) {
//                size_t idx = std::clamp((size_t)0, _buttons.size()-1, (size_t)(off.y / RowHeight));
//                mouseOverButton = _buttons[idx];
//            }
//        }
//        
//        if (_mouseOverButton != mouseOverButton) {
//            _mouseOverButton = mouseOverButton;
//            _drawNeeded = true;
//        }
//        
//        return _mouseOverButton;
//    }
    
    const MenuButton* updateMousePosition(const Point& p) {
        Rect frame = _Panel::frame();
        Size inset = {BorderSize+InsetX, BorderSize};
        Rect innerBounds = Inset({{}, frame.size}, inset);
        Point off = p-frame.point;
        
        const MenuButton* mouseOverButton = nullptr;
        if (!Empty(Intersection(innerBounds, {off, {1,1}}))) {
            off -= innerBounds.point;
            size_t idx = std::min(_buttons.size()-1, (size_t)(off.y / RowHeight));
            MenuButton& button = _buttons[idx];
            mouseOverButton = &button;
        }
        
        if (_mouseOverButton != mouseOverButton) {
            _mouseOverButton = mouseOverButton;
            _drawNeeded = true;
        }
        
        return _mouseOverButton;
    }
    
    void draw() {
        erase();
        
        const int w = bounds().size.x;
        size_t idx = 0;
        for (const MenuButton& button : _buttons) {
//        for (size_t i=0; i<_buttonCount; i++) {
//            ::wmove(*this, p.y, p.x);
//            ::vw_printw(*this, fmt, args);
            
//            UI::Attr attr(shared_from_this(), _buttons[i]==_mouseOverButton ? A_UNDERLINE : A_NORMAL);
            
            int y = 1 + (int)idx*RowHeight;
            
            // Draw button name
            {
//                UI::Attr attr(shared_from_this(), _colors.menu);
                UI::Attr attr;
                
                if (&button==_mouseOverButton && button.enabled) {
                    attr = UI::Attr(shared_from_this(), _colors.menu|A_BOLD);
                
                } else if (!button.enabled) {
                    attr = UI::Attr(shared_from_this(), _colors.subtitleText);
                
                } else {
                    attr = UI::Attr(shared_from_this(), A_NORMAL);
                }
                
                drawText({BorderSize+InsetX, y}, "%s", button.name.c_str());
            }
            
//            wchar_t str[] = L"⌫";
//            mvwaddchnstr(*this, y, InsetX, (chtype*)str, 1);
            
//            drawText({InsetX, y}, "⌫");
//            drawText({InsetX, y}, "%s", button.name.c_str());
            
            // Draw button key
            {
                UI::Attr attr(shared_from_this(), _colors.subtitleText);
                drawText({w-BorderSize-InsetX-(int)button.key.size(), y}, "%s", button.key.c_str());
            }
            
            // Draw separator
            
            if (&button != &_buttons.back()) {
                UI::Attr attr(shared_from_this(), _colors.menu);
                drawLineHoriz({0,y+1}, w);
            }
            
//            if (_buttons[i] == _mouseOverButton) {
//                drawLineHoriz({0,y}, 10);
//            }
            
            idx++;
        }
        
        // Draw border
        {
            UI::Attr attr(shared_from_this(), _colors.menu);
            drawBorder();
        }
        
        _drawNeeded = false;
    }
    
    void drawIfNeeded() {
        if (_drawNeeded) {
            draw();
        }
    }
    
private:
    static constexpr int BorderSize = 1;
    static constexpr int InsetX     = 1;
    static constexpr int KeySpacing = 2;
    static constexpr int RowHeight  = 2;
    const ColorPalette& _colors;
    std::vector<MenuButton> _buttons;
    const MenuButton* _mouseOverButton = nullptr;
    bool _drawNeeded = false;
};

using Menu = std::shared_ptr<_Menu>;

} // namespace UI
