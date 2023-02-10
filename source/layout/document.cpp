// document.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <cmath>

#include "pdf/file.h"  // for File
#include "pdf/font.h"  // for Font
#include "pdf/units.h" // for PdfScalar

#include "font/font_file.h"

#include "sap/style.h"       // for Style
#include "sap/units.h"       // for Length
#include "sap/font_family.h" // for FontSet, FontStyle, FontStyle::Regular
#include "sap/document_settings.h"

#include "tree/document.h"
#include "tree/container.h"

#include "interp/interp.h"
#include "interp/basedefs.h" // for DocumentObject
#include "interp/builtin_types.h"

#include "layout/base.h"      // for Cursor, LayoutObject, Interpreter, Rec...
#include "layout/image.h"     //
#include "layout/document.h"  // for Document
#include "layout/paragraph.h" // for Paragraph

namespace sap::layout
{
	const Style* getDefaultStyle(interp::Interpreter* cs);
	DocumentSettings fillDefaultSettings(interp::Interpreter* cs, DocumentSettings settings);

	static Length resolve_len(const DocumentSettings& settings, DynLength len)
	{
		auto pt = [](auto x) {
			return sap::Length(25.4 * (x / 72.0));
		};

		auto font_size = settings.font_size->resolveWithoutFont(pt(12), pt(12));
		return len.resolveWithoutFont(font_size, font_size);
	}

	static Size2d paper_size(const DocumentSettings& settings)
	{
		return Size2d {
			resolve_len(settings, settings.paper_size->x),
			resolve_len(settings, settings.paper_size->y),
		};
	}

	static PageLayout::Margins make_margins(const DocumentSettings& settings)
	{
		return PageLayout::Margins {
			.top = resolve_len(settings, *settings.margins->top),
			.bottom = resolve_len(settings, *settings.margins->bottom),
			.left = resolve_len(settings, *settings.margins->left),
			.right = resolve_len(settings, *settings.margins->right),
		};
	}

	Document::Document(const DocumentSettings& settings)
		: m_page_layout(PageLayout(paper_size(settings), make_margins(settings)))
	{
		auto default_style = util::make<sap::Style>();

		default_style->set_font_family(*settings.font_family)
			.set_font_style(sap::FontStyle::Regular)
			.set_font_size(resolve_len(settings, *settings.font_size))
			.set_root_font_size(resolve_len(settings, *settings.font_size))
			.set_line_spacing(*settings.line_spacing)
			.set_paragraph_spacing(resolve_len(settings, *settings.paragraph_spacing))
			.set_alignment(Alignment::Justified);

		this->setStyle(default_style);
	}


	pdf::File& Document::pdf()
	{
		return m_pdf_document;
	}

	const pdf::File& Document::pdf() const
	{
		return m_pdf_document;
	}

	void Document::addObject(std::unique_ptr<LayoutObject> obj)
	{
		m_objects.push_back(std::move(obj));
	}

	void Document::write(pdf::Writer* stream)
	{
		auto pages = m_page_layout.render();
		for(auto& page : pages)
			m_pdf_document.addPage(page);

		m_pdf_document.write(stream);
	}
}

namespace sap::tree
{
	std::unique_ptr<layout::Document> Document::layout(interp::Interpreter* cs)
	{
		// the preamble is only ever run once
		cs->setCurrentPhase(ProcessingPhase::Preamble);

		if(m_document_start == nullptr)
			ErrorMessage(&cs->typechecker(), "cannot layout a document with no body").showAndExit();

		cs->evaluator().pushStyle(layout::getDefaultStyle(cs));

		if(auto e = cs->run(m_preamble.get()); e.is_err())
			e.error().showAndExit();

		DocumentSettings settings {};
		if(auto e = cs->run(m_document_start.get()); e.ok())
		{
			auto val = e.take_value().take();
			settings = interp::builtin::BS_DocumentSettings::unmake(&cs->evaluator(), val).unwrap();
		}
		else
		{
			e.error().showAndExit();
		}

		auto layout_doc = std::make_unique<layout::Document>(layout::fillDefaultSettings(cs, std::move(settings)));
		cs->evaluator().pushStyle(layout_doc->style());
		cs->evaluator().setPageLayout(&layout_doc->pageLayout());

		size_t layout_pass = 0;
		while(true)
		{
			auto run_hooks_for_phase = [this](interp::Interpreter* cs) {
				auto _ = cs->evaluator().pushBlockContext(std::nullopt);
				if(auto e = cs->runHooks(); e.is_err())
					e.error().showAndExit();

				m_container->evaluateScripts(cs);
			};


			cs->evaluator().commenceLayoutPass(++layout_pass);
			cs->setCurrentPhase(ProcessingPhase::Layout);

			auto available_space = Size2d(layout_doc->pageLayout().contentSize().x(), Length(INFINITY));

			auto cursor = layout_doc->pageLayout().newCursor();
			auto container_or_err = m_container->createLayoutObject(cs, layout_doc->style(), available_space);
			if(container_or_err.is_err())
				container_or_err.error().showAndExit();

			if(not container_or_err->object.has_value())
				ErrorMessage(cs->evaluator().loc(), "empty document").showAndExit();

			cs->setCurrentPhase(ProcessingPhase::Position);
			run_hooks_for_phase(cs);

			auto container = std::move(*container_or_err.unwrap().object);
			container->positionChildren(cursor);

			cs->setCurrentPhase(ProcessingPhase::PostLayout);
			run_hooks_for_phase(cs);

			if(cs->evaluator().layoutRequested())
				continue;

			layout_doc->pageLayout().addObject(std::move(container));
			break;
		}

		return layout_doc;
	}


	void Document::addObject(std::unique_ptr<BlockObject> obj)
	{
		m_container->contents().push_back(std::move(obj));
	}

	std::unique_ptr<tree::Container> Document::takeContainer() &&
	{
		return std::move(m_container);
	}

	std::unique_ptr<interp::Block> Document::takePreamble() &&
	{
		return std::move(m_preamble);
	}

	std::unique_ptr<interp::FunctionCall> Document::takeDocStart() &&
	{
		return std::move(m_document_start);
	}

	Document::Document(std::unique_ptr<interp::Block> preamble, std::unique_ptr<interp::FunctionCall> doc_start)
		: m_container(tree::Container::makeVertBox())
		, m_preamble(std::move(preamble))
		, m_document_start(std::move(doc_start))
	{
	}

	BlockObject::~BlockObject()
	{
	}
}
