// document.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/image.h"
#include "tree/document.h"
#include "tree/paragraph.h"
#include "tree/container.h"

#include "layout/base.h"
#include "layout/document.h"

#include "interp/interp.h"      // for Interpreter
#include "interp/basedefs.h"    // for DocumentObject
#include "interp/eval_result.h" // for EvalResult

namespace sap::tree
{
	void Document::addObject(std::unique_ptr<DocumentObject> obj)
	{
		m_objects.push_back(std::move(obj));
	}
}
