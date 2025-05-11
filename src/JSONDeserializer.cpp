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
    if (is.eof() || is.peek() != '"') {
        throw json_deserialize_exception("Expected '\"' while deserializing JSON string");
    }
    is.get();

    str.clear();
    char c;
    while (true) {
        c = getc(is);
        if (c == '"') {
            break;
        }

        if (c == '\\') {
            c = getc(is);
            switch (c) {
                case '"':  str.push_back('"');  break;
                case '\\': str.push_back('\\'); break;
                case '/':  str.push_back('/');  break;
                case 'b':  str.push_back('\b'); break;
                case 'f':  str.push_back('\f'); break;
                case 'n':  str.push_back('\n'); break;
                case 'r':  str.push_back('\r'); break;
                case 't':  str.push_back('\t'); break;
                case 'u': {
                    std::string hex_digits;
                    hex_digits.reserve(4);
                    for (int i = 0; i < 4; ++i) {
                        char hex_char = getc(is);
                        if (!std::isxdigit(static_cast<unsigned char>(hex_char))) {
                            throw json_deserialize_exception("Invalid Unicode escape sequence: non-hex character '" + std::string(1, hex_char) + "'");
                        }
                        hex_digits.push_back(hex_char);
                    }
                    
                    unsigned long codepoint_ul;
                    try {
                        codepoint_ul = std::stoul(hex_digits, nullptr, 16);
                    } catch (const std::out_of_range& oor) {
                        throw json_deserialize_exception("Invalid Unicode escape sequence: value out of range for stoul");
                    } catch (const std::invalid_argument& ia) {
                        throw json_deserialize_exception("Invalid Unicode escape sequence: invalid argument for stoul");
                    }
                    
                    // Ensure codepoint fits in a standard range (e.g. for char32_t or similar)
                    // Here we cast to unsigned int as a common intermediate.
                    unsigned int codepoint = static_cast<unsigned int>(codepoint_ul);

                    // Convert codepoint to UTF-8 and append to str
                    // This simple conversion handles BMP characters (U+0000 to U+FFFF)
                    // It does not handle surrogate pairs for characters outside BMP.
                    if (codepoint <= 0x7F) { // 1-byte sequence (ASCII)
                        str.push_back(static_cast<char>(codepoint));
                    } else if (codepoint <= 0x7FF) { // 2-byte sequence
                        str.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
                        str.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                    } else if (codepoint <= 0xFFFF) { // 3-byte sequence
                        // Check for UTF-16 surrogate pairs, which are invalid if not part of a pair
                        if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
                            throw json_deserialize_exception("Invalid Unicode escape sequence: lone surrogate U+" + hex_digits);
                        }
                        str.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
                        str.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                        str.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                    } else {
                        // Codepoints > 0xFFFF would require 4-byte UTF-8 sequences and handling of
                        // JSON's \uXXXX\uYYYY surrogate pair mechanism if full Unicode support is needed.
                        // For this implementation, we'll consider this an error or unsupported.
                        throw json_deserialize_exception("Unicode codepoint U+" + hex_digits + " outside BMP or unhandled surrogate pair");
                    }
                    break;
                }
                default:
                    throw json_deserialize_exception("Invalid escape sequence: \\" + std::string(1, c));
            }
        } else {
            if (static_cast<unsigned char>(c) < 0x20) {
                 throw json_deserialize_exception("Unescaped control character in string: code " + std::to_string(static_cast<unsigned char>(c)));
            }
            str.push_back(c);
        }
    }
}

_JSONObjectConsumer JSONDeserializerState::_consume_object() {
    consume_whitespaces();
    return _JSONObjectConsumer(is);
}

JSONArrayConsumer JSONDeserializerState::consume_array() {
    consume_whitespaces();
    return JSONArrayConsumer(is);
}

_JSONObjectConsumer::_JSONObjectConsumer(std::istream& is) : is(is) {
    if (is.peek() != '{')
        throw json_deserialize_exception("Expected '{' while consuming object");
    is.get();
}

_JSONObjectConsumer::~_JSONObjectConsumer() {
    if (!end)
        std::cerr << "Warning: JSONObjectConsumer destroyed without consuming all values\n";
}

