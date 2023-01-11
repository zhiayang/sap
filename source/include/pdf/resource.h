// resource.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace pdf
{
	struct Name;
	struct Object;

	struct Resource
	{
		virtual ~Resource();
		zst::str_view resourceName() const;
		zst::str_view resourceKindString() const;

		virtual void serialise() const = 0;
		virtual Object* resourceObject() const = 0;

	protected:
		enum Kind
		{
			KIND_FONT = 1,
			KIND_XOBJECT = 2,
			KIND_EXTGSTATE = 3,
		};

		Resource(Kind kind);

	private:
		Kind m_kind;
		std::string m_name;
	};
}
