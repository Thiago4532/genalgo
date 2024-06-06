#ifndef GENALGO_JSONDESERIALIZER_HPP
#define GENALGO_JSONDESERIALIZER_HPP

#include "JSONDeserializer/fwd.hpp"
#include <iosfwd>
#include <stdexcept>

#if GA_HAS_CPP20
#include <concepts>

GA_NAMESPACE_BEGIN

class json_deserialize_exception : public std::runtime_error {
public:
    explicit json_deserialize_exception(const std::string& what) : std::runtime_error(what) {}
    explicit json_deserialize_exception(const char* what) : std::runtime_error(what) {}
};

template <typename T>
concept JSONDeserializable = requires(JSONDeserializerState& state, T& obj) {
    { deserialize(state, obj) } -> std::same_as<void>;
};

class JSONDeserializerState {
public:
    JSONDeserializerState(std::istream& is)
        : is(is) {}

    JSONDeserializerState(const JSONDeserializerState&) = delete;
    JSONDeserializerState& operator=(const JSONDeserializerState&) = delete;

    template <JSONDeserializable T>
    void consume(T& value) {
        deserialize(*this, value);
    }

    template<std::integral T>
    void consume_number(T& value);
    void consume_string(std::string& value);
    JSONObjectConsumer consume_object();
    JSONArrayConsumer consume_array();
    void consume_whitespaces();

private:
    std::istream& is;
};

class JSONObjectConsumer {
public:
    ~JSONObjectConsumer();
    JSONObjectConsumer(JSONObjectConsumer const&) = delete;
    JSONObjectConsumer& operator=(JSONObjectConsumer const&) = delete;

    bool consume_key(std::string& key);
    
    template <JSONDeserializable T>
    void consume_value(T& value) {
        if (!key_consumed)
            throw json_deserialize_exception("Object: Key must be consumed before consuming value");
        JSONDeserializerState(is).consume(value);
        key_consumed = false;
    }

    [[noreturn]] void throw_unexpected_key(std::string const& key) {
        throw json_deserialize_exception("Object: Unexpected key: " + key);
    }

    void consume_end();
private:
    friend JSONDeserializerState;
    JSONObjectConsumer(std::istream& is);

    std::istream& is;
    bool separator = false;
    bool key_consumed = false;
    bool end = false;
};

class JSONArrayConsumer {
public:
    ~JSONArrayConsumer();
    JSONArrayConsumer(JSONArrayConsumer const&) = delete;
    JSONArrayConsumer& operator=(JSONArrayConsumer const&) = delete;

    template <JSONDeserializable T>
    bool try_consume_value(T& value) {
        if (!consume_separator_or_end())
            return false;

        JSONDeserializerState(is).consume(value);
        return true;
    }

    template <JSONDeserializable T>
    void consume_value(T& value) {
        if (!try_consume_value(value))
            throw json_deserialize_exception("Array: Expected value");
    }

    void consume_end();

private:
    friend JSONDeserializerState;
    JSONArrayConsumer(std::istream& is);

    bool consume_separator_or_end();

    std::istream& is;
    bool separator = false;
    bool end = false;
};

template<std::integral T>
inline void deserialize(JSONDeserializerState& state, T& value) {
    state.consume_number(value);
}

inline void deserialize(JSONDeserializerState& state, std::string& value) {
    state.consume_string(value);
}

namespace json {

namespace detail {

struct deserialize_fn {
    template <JSONDeserializable T>
    void operator()(JSONDeserializerState& state, T& value) const {
        using genalgo::deserialize;
        deserialize(state, value);
    }

    template <JSONDeserializable T>
    void operator()(std::istream& os, T& value) const {
        JSONDeserializerState(os).consume(value);
    }
};

}

inline namespace __CPO { inline constexpr detail::deserialize_fn deserialize {}; }

}

GA_NAMESPACE_END

#endif // GA_HAS_CPP20

#endif // GENALGO_JSONDESERIALIZER_HPP
