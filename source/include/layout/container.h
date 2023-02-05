// container.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "layout/layout_object.h"

namespace sap::layout
{
	struct LayoutBase;

	struct Container : LayoutObject
	{
		Container(RelativePos position, Size2d size);

		void addObject(std::unique_ptr<LayoutObject> obj);
		std::vector<std::unique_ptr<LayoutObject>>& objects();
		const std::vector<std::unique_ptr<LayoutObject>>& objects() const;

		virtual void render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

	private:
		std::vector<std::unique_ptr<LayoutObject>> m_objects;
	};
}
