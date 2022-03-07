// sap.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/document.h"

namespace pdf
{
	struct Document;
}

namespace sap
{
	pdf::Document compile();
}






#if 0

essay time
----------

The general flow of text is like this:
lexer -> parser -> interp -> layout -> [interp] -> pdf

The interpreter is reponsible both for running any script files or expressions, and for collecting
the parsed words into paragraphs. Note that the interp needs to be run twice, because usercode should
be able to modify the layout of frames after their automatic placement (if desired).


As a first step, the stage 1 interpreter simply needs to collect adjacent Word tokens into Paragraphs
and pass it to the layout stage; the parser constructs Word objects and passes it to the interpreter.



#endif
