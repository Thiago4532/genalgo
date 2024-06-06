#include "JSONDeserializer.hpp"

#include <iostream>
#include <type_traits>
#include <limits>

GA_NAMESPACE_BEGIN

[[noreturn]] static void throw_overflow() {
    throw json_deserialize_exception("Overflow while deserializing JSON number");
}

static bool is_whitespace(char c) {
    return c == ' '
        || c == '\n'
        || c == '\r'
        || c == '\t';
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static char getc(std::istream& is) {
    char c;
    if (!is.get(c))
        throw json_deserialize_exception("Unexpected end of input");
    return c;
}

void JSONDeserializerState::consume_whitespaces() {
    while (is_whitespace(is.peek()))
        is.get();
}

static u64 read_number(std::istream& is, u64 max) {
    u64 value = 0;
    while (is_digit(is.peek())) {
        char c = is.get();
        u64 digit = c - '0';
        if (10 * value + digit < value)
            throw_overflow();
        value = 10 * value + digit;
        if (value > max)
            throw_overflow();
    }
    return value;
}

template<std::integral T>
void JSONDeserializerState::consume_number(T& value) {
    consume_whitespaces();
    if (std::is_signed_v<T> && is.peek() == '-') {
        is.get();
        u64 MAX = static_cast<u64>(std::numeric_limits<T>::max()) + 1;
        value = -static_cast<T>(read_number(is, MAX));
        return;
    }
    if (!is_digit(is.peek()))
        throw json_deserialize_exception("Expected number while deserializing JSON number");

    value = static_cast<T>(read_number(is, std::numeric_limits<T>::max()));
}

template void JSONDeserializerState::consume_number<i8>(i8& value);
template void JSONDeserializerState::consume_number<u8>(u8& value);
template void JSONDeserializerState::consume_number<i16>(i16& value);
template void JSONDeserializerState::consume_number<u16>(u16& value);
template void JSONDeserializerState::consume_number<i32>(i32& value);
template void JSONDeserializerState::consume_number<u32>(u32& value);
template void JSONDeserializerState::consume_number<i64>(i64& value);
template void JSONDeserializerState::consume_number<u64>(u64& value);

void JSONDeserializerState::consume_string(std::string& str) {
    consume_whitespaces();
    if (is.peek() != '"')
        throw json_deserialize_exception("Expected '\"' while deserializing JSON string");
    is.get();

    str.clear();
    char c;
    while ((c = getc(is)) != '"') {
        // TODO: handle escape sequences
        str.push_back(c);
    }
}

JSONObjectConsumer JSONDeserializerState::consume_object() {
    consume_whitespaces();
    return JSONObjectConsumer(is);
}

JSONArrayConsumer JSONDeserializerState::consume_array() {
    consume_whitespaces();
    return JSONArrayConsumer(is);
}

JSONObjectConsumer::JSONObjectConsumer(std::istream& is) : is(is) {
    if (is.peek() != '{')
        throw json_deserialize_exception("Expected '{' while consuming object");
    is.get();
}

JSONObjectConsumer::~JSONObjectConsumer() {
    if (!end)
        std::cerr << "Warning: JSONObjectConsumer destroyed without consuming all values\n";
}

bool JSONObjectConsumer::consume_key(std::string& key) {
    if (end)
        return false;
    if (key_consumed)
        throw json_deserialize_exception("Object: Value must be consumed before consuming another key");

    JSONDeserializerState state(is);
    state.consume_whitespaces();

    char next = is.peek();

    if (next == '}') {
        is.get();
        end = true;
        return false;
    }

    if (separator) {
        if (next != ',')
            throw json_deserialize_exception("Object: Expected ',' while consuming key");
        is.get();
    } else {
        separator = true;
    }

    state.consume_string(key);
    if (is.peek() != ':')
        throw json_deserialize_exception("Object: Expected ':' after key");
    is.get();
    key_consumed = true;
    return true;
}

void JSONObjectConsumer::consume_end() {
    if (end)
        return;
    JSONDeserializerState state(is);
    state.consume_whitespaces();
    if (is.peek() != '}')
        throw json_deserialize_exception("Object: Expected '}' after consuming all values");
    is.get();
    end = true;
}

JSONArrayConsumer::JSONArrayConsumer(std::istream& is) : is(is) {
    if (is.peek() != '[')
        throw json_deserialize_exception("Expected '[' while consuming array");
    is.get();
}

JSONArrayConsumer::~JSONArrayConsumer() {
    if (!end)
        std::cerr << "Warning: JSONArrayConsumer destroyed without consuming all values\n";
}

bool JSONArrayConsumer::consume_separator_or_end() {
    if (end)
        return false;
    JSONDeserializerState state(is);
    state.consume_whitespaces();

    if (is.peek() == ']') {
        is.get();
        end = true;
        return false;
    }

    if (separator) {
        if (is.peek() != ',')
            throw json_deserialize_exception("Array: Expected ',' while consuming value");
        is.get();
    } else {
        separator = true;
    }

    return true;
}

void JSONArrayConsumer::consume_end() {
    if (end)
        return;
    JSONDeserializerState state(is);
    state.consume_whitespaces();
    if (is.peek() != ']')
        throw json_deserialize_exception("Array: Expected ']' after consuming all values");
    is.get();
    end = true;
}

GA_NAMESPACE_END
