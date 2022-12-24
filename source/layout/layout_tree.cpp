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

		for(auto& obj : para->contents())
		{
			if(auto txt = std::dynamic_pointer_cast<tree::Text>(obj); txt != nullptr)
				ret->add(Text::fromTreeText(*txt));

			else if(auto sep = std::dynamic_pointer_cast<tree::Separator>(obj); sep != nullptr)
				ret->add(Text::separator());

			else
				sap::internal_error("coeu");
		}

		return ret;
	}

	Document createDocumentLayout(interp::Interpreter* cs, tree::Document& treedoc)
	{
		Document document {};
		for(auto& obj : treedoc.objects())
		{
			if(auto para = std::dynamic_pointer_cast<tree::Paragraph>(obj); para != nullptr)
			{
				document.addObject(layoutParagraph(cs, para));
			}
			else if(auto scr = std::dynamic_pointer_cast<tree::ScriptObject>(obj); scr != nullptr)
			{
				// TODO
			}
		}

		return document;
	}
}
