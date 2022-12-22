// layout_tree.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "error.h"

#include "sap/document.h"

#include "interp/tree.h"
#include "interp/state.h"

// functions for converting tree::* structures into layout::* structures

namespace sap::layout
{
	static Word make_word(const tree::Word& word)
	{
		// TODO: determine whether this 'kind' thing is even useful
		auto ret = Word(Word::KIND_LATIN, word.text());
		ret.setStyle(word.style());

		return ret;
	}



	static std::unique_ptr<Paragraph> layoutParagraph(interp::Interpreter* cs, const std::shared_ptr<tree::Paragraph>& para)
	{
		auto ret = std::make_unique<Paragraph>();

		for(auto& obj : para->m_contents)
		{
			if(auto word = std::dynamic_pointer_cast<tree::Word>(obj); word != nullptr)
			{
				ret->add(make_word(*word));
			}
			else if(auto iscr = std::dynamic_pointer_cast<tree::ScriptCall>(obj); iscr != nullptr)
			{
				if(auto tmp = cs->run(iscr->call.get()); tmp != nullptr)
				{
					// TODO: clean this up
					if(auto word = dynamic_cast<tree::Word*>(tmp.get()); word)
						ret->add(make_word(*word));

					else
						error("layout", "invalid object returned from script call");
				}
			}
			else if(auto iscb = std::dynamic_pointer_cast<tree::ScriptBlock>(obj); iscb != nullptr)
			{
				error("interp", "unsupported");
			}
		}

		return ret;
	}

	Document createDocumentLayout(interp::Interpreter* cs, tree::Document& treedoc)
	{
		Document document {};
		for(auto& obj : treedoc.m_objects)
		{
			if(auto para = std::dynamic_pointer_cast<tree::Paragraph>(obj); para != nullptr)
				document.addObject(layoutParagraph(cs, para));
		}

		return document;
	}
}
