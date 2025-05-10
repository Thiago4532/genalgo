#ifndef GENALGO_JSONDESERIALIZER_HPP
#define GENALGO_JSONDESERIALIZER_HPP

#include "JSONDeserializer/fwd.hpp"
#include <iosfwd>
#include <stdexcept>
#include <array>
#include <functional>

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

    void discard_value();

    // template <JSONDeserializable T, typename... Args> requires (sizeof...(Args)%2 == 0)
    // void consume_object(const char* key, T& value, Args&&... args) {
    //     object_consumer_antigo obj = consume_object();
    //     obj.consume_value(value);
    //     obj.consume_end();
    // }

    template <typename Fn>
    void consume_object_with(Fn&& fn, bool strict = true);

    template <typename... Args>
    void consume_object_from_pairs(std::pair<std::string_view, Args&> const&... pairs);

    inline void consume_object() {
        return consume_object_from_pairs();
    }

    template <typename... Args> requires (sizeof...(Args)%2 == 0)
    void consume_object(Args&&... args) {
        constexpr std::size_t num_pairs = sizeof...(Args) / 2;

        static constexpr auto make_pair = []<typename T>(std::string_view key, T&& value) {
            return std::pair<std::string_view, std::remove_reference_t<T>&>(key, std::forward<T>(value));
        };

        if constexpr (num_pairs == 0) {
            return consume_object_from_pairs();
        } else {
            auto args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);

            return [&]<std::size_t... I>(std::index_sequence<I...>) {
                return consume_object_from_pairs(
                        make_pair(
                            std::get<2 * I>(args_tuple),
                            std::get<2 * I + 1>(args_tuple)
                            )...
                        );
            }(std::make_index_sequence<num_pairs>{});
        }
    }

    array_consumer_antigo consume_array();
    void consume_whitespaces();

    static void my_main();
private:
    _JSONObjectConsumer _consume_object();
    std::istream& is;

    template<JSONDeserializable... Args>
    class ObjectFieldConsumer {
        public:
        static constexpr std::size_t N = sizeof...(Args);

        static ObjectFieldConsumer from_pairs(JSONDeserializerState& state, std::pair<std::string_view, Args&> const&... pairs) {
            return ObjectFieldConsumer (
                state,
                { std::pair<std::string_view, bool>{pairs.first, false}... },
                std::tuple<Args&...>(pairs.second...)
            );
        }

        bool consume(std::string_view key) {
            bool hit = false;
            [&]<std::size_t... I>(std::index_sequence<I...>) {
                ((hit || (keys[I].first != key) || (consume<I>(), hit = true)), ...);
            }(std::make_index_sequence<N>{});
            return hit;
        }

        bool all_fields_consumed() const {
            return num_found == N;
        }
    private:
        template<std::size_t I> requires (I < N)
        void consume() {
            // TODO: We can create a strict mode
            if (!keys[I].second) {
                keys[I].second = true;
                ++num_found;
            }
            state.consume(std::get<I>(values));
        }

        ObjectFieldConsumer(JSONDeserializerState& state, std::array<std::pair<std::string_view, bool>, N> keys, std::tuple<Args&...> values)
            : state(state), keys(keys), values(values) {}
        
        std::size_t num_found = 0;

        JSONDeserializerState& state;
        std::array<std::pair<std::string_view, bool>, N> keys;
        std::tuple<Args&...> values; 
    };

    template<typename... Args>
    static ObjectFieldConsumer<Args...> make_field_consumer_from_pairs(JSONDeserializerState& state, std::pair<std::string_view, Args&> const&... pairs) {
        return ObjectFieldConsumer<Args...>::from_pairs(state, pairs...);
    }

    void _discard_object(); 
    void _discard_array();
    void _discard_string();
    void _discard_number();
    void _discard_literal(const char* literal_name, std::size_t len);
};

class _JSONObjectConsumer {
public:
    ~_JSONObjectConsumer();
    _JSONObjectConsumer(_JSONObjectConsumer const&) = delete;
    _JSONObjectConsumer& operator=(_JSONObjectConsumer const&) = delete;

    bool consume_key(std::string& key);
    
    template <JSONDeserializable T>
    void consume_value(T& value) {
        if (!key_consumed)
            throw json_deserialize_exception("Object: Key must be consumed before consuming value");
        JSONDeserializerState(is).consume(value);
        key_consumed = false;
    }

    void discard_value() {
        if (!key_consumed)
            throw json_deserialize_exception("Object: Key must be consumed before discarding value");
        JSONDeserializerState(is).discard_value();
        key_consumed = false;
    }

    [[noreturn]] void throw_unexpected_key(std::string const& key) {
        throw json_deserialize_exception("Object: Unexpected key: " + key);
    }

    void consume_end();
private:
    friend JSONDeserializerState;
    _JSONObjectConsumer(std::istream& is);

    std::istream& is;
    bool separator = false;
    bool key_consumed = false;
    bool end = false;
};

template<typename Fn>
inline void JSONDeserializerState::consume_object_with(Fn&& fn, bool strict) {
    auto obj = _consume_object();
    std::string key;
    auto consume_value = [&](JSONDeserializable auto& value) {
        obj.consume_value(value);
    };

    while (obj.consume_key(key)) {
        if (!std::invoke(fn, consume_value, key)) {
            if (strict) {
                obj.throw_unexpected_key(key);
            } else {
                obj.discard_value();
            }
        }
    }
    obj.consume_end();
}

template <typename... Args>
void JSONDeserializerState::consume_object_from_pairs(std::pair<std::string_view, Args&> const&... pairs) {
    static constexpr bool strict = true;

    auto obj = _consume_object();
    auto consumer = make_field_consumer_from_pairs(*this, pairs...);
    std::string key;
    while (obj.consume_key(key)) {
        if (!consumer.consume(key)) {
            if (strict) {
                obj.throw_unexpected_key(key);
            } else {
                obj.discard_value();
            }
        } else {
            obj.key_consumed = false;
        }
    }
    obj.consume_end();

    if (!consumer.all_fields_consumed()) {
        throw json_deserialize_exception("Object: Missing required fields");
    }
}

// template<typename... Args> requires (sizeof...(Args)%2 == 0 && sizeof...(Args) > 0)
// inline void JSONDeserializerState::consume_object(Args&&... args) {
//     constexpr std::size_t num_pairs = sizeof...(Args) / 2;

//     static constexpr auto make_pair = []<typename T>(std::string_view key, T& value) {
//         return std::pair<std::string_view, std::remove_reference_t<T>&>(key, std::forward<T>(value));
//     };

//     auto obj = _consume_object();

//     auto args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);
//     return [&]<std::size_t... I>(std::index_sequence<I...>) {
//         return make_container(
//                 make_pair(
//                     std::get<2 * I>(args_tuple),
//                     std::get<2 * I + 1>(args_tuple)
//                     )...
//                 );
//     }(std::make_index_sequence<num_pairs>{});

//     obj.consume_end();
// }

class array_consumer_antigo {
public:
    ~array_consumer_antigo();
    array_consumer_antigo(array_consumer_antigo const&) = delete;
    array_consumer_antigo& operator=(array_consumer_antigo const&) = delete;

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
    array_consumer_antigo(std::istream& is);

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
