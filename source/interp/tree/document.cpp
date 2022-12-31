// document.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/tree.h"        // for Document, Paragraph, ScriptBlock
#include "interp/interp.h"      // for Interpreter
#include "interp/basedefs.h"    // for DocumentObject
#include "interp/eval_result.h" // for EvalResult

namespace sap::tree
{
	static void run_script_block(interp::Interpreter* cs, ScriptBlock* script_block)
	{
		for(auto& stmt : script_block->statements)
		{
			auto result_or_err = cs->run(stmt.get());
			if(result_or_err.is_err())
				error("interp", "evaluation failed: {}", result_or_err.error());

			auto result = result_or_err.take_value();
			if(result.isReturn())
			{
				// TODO: handle returning values here
				if(result.hasValue())
					error("interp", "TODO: returning values from \\script{} is unimplemented");
			}
			else if(not result.isNormal())
			{
				error("interp", "unexpected statement in function body");
			}
		}
	}

	void Document::evaluateScripts(interp::Interpreter* cs)
	{
		for(auto it = m_objects.begin(); it != m_objects.end();)
		{
			if(auto blk = dynamic_cast<ScriptBlock*>(*it); blk != nullptr)
			{
				run_script_block(cs, blk);
				it = m_objects.erase(it);
			}
			else if(auto para = dynamic_cast<Paragraph*>(*it); para != nullptr)
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
			if(auto para = dynamic_cast<Paragraph*>(obj); para != nullptr)
				para->processWordSeparators();
			else
				sap::internal_error("boeu");
		}
	}

	void Document::addObject(std::unique_ptr<DocumentObject> obj)
	{
		m_objects.push_back(obj.get());
		m_all_objects.push_back(std::move(obj));
	}
}
