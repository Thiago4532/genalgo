#ifndef GENALGO_JSONSERIALIZER_VECTOR_SERIALIZER_HPP
#define GENALGO_JSONSERIALIZER_VECTOR_SERIALIZER_HPP

#include "JSONSerializer.hpp"
#include <vector>

GA_NAMESPACE_BEGIN

template <JSONSerializable T>
void serialize(JSONSerializerState& state, const std::vector<T>& vec) {
    JSONArrayBuilder a = state.return_array();
    for (const T& t : vec) {
        a.add(t);
    }
}

GA_NAMESPACE_END

#endif // GENALGO_JSONSERIALIZER_VECTOR_SERIALIZER_HPP
