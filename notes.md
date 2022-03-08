## notes


### language design

The goal is to keep the number of "special" character as small as possible. Currently, we have 4: `\`, `{`, `}`, and `#`.
It is probably a good idea to enforce that the scripting context is always introduced by `\`. I'm not sure if we want
'bare' `{` and `}` to have special meaning (like in latex).

Part of the issue is that we have a few different kinds of "things":

1. string literals, in the context of the scripting language (eg. `"foobar"`)
2. user text, as in paragraphs, words, titles, etc.
3. code blocks, ie. a list of statements

For denoting (1), it's very easy -- just use double quotes in a scripting context. For (3), it is convenient to use
`\{ ... }` since we already want `\` to unambiguously introduce a script block. The issue here is (2), beacuse
usertext will most likely have spaces.

We could do `{` without `\` to mean a text block, but then we need to escape `{` itself, which would be `\{`... An
alternative is to do something like `\script{...}` to create a script block, then bare `{` and `}` for text blocks,
letting us escape them with `\{`.


There's also something we probably want like swift's "trailing closures", except that the block isn't a script block
but a text block. For example we could have something like this:

```
\style(weight: "bold") {
	this is some bold text
}
```

there's some additional considerations wrt. the semantics of text blocks, eg. whether they are considered inline or
paragraphs by default. We could have them be inline by default and introduce a `\p { ... }` construct for explicit paragraphs;
this does mean that the parser must be (somewhat) flexible in how it parses the trailing block.

Not sure if we want to support multiple trailing blocks... probably? might be useful for certain "environment" situations.














### on CFF subsetting

So we got OpenType-CFF subsetting mostly working, but it doesn't work consistently in all viewers. So, instead
of embedding the whole OTF, we only embed the raw CFF data. This follows what TeX does, which means it's
probably fine.



### on font size optimisation

There are quite a number of potential size savings that we haven't taken advantage of, in
increasing order of complexity:

1. remove the `vhea`/`vmtx` table if we did not use the font in vertical writing mode

2. chopping off glyph ids after the last one that we use, letting us:
	- cut the size of the `loca` table (esp for large fonts that need 4 bytes per entry)
	- cut the size of the `hmtx` table

3. renumber the glyphs entirely:
	- completely eliminates the bulk of the `cmap` table
	- also cuts `loca` and `hmtx` dramatically
	- also drastically shrinks the size of the text itself (since we no longer need 2 bytes for glyph ids)


### on unicode (de)composition

The problem is that unicode combining characters let people write arbitrary things that combine with
arbitrary other things. Normally, this isn't a problem (if your font doesn't have it, then it won't show).

However, combining diacritic marks are a (potentially common) thing, but fonts (eg. Myriad Pro) might not
actually contain a glyph for the diacritic mark --- instead having one glyph for each (valid) combination
of base character + mark.

This means that, for maximum flexibility, if a font doesn't contain a glyph for a combining character, we
should attempt to compose the codepoint, see if the font contains a glyph for that codepoint, and if
so replace it. The conerse is also true --- if for some reason the font has the combining character glyph
but not one for the composed form, then we should attempt to decompose the codepoint.


update: it probably makes more sense to maximally compose codepoints (i think we can do that with utf8proc),
since we want to prefer an "actual" glyph eg. for an accented character if that exists. If that fails, then
we should try the decomposed version, and if that also fails, `.notdef` time.



## references

[Global Multiple Objective Line Breaking (Holkner, 2006)](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.585.8487&rep=rep1&type=pdf)
