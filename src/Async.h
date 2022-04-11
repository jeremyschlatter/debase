#pragma once
#include <sys/socket.h>
#include <optional>
#include "SignalDescriptor.h"

template <typename Fn>
class Async {
public:
    using RetType = typename std::invoke_result<Fn>::type;
    using RetTypeRef = std::add_lvalue_reference_t<RetType>;
    
    Async(Fn&& fn) {
        _state = std::make_shared<_State>();
        
        std::thread([=] {
            try {
                // Void return type
                if constexpr (_RetTypeEmpty) {
                    fn();
                    _state->val = _Empty();
                
                // Non-void return type
                } else {
                    _state->val = fn();
                }
            } catch (...) {
                _state->err = std::current_exception();
            }
            
            // Signal the fd
            _state->signal.signal();
        }).detach();
    }
    
    RetTypeRef get() {
        // Wait for the signal
        _state->signal.wait();
        
        // We must either have a value or an exception
        assert(_state->val || _state->err);
        if (_state->err) {
            std::rethrow_exception(_state->err);
        }
        
        if constexpr (!std::is_same_v<RetType, void>) {
            return *_state->val;
        }
    }
    
    const Toastbox::FileDescriptor& signal() {
        return _state->signal;
    }
    
    bool done() const {
        return _state->signal.signaled();
    }
    
private:
    struct _Empty {};
    static constexpr bool _RetTypeEmpty = std::is_same_v<RetType, void>;
    using _RetTypeOrEmpty = std::conditional_t<_RetTypeEmpty, _Empty, RetType>;
    
    struct _State {
        SignalDescriptor signal;
        std::exception_ptr err;
        std::optional<_RetTypeOrEmpty> val; // Optional so that a default constructor isn't required
    };
    using _StatePtr = std::shared_ptr<_State>;
    
    _StatePtr _state;
};
