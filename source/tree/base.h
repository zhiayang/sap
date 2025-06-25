// base.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/units.h"
#include "sap/style.h"
#include "sap/annotation.h"

namespace sap::layout
{
	struct PageCursor;
	struct LayoutBase;
	struct LayoutObject;
	struct LayoutSpan;
}

namespace sap::interp
{
	struct Interpreter;

	namespace ast
	{
		struct Block;
		struct FunctionCall;
	}

	namespace cst
	{
		struct Stmt;
		struct FunctionCall;
	}
}

namespace sap::tree
{
	/*
	    This defines the main tree structure for the entire system. This acts as the "AST" of the entire
	    document, and is what the metaprogram will traverse/manipulate (in the future).
	*/

	struct InlineObject;
	struct BlockObject;

	using LinkDestination = std::variant<std::monostate, //
	    layout::AbsolutePagePos,
	    InlineObject*,
	    BlockObject*,
	    layout::LayoutObject*>;

	struct LayoutResult
	{
		std::optional<std::unique_ptr<layout::LayoutObject>> object;

		// fucking hate c++
		LayoutResult(LayoutResult&&) = default;
		LayoutResult& operator=(LayoutResult&&) = default;

		~LayoutResult();

		static LayoutResult empty();
		static LayoutResult make(std::unique_ptr<layout::LayoutObject> obj);

	private:
		LayoutResult(decltype(object) x);
	};

	struct Text;
	struct Separator;
	struct InlineSpan;
	struct ScriptCall;

	struct InlineObject : zst::IntrusiveRefCounted<InlineObject>, Stylable
	{
		virtual ~InlineObject() = 0;

		void setGeneratedLayoutObject(layout::LayoutObject* obj) const { m_generated_layout_object = obj; }
		std::optional<layout::LayoutObject*> getGeneratedLayoutObject() const { return m_generated_layout_object; }

		Length raiseHeight() const { return m_raise_height; }
		void setRaiseHeight(Length raise) { m_raise_height = raise; }
		void addRaiseHeight(Length raise) { m_raise_height += raise; }

		LinkDestination linkDestination() const;
		void setLinkDestination(LinkDestination dest);
		void copyAttributesFrom(const InlineObject& obj);

		InlineSpan* parentSpan() const { return m_parent_span; }
		void setParentSpan(InlineSpan* span) { m_parent_span = span; }

		bool isSpan() const { return m_kind == Kind::Span; }
		bool isText() const { return m_kind == Kind::Text; }
		bool isSeparator() const { return m_kind == Kind::Separator; }
		bool isScriptCall() const { return m_kind == Kind::ScriptCall; }

		const Text* castToText() const;
		const Separator* castToSeparator() const;
		const InlineSpan* castToSpan() const;
		const ScriptCall* castToScriptCall() const;

		Text* castToText();
		Separator* castToSeparator();
		InlineSpan* castToSpan();
		ScriptCall* castToScriptCall();

		static void* operator new(size_t count);
		static void operator delete(void* ptr, size_t count);

	protected:
		enum class Kind
		{
			Text,
			Separator,
			Span,
			ScriptCall,
		};

		InlineObject(Kind kind) : m_kind(kind) { }

		Kind m_kind;
		mutable std::optional<layout::LayoutObject*> m_generated_layout_object {};

	private:
		Length m_raise_height = 0;
		LinkDestination m_link_destination {};
		InlineSpan* m_parent_span = nullptr;
	};

	struct InlineSpan : InlineObject
	{
		explicit InlineSpan(bool glue);
		explicit InlineSpan(bool glue, zst::SharedPtr<InlineObject> obj);
		explicit InlineSpan(bool glue, std::vector<zst::SharedPtr<InlineObject>> objs);

		void addObject(zst::SharedPtr<InlineObject> obj);
		void addObjects(std::vector<zst::SharedPtr<InlineObject>> objs);

		const std::vector<zst::SharedPtr<InlineObject>>& objects() const { return m_objects; }
		std::vector<zst::SharedPtr<InlineObject>>& objects() { return m_objects; }

		bool canSplit() const { return not m_override_width.has_value() and not m_is_glued; }

		void overrideWidth(Length length) { m_override_width = length; }
		bool hasOverriddenWidth() const { return m_override_width.has_value(); }
		std::optional<Length> getOverriddenWidth() const { return m_override_width; }

		void addGeneratedLayoutSpan(layout::LayoutSpan* span) const;
		const std::vector<layout::LayoutSpan*>& generatedLayoutSpans() { return m_generated_layout_spans; }

	private:
		std::vector<zst::SharedPtr<InlineObject>> m_objects;
		std::optional<Length> m_override_width {};
		bool m_is_glued = false;

		mutable std::vector<layout::LayoutSpan*> m_generated_layout_spans {};

		friend InlineObject;
	};

	struct Image;
	struct Spacer;
	struct RawBlock;
	struct Container;
	struct Paragraph;
	struct ScriptBlock;
	struct WrappedLine;

