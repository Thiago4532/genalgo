#include "GlobalConfig.hpp"
#include <cstdio>
#include <cstring>
#include <limits>
#include <random>

GA_NAMESPACE_BEGIN
GlobalConfig globalCfg;

static bool print_usage(bool in_help = false) {
    FILE* out = in_help ? stdout : stderr;
    std::fprintf(out, "Usage: genalgo -i <image> [options]\n");
    std::fprintf(out, "Options:\n");
    std::fprintf(out, "  -i, --input <image>      Input image file\n");
    std::fprintf(out, "  -o, --output <svg>       Output SVG file\n");
    std::fprintf(out, "  -gi, --gen-input <file>  Input file to continue from\n");
    std::fprintf(out, "  -go, --gen-output <file> Output file to save the generation\n");
    std::fprintf(out, "  -s, --seed <seed>        Seed for the random number generator (default = <platform-specific-random>)\n");
    std::fprintf(out, "  -e, --engine <engine>    Fitness engine to use (default = CUDA)\n");
    std::fprintf(out, "  --period <n>             Number of generations between renders/logging (default = 50)\n");
    std::fprintf(out, "  --no-render              Disable rendering\n");
    std::fprintf(out, "  --no-breed               Disable breeding\n");
    if (!in_help) return false;
    std::fprintf(out, "Renderer keybindings:\n");
    std::fprintf(out, "  S                        Toggle showing the original image\n");
    std::fprintf(out, "  R                        Reset to the best individual and update the display\n");
    std::fprintf(out, "  N                        Show the next individual in the population\n");
    std::fprintf(out, "  P                        Show the previous individual in the population\n");
    std::fprintf(out, "  Up Arrow                 Increase render scale\n");
    std::fprintf(out, "  Down Arrow               Decrease render scale\n");
    std::fflush(out);
    return false; // To be used in return statements
}

static bool is_lopt(const char* name, const char *lopt) {
    if (*(name++) != '-') return false;
    if (*(name++) != '-') return false;
    return std::strcmp(name, lopt) == 0;
}

static bool is_opt(const char* name, const char *sopt, const char *lopt) {
    if (*name != '-') return false;
    name++;
    if (*name != '-')
        return std::strcmp(name, sopt) == 0;
    name++;
    return std::strcmp(name, lopt) == 0;
}

static bool to_u32(const char* str, u32* out) {
    char* end;
    u32 val = std::strtoul(str, &end, 10);
    if (*end != '\0')
        return false;
    if (val > std::numeric_limits<u32>::max())
        return false;
    *out = val;
    return true;
}

bool GlobalConfig::setup(int argc, char* argv[]) {
    inputFilename = nullptr;
    outputFilename = nullptr;
    outputSVG = nullptr;
    fitnessEngine = "CUDA";
    breedDisabled = false;

    const char* imageFilename = nullptr;
    bool seedSet = false;

    u32 period = 50;

    for (i32 i = 1; i < argc; i++) {
        const char* arg = argv[i];

        if (is_opt(arg, "i", "input")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "genalgo: Missing filename after -i/--input\n");
                return print_usage();
            }
            imageFilename = argv[++i];
        } else if (is_opt(arg, "o", "output")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "genalgo: Missing filename after -o/--output\n");
                return print_usage();
            }
            outputSVG = argv[++i];
        } else if (is_opt(arg, "gi", "gen-input")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "genalgo: Missing filename after -gi/--gen-input\n");
                return print_usage();
            }
            inputFilename = argv[++i];
        } else if (is_opt(arg, "go", "gen-output")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "genalgo: Missing filename after -go/--gen-output\n");
                return print_usage();
            }
            outputFilename = argv[++i];
        } else if (is_opt(arg, "g", "gen-file")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "genalgo: Missing filename after -g/--gen-file\n");
                return print_usage();
            }
            inputFilename = argv[++i];
            outputFilename = argv[i];
        } else if (is_opt(arg, "s", "seed")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "genalgo: Missing seed after -s/--seed\n");
                return print_usage();
            }
            if (!to_u32(argv[++i], &seed)) {
                fprintf(stderr, "genalgo: Invalid seed, must be a u32 number\n");
                return print_usage();
            }
            seedSet = true;
        } else if (is_opt(arg, "e", "engine")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "genalgo: Missing engine after -e/--engine\n");
                return print_usage();
            }
            fitnessEngine = argv[++i];
        } else if (is_lopt(arg, "period")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "genalgo: Missing period after --period\n");
                return print_usage();
            }
            if (!to_u32(argv[++i], &period)) {
                fprintf(stderr, "genalgo: Invalid period, must be a u32 number\n");
                return print_usage();
            }
        } else if (is_lopt(arg, "no-render")) {
            renderDisabled = true;
        } else if (is_lopt(arg, "no-breed")) {
            breedDisabled = true;
        } else if (is_opt(arg, "h", "help")) {
            return print_usage(true);
        } else {
            fprintf(stderr, "genalgo: Unknown option: %s\n", arg);
            return print_usage();
        }
    }

    if (imageFilename == nullptr) {
        fprintf(stderr, "genalgo: A image file must be provided\n");
        return print_usage();
    }

    if (!targetImage.load(imageFilename)) {
        std::fprintf(stderr, "genalgo: Failed to load image: %s\n", imageFilename);
        return false;
    }

    if (!seedSet)
        seed = std::random_device{}();

    logPeriod = period;
    renderPeriod = period;

    // TODO: No configuration should be hardcoded, maybe load those from a file
    // or from the command line itself.
    loadConstants();
    return true;
}

void GlobalConfig::loadConstants() {
    svgScale = 16;

    // Number of individuals in the population
    populationSize = 200;

    // Number of triangles in each individual
    numTriangles = 100;
    maxTriangles = 1100; 

    // Number of elite individuals
    eliteSize = 3;
    breedPoolSize = 25;

    // Mutation parameters
    //   * Probabilities are mutually exclusive, they must sum to <= 1
    mutationChanceAdd = 0.05;
    mutationChanceRemove = 0.05;
    mutationChanceReplace = 0.20;
    mutationChanceSwap = 0.05;
    mutationChanceMerge = 0.15;
    mutationChanceSplit = 0.00;
    mutationChanceShape = 0.30;

    // Shape-specific mutation parameters 
    //   * Probabilities are mutually exclusive, they must sum to <= 1
    mutationShapeFineColorChance = 0.20;
    mutationShapeFineMoveXChance = 0.20;
    mutationShapeFineMoveYChance = 0.20;
    mutationShapeFineScaleChance = 0.20;
    mutationShapeFineRotateChance = 0.20;

    // Penalty for each triangle in the individual
    penalty = 0.00001;

    // Renderer parameters
    renderScale = 1;
}

GA_NAMESPACE_END
