#ifndef GENALGO_JSONDESERIALIZER_VECTOR_DESERIALIZER_HPP
#define GENALGO_JSONDESERIALIZER_VECTOR_DESERIALIZER_HPP

#include "JSONDeserializer.hpp"
#include <vector>

GA_NAMESPACE_BEGIN

template <JSONDeserializable T>
void deserialize(JSONDeserializerState& state, std::vector<T>& vec) {
    array_consumer_antigo arr = state.consume_array();

    while (true) {
        T value;
        if (!arr.try_consume_value(value))
            break;
        vec.push_back(std::move(value));
    } 
}

GA_NAMESPACE_END

#endif // GENALGO_JSONDESERIALIZER_VECTOR_DESERIALIZER_HPP
