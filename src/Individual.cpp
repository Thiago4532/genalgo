#include "Individual.hpp"

#include "globalRNG.hpp"
#include "globalConfig.hpp"
#include <algorithm>
#include <iostream>

GA_NAMESPACE_BEGIN

bool Individual::mutateAdd() {
    if (size() >= globalCfg.maxTriangles)
        return false;
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

    i32 id = randomI32(0, size());
    triangles.insert(begin() + id, t);
    return true;
}

bool Individual::mutateRemove() {
    if (triangles.size() <= 1)
        return false;

    i32 id = randomI32(0, size() - 1);
    triangles.erase(begin() + id);
    return true;
}

bool Individual::mutateSwap() {
    if (triangles.size() <= 1)
        return false;

    i32 i = randomI32(1, size() - 1);
    std::swap(triangles[i - 1], triangles[i]);
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

    Individual child;
    if (std::addressof(*this) == std::addressof(other)) {
        child = *this;
        child.bornType = BornType::None;
    } else {
        i32 szMin = size();
        i32 szMax = other.size();
        if (szMin > szMax)
            std::swap(szMin, szMax);

        i32 sz = randomI32(szMin - 1, szMax + 1);
        sz = std::clamp(sz, 1, globalCfg.maxTriangles); 
        child.reserve(sz);

        i32 sz_l = std::min(size(), (sz + 1) / 2);
        i32 sz_r = sz - sz_l;

        for (i32 i = 0; i < sz_l; i++)
            child.push_back((*this)[i]);

        for (i32 i = sz_r; i > 0; i--)
            child.push_back(other[other.size() - i]); 
    }

    if (child.mutate()) {
        if (child.bornType == BornType::None)
            child.bornType = BornType::Crossover;
        else
            child.bornType = BornType::CrossMutation;
    }
    return child;
}

GA_NAMESPACE_END
