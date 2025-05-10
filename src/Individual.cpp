#include "Individual.hpp"

#include "globalRNG.hpp"
#include "GlobalConfig.hpp"
#include <algorithm>
#include "JSONSerializer/vector_serializer.hpp"
#include "JSONDeserializer/vector_deserializer.hpp"
#include <iomanip>
#include <ostream>

GA_NAMESPACE_BEGIN

static Triangle randomTriangle() {
    i32 width = globalCfg.targetImage.getWidth();
    i32 height = globalCfg.targetImage.getHeight();

    std::uniform_int_distribution<i32> xDist(0, width - 1);
    std::uniform_int_distribution<i32> yDist(0, height - 1);
    std::uniform_int_distribution<u8> colorDist(0, 255);
    std::uniform_int_distribution<u8> alphaDist(50, 255);

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
    t.color.a = alphaDist(globalRNG);

    return t;
}

template<typename F>
static i32 select(i32 n, F&& f) {
    std::vector<std::pair<double, i32>> probs;
    probs.reserve(n);

    f64 total = 0;
    for (i32 i = 0; i < n; i++) {
        std::pair<double, i32> p = f(i);

        probs.emplace_back(p);
        total += p.first;
    }

    f64 prob = randomF64(0, total);
    i32 i = 0;
    while (prob > probs[i].first) {
        prob -= probs[i].first;
        i++;
    }

    return probs[i].second;
}

bool Individual::mutateAdd() {
    if (size() >= globalCfg.maxTriangles)
        return mutateReplace();
    i32 width = globalCfg.targetImage.getWidth();
    i32 height = globalCfg.targetImage.getHeight();

    // i32 id = randomI32(0, size());
    // triangles.insert(begin() + id, randomTriangle());
    triangles.push_back(randomTriangle());
    return true;
}

bool Individual::mutateRemove() {
    if (triangles.size() <= 1)
        return mutateReplace();

    i32 id = select(size(), [&](i32 i) {
        f64 prob = 1.0 / (std::sqrt(triangles[i].area()) * triangles[i].color.a);
        return std::make_pair(prob, i);
    });
    triangles.erase(begin() + id);
    return true;
}

bool Individual::mutateReplace() {
    if (triangles.empty())
        return false;

    i32 i = select(triangles.size(), [&](i32 i) {
        f64 prob = 1.0 / (std::sqrt(triangles[i].area()) * triangles[i].color.a);

        return std::make_pair(prob, i);
    });
    

    triangles[i] = randomTriangle();
    // i32 j = randomI32(0, size() - 1);
    // triangles.erase(begin() + i);
    // triangles.insert(begin() + j, randomTriangle());
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

bool Individual::mutateMerge() {
    if (triangles.size() <= 1)
        return false;

    i32 i = randomI32(0, size() - 1);
    i32 j = select(size() - 1,
        [&](i32 j) {
            if (j >= i) ++j;

            f64 prob = 1.0 / std::sqrt(triangles[i].squareDistance(triangles[j]));
            return std::make_pair(prob, j);
        }
    );

    triangles[i].merge(triangles[j]);
    triangles.erase(begin() + j);
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

    if (prob < globalCfg.mutationChanceMerge)
        return mutateMerge();
    
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

        for (i32 i = 0; i < sz_l; i++)
            child.push_back((*this)[i]);

        for (i32 i = sz_r; i > 0; i--)
            child.push_back(other[other.size() - i]); 
    }

    child.mutate();
    while (dist(globalRNG) < 0.5)
        child.mutate();

    return child;
}

void serialize(JSONSerializerState& state, Individual const& self) {
    state.return_value(self.triangles);
}

void deserialize(JSONDeserializerState& state, Individual& self) {
    state.consume(self.triangles);
}

void Individual::toSVG(std::ostream& os) const {
    os << std::fixed << std::setprecision(4);

    i32 width = globalCfg.targetImage.getWidth();
    i32 height = globalCfg.targetImage.getHeight();
    i32 scaledWidth = width * globalCfg.svgScale;
    i32 scaledHeight = height * globalCfg.svgScale;

    os << "<svg xmlns=\"http://www.w3.org/2000/svg\" " <<
          "style=\"background-color: #000;\" " <<
          "width=\"" << scaledWidth << "\" height=\"" << scaledHeight << "\" " <<
          "viewBox=\"0 0 " << width << " " << height << "\">\n";

    for (Triangle const& t : triangles) {
        i32 r = t.color.r;
        i32 g = t.color.g;
        i32 b = t.color.b;
        f64 alpha = t.color.a / 255.0;
        os << "  <polygon points=\"";
        os << t.a.x << "," << t.a.y << " ";
        os << t.b.x << "," << t.b.y << " ";
        os << t.c.x << "," << t.c.y << "\" ";
        os << "fill=\"rgba(" << r << "," << g << "," << b << "," << alpha << ")\" ";
        os << "/>\n";
    }
    os << "</svg>";
}

GA_NAMESPACE_END
