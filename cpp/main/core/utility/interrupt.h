#pragma once

#include <atomic>
#include <stdexcept>

namespace core::utility{
    /**
     * Cooperative cancellation of long running computations (set by the server
     * when the user presses "Stop", polled by propagation loops and the Lua hook).
     */
    inline std::atomic<bool> interrupt_flag{false};

    class interrupted_error: public std::runtime_error{
    public:
        interrupted_error(): std::runtime_error("Interrupted"){}
    };

    inline void check_interrupt(){
        if(interrupt_flag.load(std::memory_order_relaxed)) throw interrupted_error();
    }
}
