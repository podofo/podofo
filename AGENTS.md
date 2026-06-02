# AGENTS.md

This is a C++ project for a PDF Manipulation library.

## Code style

Read CODING-STYLE.md, but don't follow any hyperlink there.

## Developing hints
- No excess tests: one or two simple tests per small fix/feature are usually fine, unless you really need to be exhaustive
- If you need to craft a PDF file for test fixture, don't create it on the fly with text writing operations but precompute it as a constant string
- Avoid Catch SECTION(s) in tests, if possibles
- No long, repetitive comments. If you need to repeat a comment, it must be a one/max two lines
