# LatexLabel Test Suite

This directory contains markdown test files for testing the LatexLabel widget's rendering capabilities.

## Test Files

### `basic_markdown.md`
Tests fundamental markdown syntax elements:
- All heading levels (H1-H6)
- Text formatting (bold, italic, strikethrough)
- Links and inline code
- Simple lists (ordered and unordered)
- Blockquotes
- Horizontal rules
- Code blocks
- Basic paragraphs

### `latex_math.md`
Focuses on LaTeX mathematical expressions:
- Inline math expressions ($...$)
- Display math expressions ($$...$$)
- Complex mathematical formulas
- Greek letters and special symbols
- Fractions, integrals, and summations
- Maxwell's equations
- Matrices and advanced mathematical notation

### `nested_lists.md`
Tests complex list structures:
- Basic nested lists
- Mixed list types (ordered within unordered, etc.)
- Lists containing mathematical expressions
- Deep nesting (6+ levels)
- Lists with display math
- Complex combinations of lists and math

### `mixed_content.md`
Comprehensive test combining various elements:
- Mathematical concepts in lists
- Code blocks with math comments
- Blockquotes containing math
- Tables with mathematical formulas
- Complex nested structures
- Real-world content mixing markdown and LaTeX

### `edge_cases.md`
Tests unusual and potentially problematic scenarios:
- Empty elements
- Unmatched dollar signs
- Math expressions in various contexts
- Escaped characters
- Malformed content
- Unicode and special characters
- Extreme nesting scenarios
- Line break edge cases

### `simple_test.md`
A minimal test file for quick testing:
- Basic text with simple math
- Short list with math
- Single display equation
- Simple formatting

## Usage

When you run the LaTeX Label Test Suite application:

1. **Auto-loading**: The first available test file will be loaded automatically
2. **File Selector**: Use the dropdown to choose different test files
3. **Load Button**: Click to load the selected test file
4. **Browse Button**: Load any markdown file from your filesystem

## Creating New Test Files

To add new test files:

1. Create a new `.md` file in this directory
2. Add your markdown content with LaTeX math expressions
3. Restart the application - new files will appear in the dropdown automatically

## LaTeX Math Syntax

- Inline math: `$expression$`
- Display math: `$$expression$$`
- Common expressions:
  - Fractions: `\frac{numerator}{denominator}`
  - Superscripts: `x^2` or `x^{expression}`
  - Subscripts: `x_1` or `x_{expression}`
  - Greek letters: `\alpha`, `\beta`, `\gamma`, etc.
  - Integrals: `\int_a^b f(x) dx`
  - Summations: `\sum_{i=1}^n i`

## Notes

- The LatexLabel widget supports the md4c markdown parser with LaTeX math extensions
- Math rendering is provided by MicroTeX
- Test files are loaded with UTF-8 encoding
- Empty or malformed files may cause rendering issues - check the console for error messages 