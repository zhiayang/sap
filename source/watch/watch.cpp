// watch.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "kqueue.h"

namespace sap::watch
{
	bool isSupportedPlatform()
	{
#if defined(SAP_HAVE_WATCH)
		return true;
#else
		return false;
#endif
	}
}
