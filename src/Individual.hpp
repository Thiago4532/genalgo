#ifndef GENALGO_INDIVIDUAL_HPP
#define GENALGO_INDIVIDUAL_HPP

#include "base.hpp"
#include "Triangle.hpp"
#include <vector>

GA_NAMESPACE_BEGIN

class Individual {
public:
    Individual() noexcept = default;
    Individual(Individual const& other) noexcept = default;
    Individual(Individual&& other) noexcept = default;

    Individual& operator=(Individual const& other) noexcept = default;
    Individual& operator=(Individual&& other) noexcept = default;

    bool mutateAdd();
    bool mutateRemove();
    bool mutateReplace();
    bool mutateSwap();
    bool mutateMerge();
    bool mutateSplit();
    bool mutateShape();

    bool mutate();
    Individual crossover(Individual const& other) const;

    f64 getFitness() const noexcept { return fitness; }
    f64 getWeightedFitness() const noexcept { return weightedFitness; }
    void setFitness(f64 fitness) noexcept { this->fitness = fitness; }
    void setWeightedFitness(f64 weightedFitness) noexcept { this->weightedFitness = weightedFitness; }

    auto begin() noexcept { return triangles.begin(); }
    auto end() noexcept { return triangles.end(); }

    auto begin() const noexcept { return triangles.begin(); }
    auto end() const noexcept { return triangles.end(); }

    Triangle& operator[](std::size_t index) noexcept { return triangles[index]; }
    Triangle const& operator[](std::size_t index) const noexcept { return triangles[index]; }

    i32 size() const noexcept { return static_cast<i32>(triangles.size()); }
    void resize(i32 size) { triangles.resize(size); }
    void reserve(i32 size) { triangles.reserve(size); }
    void clear() noexcept { triangles.clear(); }

    void push_back(Triangle const& triangle) { triangles.push_back(triangle); }
    void push_back(Triangle&& triangle) { triangles.push_back(triangle); } 

    friend void serialize(JSONSerializerState& state, Individual const& self);
    friend void deserialize(JSONDeserializerState& state, Individual& self);

    void toSVG(std::ostream& os) const;
private:
    std::vector<Triangle> triangles;
    f64 fitness = 1e18;
    f64 weightedFitness = 1e18;
};

GA_NAMESPACE_END

#endif // GENALGO_INDIVIDUAL_HPP
