// paragraph.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for dynamic_pointer_cast

#include "interp/ast.h"      // for FunctionCall
#include "interp/tree.h"     // for Text, Separator, Paragraph, ScriptCall
#include "interp/value.h"    // for Value
#include "interp/interp.h"   // for Interpreter
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
				auto value_or_err = cs->evaluate(iscr->call.get());
				if(value_or_err.is_err())
					error("interp", "evaluation failed: {}", value_or_err.error());

				auto value_or_empty = value_or_err.take_value();
				if(not value_or_empty.has_value())
				{
					obj.reset(new tree::Text(U"", obj->style()));
					continue;
				}

				auto value = std::move(*value_or_empty);
				if(value.isTreeInlineObj())
					obj.reset(std::move(value).takeTreeInlineObj().release());
				else if(value.isPrintable())
					obj.reset(new tree::Text(value.toString(), obj->style()));
				else
					error("layout", "cannot insert value of type '{}' into paragraph", value.type());
			}
			else if(auto iscb = util::dynamic_pointer_cast<tree::ScriptBlock>(obj); iscb != nullptr)
			{
				error("interp", "unsupported");
			}
			else
			{
				error("interp", "unsupported");
			}
		}
	}

	void Paragraph::processWordSeparators()
	{
		std::vector<std::shared_ptr<InlineObject>> ret {};

		bool first_obj = true;
		bool seen_whitespace = false;
		for(auto& uwu : m_contents)
		{
			auto tree_text = util::dynamic_pointer_cast<tree::Text>(uwu);
			assert(tree_text != nullptr);

			auto current_text = std::make_shared<Text>(U"");
			current_text->setStyle(tree_text->style());

			for(char32_t c : tree_text->contents())
			{
				if(c == U' ' || c == U'\t' || c == U'\n' || c == U'\r')
				{
					if(not seen_whitespace)
					{
						if(not current_text->contents().empty())
						{
							ret.push_back(std::move(current_text));
							ret.push_back(std::make_shared<Separator>(Separator::SPACE));

							current_text = std::make_shared<Text>(U"");
							current_text->setStyle(tree_text->style());
						}
						else if(not first_obj)
						{
							ret.push_back(std::make_shared<Separator>(Separator::SPACE));
						}

						seen_whitespace = true;
					}
					else
					{
						// do nothing (consume more whitespace)
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
			first_obj = false;
		}

		m_contents.swap(ret);
	}
}
