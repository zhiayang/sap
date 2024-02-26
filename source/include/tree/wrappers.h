// wrappers.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"
#include "interp/value.h"

namespace sap::tree
{
	struct WrappedLine : BlockObject
	{
		WrappedLine() : BlockObject(Kind::WrappedLine) { }
		explicit WrappedLine(std::vector<zst::SharedPtr<InlineObject>> objs);

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;
		void addObjects(std::vector<zst::SharedPtr<InlineObject>> objs);

		const std::vector<zst::SharedPtr<InlineObject>>& objects() const { return m_objects; }

	private:
		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
		    const Style& parent_style,
		    Size2d available_space) const override;

	private:
		std::vector<zst::SharedPtr<InlineObject>> m_objects;
	};

	/*
	    because i forgot: a deferred block is a tree::Block that holds a callback from userspace;
	    when we need to generate the LayoutObject, we call the callback (which only returns another
	    tree::Block -- we then do createLayoutObject on that returned TBO)

	    i think the intended purpose is to have "late-stage" TBOs that depend on the state of other
	    TBOs (but need to appear before them? something like that).

	    not sure if it's going to end up being useful though.
	*/
	struct DeferredBlock : BlockObject
	{
		using CallbackType = zst::SharedPtr<BlockObject>(interp::Evaluator*,
		    const interp::Value&,
		    const interp::Value&);

		explicit DeferredBlock(interp::Value context, interp::Value function, CallbackType* callback)
		    : BlockObject(Kind::DeferredBlock)
		    , m_context(std::move(context))
		    , m_function(std::move(function))
		    , m_callback(callback)
		{
		}

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;

	private:
		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
		    const Style& parent_style,
		    Size2d available_space) const override;

	private:
		interp::Value m_context;
		interp::Value m_function;
		CallbackType* m_callback;
	};
}
