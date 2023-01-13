// container.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"

namespace sap::tree
{
	struct BlockContainer : BlockObject
	{
		virtual std::optional<LayoutFn> getLayoutFunction() const override;

		std::vector<std::unique_ptr<BlockObject>>& contents() { return m_objects; }
		const std::vector<std::unique_ptr<BlockObject>>& contents() const { return m_objects; }

	private:
		static layout::LineCursor layout_fn(interp::Interpreter* cs, layout::LayoutBase* layout, layout::LineCursor cursor,
		    const Style* style, const DocumentObject* obj);

	private:
		std::vector<std::unique_ptr<BlockObject>> m_objects;
	};

	struct CentredContainer : BlockObject
	{
		explicit CentredContainer(std::unique_ptr<BlockObject> inner);

		virtual std::optional<LayoutFn> getLayoutFunction() const override;

		const BlockObject& inner() const { return *m_inner.get(); }

	private:
		static layout::LineCursor layout_fn(interp::Interpreter* cs, layout::LayoutBase* layout, layout::LineCursor cursor,
		    const Style* style, const DocumentObject* obj);

	private:
		std::unique_ptr<BlockObject> m_inner;
	};

}
