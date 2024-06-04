#include "SignalHandler.hpp"
#include <signal.h>

GA_NAMESPACE_BEGIN

std::atomic<bool> SignalHandler::interrupted_ = false;


void SignalHandler::setup() {
    auto handler = [](int) {
        interrupted_.store(true, std::memory_order_relaxed);
    };

#ifdef _WIN32
    signal(SIGINT, handler);
#else
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
#endif
}

GA_NAMESPACE_END
