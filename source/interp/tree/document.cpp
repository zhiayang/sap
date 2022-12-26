// document.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for dynamic_pointer_cast

#include "interp/tree.h"   // for Document, Paragraph, ScriptBlock
#include "interp/interp.h" // for Interpreter

namespace sap::tree
{
	void Document::evaluateScripts(interp::Interpreter* cs)
	{
		// for(auto& obj : m_objects)
		for(auto it = m_objects.begin(); it != m_objects.end();)
		{
			if(auto blk = util::dynamic_pointer_cast<ScriptBlock>(*it); blk != nullptr)
			{
				// TODO:
				zpr::println("not implemented: script block");
				it = m_objects.erase(it);
			}
			else if(auto para = util::dynamic_pointer_cast<Paragraph>(*it); para != nullptr)
			{
				para->evaluateScripts(cs);
				++it;
			}
			else
			{
				sap::internal_error("aoeu");
			}
		}
	}

	void Document::processWordSeparators()
	{
		for(auto& obj : m_objects)
		{
			if(auto para = util::dynamic_pointer_cast<Paragraph>(obj); para != nullptr)
			{
				para->processWordSeparators();
			}
			else
			{
				sap::internal_error("boeu");
			}
		}
	}
}
