## notes


### glyph positioning, substitution, and rendering

The correct way to lookup substitutions and positioning adjustments is already documented in `features.h` (but
not implemented yet, as of now).

During the layout, we already convert words (characters) to glyphs wholesale. I think the GSUB and GPOS interfaces
need to change so that they operate on an entire glyphstring at once.

For instance, there are a number of contextual lookup types that possibly require lookahead *and* lookbehind, so it
probably makes more sense to just pass a span of glyph ids, and put all the substitution/positioning stuff into
the font-side code. We would return a (new) list of glyphs for GSUB, and probably a pair of glyphid+GlyphAdjustment
for GPOS.

We might combine this with the GPOS step or have it be a separate step; either way, we need to compute adjusted GlyphMetrics
from the builtin metrics for the base glyph, adjusted by the adjustment for that glyph. After this, computing the
word spacing becomes rather easy without having to explicitly deal with kerning.

During rendering, we probably still need to keep the adjustment values around, because (eg.) we must adjust positioning
(eg. vertically for super/subscripts, horizontally for kerning) *after* accounting for the "default" movement due to
the glyph width done by PDF.













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
