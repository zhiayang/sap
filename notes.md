## notes


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
