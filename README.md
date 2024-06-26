# GenAlgo

GenAlgo is a genetic algorithm-based image approximation project. This project utilizes semi-transparent triangles to approximate a target image and leverages GPU acceleration with CUDA for efficient computation of the fitness function.

## Dependencies

- SFML 2.6
- CUDA 12.5

## Tested Hardware

- NVIDIA RTX 3060 Mobile
- NVIDIA GTX 1650 Mobile
- NVIDIA RTX 3060

## Compiler

- GCC 14.1.1

## Usage

To run GenAlgo, use the following command:

```
genalgo -i <image> [options]
```

### Options

- `-i, --input <image>`: Input image file.
- `-o, --output <svg>`: Output SVG file.
- `-gi, --gen-input <file>`: Input file to continue from.
- `-go, --gen-output <file>`: Output file to save the generation.
- `-s, --seed <seed>`: Seed for the random number generator (default = platform-specific random).
- `--period <n>`: Number of generations between renders/logging (default = 50).
- `--no-render`: Disable rendering.
- `--no-breed`: Disable breeding.

### Renderer Keybindings

- `S`: Toggle showing the original image.
- `R`: Reset to the best individual and update the display.
- `N`: Show the next individual in the population.
- `P`: Show the previous individual in the population.
- `Up Arrow`: Increase render scale.
- `Down Arrow`: Decrease render scale.

## Building

To build GenAlgo, make sure you have all the dependencies installed and run the following commands:

```
mkdir build
cd build
cmake ..
make
```

## Running

After building, you can run GenAlgo with the desired input image and options:

```
./genalgo -i input_image.png --period 100 --output result.svg
```

## License

This project is licensed under the MIT License.
