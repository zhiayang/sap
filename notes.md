## notes

### on substitutions and ligatures

While we can parse the most common GSUB and GPOS formats in the font and act on them, we are unable to do
it intelligently at all. Quite a number of the GSUB substitutions are not simply "normal" ligatures; there's
stuff like discretionary ligatures (which somehow turns `ます` into `〼`).

The same is true of single-glyph GPOS tables (with the "placement" value) -- they're probably tied to some kind
of specific feature (eg. sub/superscripts).

The correct way is to look at the feature table, and only use the layout tables that correspond to the features
that are enabled. For now, ligatures are disabled till we implement this discrimination.



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







## references

(Global Multiple Objective Line Breaking (Holkner, 2006))[http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.585.8487&rep=rep1&type=pdf]
