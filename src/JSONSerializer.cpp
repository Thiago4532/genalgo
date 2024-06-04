#include "JSONSerializer.hpp"

#include <iostream>
#include <vector>

GA_NAMESPACE_BEGIN

[[noreturn]] static void throw_multiple_returns() {
    throw json_serialize_exception("Return value already set");
}

JSONObjectBuilder::JSONObjectBuilder(std::ostream& os) : os(os) {
    os << '{';
}

JSONObjectBuilder::~JSONObjectBuilder() {
    os << '}';
}

void JSONObjectBuilder::add_key(std::string_view name) {
    if (separator) {
        os << ',';
    } else {
        separator = true;
    }

    os << '"' << name << "\":";
}

JSONArrayBuilder::JSONArrayBuilder(std::ostream& os) : os(os) {
    os << '[';
}

JSONArrayBuilder::~JSONArrayBuilder() {
    os << ']';
}

void JSONArrayBuilder::add_separator() {
    if (separator) {
        os << ',';
    } else {
        separator = true;
    }
}

void JSONSerializerState::return_i32(i32 value) {
    if (has_value)
        throw_multiple_returns();
    has_value = true;

    os << value;
}

void JSONSerializerState::return_u32(u32 value) {
    if (has_value)
        throw_multiple_returns();
    has_value = true;

    os << value;
}

void JSONSerializerState::return_i64(i64 value) {
    if (has_value)
        throw_multiple_returns();
    has_value = true;

    os << value;
}

void JSONSerializerState::return_u64(u64 value) {
    if (has_value)
        throw_multiple_returns();
    has_value = true;

    os << value;
}

void JSONSerializerState::return_string(std::string_view value) {
    if (has_value)
        throw_multiple_returns();
    has_value = true;
    
    // TODO: Escape characters
    os << '"' << value << '"';
}

void JSONSerializerState::return_null() {
    if (has_value)
        throw_multiple_returns();
    has_value = true;

    os << "null";
}

JSONObjectBuilder JSONSerializerState::return_object() {
    if (has_value)
        throw_multiple_returns();
    has_value = true;

    return JSONObjectBuilder(os);
}

JSONArrayBuilder JSONSerializerState::return_array() {
    if (has_value)
        throw_multiple_returns();
    has_value = true;

    return JSONArrayBuilder(os);
}

GA_NAMESPACE_END