	struct BlockObject : zst::IntrusiveRefCounted<BlockObject>, Stylable
	{
		virtual ~BlockObject() = 0;

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const = 0;
		virtual ErrorOr<LayoutResult> createLayoutObject(interp::Interpreter* cs,
		    const Style& parent_style,
		    Size2d available_space) const final;

		std::optional<layout::LayoutObject*> getGeneratedLayoutObject() const { return m_generated_layout_object; }

		void overrideLayoutWidth(Length x);
		void overrideLayoutHeight(Length y);

		void offsetRelativePosition(Size2d offset);
		void overrideAbsolutePosition(layout::AbsolutePagePos pos);

		LinkDestination linkDestination() const;
		void setLinkDestination(LinkDestination dest);

		bool isPath() const { return m_kind == Kind::Path; }
		bool isImage() const { return m_kind == Kind::Image; }
		bool isSpacer() const { return m_kind == Kind::Spacer; }
		bool isRawBlock() const { return m_kind == Kind::RawBlock; }
		bool isContainer() const { return m_kind == Kind::Container; }
		bool isParagraph() const { return m_kind == Kind::Paragraph; }
		bool isScriptBlock() const { return m_kind == Kind::ScriptBlock; }
		bool isWrappedLine() const { return m_kind == Kind::WrappedLine; }

		Image* castToImage();
		Spacer* castToSpacer();
		RawBlock* castToRawBlock();
		Container* castToContainer();
		Paragraph* castToParagraph();
		ScriptBlock* castToScriptBlock();
		WrappedLine* castToWrappedLine();
		const Image* castToImage() const;
		const Spacer* castToSpacer() const;
		const RawBlock* castToRawBlock() const;
		const Container* castToContainer() const;
		const Paragraph* castToParagraph() const;
		const ScriptBlock* castToScriptBlock() const;
		const WrappedLine* castToWrappedLine() const;

		static void* operator new(size_t count);
		static void operator delete(void* ptr, size_t count);

	protected:
		enum class Kind
		{
			Path,
			Image,
			Spacer,
			RawBlock,
			Container,
			Paragraph,
			ScriptBlock,
			WrappedLine,
			DeferredBlock,
		};

		BlockObject(Kind kind) : m_kind(kind) { }

		Kind m_kind;
		mutable std::optional<layout::LayoutObject*> m_generated_layout_object {};

	private:
		virtual ErrorOr<LayoutResult>
		create_layout_object_impl(interp::Interpreter* cs, const Style& parent_style, Size2d available_space) const = 0;

	protected:
		std::optional<Length> m_override_width {};
		std::optional<Length> m_override_height {};
		std::optional<Size2d> m_rel_position_offset {};
		std::optional<layout::AbsolutePagePos> m_abs_position_override {};

		LinkDestination m_link_destination {};
	};

	/*
	    Since inline objects are not document objects, we do not have a diamond problem by
	    inheriting both of them. We want this hierarchy because script blocks and calls can
	    appear both at the top-level (with blocks or floats), and within blocks/floats as well.
	*/
	struct ScriptObject
	{
		explicit ScriptObject(ProcessingPhase phase) : m_run_phase(phase) { }
		ProcessingPhase runPhase() const { return m_run_phase; }

	protected:
		ProcessingPhase m_run_phase = ProcessingPhase::Layout;
	};

	struct ScriptBlock : BlockObject, ScriptObject
	{
		explicit ScriptBlock(ProcessingPhase phase);

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;

		std::unique_ptr<interp::ast::Block> body;

	private:
		mutable std::unique_ptr<interp::cst::Stmt> m_typechecked_stmt;

		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
		    const Style& parent_style,
		    Size2d available_space) const override;
	};

	struct ScriptCall : InlineObject, ScriptObject
	{
		explicit ScriptCall(ProcessingPhase phase);

		ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const;
		ErrorOr<void> typecheckCall(interp::Typechecker* ts) const;
		interp::cst::FunctionCall* getTypecheckedFunctionCall() const { return m_typechecked_call.get(); }

		std::unique_ptr<interp::ast::FunctionCall> call;

	private:
		using ScriptEvalResult = Either<zst::SharedPtr<InlineSpan>, std::unique_ptr<layout::LayoutObject>>;
		ErrorOr<std::optional<ScriptEvalResult>> evaluate_script(interp::Interpreter* cs,
		    const Style& parent_style,
		    Size2d available_space) const;

		ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
		    const Style& parent_style,
		    Size2d available_space) const;

		// befriend Paragraph so it can use our evaluate_script
		friend struct Paragraph;

	private:
		mutable std::unique_ptr<interp::cst::FunctionCall> m_typechecked_call;
		mutable std::vector<zst::SharedPtr<BlockObject>> m_created_tbos;
	};


	ErrorOr<std::vector<zst::SharedPtr<InlineObject>>> processWordSeparators(std::vector<
	    zst::SharedPtr<InlineObject>> vec);

	ErrorOr<std::vector<zst::SharedPtr<InlineObject>>> performReplacements(const Style& parent_style,
	    std::vector<zst::SharedPtr<InlineObject>> vec);
}
