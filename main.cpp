#include <iostream>
#include <unistd.h>
#include <vector>
#include <cassert>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <deque>
#include <set>
#include "Git.h"
#include "Window.h"
#include "Panel.h"
#include "RuntimeError.h"
#include "xterm-256color.h"
#include "RuntimeError.h"
#include "CommitPanel.h"
#include "ShadowPanel.h"
#include "BranchColumn.h"

struct Selection {
    BranchColumn* column = nullptr;
    CommitPanelSet panels;
};

static Window _RootWindow;
static std::vector<BranchColumn> _BranchColumns;
static Selection _Selection;

static void _Draw() {
    for (BranchColumn& col : _BranchColumns) col.draw();
    Window::Redraw();
}

static std::tuple<BranchColumn*,CommitPanel*> _HitTest(const Point& p) {
    for (BranchColumn& col : _BranchColumns) {
        if (CommitPanel* panel = col.hitTest(p)) {
            return std::make_tuple(&col, panel);
        }
    }
    return {};
}

static std::tuple<BranchColumn*,CommitPanelVecIter> _FindInsertPoint(const Point& p) {
    BranchColumn* insertCol = nullptr;
    CommitPanelVec::iterator insertIter;
    std::optional<int> insertLeastDistance;
    for (BranchColumn& col : _BranchColumns) {
        CommitPanelVec& panels = col.panels();
        const Rect lastRect = panels.back().rect();
        const int midX = lastRect.point.x + lastRect.size.x/2;
        const int endY = lastRect.point.y + lastRect.size.y;
        
        for (auto it=panels.begin();; it++) {
            const int x = (it!=panels.end() ? it->rect().point.x+it->rect().size.x/2 : midX);
            const int y = (it!=panels.end() ? it->rect().point.y : endY);
            int dist = (p.x-x)*(p.x-x)+(p.y-y)*(p.y-y);
            
            if (!insertLeastDistance || dist<insertLeastDistance) {
                insertCol = &col;
                insertIter = it;
                insertLeastDistance = dist;
            }
            if (it==panels.end()) break;
        }
    }
    
    // Adjust the insert point so that it doesn't occur within a selection
    assert(insertCol);
    CommitPanelVec& insertColPanels = insertCol->panels();
    while (insertIter!=insertColPanels.begin() && Contains(_Selection.panels, &*std::prev(insertIter))) {
        insertIter--;
    }
    
    return std::make_tuple(insertCol, insertIter);
}

