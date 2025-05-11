#ifndef GENALGO_APPSTATE_HPP
#define GENALGO_APPSTATE_HPP

#include "base.hpp"
#include "Population.hpp"
#include "GlobalConfig.hpp"

GA_NAMESPACE_BEGIN

// A struct that holds the references to states that need to be saved/loaded.
struct AppState {
    Population& population;
    i64& generation;
    u32& seed;
    Point<i32> size;
    
    friend void serialize(JSONSerializerState& state, AppState const& self) {
        JSONObjectBuilder obj = state.serialize_object();
        obj.add("seed", self.seed);
        obj.add("generation", self.generation);
        obj.add("size", self.size);
        obj.add("population", self.population);
    }

    friend void deserialize(JSONDeserializerState& state, AppState& self) {
        // TODO: When the JSON API become mature enough, allow size to be a optional field
        // self.size = {globalCfg.targetImage.getWidth(), globalCfg.targetImage.getHeight()};

        state.consume_object(
            "seed", self.seed,
            "generation", self.generation,
            "size", self.size,
            "population", self.population
        );

        if (self.generation < 0)
            throw json_deserialize_exception("AppState: Generation must be non-negative");
    }
};

GA_NAMESPACE_END

#endif // GENALGO_APPSTATE_HPP
