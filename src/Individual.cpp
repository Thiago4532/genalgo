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
        return false;
    i32 width = globalCfg.targetImage.getWidth();
    i32 height = globalCfg.targetImage.getHeight();

    i32 id = randomI32(0, size());
    triangles.insert(begin() + id, randomTriangle());
    return true;
}

bool Individual::mutateRemove() {
    if (triangles.size() <= globalCfg.minTriangles)
        return false;

    i32 id = select(size(), [&](i32 i) {
        f64 prob = 1.0 / (std::sqrt(triangles[i].area()) * triangles[i].color.a);
        return std::make_pair(prob, i);
    });
    triangles.erase(begin() + id);
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

bool Individual::mutate() {
    bool mutated = false;
    if (randomF64(0, 1) < globalCfg.mutationChanceAdd)
        mutated |= mutateAdd();
    if (randomF64(0, 1) < globalCfg.mutationChanceRemove)
        mutated |= mutateRemove();
    // if (randomF64(0, 1) < globalCfg.mutationChanceReplace)
    //     mutated |= mutateReplace();
    if (randomF64(0, 1) < globalCfg.mutationChanceSwap)
        mutated |= mutateSwap();
    if (randomF64(0, 1) < globalCfg.mutationChanceMerge)
        mutated |= mutateMerge();

    if (randomF64(0, 1) < globalCfg.mutationChanceShapeOverall && !triangles.empty()) {
        i32 n = std::max(1, (i32)(size() * globalCfg.mutationShapePercentage));

        for (i32 k = 0; k < n && !triangles.empty(); ++k) {
            i32 idx = randomI32(0, size() - 1);
            mutated |= triangles[idx].mutate();
        }
}
    return mutated;
}

void serialize(JSONSerializerState& state, Individual const& self) {
    state.serialize(self.triangles);
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