static void _TrackMouse(MEVENT mouseDownEvent) {
    struct {
        BranchColumn* column = nullptr;
        CommitPanel* panel = nullptr;
        Size delta;
        bool wasSelected = false;
    } mouseDownCommit;
    
    std::tie(mouseDownCommit.column, mouseDownCommit.panel) = _HitTest({mouseDownEvent.x, mouseDownEvent.y});
    
    if (mouseDownCommit.panel) {
        const Rect panelRect = mouseDownCommit.panel->rect();
        mouseDownCommit.delta = {
            panelRect.point.x-mouseDownEvent.x,
            panelRect.point.y-mouseDownEvent.y,
        };
        mouseDownCommit.wasSelected = Contains(_Selection.panels, mouseDownCommit.panel);
    }
    
    const bool shift = (mouseDownEvent.bstate & BUTTON_SHIFT);
    Selection selectionOld = _Selection;
    
    if (mouseDownCommit.panel) {
        // Clear the selection if the mouse-down is in a different column than the current selection,
        // or an unselected commit was clicked without holding shift
        if ((!_Selection.panels.empty() && mouseDownCommit.column!=_Selection.column) || (!mouseDownCommit.wasSelected && !shift)) {
            _Selection = {
                .column = mouseDownCommit.column,
                .panels = {mouseDownCommit.panel},
            };
        
        } else {
            assert(_Selection.panels.empty() || (_Selection.column==mouseDownCommit.column));
            _Selection.column = mouseDownCommit.column;
            _Selection.panels.insert(mouseDownCommit.panel);
        }
    }
    
    struct {
        std::optional<CommitPanel> titlePanel;
        std::vector<ShadowPanel> shadowPanels;
    } drag;
    
    MEVENT mouse = mouseDownEvent;
    for (;;) {
        const bool mouseUp = mouse.bstate & BUTTON1_RELEASED;
        const bool option = mouse.bstate & BUTTON_ALT;
        
        const int x = std::min(mouseDownEvent.x, mouse.x);
        const int y = std::min(mouseDownEvent.y, mouse.y);
        const int w = std::abs(mouseDownEvent.x - mouse.x);
        const int h = std::abs(mouseDownEvent.y - mouse.y);
        
        // Mouse-down within a commit without `shift` key:
        // Handle commit selection and selection dragging
        if (!shift && mouseDownCommit.panel) {
            assert(!_Selection.panels.empty());
            const bool startDrag = w>1 || h>1;
            
            if (!drag.titlePanel && startDrag) {
                const CommitPanel& titlePanel = *(*_Selection.panels.begin());
                // Draw title panel
                Git::Commit titleCommit = titlePanel.commit();
                drag.titlePanel.emplace(titlePanel.commit(), 0, titlePanel.rect().size.x);
                drag.titlePanel->setOutlineColor(Colors::SelectionMove);
                drag.titlePanel->draw();
                
                // Create shadow panels
                Size shadowSize = (*_Selection.panels.begin())->rect().size;
                for (size_t i=0; i<_Selection.panels.size()-1; i++) {
                    drag.shadowPanels.emplace_back(shadowSize);
                }
                
                // Hide the original CommitPanels while we're dragging
                for (auto it=_Selection.panels.begin(); it!=_Selection.panels.end(); it++) {
                    (*it)->setVisible(false);
                }
                
                // Order all the title panel and shadow panels
                for (auto it=drag.shadowPanels.rbegin(); it!=drag.shadowPanels.rend(); it++) {
                    it->orderFront();
                }
                drag.titlePanel->orderFront();
            }
            
            if (drag.titlePanel) {
                const Point pos0 = {
                    mouse.x+mouseDownCommit.delta.x,
                    mouse.y+mouseDownCommit.delta.y,
                };
                
                // Position title panel
                drag.titlePanel->setPosition(pos0);
                
                // Position shadowPanels
                int off = 1;
                for (Panel& p : drag.shadowPanels) {
                    const Point pos = {pos0.x+off, pos0.y+off};
                    p.setPosition(pos);
                    off++;
                }
                
                // Find insertion position
                auto [insertCol, insertIter] = _FindInsertPoint({mouse.x, mouse.y});
                
                // Draw insertion marker
                {
                    constexpr int InsertExtraWidth = 6;
                    CommitPanelVec& insertColPanels = insertCol->panels();
                    const Rect lastRect = insertColPanels.back().rect();
                    const int endY = lastRect.point.y + lastRect.size.y;
                    const int insertY = (insertIter!=insertColPanels.end() ? insertIter->rect().point.y : endY+1);
                    Window::Attr attr = _RootWindow.setAttr(COLOR_PAIR(Colors::SelectionMove));
                    _RootWindow.erase();
                    _RootWindow.drawLineHoriz({lastRect.point.x-InsertExtraWidth/2,insertY-1}, lastRect.size.x+InsertExtraWidth);
                }
            
            } else {
                if (mouseUp) {
                    if (shift) {
                        if (mouseDownCommit.wasSelected) {
                            _Selection.panels.erase(mouseDownCommit.panel);
                        }
                    } else {
                        _Selection.panels = {mouseDownCommit.panel};
                    }
                }
            }
            
            if (drag.titlePanel) {
                drag.titlePanel->setOutlineColor(option ? Colors::SelectionCopy : Colors::SelectionMove);
                
                for (ShadowPanel& panel : drag.shadowPanels) {
                    panel.setOutlineColor(option ? Colors::SelectionCopy : Colors::SelectionMove);
                }
            }
            
            // Update selection states of CommitPanels
            for (BranchColumn& col : _BranchColumns) {
                for (CommitPanel& panel : col.panels()) {
                    bool selected = Contains(_Selection.panels, &panel);
                    if (selected) {
                        if (option) panel.setOutlineColor(Colors::SelectionCopy);
                        else        panel.setOutlineColor(Colors::SelectionMove);
                    } else {
                        panel.setOutlineColor(std::nullopt);
                    }
                }
            }
        
        // Mouse-down outside of a commit:
        // Handle selection rect drawing / selecting commits
        } else {
            const Rect selectionRect = {{x,y}, {std::max(1,w),std::max(1,h)}};
            
            // Update selection
            {
                Selection selectionNew;
                BranchColumn* preferredColumn = (!selectionOld.panels.empty() ? selectionOld.column : nullptr);
                
                if (preferredColumn) {
                    for (CommitPanel& panel : preferredColumn->panels()) {
                        if (!Empty(Intersection(selectionRect, panel.rect()))) {
                            selectionNew.column = preferredColumn;
                            selectionNew.panels.insert(&panel);
                        }
                    }
                }
                
                if (selectionNew.panels.empty()) {
                    for (BranchColumn& col : _BranchColumns) {
                        for (CommitPanel& panel : col.panels()) {
                            if (!Empty(Intersection(selectionRect, panel.rect()))) {
                                selectionNew.column = &col;
                                selectionNew.panels.insert(&panel);
                            }
                        }
                        if (!selectionNew.panels.empty()) break;
                    }
                }
                
                if (shift && (selectionNew.panels.empty() || selectionOld.column==selectionNew.column)) {
                    Selection selection = {
                        .column = selectionOld.column,
                    };
                    
                    // selection = _Selection XOR selectionNew
                    std::set_symmetric_difference(
                        selectionOld.panels.begin(), selectionOld.panels.end(),
                        selectionNew.panels.begin(), selectionNew.panels.end(),
                        std::inserter(selection.panels, selection.panels.begin())
                    );
                    
                    _Selection = selection;
                
                } else {
                    _Selection = selectionNew;
                }
            }
            
            // Redraw selection rect
            if (drag.underway) {
                _RootWindow.erase();
                _RootWindow.drawRect(selectionRect);
            }
            
//            for (ShadowPanel& panel : drag.shadowPanels) {
////                bool selected = Contains(_Selection.panels, &panel);
////                if (selected) panel.setOutlineColor(Colors::SelectionMove);
////                else          panel.setOutlineColor(std::nullopt);
//            }
            
            // Update selection states of CommitPanels
            for (BranchColumn& col : _BranchColumns) {
                for (CommitPanel& panel : col.panels()) {
                    bool selected = Contains(_Selection.panels, &panel);
                    if (selected) panel.setOutlineColor(Colors::SelectionMove);
                    else          panel.setOutlineColor(std::nullopt);
                }
            }
        }
        
        _Draw();
        
        if (mouseUp) break;
        
        // Wait for another mouse event
        for (;;) {
            int key = _RootWindow.getChar();
            if (key != KEY_MOUSE) continue;
            int ir = ::getmouse(&mouse);
            if (ir != OK) continue;
            break;
        }
    }
    
    for (CommitPanel* p : _Selection.panels) {
        p->setVisible(true);
    }
    
    // Clear selection rect when returning
    _RootWindow.erase();
}

