#include "Individual.hpp"

#include "globalRNG.hpp"
#include "globalConfig.hpp"
#include <algorithm>
#include "JSONSerializer/vector_serializer.hpp"
#include "JSONDeserializer/vector_deserializer.hpp"

GA_NAMESPACE_BEGIN

static Triangle randomTriangle() {
    i32 width = globalCfg.targetImage.getWidth();
    i32 height = globalCfg.targetImage.getHeight();

    std::uniform_int_distribution<i32> xDist(0, width - 1);
    std::uniform_int_distribution<i32> yDist(0, height - 1);
    std::uniform_int_distribution<u8> colorDist;

    Triangle t;
    t.a.x = xDist(globalRNG);
    t.a.y = yDist(globalRNG);
    t.b.x = xDist(globalRNG);
    t.b.y = yDist(globalRNG);
    t.c.x = xDist(globalRNG);
    t.c.y = yDist(globalRNG);
    t.color.r = colorDist(globalRNG);
    t.color.g = colorDist(globalRNG);
    t.color.b = colorDist(globalRNG);
    t.color.a = colorDist(globalRNG);

    return t;
}

bool Individual::mutateAdd() {
    if (size() >= globalCfg.maxTriangles)
        return mutateReplace();
    i32 width = globalCfg.targetImage.getWidth();
    i32 height = globalCfg.targetImage.getHeight();

    i32 id = randomI32(0, size());
    triangles.insert(begin() + id, randomTriangle());
    return true;
}

bool Individual::mutateRemove() {
    if (triangles.size() <= 1)
        return mutateReplace();

    i32 id = randomI32(0, size() - 1);
    triangles.erase(begin() + id);
    return true;
}

bool Individual::mutateReplace() {
    if (triangles.empty())
        return false;

    i32 i = randomI32(0, size() - 1);
    i32 j = randomI32(0, size() - 1);
    triangles.erase(begin() + i);
    triangles.insert(begin() + j, randomTriangle());
    return true;
}

bool Individual::mutateSwap() {
    if (triangles.size() <= 1)
        return false;

    i32 i = randomI32(0, size() - 1);
    i32 j = randomI32(0, size() - 2);
    if (j >= i)
        j++;
    using std::swap;
    swap(triangles[i], triangles[j]);
    return true;
}

bool Individual::mutateShape() {
    if (triangles.empty())
        return false;
    
    i32 i = randomI32(0, size() - 1);
    return triangles[i].mutate();
}

bool Individual::mutate() {
    f64 prob = randomF64(0, 1);

    if (prob < globalCfg.mutationChanceAdd)
        return mutateAdd();
    prob -= globalCfg.mutationChanceAdd;
    
    if (prob < globalCfg.mutationChanceRemove)
        return mutateRemove();
    prob -= globalCfg.mutationChanceRemove;

    if (prob < globalCfg.mutationChanceReplace)
        return mutateReplace();
    prob -= globalCfg.mutationChanceReplace;
    
    if (prob < globalCfg.mutationChanceSwap)
        return mutateSwap();
    prob -= globalCfg.mutationChanceSwap;
    
    if (prob < globalCfg.mutationChanceShape)
        return mutateShape();
    prob -= globalCfg.mutationChanceShape;

    return false;
}

Individual Individual::crossover(Individual const& other) const {
    static std::uniform_real_distribution<f64> dist(0, 1);

    i32 imWidth = globalCfg.targetImage.getWidth();
    i32 imHeight = globalCfg.targetImage.getHeight();

    Individual child;
    if (std::addressof(*this) == std::addressof(other)) {
        child = *this;
    } else {
        i32 szMin = size();
        i32 szMax = other.size();
        if (szMin > szMax)
            std::swap(szMin, szMax);

        i32 sz = randomI32(szMin - 1, szMax + 1);
        sz = std::clamp(sz, 1, szMin + szMax);
        sz = std::min(sz, globalCfg.maxTriangles);
        child.reserve(sz);

        i32 sz_l = std::min(size(), (sz + 1) / 2);
        i32 sz_r = sz - sz_l;

        // for (i32 i = 0; i < sz_l; i++)
        //     child.push_back((*this)[i]);

        // for (i32 i = sz_r; i > 0; i--)
        //     child.push_back(other[other.size() - i]); 

        for (Triangle const& t : *this) {
            i32 minX = std::min({t.a.x, t.b.x, t.c.x});
            if (minX <= imWidth / 2) {
                child.push_back(t);
            }
        }

        for (Triangle const& t : other) {
            i32 minX = std::min({t.a.x, t.b.x, t.c.x});
            if (minX > imWidth / 2) {
                child.push_back(t);
            }
        }
    }

    child.mutate();
    return child;
}

void serialize(JSONSerializerState& state, Individual const& self) {
    state.return_value(self.triangles);
}

void deserialize(JSONDeserializerState& state, Individual& self) {
    state.consume(self.triangles);
}

GA_NAMESPACE_END
