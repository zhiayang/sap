// compile.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "sap/config.h"
#include "sap/frontend.h"

#include "interp/interp.h"

#include "tree/document.h"
#include "tree/container.h"

#include "layout/document.h"

#include "pdf/font.h"
#include "pdf/writer.h"

namespace sap
{
	bool compile(zst::str_view input_file, zst::str_view output_file)
	{
		auto interp = interp::Interpreter();
		auto file = interp.loadFile(input_file);

		auto document = frontend::parse(input_file, file.chars());
		if(document.is_err())
			return document.error().display(), false;

		// if we do not have a start_document, we should just run the "preamble"
		// as a script and then exit.
		if(not document->haveDocStart())
		{
			auto r = document->runPreamble(&interp);
			if(r.is_err())
				return r.error().display(), false;

			return true;
		}

		// load microtype configs
		for(auto& lib_path : paths::librarySearchPaths())
		{
			auto tmp = stdfs::path(lib_path) / "data" / "microtype";
			if(not stdfs::exists(tmp))
				continue;

			for(auto& de : stdfs::directory_iterator(tmp))
			{
				if(not de.is_regular_file() || de.path().extension() != ".cfg")
					continue;

				util::log("loading microtype cfg '{}'", de.path().filename().string());

				auto cfg = interp.loadFile(de.path().string()).chars();
				while(not cfg.empty())
				{
					auto ret = config::parseMicrotypeConfig(cfg);
					if(ret.is_err())
					{
						ErrorMessage(Location::builtin(),
						    zpr::sprint("failed to load {}: {}", de.path().filename().string(), ret.error()))
						    .display();
						return false;
					}

					interp.addMicrotypeConfig(std::move(*ret));
				}
			}
		}

		auto layout_doc = document.unwrap().layout(&interp);
		if(layout_doc.is_err())
			return layout_doc.error().display(), false;

		interp.setCurrentPhase(ProcessingPhase::Render);

		auto writer = pdf::Writer(output_file);
		layout_doc.unwrap()->write(&writer);
		writer.close();

		return true;
	}

	static bool g_draft_mode = false;
	bool isDraftMode()
	{
		return g_draft_mode;
	}

	void set_draft_mode(bool draft)
	{
		g_draft_mode = draft;
	}
}
