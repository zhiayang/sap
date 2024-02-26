## sap

> Sap is distinct from latex ...; it is a separate substance, separately produced, and with different components and functions.

— Wikipedia


### introduction

**sap** (not to be confused with [Systeme, Anwendungen und Produkte in der Datenverarbeitung](https://www.sap.com))
is a document typesetting/preparation system similar in concept to [LaTeX](https://www.latex-project.org) (and
derivatives), with the following major differences:

1. it is not from 1978 (TeX), 1984 (LaTeX), or 1994 (LaTeX2e)
2. it is not a glorified macro system
3. it is not a fragile amalgamation of packages all trying not to step on each others' toes
4. the programming 'metalanguage' is familiar (and sane)

This is currently a work-in-progress, so features are quite limited. An incomplete list of features can
be found at the bottom of this document.

### goals

The eventual goal of this project is to completely replace LaTeX as my document typesetting system. To this end,
there are some "big features" that will be implemented eventually:

1. Graphics support — at least the (very very) basic features of PGF/TikZ
2. Full document metaprogramming (freely manipulating text)
3. Some basic level of typesetting mathematical expressions
4. ...?

### non-goals
Non-goals are split into two categories: things I can't do because this is a one-person effort, and things
I won't do because they are not in the scope of this project. For the former, pull requests are very appreciated (:

#### can't

These are mostly because (a) I won't be writing documents in any of these languages, and
(b) I don't know any of these languages.

1. Right-to-left languages
2. "Complex" latin scripts, eg. Vietnamese
3. "Complex" non-latin scripts, eg. Arabic, Urdu

#### won't

While I appreciate the vast ecosystem that LaTeX has, having any kind of compatibility is simply not possible for obvious reasons.

1. Any sort of TeX/LaTeX compatibility
2. Graphical parity with LaTeX
3. Non-OpenType font support (eg. Type1)
4. Generating LaTeX source as an output


### current featureset

This also acts as a sort of TODO; as time goes on completed features may be removed from this list if they are deemed "trivial".

- [ ] Unicode substitution (eg. codepoint (de)composition)
- [ ] More advanced typography support (`BASE`, `MATH`, `JSTF`, AAT `kerx`)
- [ ] Further size optimisation for embedded fonts
- [ ] Optimising bytecode compiler for the scripting language
- [ ] Tables
- [ ] SVG embedding
- [ ] PDF embedding
- [ ] Some manipulation of embedded images (eg. rotation)
- [ ] Reflection over document contents (not just adding new stuff)
- [ ] Various (advanced) scripting language features
- [ ] Coloured text (separate from coloured lines...)
- [x] (partial) colour support
- [x] PDF paths (curves and lines)
- [x] PDF Links (annotations)
- [x] Rudimentary and crude bibliography support
- [x] Slightly more advanced layout capabilities (hbox, vbox, zbox)
- [x] Layout integration with scripting language
- [x] Embedding images
- [x] Automatic hyphenation (English only, parsed from `hyph-en-gb.tex`)
- [x] OpenType font (otf/ttf) loading and embedding
- [x] Embedded font subsetting for TrueType fonts (CFF fonts are a little iffy)
- [x] Glyph positioning (`kern` & GPOS lookups 1, 2, 7, 8, 9)
- [x] Glyph substitutions (incl. ligatures) (`morx` & OpenType GSUB lookups 1, 2, 4, 5, 6, 7)
- [x] Text extraction (eg. copy/paste) support — `ToUnicode` table
- [x] Basic paragraph layout, linebreaking, and pagination
- [x] Some meta language
- [ ] Vertical writing
- [ ] ~~Variable font support~~ Unlikely to support variable fonts, since it's a pain


### alternatives

But what about `<insert_tool_here>`?

- **typst**: it wasn't released when i started sap ):
- **(g)roff**: the language (doesn't seem) powerful enough, seems to require strange things to use unicode, and the syntax is too terse.
- **patoline**: development seems slow/abandoned, basic things don't work (eg. copying text)
- **html/css**: no. just no.


### license

sap is licensed under the [Apache License version 2.0](./LICENSE). [utf8proc](./external/utf8proc/LICENSE.md)
and [libdeflate](./external/libdeflate/LICENSE) are licensed under their respective licenses.

The hyphenation list is MIT licensed, see the hyphenation file [hyph-en-gb.tex](./hyph-en-gb.tex) or [the repo](https://github.com/hyphenation/tex-hyphen).
