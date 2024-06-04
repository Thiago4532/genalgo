#ifndef GENALGO_SIGNALHANDLER_HPP
#define GENALGO_SIGNALHANDLER_HPP

#include "base.hpp"
#include <atomic>

GA_NAMESPACE_BEGIN

class SignalHandler {
public:
    SignalHandler() = delete;
    SignalHandler(const SignalHandler&) = delete;
    SignalHandler& operator=(const SignalHandler&) = delete;

    static void setup();

    static bool interrupted() noexcept {
        return interrupted_.load(std::memory_order_relaxed);
    }

private:
    static std::atomic<bool> interrupted_;
};

GA_NAMESPACE_END

#endif // GENALGO_SIGNALHANDLER_HPP
