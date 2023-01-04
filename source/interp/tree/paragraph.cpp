// paragraph.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"         // for FunctionCall
#include "interp/tree.h"        // for Text, Separator, Paragraph, ScriptCall
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter
#include "interp/basedefs.h"    // for InlineObject
#include "interp/eval_result.h" // for EvalResult

namespace sap::tree
{
	void Paragraph::evaluateScripts(interp::Interpreter* cs)
	{
		std::vector<std::unique_ptr<InlineObject>> ret {};
		ret.reserve(m_contents.size());

		for(auto& obj : m_contents)
		{
			if(auto word = dynamic_cast<tree::Text*>(obj.get()); word != nullptr)
			{
				ret.push_back(std::move(obj));
			}
			else if(auto iscr = dynamic_cast<tree::ScriptCall*>(obj.get()); iscr != nullptr)
			{
				auto value_or_err = cs->run(iscr->call.get());
				if(value_or_err.is_err())
					error("interp", "evaluation failed: {}", value_or_err.error());

				auto value_or_empty = value_or_err.take_value();
				if(not value_or_empty.hasValue())
				{
					ret.emplace_back(new tree::Text(U"", obj->style()));
					continue;
				}

				auto value = std::move(value_or_empty).take();

				if(value.isTreeInlineObj())
				{
					auto objs = std::move(value).takeTreeInlineObj();
					ret.insert(ret.end(), std::move_iterator(objs.begin()), std::move_iterator(objs.end()));
				}
				else if(value.isPrintable())
				{
					ret.emplace_back(new tree::Text(value.toString(), obj->style()));
				}
				else
				{
					error("layout", "cannot insert value of type '{}' into paragraph", value.type());
				}
			}
			else if(auto iscb = dynamic_cast<tree::ScriptBlock*>(obj.get()); iscb != nullptr)
			{
				error("interp", "script block unsupported");
			}
			else
			{
				error("interp", "unsupported");
			}
		}

		m_contents.swap(ret);
	}

	void Paragraph::processWordSeparators()
	{
		std::vector<std::unique_ptr<InlineObject>> ret {};

		bool first_obj = true;
		bool seen_whitespace = false;
		for(auto& uwu : m_contents)
		{
			auto tree_text = dynamic_cast<tree::Text*>(uwu.get());
			assert(tree_text != nullptr);

			auto current_text = std::make_unique<Text>(U"");
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
							ret.push_back(std::make_unique<Separator>(Separator::SPACE));

							current_text = std::make_unique<Text>(U"");
							current_text->setStyle(tree_text->style());
						}
						else if(not first_obj)
						{
							ret.push_back(std::make_unique<Separator>(Separator::SPACE));
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

	void Paragraph::addObject(std::unique_ptr<InlineObject> obj)
	{
		m_contents.push_back(std::move(obj));
	}
}
