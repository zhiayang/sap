## sap

> Sap is distinct from latex ...; it is a separate substance, separately produced, and with different components and functions.

— Wikipedia


### introduction

sap (not to be confused with [Systeme, Anwendungen und Produkte in der Datenverarbeitung](https://www.sap.com)) is a document typesetting/preparation system similar in concept to [LaTeX](https://www.latex-project.org) (and derivatives), with the following major differences:

1. it is not from 1984 (or 1994 if you count LaTex 2e)
2. it is not a glorified macro system
3. the programming 'metalanguage' is familiar (and sane)

### current featureset

Since this is a work-in-progress, features are very limited.

- [x] PDF generation
- [x] Unicode support (for the most part)
- [x] OpenType font (otf/ttf) loading (and embedding into PDF)
- [ ] Embedded font subsetting
- [ ] Compressed PDF streams
- [x] Glyph kerning (via GPOS table)
- [ ] Glyph ligatures (via GSUB table)
- [ ] Unicode substitution (eg. codepoint (de)composition)
- [ ] Vertical writing
- [ ] Literally any text typesetting
- [ ] Literally any metalanguage


### goals

The eventual goal of this project is to completely replace LaTeX as my document typesetting system. To this end, there are some "big features" that are yet to come:

1. Graphics support rivalling at least the basic features of PGF/TikZ
2. Full document metaprogramming (freely manipulating text)
3. Some basic level of typesetting mathematical expressions
4. Some level of bibliography/referencing support
5. ...?

### non-goals
Non-goals are split into two categories: things I can't do because this is a one-person effort, and things I won't do because they are not in the scope of this project. For the former, pull requests are very appreciated (:

#### can't

1. Right-to-left languages
2. "Complex" non-latin scripts, eg. Arabic, Urdu
3. "Complex" latin scripts, eg. Vietnamese

#### won't

1. Any sort of TeX/LaTeX compatibility
2. "Graphical parity" with LaTeX
3. Non-OpenType font support (eg. Type1)


### license

sap is licensed under the [Apache License version 2.0](./LICENSE).