bool _JSONObjectConsumer::consume_key(std::string& key) {
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

void _JSONObjectConsumer::consume_end() {
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

void JSONDeserializerState::_discard_string() {
    char c = getc(is); // Consume opening '"'. Error if not '\"' handled by caller or initial check.
    // (Caller of _discard_string should ensure it was called because a '"' was peeked)

    bool escape = false;
    while (true) {
        c = getc(is); // Throws on EOF if string is unterminated
        if (escape) {
            escape = false; 
        } else if (c == '\\') {
            escape = true;
        } else if (c == '"') {
            break; // End of string
        }
    }
}

void JSONDeserializerState::_discard_number() {
    // Consume an optional sign
    if (!is.eof() && is.peek() == '-') {
        getc(is);
    }

    // Consume integer part (digits)
    if (!is.eof() && !is_digit(is.peek())) { // Must have at least one digit if no fraction/exponent
        // If it's just '-' it's not a valid number.
        // This check might be too strict if called after already confirming it's a number.
        // Assume caller confirmed it starts like a number.
    }
    while (!is.eof() && is_digit(is.peek())) {
        getc(is);
    }

    // Consume fractional part
    if (!is.eof() && is.peek() == '.') {
        getc(is); // consume '.'
        if (is.eof() || !is_digit(is.peek())) {
             throw json_deserialize_exception("Number has '.' but no digits after it during discard");
        }
        while (!is.eof() && is_digit(is.peek())) {
            getc(is);
        }
    }

    // Consume exponent part
    if (!is.eof() && (is.peek() == 'e' || is.peek() == 'E')) {
        getc(is); // consume 'e' or 'E'
        if (!is.eof() && (is.peek() == '+' || is.peek() == '-')) {
            getc(is); // consume sign
        }
        if (is.eof() || !is_digit(is.peek())) {
            throw json_deserialize_exception("Number has exponent 'e'/'E' but no digits after it (or after sign) during discard");
        }
        while (!is.eof() && is_digit(is.peek())) {
            getc(is);
        }
    }
}

void JSONDeserializerState::_discard_literal(const char* literal_name, std::size_t len) {
    for (std::size_t i = 0; i < len; ++i) {
        char c = getc(is);
        if (c != literal_name[i]) {
            throw json_deserialize_exception(std::string("Expected literal '") + literal_name + "' but found mismatch during discard");
        }
    }
}

void JSONDeserializerState::_discard_object() {
    char c = getc(is);
    consume_whitespaces();

    if (!is.eof() && is.peek() == '}') {
        getc(is);
        return;
    }

    while (true) {
        // Discard key (which is a string)
        consume_whitespaces();
        if (is.eof() || is.peek() != '"') throw json_deserialize_exception("Expected string key in object discard");
        _discard_string();
        consume_whitespaces();

        c = getc(is);
        if (c != ':') throw json_deserialize_exception("Expected ':' after key in object discard");

        discard_value();
        consume_whitespaces();

        if (is.eof()) throw json_deserialize_exception("Unexpected EOF in object discard");
        c = static_cast<char>(is.peek());
        if (c == '}') {
            getc(is);
            break;
        }
        if (c != ',') throw json_deserialize_exception("Expected ',' or '}' in object discard");
        getc(is);
    }
}

void JSONDeserializerState::_discard_array() {
    char c = getc(is); // Consume '[' (caller should have peeked this)
    consume_whitespaces();

    if (!is.eof() && is.peek() == ']') {
        getc(is); // Consume ']'
        return;
    }

    while (true) {
        discard_value();
        consume_whitespaces();

        if (is.eof()) throw json_deserialize_exception("Unexpected EOF in array discard");
        c = static_cast<char>(is.peek());
        if (c == ']') {
            getc(is); // Consume ']'
            break;
        }
        if (c != ',') throw json_deserialize_exception("Expected ',' or ']' in array discard");
        getc(is); // Consume ','
    }
}

void JSONDeserializerState::discard_value() {
    consume_whitespaces();
    if (is.eof()) {
        throw json_deserialize_exception("Unexpected EOF: trying to discard a value but stream is empty");
    }

    char next_char = static_cast<char>(is.peek());

    switch (next_char) {
        case '{':
            _discard_object();
            break;
        case '[':
            _discard_array();
            break;
        case '"':
            _discard_string();
            break;
        case 't': // true
            _discard_literal("true", 4);
            break;
        case 'f': // false
            _discard_literal("false", 5);
            break;
        case 'n': // null
            _discard_literal("null", 4);
            break;
        default:
            if (next_char == '-' || (next_char >= '0' && next_char <= '9')) {
                _discard_number();
            } else {
                throw json_deserialize_exception("Unexpected character '" + std::string(1, next_char) + "' while starting to discard value");
            }
            break;
    }
}

GA_NAMESPACE_END
