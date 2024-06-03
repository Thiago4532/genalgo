# Genetic Algorithm

## Classes

### Point
    - x: num
    - y: num

### Color
    - r: u8
    - g: u8
    - b: u8
    - a: u8

### Triangle
    - a, b, c: Point<i32>
    - color: Color

### Individual
    - triangles: Triangle[]
    - fitness: double
    
### Population
    - individiuals: Individual[]

### Renderer
    * isRenderAvailable: () -> bool
    * render: (best: Individual, pop: Population) -> void

## Plano

Inicialmente gere a população de forma aleatória.
Para cada geração, chame a função para calcular o fitness da população,
controlado por uma FitnessEngine.
Agora, basta chamar a função de reprodução de Population, que retorna uma nova
população. Substitua a população passada pela nova e repita o processo enquanto
for necessário.
