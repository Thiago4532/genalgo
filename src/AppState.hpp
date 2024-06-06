#ifndef GENALGO_APPSTATE_HPP
#define GENALGO_APPSTATE_HPP

#include "base.hpp"
#include "Population.hpp"
#include "globalConfig.hpp"

GA_NAMESPACE_BEGIN

// A struct that holds the references to states that need to be saved/loaded.
struct AppState {
    Population& population;
    i64& generation;
    u32& seed;
    
    friend void serialize(JSONSerializerState& state, AppState const& self) {
        JSONObjectBuilder obj = state.return_object();
        obj.add("seed", self.seed);
        obj.add("generation", self.generation);
        obj.add("population", self.population);
    }

    friend void deserialize(JSONDeserializerState& state, AppState& self) {
        auto obj = state.consume_object();
        std::string key;

        bool mark[3] = {0};
        while (obj.consume_key(key)) {
            if (key == "seed" && !mark[0]) {
                obj.consume_value(self.seed);
                mark[0] = true;
            } else if (key == "generation" && !mark[1]) {
                obj.consume_value(self.generation);
                mark[1] = true;
            } else if (key == "population" && !mark[2]) {
                obj.consume_value(self.population);
                mark[2] = true;
            } else {
                obj.throw_unexpected_key(key);
            }
        }

        if (self.generation < 0)
            throw json_deserialize_exception("AppState: Generation must be non-negative");

        for (bool m : mark) {
            if (!m)
                throw json_deserialize_exception("AppState: Missing required fields");
        }
    }
};

GA_NAMESPACE_END

#endif // GENALGO_APPSTATE_HPP
