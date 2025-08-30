# Mixed Content Test

This file combines various markdown elements with LaTeX math.

## Introduction

Welcome to a comprehensive test of **markdown** and *LaTeX* integration. This document contains various elements mixed together to test rendering quality.

## Mathematical Concepts in Lists

### Calculus Fundamentals

1. **Derivatives** - The rate of change
   - Basic rule: $\frac{d}{dx}(x^n) = nx^{n-1}$
   - Product rule: $\frac{d}{dx}(uv) = u'v + uv'$
   - Chain rule: $\frac{d}{dx}f(g(x)) = f'(g(x))g'(x)$

2. **Integrals** - The accumulation of change

   $$\int_a^b f(x) dx = F(b) - F(a)$$

   - Fundamental theorem connects derivatives and integrals
   - Applications include area under curves

### Linear Algebra

- **Vectors**: $\vec{v} = \langle a, b, c \rangle$
  - Dot product: $\vec{u} \cdot \vec{v} = u_1v_1 + u_2v_2 + u_3v_3$
  - Cross product in 3D: $\vec{u} \times \vec{v}$

- **Matrices**: Operations on arrays of numbers
  ```
  Matrix multiplication:
  (AB)_ij = Σ A_ik * B_kj
  ```
-test

## Code with Math Comments

```python
# Calculate the quadratic formula: x = (-b ± √(b²-4ac)) / 2a
def quadratic_formula(a, b, c):
    discriminant = b**2 - 4*a*c  # b² - 4ac
    if discriminant < 0:
        return None  # No real solutions

    x1 = (-b + discriminant**0.5) / (2*a)
    x2 = (-b - discriminant**0.5) / (2*a)
    return x1, x2
```

The discriminant $\Delta = b^2 - 4ac$ determines the nature of solutions.

## Blockquotes with Math

> *"Mathematics is the language with which God has written the universe."* - Galileo Galilei
>
> This famous quote reminds us that mathematical relationships like $F = ma$ and $E = mc^2$ describe fundamental laws of physics.
>
> Even simple equations like $a^2 + b^2 = c^2$ (Pythagorean theorem) reveal profound geometric truths.

## Complex Nested Example

### Physics Problem Set

1. **Mechanics**
   - Newton's Laws
     - First law: An object at rest stays at rest unless acted upon by force
     - Second law: $\vec{F} = m\vec{a}$
     - Third law: For every action, there's an equal and opposite reaction

   - Energy conservation: $KE + PE = \text{constant}$

     $$E_{\text{kinetic}} = \frac{1}{2}mv^2$$

     $$E_{\text{potential}} = mgh$$

2. **Thermodynamics**
   - First law: $\Delta U = Q - W$ (energy conservation)
   - Second law: Entropy always increases in isolated systems
   - Ideal gas law: $PV = nRT$

## Tables with Math

| Formula | Description | Example |
|---------|-------------|---------|
| $A = \pi r^2$ | Area of circle | $r = 5 \Rightarrow A = 25\pi$ |
| $V = \frac{4}{3}\pi r^3$ | Volume of sphere | Sphere calculations |
| $c^2 = a^2 + b^2$ | Pythagorean theorem | Right triangles |

## Conclusion

This document demonstrates the integration of:
- **Bold** and *italic* text
- Mathematical expressions: $\sum_{i=1}^n i = \frac{n(n+1)}{2}$
- Code blocks with syntax highlighting
- Nested lists with varying depths
- Blockquotes with mathematical content

The final equation to remember:

$$\int_{-\infty}^{\infty} e^{-x^2} dx = \sqrt{\pi}$$

---

*End of mixed content test.*
