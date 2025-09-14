# Test Heading
```
# Calculate the quadratic formula: x = (-b ± √(b²-4ac)) / 2a
def quadratic_formula(a, b, c):
    discriminant = b**2 - 4*a*c  # b² - 4ac
    if discriminant < 0:
        return None  # No real solutions

    x1 = (-b + discriminant**0.5) / (2*a)
    x2 = (-b - discriminant**0.5) / (2*a)
    return x1, x2
```
