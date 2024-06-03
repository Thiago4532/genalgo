#ifndef _DEFER_HPP
#define _DEFER_HPP

#include <utility>

namespace defer_internal {

template <typename F>
class deferrer {
public:
    deferrer(F const& f) : f_(f) {}
    deferrer(F&& f) : f_(std::move(f)) {}

    ~deferrer() { f_(); }
private:
    F f_;
};

} // namespace defer_internal

#define _DEFER_CONCAT_IMPL(x, y) x##y
#define _DEFER_CONCAT(x, y) _DEFER_CONCAT_IMPL(x, y)

#define defer \
    ::defer_internal::deferrer _DEFER_CONCAT(_defer__, __LINE__) = [&]()

#endif // _DEFER_HPP
