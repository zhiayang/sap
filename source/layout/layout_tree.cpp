// layout_tree.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/tree.h"
#include "sap/document.h"

// functions for converting tree::* structures into layout::* structures

namespace sap::layout
{
	static Word make_word(const tree::Word& word)
	{
		// TODO: determine whether this 'kind' thing is even useful
		return Word(Word::KIND_LATIN, word.m_text);
	}

	static std::unique_ptr<Paragraph> layoutParagraph(const std::shared_ptr<tree::Paragraph>& para)
	{
		auto ret = std::make_unique<Paragraph>();

		for(auto& obj : para->m_contents)
		{
			if(auto word = std::dynamic_pointer_cast<tree::Word>(obj); word != nullptr)
				ret->add(make_word(*word));
		}

		return ret;
	}

	Document createDocumentLayout(tree::Document& treedoc)
	{
		Document document {};
		for(auto& obj : treedoc.m_objects)
		{
			if(auto para = std::dynamic_pointer_cast<tree::Paragraph>(obj); para != nullptr)
				document.addObject(layoutParagraph(para));
		}

		return document;
	}
}
