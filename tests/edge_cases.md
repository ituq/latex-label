# Edge Cases Test

This file tests unusual and edge cases for markdown and LaTeX rendering.

## Empty Elements

Empty blockquote:
>

Empty list:
- 

Empty code block:
```

```

## Math Edge Cases

Single dollar: $ (should not trigger math mode)

Unmatched dollars: $incomplete math

Multiple dollars: $$$ invalid display $$$

Math with special characters: $a_1 + b_2 = c_3$

Math with spaces: $ x + y = z $ (spaces around content)

## Nested Edge Cases

- List item with unmatched dollar: $unclosed
  - Nested item: $x = y$ complete
- Another item: $\frac{1}{2}$

## Inline Code with Math-like Content

This is `$not_math$` in code.

Code with backslashes: `\frac{1}{2}` should not render as math.

## Escaped Characters

Escaped dollar: \$not_math\$

Escaped asterisk: \*not_bold\*

Escaped underscore: \_not_italic\_

## Mixed Escaping and Math

Text with \$escaped\$ and $real_{math}$ mixed.

## Long Math Expressions

Very long inline math that should test line wrapping: $f(x) = a_0 + a_1x + a_2x^2 + a_3x^3 + a_4x^4 + a_5x^5 + a_6x^6 + a_7x^7 + a_8x^8 + a_9x^9$

## Math in Various Contexts

### In Headings with $\alpha + \beta$

**Bold with $E = mc^2$ math**

*Italic with $\pi \approx 3.14$ math*

~~Strikethrough with $x^2$ math~~

## Complex Nesting

1. Ordered list
   > Blockquote in list with $\sum_{i=1}^n i$ math
   > 
   > More blockquote text
   
   - Nested unordered in ordered
     - With math: $\int_0^1 x dx$
     
     ```
     Code block in nested list
     with $fake_math$ that shouldn't render
     ```

## URL-like Math

Math that looks like URLs: $http://example.com$ (should render as math)

Links with math in text: [Link with $x^2$](https://example.com)

## Special Unicode

Math with unicode: $α + β = γ$ (Greek letters directly)

Emoji in math context: $😀 + 😊 = 😄$ (should still try to render)

## Malformed HTML-like Tags

<not>a tag</not> with $math$ mixed

## Multiple Paragraphs with Mixed Content

First paragraph with $a^2$ math.

Second paragraph with **bold** and *italic*.

Third paragraph with:
- List item with $b^2$ math
- Another item

Fourth paragraph with display math:

$$\int_{-\infty}^{\infty} e^{-x^2} dx = \sqrt{\pi}$$

And continuing text.

## Line Break Edge Cases

Math before line break: $x + y$  
Line break here.

Math after line break:  
$a + b = c$

## Zero-width Content

Math with subscript: $x_{}$ (empty subscript)

Math with superscript: $x^{}$ (empty superscript)

## Extreme Nesting

- Level 1 with $a$
  - Level 2 with $b$
    - Level 3 with $c$
      - Level 4 with $d$
        - Level 5 with $e$
          > Blockquote in deep list
          > With $nested_{math}$
          - Level 6 continues

## End Test

Final math expression: $\lim_{x \to \infty} \frac{1}{x} = 0$ 