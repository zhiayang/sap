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
	static std::unique_ptr<Paragraph> layoutParagraph(interp::Interpreter* cs, const std::shared_ptr<tree::Paragraph>& para)
	{
		auto ret = std::make_unique<Paragraph>();

		std::optional<Word> prev {};

		for(auto& obj : para->m_contents)
		{
			if(auto word = std::dynamic_pointer_cast<tree::Word>(obj); word != nullptr)
			{
				ret->add(Word::fromTreeWord(*word));
			}
			else if(auto iscr = std::dynamic_pointer_cast<tree::ScriptCall>(obj); iscr != nullptr)
			{
				if(auto tmp = cs->run(iscr->call.get()); tmp != nullptr)
				{
					// TODO: clean this up
					if(auto word = dynamic_cast<tree::Word*>(tmp.get()); word)
					{
						word->stick_to_left = iscr->stick_to_left;
						word->stick_to_right = iscr->stick_to_right;
						ret->add(Word::fromTreeWord(*word));
					}
					else
					{
						error("layout", "invalid object returned from script call");
					}
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