int main(int argc, const char* argv[]) {
//    volatile bool a = false;
//    while (!a);
    
    try {
        // Handle args
        std::vector<std::string> branches;
        {
            for (int i=1; i<argc; i++) {
                branches.push_back(argv[i]);
            }
        }
        
        // Init ncurses
        {
            // Default linux installs may not contain the /usr/share/terminfo database,
            // so provide a fallback terminfo that usually works.
            nc_set_default_terminfo(xterm_256color, sizeof(xterm_256color));
            
            // Override the terminfo 'kmous' and 'XM' properties
            //   kmous = the prefix used to detect/parse mouse events
            //   XM    = the escape string used to enable mouse events (1006=SGR 1006 mouse
            //           event mode; 1003=report mouse-moved events in addition to clicks)
            setenv("TERM_KMOUS", "\x1b[<", true);
            setenv("TERM_XM", "\x1b[?1006;1003%?%p1%{1}%=%th%el%;", true);
            
            ::initscr();
            ::noecho();
            ::cbreak();
            
            ::use_default_colors();
            ::start_color();
            
            #warning TODO: cleanup color logic
            #warning TODO: fix: colors aren't restored when exiting
            // Redefine colors to our custom palette
            {
                int c = 1;
                
                ::init_color(c, 0, 0, 1000);
                ::init_pair(Colors::SelectionMove, c, -1);
                c++;
                
                ::init_color(c, 0, 1000, 0);
                ::init_pair(Colors::SelectionCopy, c, -1);
                c++;
                
                ::init_color(c, 300, 300, 300);
                ::init_pair(Colors::SubtitleText, c, -1);
                c++;
            }
            
            // Hide cursor
            ::curs_set(0);
        }
        
        // Init libgit2
        {
            git_libgit2_init();
        }
        
//        volatile bool a = false;
//        while (!a);
        
        _RootWindow = Window(::stdscr);
        
        // Create a BranchColumn for each specified branch
        {
            constexpr int InsetX = 3;
            constexpr int ColumnWidth = 32;
            constexpr int ColumnSpacing = 6;
            Git::Repo repo = Git::RepoOpen(".");
            
            int OffsetX = InsetX;
            branches.insert(branches.begin(), "HEAD");
            for (const std::string& branch : branches) {
                std::string displayName = branch;
                if (branch == "HEAD") {
                    std::string currentBranchName = Git::CurrentBranchName(repo);
                    if (!currentBranchName.empty()) {
                        displayName = currentBranchName + " (HEAD)";
                    }
                }
                _BranchColumns.emplace_back(_RootWindow, repo, branch, displayName, OffsetX, ColumnWidth);
                OffsetX += ColumnWidth+ColumnSpacing;
            }
        }
        
        mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
        mouseinterval(0);
        for (int i=0;; i++) {
            _Draw();
            int key = _RootWindow.getChar();
            if (key == KEY_MOUSE) {
                MEVENT mouse = {};
                int ir = ::getmouse(&mouse);
                if (ir != OK) continue;
                if (mouse.bstate & BUTTON1_PRESSED) {
                    _TrackMouse(mouse);
                }
            
            } else if (key == KEY_RESIZE) {
                throw std::runtime_error("hello");
            }
        }
    
    } catch (const std::exception& e) {
        ::endwin();
        fprintf(stderr, "Error: %s\n", e.what());
    }
    
    return 0;
}
