#pragma once
#include <optional>
#include <ctype.h>
#include "Panel.h"
#include "Color.h"
#include "ModalPanel.h"
#include "TextField.h"
#include "LabelTextField.h"
#include "LinkButton.h"

namespace UI {

class RegisterPanel : public ModalPanel {
public:
    RegisterPanel() {
        message()->centerSingleLine(false);
        
        _purchaseMessage->text("To purchase a license, please visit:");
        _purchaseMessage->wrap(true);
        
        _purchaseLink->label()->text(ToasterDisplayURL);
        _purchaseLink->link(DebasePurchaseURL);
        _purchaseLink->enabled(true);
        
        auto valueChanged = [&] (TextField& field) { _fieldValueChanged(field); };
        auto requestFocus = [&] (TextField& field) { _fieldRequestFocus(field); };
        auto releaseFocus = [&] (TextField& field, bool done) { _fieldReleaseFocus(field, done); };
        
        _email->label()->text               ("       Email: ");
        _email->label()->textAttr           (colors().menu|A_BOLD);
        _email->spacingX                    (3);
        _email->textField()->valueChanged   (valueChanged);
        _email->textField()->requestFocus   (requestFocus);
        _email->textField()->releaseFocus   (releaseFocus);
        _email->textField()->focus          (true);
        
        _code->label()->text                ("License Code: ");
        _code->label()->textAttr            (colors().menu|A_BOLD);
        _code->spacingX                     (3);
        _code->textField()->valueChanged    (valueChanged);
        _code->textField()->requestFocus    (requestFocus);
        _code->textField()->releaseFocus    (releaseFocus);
        
        _okButton->label()->text            ("OK");
        _okButton->drawBorder               (true);
        
        _cancelButton->label()->text        ("Cancel");
        _cancelButton->label()->textAttr    (A_NORMAL);
        _cancelButton->drawBorder           (true);
        _cancelButton->action               (std::bind(&RegisterPanel::_actionCancel, this));
        _cancelButton->enabled              (true);
    }
    
    // MARK: - ModalPanel Overrides
    int contentHeight(int width) const override {
        const int purchaseMessageHeight = _purchaseMessage->sizeIntrinsic({width, ConstraintNone}).y;
        return
            purchaseMessageHeight + _ElementSpacingY +
            _PurchaseLinkHeight + _ElementSpacingY + _ElementSpacingY +
            _FieldHeight + _ElementSpacingY +
            _FieldHeight + _ElementSpacingY +
            _ButtonHeight;
    }
    
//    LabelTextFieldPtr focus() const {
//        if (_email->textField()->focus()) return _email;
//        else if (_code->textField()->focus()) return _code;
//        return nullptr;
//    }
//    
//    bool focus(LabelTextFieldPtr field) {
//        _email->textField()->focus(false);
//        _code->textField()->focus(false);
//        if (field) field->textField()->focus(true);
//        return true;
//    }
//    
    // MARK: - ModalPanel Overrides
//    bool suppressEvents(bool x) override {
//        if (!ModalPanel::suppressEvents(x)) return false;
//        if (ModalPanel::suppressEvents()) {
//            // Save focused field
//            _focusPrev = focus();
//            focus(nullptr);
//        } else {
//            // Restore focused field
//            focus(_focusPrev);
//            _focusPrev = nullptr;
//        }
//        return true;
//    }
    
    // MARK: - View Overrides
    
//    void draw() override {
//        ModalPanel::draw();
//        drawRect(contentFrame());
//    }
    
    void layout() override {
        ModalPanel::layout();
        
        const Rect cf = contentFrame();
        int offY = cf.t();
        
        _purchaseMessage->sizeToFit({cf.w(), ConstraintNone});
        _purchaseMessage->origin({cf.origin.x, offY});
        offY += _purchaseMessage->frame().h() + _ElementSpacingY;
        
        _purchaseLink->frame({{cf.origin.x, offY}, {cf.w(), _PurchaseLinkHeight}});
        offY += _PurchaseLinkHeight + _ElementSpacingY + _ElementSpacingY;
        
        _email->frame({{cf.origin.x, offY}, {cf.size.x, _FieldHeight}});
        offY += _FieldHeight+_ElementSpacingY;
        _code->frame({{cf.origin.x, offY}, {cf.size.x, _FieldHeight}});
        offY += _FieldHeight+_ElementSpacingY;
        
        _okButton->frame({{cf.r()-_ButtonWidth,offY}, {_ButtonWidth, _ButtonHeight}});
        _cancelButton->frame({{_okButton->frame().l()-_ButtonSpacingX-_ButtonWidth,offY}, {_ButtonWidth, _ButtonHeight}});
        
        _cancelButton->visible((bool)dismissAction());
    }
    
    bool handleEvent(const Event& ev) override {
        // Dismiss upon mouse-up
        if (ev.type == Event::Type::KeyEscape) {
            _actionCancel();
        }
        return true;
    }
    
    auto& email() { return _email->textField(); }
    auto& code() { return _code->textField(); }
    auto& okButton() { return _okButton; }
    auto& cancelButton() { return _cancelButton; }
    
private:
    static constexpr int _ElementSpacingY       = 1;
    static constexpr int _PurchaseLinkHeight    = 1;
    static constexpr int _FieldHeight           = 1;
    static constexpr int _ButtonWidth           = 10;
    static constexpr int _ButtonHeight          = 3;
    static constexpr int _ButtonSpacingX        = 1;
    
    void _fieldValueChanged(TextField& field) {
        const bool fieldsValue =
            !_code->textField()->value().empty()    &&
            !_email->textField()->value().empty()   ;
        
        _okButton->enabled(fieldsValue);
    }
    
    void _fieldRequestFocus(TextField& field) {
        _email->textField()->focus(false);
        _code->textField()->focus(false);
        field.focus(true);
    }
    
    void _fieldReleaseFocus(TextField& field, bool done) {
        // Return behavior
        if (done) {
            const bool fieldsFilled = !_email->textField()->value().empty() && !_code->textField()->value().empty();
            if (fieldsFilled) {
                if (_okButton->action()) {
                    _okButton->action()(*_okButton);
                }
            
            } else {
                _email->textField()->focus(false);
                _code->textField()->focus(false);
                
                if (field.value().empty()) {
                    field.focus(true);
                
                } else {
                    if (_email->textField()->value().empty())     _email->textField()->focus(true);
                    else if (_code->textField()->value().empty()) _code->textField()->focus(true);
                }
            }
        
        // Tab behavior
        } else {
            _email->textField()->focus(false);
            _code->textField()->focus(false);
            
            if (&field == _email->textField().get()) {
                _code->textField()->focus(true);
            } else if (&field == _code->textField().get()) {
                _email->textField()->focus(true);
            }
        }
    }
    
    void _actionOK() {
        if (dismissAction()) {
            dismissAction()(*this);
        }
    }
    
    void _actionCancel() {
        if (dismissAction()) {
            dismissAction()(*this);
        }
    }
    
    LabelPtr _purchaseMessage   = subviewCreate<Label>();
    LinkButtonPtr _purchaseLink = subviewCreate<LinkButton>();
    
    LabelTextFieldPtr _email    = subviewCreate<LabelTextField>();
    LabelTextFieldPtr _code     = subviewCreate<LabelTextField>();
    ButtonPtr _okButton         = subviewCreate<Button>();
    ButtonPtr _cancelButton     = subviewCreate<Button>();
    
    LabelTextFieldPtr _focusPrev;
};

using RegisterPanelPtr = std::shared_ptr<RegisterPanel>;

} // namespace UI
