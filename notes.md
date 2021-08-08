## notes

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


### on CFF subsetting

CFF is a real pain, it's basically an entirely separate format embedded as a table in OTF. sucks. In theory
it should be possible to subset, with a few steps:

1. parse the basic structure of the CFF font
2. find some relation between the OTF's cmap and the CFF's encoding/charset
3. copy only the required glyphs
4. for extra points, remove unnecessary `subrs` if any



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
