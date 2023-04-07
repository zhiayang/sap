// annotation.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/destination.h"

namespace pdf
{
	struct File;
	struct Dictionary;

	struct OutlineItem
	{
		enum class State
		{
			Open,
			Closed
		};

		OutlineItem(std::string name, Destination dest, State state = State::Closed);

		void addChild(OutlineItem child);

		const std::string& name() const { return m_name; }
		State state() const { return m_default_state; }
		const std::vector<OutlineItem>& children() const { return m_children; }

		Dictionary* toDictionary(Dictionary* parent, File* file) const;
		static std::pair<Dictionary*, Dictionary*> linkChildren(const std::vector<OutlineItem>& children,
			Dictionary* parent,
			File* file);

	private:
		std::string m_name;
		Destination m_destination;
		State m_default_state;

		std::vector<OutlineItem> m_children;
	};

	struct Annotation
	{
		virtual Dictionary* toDictionary(File* file) const = 0;

	protected:
		Annotation(pdf::Position2d position, pdf::Size2d size);

		pdf::Position2d m_position;
		pdf::Size2d m_size;
	};

	struct LinkAnnotation : Annotation
	{
		LinkAnnotation(pdf::Position2d position, pdf::Size2d size, Destination dest);

		virtual Dictionary* toDictionary(File* file) const override;

	private:
		Destination m_destination;
	};
}
