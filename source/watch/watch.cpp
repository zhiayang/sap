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

#if !defined(SAP_HAVE_WATCH)
	StrErrorOr<void> addFileToWatchList(zst::str_view path)
	{
		(void) path;
		return Ok();
	}

	void start(zst::str_view main_file, zst::str_view output_file)
	{
		(void) main_file;
		(void) output_file;
		abort();
	}
#endif
}
