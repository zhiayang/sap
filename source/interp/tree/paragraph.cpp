// paragraph.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for dynamic_pointer_cast

#include "interp/tree.h"     // for Text, Separator, Paragraph, ScriptCall
#include "interp/state.h"    // for Interpreter
#include "interp/interp.h"   // for FunctionCall
#include "interp/basedefs.h" // for InlineObject

namespace sap::tree
{
	void Paragraph::evaluateScripts(interp::Interpreter* cs)
	{
		for(auto& obj : m_contents)
		{
			if(auto word = util::dynamic_pointer_cast<tree::Text>(obj); word != nullptr)
			{
				// do nothing
			}
			else if(auto iscr = util::dynamic_pointer_cast<tree::ScriptCall>(obj); iscr != nullptr)
			{
				if(auto tmp = cs->run(iscr->call.get()); tmp != nullptr)
				{
					// TODO: clean this up
					if(auto text = dynamic_cast<tree::Text*>(tmp.get()); text)
					{
						obj.reset(tmp.release());
					}
					else
					{
						error("layout", "invalid object returned from script call");
					}
				}
				else
				{
					error("interp", "unsupported");
				}
			}
			else if(auto iscb = util::dynamic_pointer_cast<tree::ScriptBlock>(obj); iscb != nullptr)
			{
				error("interp", "unsupported");
			}
			else
			{
				error("interp", "??????");
			}
		}
	}

	void Paragraph::processWordSeparators()
	{
		std::vector<std::shared_ptr<InlineObject>> ret {};

		bool seen_whitespace = false;
		for(auto& uwu : m_contents)
		{
			auto tree_text = util::dynamic_pointer_cast<tree::Text>(uwu);
			assert(tree_text != nullptr);

			auto current_text = std::make_shared<Text>("");
			current_text->setStyle(tree_text->style());

			for(char c : tree_text->contents())
			{
				if(c == ' ' || c == '\t' || c == '\n' || c == '\r')
				{
					if(not seen_whitespace)
					{
						if(not current_text->contents().empty())
						{
							ret.push_back(std::move(current_text));
							ret.push_back(std::make_shared<Separator>());

							current_text = std::make_shared<Text>("");
							current_text->setStyle(tree_text->style());
						}

						seen_whitespace = true;
					}
					else
					{
						// do nothing
					}
				}
				else
				{
					current_text->contents() += c;
					seen_whitespace = false;
				}
			}

			if(not current_text->contents().empty())
				ret.push_back(std::move(current_text));
		}

		m_contents.swap(ret);
	}
}
