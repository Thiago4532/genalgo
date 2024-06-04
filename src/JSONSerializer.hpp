#ifndef GENALGO_JSONSERIALIZER_HPP
#define GENALGO_JSONSERIALIZER_HPP

#include "JSONSerializer/fwd.hpp"
#include <iosfwd>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#if GA_HAS_CPP20
#include <concepts>

GA_NAMESPACE_BEGIN

class json_serialize_exception : public std::runtime_error {
public:
    explicit json_serialize_exception(const std::string& what) : std::runtime_error(what) {}
    explicit json_serialize_exception(const char* what) : std::runtime_error(what) {}
};

template <typename T>
concept JSONHasSerializeMember = requires(JSONSerializerState& state, const T& t) {
    { t.serialize(state) } -> std::same_as<void>;
};

template <typename T>
concept JSONCallSerializable = !JSONHasSerializeMember<T> &&
    requires(JSONSerializerState& state, const T& t) {
        { t(state) } -> std::same_as<void>;
    };

template <typename T>
concept JSONSerializable = requires(JSONSerializerState& state, const T& t) {
    { serialize(state, t) } -> std::same_as<void>;
};

class JSONSerializerState {
public:
    JSONSerializerState(std::ostream& os)
        : os(os) {}

    ~JSONSerializerState() {
        if (!has_value)
            return_null();
    }

    JSONSerializerState(const JSONSerializerState&) = delete;
    JSONSerializerState& operator=(const JSONSerializerState&) = delete;

    template <JSONSerializable T>
    void return_value(const T& value) {
        serialize(*this, value);
    }

    void return_i32(i32 value);
    void return_u32(u32 value);
    void return_i64(i64 value);
    void return_u64(u64 value);
    void return_string(std::string_view value);
    void return_null();

    JSONObjectBuilder return_object();
    JSONArrayBuilder return_array();
private:
    std::ostream& os;
    bool has_value = false;
};

class JSONObjectBuilder {
public:
    JSONObjectBuilder(const JSONObjectBuilder&) = delete;
    JSONObjectBuilder& operator=(const JSONObjectBuilder&) = delete;

    ~JSONObjectBuilder();

    template <JSONSerializable T>
    JSONObjectBuilder& add(std::string_view name, const T& value) {
        add_key(name);
        JSONSerializerState field_state(os);
        serialize(field_state, value);
        return *this;
    }

private:
    friend JSONSerializerState;
    JSONObjectBuilder(std::ostream& os);

    void add_key(std::string_view name);

    std::ostream& os;
    bool separator = false;
};

class JSONArrayBuilder {
public:
    JSONArrayBuilder(const JSONArrayBuilder&) = delete;
    JSONArrayBuilder& operator=(const JSONArrayBuilder&) = delete;

    ~JSONArrayBuilder();

    template <JSONSerializable T>
    JSONArrayBuilder& add(const T& value) {
        add_separator();
        JSONSerializerState element_state(os);
        serialize(element_state, value);
        return *this;
    }
private:
    friend JSONSerializerState;
    JSONArrayBuilder(std::ostream& os);

    void add_separator();

    std::ostream& os;
    bool separator = false;
};

template<typename T> requires std::same_as<T, i32>
inline void serialize(JSONSerializerState& state, T value) {
    state.return_i32(value);
}

template<typename T> requires std::same_as<T, u32>
inline void serialize(JSONSerializerState& state, T value) {
    state.return_u32(value);
}

template<typename T> requires std::same_as<T, i64>
inline void serialize(JSONSerializerState& state, T value) {
    state.return_i64(value);
}

template<typename T> requires std::same_as<T, u64>
inline void serialize(JSONSerializerState& state, T value) {
    state.return_u64(value);
}

inline void serialize(JSONSerializerState& state, std::string_view value) {
    state.return_string(value);
}

template <JSONHasSerializeMember T>
void serialize(JSONSerializerState& state, const T& value) {
    value.serialize(state);
}

template <JSONCallSerializable T>
void serialize(JSONSerializerState& state, const T& value) {
    value(state);
}

namespace json {

namespace detail {

struct serialize_fn {
    template <JSONSerializable T>
    void operator()(JSONSerializerState& state, const T& value) const {
        using genalgo::serialize;
        serialize(state, value);
    }

    template <JSONSerializable T>
    void operator()(std::ostream& os, const T& value) const {
        JSONSerializerState(os).return_value(value);
    }
};

}

inline namespace __CPO { inline constexpr detail::serialize_fn serialize {}; }

}

GA_NAMESPACE_END

#endif // GA_HAS_CPP20

#endif // GENALGO_JSONSERIALIZER_HPP
