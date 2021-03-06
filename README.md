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
4. Some level of bibliography/referencing support
5. ...?

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

- [x] OpenType font (otf/ttf) loading and embedding
- [x] Embedded font subsetting (both TrueType and CFF)
- [ ] Variable font support
- [ ] Further size optimisation for embedded fonts
- [x] Glyph positioning (OpenType GPOS, lookups 1, 2, 7, 8, 9)
- [x] Glyph substitutions (incl. ligatures) (OpenType GSUB, lookups 1, 2, 4, 5, 6, 7)
- [ ] Truetype/AAT `kern` table
- [ ] AAT tables (`kerx`, `morx`)
- [ ] More advanced typography support (`BASE`, `MATH`, `JSTF`)
- [x] Text extraction (eg. copy/paste) support — `ToUnicode` table
- [ ] Unicode substitution (eg. codepoint (de)composition)
- [ ] Vertical writing
- [x] Very very (very very...) basic paragraph layout, linebreaking, and pagination
- [ ] Literally any metalanguage


### alternatives

But what about `<insert_tool_here>`?

- **(g)roff**: the language (doesn't seem) powerful enough, seems to require strange things to use unicode, and the syntax is too terse.
- **patoline**: development seems slow/abandoned, basic things don't work (eg. copying text)
- **html/css**: no. just no.


### license

sap is licensed under the [Apache License version 2.0](./LICENSE). [utf8proc](./external/utf8proc/LICENSE.md)
and [miniz](./external/miniz/LICENSE) are licensed under their respective licenses.
