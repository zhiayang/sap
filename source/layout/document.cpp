// document.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <cmath>

#include "pdf/file.h"  // for File
#include "pdf/font.h"  // for Font
#include "pdf/units.h" // for PdfScalar
#include "pdf/destination.h"

#include "font/font_file.h"

#include "sap/style.h"       // for Style
#include "sap/units.h"       // for Length
#include "sap/font_family.h" // for FontSet, FontStyle, FontStyle::Regular
#include "sap/annotation.h"
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
		auto sz = sap::Length(25.4 * (DocumentSettings::DEFAULT_FONT_SIZE / 72.0));

		auto font_size = settings.font_size->resolveWithoutFont(sz, sz);
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
	    : m_page_layout(this, paper_size(settings), make_margins(settings))
	{
		auto default_style = util::make<sap::Style>();

		default_style->set_font_family(*settings.font_family)
		    .set_font_style(sap::FontStyle::Regular)
		    .set_font_size(resolve_len(settings, *settings.font_size))
		    .set_root_font_size(resolve_len(settings, *settings.font_size))
		    .set_line_spacing(*settings.line_spacing)
		    .set_sentence_space_stretch(*settings.sentence_space_stretch)
		    .set_paragraph_spacing(resolve_len(settings, *settings.paragraph_spacing))
		    .set_alignment(Alignment::Justified)
		    .enable_smart_quotes(true);

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


	static pdf::OutlineItem convert_outline_item(const std::vector<pdf::Page*>& pages, sap::OutlineItem item)
	{
		if(item.position.page_num >= pages.size())
		{
			sap::internal_error("outline item '{}' out of range: page {} does not exist", item.title,
			    item.position.page_num);
		}

		auto ret = pdf::OutlineItem(std::move(item.title),
		    pdf::Destination {
		        .page = item.position.page_num,
		        .zoom = 0,
		        .position = pages[item.position.page_num]->convertVector2(item.position.pos.into()),
		    });

		for(auto& child : item.children)
			ret.addChild(convert_outline_item(pages, std::move(child)));

		return ret;
	}

	static void convert_annotation(const std::vector<pdf::Page*>& pages, sap::LinkAnnotation annot)
	{
		if(annot.position.page_num >= pages.size())
			sap::internal_error("annotation out of range: page {} does not exist", annot.position.page_num);

		auto page = pages[annot.position.page_num];
		auto pdf_annot = util::make<pdf::LinkAnnotation>(page->convertVector2(annot.position.pos.into()),
		    annot.size.into(),
		    pdf::Destination {
		        .page = annot.destination.page_num,
		        .zoom = 0,
		        .position = page->convertVector2(annot.destination.pos.into()),
		    });

		page->addAnnotation(pdf_annot);
	}

	void Document::write(pdf::Writer* stream)
	{
		auto pages = m_page_layout.render();
		for(auto& page : pages)
			m_pdf_document.addPage(page);

		for(auto& item : m_outline_items)
			m_pdf_document.addOutlineItem(convert_outline_item(pages, std::move(item)));

		for(auto& annot : m_annotations)
			convert_annotation(pages, std::move(annot));

		m_pdf_document.write(stream);
	}

	void Document::addAnnotation(LinkAnnotation annotation)
	{
		m_annotations.push_back(std::move(annotation));
	}
}







namespace sap::tree
{
	ErrorOr<std::unique_ptr<layout::Document>> Document::layout(interp::Interpreter* cs)
	{
		// the preamble is only ever run once
		cs->setCurrentPhase(ProcessingPhase::Preamble);

		if(not this->haveDocStart())
			ErrorMessage(&cs->typechecker(), "cannot layout a document with no body").showAndExit();

		cs->evaluator().pushStyle(layout::getDefaultStyle(cs));

		DocumentSettings settings {};
		{
			auto _ = cs->typechecker().pushTree(cs->typechecker().top());

			for(size_t i = 0; i < m_preamble.size(); i++)
			{
				auto& stmt = m_preamble[i];
				auto result = TRY(cs->run(stmt.get()));

				if(i + 1 == m_preamble.size())
				{
					assert(result.hasValue());

					auto val = result.take();
					settings = interp::builtin::BS_DocumentSettings::unmake(&cs->evaluator(), val).unwrap();
				}
			}
		}

		auto layout_doc = std::make_unique<layout::Document>(layout::fillDefaultSettings(cs, std::move(settings)));
		cs->evaluator().pushStyle(layout_doc->style());
		cs->evaluator().setDocument(layout_doc.get());

		size_t layout_pass = 0;
		while(true)
		{
			auto run_hooks_for_phase = [this](interp::Interpreter* cs) -> ErrorOr<void> {
				auto _ = cs->evaluator().pushBlockContext(std::nullopt);

				TRY(cs->runHooks());
				TRY(m_container->evaluateScripts(cs));

				return Ok();
			};

			cs->evaluator().commenceLayoutPass(++layout_pass);
			cs->setCurrentPhase(ProcessingPhase::Layout);
			TRY(run_hooks_for_phase(cs));

			auto available_space = Size2d(layout_doc->pageLayout().contentSize().x(), Length(INFINITY));

			auto maybe_container = TRY(m_container->createLayoutObject(cs, Style::empty(), available_space));

			if(not maybe_container.object.has_value())
				ErrorMessage(cs->evaluator().loc(), "empty document").showAndExit();

			cs->setCurrentPhase(ProcessingPhase::Position);
			TRY(run_hooks_for_phase(cs));

			auto container = std::move(*maybe_container.object);
			container->computePosition(layout_doc->pageLayout().newCursor());

			cs->setCurrentPhase(ProcessingPhase::PostLayout);
			TRY(run_hooks_for_phase(cs));

			if(cs->evaluator().layoutRequested())
				continue;

			cs->setCurrentPhase(ProcessingPhase::Finalise);
			TRY(run_hooks_for_phase(cs));

			layout_doc->pageLayout().addObject(std::move(container));
			break;
		}

		auto doc_proxy = interp::builtin::BS_DocumentProxy::unmake(&cs->evaluator(), cs->evaluator().documentProxy());
		layout_doc->outlineItems() = std::move(doc_proxy.outline_items);
		layout_doc->annotations() = std::move(doc_proxy.link_annotations);

		return Ok(std::move(layout_doc));
	}


	void Document::addObject(zst::SharedPtr<BlockObject> obj)
	{
		m_container->contents().push_back(std::move(obj));
	}

	zst::SharedPtr<tree::Container> Document::takeContainer() &&
	{
		return std::move(m_container);
	}

	std::vector<std::unique_ptr<interp::Stmt>> Document::takePreamble() &&
	{
		return std::move(m_preamble);
	}

	Document::Document(std::vector<std::unique_ptr<interp::Stmt>> preamble, bool have_doc_start)
	    : m_container(tree::Container::makeVertBox())
	    , m_preamble(std::move(preamble))
	    , m_have_document_start(have_doc_start)
	{
		m_container->setStyle(m_container->style()->with_alignment(Alignment::Justified));
	}

	BlockObject::~BlockObject()
	{
	}
}
