// document.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/image.h"
#include "tree/document.h"
#include "tree/paragraph.h"
#include "tree/container.h"

#include "interp/interp.h"      // for Interpreter
#include "interp/basedefs.h"    // for DocumentObject
#include "interp/eval_result.h" // for EvalResult

namespace sap::tree
{
	static void run_script_block(interp::Interpreter* cs, ScriptBlock* script_block)
	{
		if(auto ret = cs->run(script_block->body.get()); ret.is_err())
			error("interp", "{}", ret.take_error());

		// TODO: do something with the result here.
	}


	void Document::evaluate_scripts(interp::Interpreter* cs, std::vector<DocumentObject*>& ret_objs, DocumentObject* obj)
	{
		if(auto blk = dynamic_cast<ScriptBlock*>(obj); blk != nullptr)
		{
			run_script_block(cs, blk);
		}
		else if(auto call = dynamic_cast<ScriptCall*>(obj); call != nullptr)
		{
			auto value_or_err = cs->run(call->call.get());

			if(value_or_err.is_err())
				error("interp", "evaluation failed: {}", value_or_err.error());

			auto value_or_empty = value_or_err.take_value();
			if(not value_or_empty.hasValue())
				return;

			if(value_or_empty.get().isTreeBlockObj())
			{
				auto obj = std::move(value_or_empty.get()).takeTreeBlockObj();
				ret_objs.push_back(obj.get());
				m_all_objects.push_back(std::move(obj));
			}
			else
			{
				auto tmp = cs->evaluator().convertValueToText(std::move(value_or_empty).take());
				if(tmp.is_err())
					error("interp", "convertion to text failed: {}", tmp.error());

				auto objs = tmp.take_value();
				auto new_para = std::make_unique<Paragraph>();
				new_para->addObjects(std::move(objs));

				ret_objs.push_back(new_para.get());
				m_all_objects.push_back(std::move(new_para));
			}
		}
		else if(auto para = dynamic_cast<Paragraph*>(obj); para != nullptr)
		{
			para->evaluateScripts(cs);
			ret_objs.push_back(para);
		}
		else if(auto img = dynamic_cast<Image*>(obj); img != nullptr)
		{
			// do nothing
			ret_objs.push_back(img);
		}
		else if(auto bc = dynamic_cast<BlockContainer*>(obj); bc != nullptr)
		{
			for(auto& obj : bc->contents())
				this->evaluate_scripts(cs, ret_objs, obj.get());
		}
		else
		{
			sap::internal_error("unsupported! {}", typeid((obj)).name());
		}
	}




	void Document::evaluateScripts(interp::Interpreter* cs)
	{
		std::vector<DocumentObject*> ret {};
		ret.reserve(m_objects.size());

		for(auto& obj : m_objects)
			this->evaluate_scripts(cs, ret, obj);

		m_objects.swap(ret);
	}


	bool Document::process_word_separators(DocumentObject* obj)
	{
		if(auto para = dynamic_cast<Paragraph*>(obj); para != nullptr)
		{
			para->processWordSeparators();
			return not para->contents().empty();
		}
		else if(auto img = dynamic_cast<Image*>(obj); img != nullptr)
		{
			// always keep
			return true;
		}
		else if(auto bc = dynamic_cast<BlockContainer*>(obj); bc != nullptr)
		{
			bool keep_any = false;
			for(auto& obj : bc->contents())
				keep_any |= process_word_separators(obj.get());

			return keep_any;
		}
		else if(auto cc = dynamic_cast<CentredContainer*>(obj); cc != nullptr)
		{
			return process_word_separators(&cc->inner());
		}
		else
		{
			sap::internal_error("?? unsupported");
		}
	}


	void Document::processWordSeparators()
	{
		for(auto it = m_objects.begin(); it != m_objects.end();)
		{
			auto keep = process_word_separators(*it);
			if(keep)
				++it;
			else
				it = m_objects.erase(it);
		}
	}

	void Document::addObject(std::unique_ptr<DocumentObject> obj)
	{
		m_objects.push_back(obj.get());
		m_all_objects.push_back(std::move(obj));
	}
}
