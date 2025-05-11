#include "JSONSerializer.hpp"

#include <iostream>
#include <vector>

GA_NAMESPACE_BEGIN

[[noreturn]] static void throw_multiple_returns() {
    throw json_serialize_exception("Multiple returns while serializing JSON");
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

void JSONSerializerState::begin_return() {
    if (has_value)
        throw_multiple_returns();
    has_value = true;
}

template<std::integral T>
void JSONSerializerState::serialize_number(T value) {
    begin_return();
    os << value;
}

template void JSONSerializerState::serialize_number<i8>(i8 value);
template void JSONSerializerState::serialize_number<u8>(u8 value);
template void JSONSerializerState::serialize_number<i16>(i16 value);
template void JSONSerializerState::serialize_number<u16>(u16 value);
template void JSONSerializerState::serialize_number<i32>(i32 value);
template void JSONSerializerState::serialize_number<u32>(u32 value);
template void JSONSerializerState::serialize_number<i64>(i64 value);
template void JSONSerializerState::serialize_number<u64>(u64 value);

void JSONSerializerState::serialize_string(std::string_view value) {
    begin_return();
    
    // TODO: Escape characters
    os << '"' << value << '"';
}

void JSONSerializerState::serialize_null() {
    if (has_value)
        throw_multiple_returns();
    has_value = true;

    os << "null";
}

JSONObjectBuilder JSONSerializerState::serialize_object() {
    if (has_value)
        throw_multiple_returns();
    has_value = true;

    return JSONObjectBuilder(os);
}

JSONArrayBuilder JSONSerializerState::serialize_array() {
    if (has_value)
        throw_multiple_returns();
    has_value = true;

    return JSONArrayBuilder(os);
}

GA_NAMESPACE_END
