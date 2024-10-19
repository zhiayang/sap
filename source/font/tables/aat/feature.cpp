// feature.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "font/aat.h"
#include "font/features.h"

namespace font
{
	// copied from https://developer.apple.com/fonts/TrueType-Reference-Manual/RM09/AppendixF.html
	enum
	{
		kAllTypographicFeaturesType = 0,
		kLigaturesType = 1,
		kCursiveConnectionType = 2,
		kLetterCaseType = 3, /* deprecated - use kLowerCaseType or kUpperCaseType instead */
		kVerticalSubstitutionType = 4,
		kLinguisticRearrangementType = 5,
		kNumberSpacingType = 6,
		kSmartSwashType = 8,
		kDiacriticsType = 9,
		kVerticalPositionType = 10,
		kFractionsType = 11,
		kOverlappingCharactersType = 13,
		kTypographicExtrasType = 14,
		kMathematicalExtrasType = 15,
		kOrnamentSetsType = 16,
		kCharacterAlternativesType = 17,
		kDesignComplexityType = 18,
		kStyleOptionsType = 19,
		kCharacterShapeType = 20,
		kNumberCaseType = 21,
		kTextSpacingType = 22,
		kTransliterationType = 23,
		kAnnotationType = 24,
		kKanaSpacingType = 25,
		kIdeographicSpacingType = 26,
		kUnicodeDecompositionType = 27,
		kRubyKanaType = 28,
		kCJKSymbolAlternativesType = 29,
		kIdeographicAlternativesType = 30,
		kCJKVerticalRomanPlacementType = 31,
		kItalicCJKRomanType = 32,
		kCaseSensitiveLayoutType = 33,
		kAlternateKanaType = 34,
		kStylisticAlternativesType = 35,
		kContextualAlternatesType = 36,
		kLowerCaseType = 37,
		kUpperCaseType = 38,
		kLanguageTagType = 39,
		kCJKRomanSpacingType = 103,
		kLastFeatureType = -1
	};

	// Selectors for feature type kAllTypographicFeaturesType
	enum
	{
		kAllTypeFeaturesOnSelector = 0,
		kAllTypeFeaturesOffSelector = 1
	};

	// Selectors for feature type kLigaturesType
	enum
	{
		kRequiredLigaturesOnSelector = 0,
		kRequiredLigaturesOffSelector = 1,
		kCommonLigaturesOnSelector = 2,
		kCommonLigaturesOffSelector = 3,
		kRareLigaturesOnSelector = 4,
		kRareLigaturesOffSelector = 5,
		kLogosOnSelector = 6,
		kLogosOffSelector = 7,
		kRebusPicturesOnSelector = 8,
		kRebusPicturesOffSelector = 9,
		kDiphthongLigaturesOnSelector = 10,
		kDiphthongLigaturesOffSelector = 11,
		kSquaredLigaturesOnSelector = 12,
		kSquaredLigaturesOffSelector = 13,
		kAbbrevSquaredLigaturesOnSelector = 14,
		kAbbrevSquaredLigaturesOffSelector = 15,
		kSymbolLigaturesOnSelector = 16,
		kSymbolLigaturesOffSelector = 17,
		kContextualLigaturesOnSelector = 18,
		kContextualLigaturesOffSelector = 19,
		kHistoricalLigaturesOnSelector = 20,
		kHistoricalLigaturesOffSelector = 21
	};

	// Selectors for feature type kCursiveConnectionType
	enum
	{
		kUnconnectedSelector = 0,
		kPartiallyConnectedSelector = 1,
		kCursiveSelector = 2
	};

	// Selectors for feature type kLetterCaseType
	enum
	{
		kUpperAndLowerCaseSelector = 0,      /* deprecated */
		kAllCapsSelector = 1,                /* deprecated */
		kAllLowerCaseSelector = 2,           /* deprecated */
		kSmallCapsSelector = 3,              /* deprecated */
		kInitialCapsSelector = 4,            /* deprecated */
		kInitialCapsAndSmallCapsSelector = 5 /* deprecated */
	};

	// Selectors for feature type kVerticalSubstitutionType
	enum
	{
		kSubstituteVerticalFormsOnSelector = 0,
		kSubstituteVerticalFormsOffSelector = 1
	};

	// Selectors for feature type kLinguisticRearrangementType
	enum
	{
		kLinguisticRearrangementOnSelector = 0,
		kLinguisticRearrangementOffSelector = 1
	};

	// Selectors for feature type kNumberSpacingType
	enum
	{
		kMonospacedNumbersSelector = 0,
		kProportionalNumbersSelector = 1,
		kThirdWidthNumbersSelector = 2,
		kQuarterWidthNumbersSelector = 3
	};

	// Selectors for feature type kSmartSwashType
	enum
	{
		kWordInitialSwashesOnSelector = 0,
		kWordInitialSwashesOffSelector = 1,
		kWordFinalSwashesOnSelector = 2,
		kWordFinalSwashesOffSelector = 3,
		kLineInitialSwashesOnSelector = 4,
		kLineInitialSwashesOffSelector = 5,
		kLineFinalSwashesOnSelector = 6,
		kLineFinalSwashesOffSelector = 7,
		kNonFinalSwashesOnSelector = 8,
		kNonFinalSwashesOffSelector = 9
	};

	// Selectors for feature type kDiacriticsType
	enum
	{
		kShowDiacriticsSelector = 0,
		kHideDiacriticsSelector = 1,
		kDecomposeDiacriticsSelector = 2
	};

	// Selectors for feature type kVerticalPositionType
	enum
	{
		kNormalPositionSelector = 0,
		kSuperiorsSelector = 1,
		kInferiorsSelector = 2,
		kOrdinalsSelector = 3,
		kScientificInferiorsSelector = 4
	};

	// Selectors for feature type kFractionsType
	enum
	{
		kNoFractionsSelector = 0,
		kVerticalFractionsSelector = 1,
		kDiagonalFractionsSelector = 2
	};

	// Selectors for feature type kOverlappingCharactersType
	enum
	{
		kPreventOverlapOnSelector = 0,
		kPreventOverlapOffSelector = 1
	};

	// Selectors for feature type kTypographicExtrasType
	enum
	{
		kHyphensToEmDashOnSelector = 0,
		kHyphensToEmDashOffSelector = 1,
		kHyphenToEnDashOnSelector = 2,
		kHyphenToEnDashOffSelector = 3,
		kSlashedZeroOnSelector = 4,
		kSlashedZeroOffSelector = 5,
		kFormInterrobangOnSelector = 6,
		kFormInterrobangOffSelector = 7,
		kSmartQuotesOnSelector = 8,
		kSmartQuotesOffSelector = 9,
		kPeriodsToEllipsisOnSelector = 10,
		kPeriodsToEllipsisOffSelector = 11
	};

	// Selectors for feature type kMathematicalExtrasType
	enum
	{
		kHyphenToMinusOnSelector = 0,
		kHyphenToMinusOffSelector = 1,
		kAsteriskToMultiplyOnSelector = 2,
		kAsteriskToMultiplyOffSelector = 3,
		kSlashToDivideOnSelector = 4,
		kSlashToDivideOffSelector = 5,
		kInequalityLigaturesOnSelector = 6,
		kInequalityLigaturesOffSelector = 7,
		kExponentsOnSelector = 8,
		kExponentsOffSelector = 9,
		kMathematicalGreekOnSelector = 10,
		kMathematicalGreekOffSelector = 11
	};

	// Selectors for feature type kOrnamentSetsType
	enum
	{
		kNoOrnamentsSelector = 0,
		kDingbatsSelector = 1,
		kPiCharactersSelector = 2,
		kFleuronsSelector = 3,
		kDecorativeBordersSelector = 4,
		kInternationalSymbolsSelector = 5,
		kMathSymbolsSelector = 6
	};

	// Selectors for feature type kCharacterAlternativesType
	enum
	{
		kNoAlternatesSelector = 0
	};

	// Selectors for feature type kDesignComplexityType
	enum
	{
		kDesignLevel1Selector = 0,
		kDesignLevel2Selector = 1,
		kDesignLevel3Selector = 2,
		kDesignLevel4Selector = 3,
		kDesignLevel5Selector = 4
	};

	// Selectors for feature type kStyleOptionsType
	enum
	{
		kNoStyleOptionsSelector = 0,
		kDisplayTextSelector = 1,
		kEngravedTextSelector = 2,
		kIlluminatedCapsSelector = 3,
		kTitlingCapsSelector = 4,
		kTallCapsSelector = 5
	};

	// Selectors for feature type kCharacterShapeType
	enum
	{
		kTraditionalCharactersSelector = 0,
		kSimplifiedCharactersSelector = 1,
		kJIS1978CharactersSelector = 2,
		kJIS1983CharactersSelector = 3,
		kJIS1990CharactersSelector = 4,
		kTraditionalAltOneSelector = 5,
		kTraditionalAltTwoSelector = 6,
		kTraditionalAltThreeSelector = 7,
		kTraditionalAltFourSelector = 8,
		kTraditionalAltFiveSelector = 9,
		kExpertCharactersSelector = 10,
		kJIS2004CharactersSelector = 11,
		kHojoCharactersSelector = 12,
		kNLCCharactersSelector = 13,
		kTraditionalNamesCharactersSelector = 14
	};

	// Selectors for feature type kNumberCaseType
	enum
	{
		kLowerCaseNumbersSelector = 0,
		kUpperCaseNumbersSelector = 1
	};

	// Selectors for feature type kTextSpacingType
	enum
	{
		kProportionalTextSelector = 0,
		kMonospacedTextSelector = 1,
		kHalfWidthTextSelector = 2,
		kThirdWidthTextSelector = 3,
		kQuarterWidthTextSelector = 4,
		kAltProportionalTextSelector = 5,
		kAltHalfWidthTextSelector = 6
	};

	// Selectors for feature type kTransliterationType
	enum
	{
		kNoTransliterationSelector = 0,
		kHanjaToHangulSelector = 1,
		kHiraganaToKatakanaSelector = 2,
		kKatakanaToHiraganaSelector = 3,
		kKanaToRomanizationSelector = 4,
		kRomanizationToHiraganaSelector = 5,
		kRomanizationToKatakanaSelector = 6,
		kHanjaToHangulAltOneSelector = 7,
		kHanjaToHangulAltTwoSelector = 8,
		kHanjaToHangulAltThreeSelector = 9
	};

	// Selectors for feature type kAnnotationType
	enum
	{
		kNoAnnotationSelector = 0,
		kBoxAnnotationSelector = 1,
		kRoundedBoxAnnotationSelector = 2,
		kCircleAnnotationSelector = 3,
		kInvertedCircleAnnotationSelector = 4,
		kParenthesisAnnotationSelector = 5,
		kPeriodAnnotationSelector = 6,
		kRomanNumeralAnnotationSelector = 7,
		kDiamondAnnotationSelector = 8,
		kInvertedBoxAnnotationSelector = 9,
		kInvertedRoundedBoxAnnotationSelector = 10
	};

	// Selectors for feature type kKanaSpacingType
	enum
	{
		kFullWidthKanaSelector = 0,
		kProportionalKanaSelector = 1
	};

	// Selectors for feature type kIdeographicSpacingType
	enum
	{
		kFullWidthIdeographsSelector = 0,
		kProportionalIdeographsSelector = 1,
		kHalfWidthIdeographsSelector = 2
	};

	// Selectors for feature type kUnicodeDecompositionType
	enum
	{
		kCanonicalCompositionOnSelector = 0,
		kCanonicalCompositionOffSelector = 1,
		kCompatibilityCompositionOnSelector = 2,
		kCompatibilityCompositionOffSelector = 3,
		kTranscodingCompositionOnSelector = 4,
		kTranscodingCompositionOffSelector = 5
	};

	// Selectors for feature type kRubyKanaType
	enum
	{
		kNoRubyKanaSelector = 0, /* deprecated - use kRubyKanaOffSelector instead */
		kRubyKanaSelector = 1,   /* deprecated - use kRubyKanaOnSelector instead */
		kRubyKanaOnSelector = 2,
		kRubyKanaOffSelector = 3
	};

	// Selectors for feature type kCJKSymbolAlternativesType
	enum
	{
		kNoCJKSymbolAlternativesSelector = 0,
		kCJKSymbolAltOneSelector = 1,
		kCJKSymbolAltTwoSelector = 2,
		kCJKSymbolAltThreeSelector = 3,
		kCJKSymbolAltFourSelector = 4,
		kCJKSymbolAltFiveSelector = 5
	};

	// Selectors for feature type kIdeographicAlternativesType
	enum
	{
		kNoIdeographicAlternativesSelector = 0,
		kIdeographicAltOneSelector = 1,
		kIdeographicAltTwoSelector = 2,
		kIdeographicAltThreeSelector = 3,
		kIdeographicAltFourSelector = 4,
		kIdeographicAltFiveSelector = 5
	};

	// Selectors for feature type kCJKVerticalRomanPlacementType
	enum
	{
		kCJKVerticalRomanCenteredSelector = 0,
		kCJKVerticalRomanHBaselineSelector = 1
	};

	// Selectors for feature type kItalicCJKRomanType
	enum
	{
		kNoCJKItalicRomanSelector = 0, /* deprecated - use kCJKItalicRomanOffSelector instead */
		kCJKItalicRomanSelector = 1,   /* deprecated - use kCJKItalicRomanOnSelector instead */
		kCJKItalicRomanOnSelector = 2,
		kCJKItalicRomanOffSelector = 3
	};

	// Selectors for feature type kCaseSensitiveLayoutType
	enum
	{
		kCaseSensitiveLayoutOnSelector = 0,
		kCaseSensitiveLayoutOffSelector = 1,
		kCaseSensitiveSpacingOnSelector = 2,
		kCaseSensitiveSpacingOffSelector = 3
	};

	// Selectors for feature type kAlternateKanaType
	enum
	{
		kAlternateHorizKanaOnSelector = 0,
		kAlternateHorizKanaOffSelector = 1,
		kAlternateVertKanaOnSelector = 2,
		kAlternateVertKanaOffSelector = 3
	};

	// Selectors for feature type kStylisticAlternativesType
	enum
	{
		kNoStylisticAlternatesSelector = 0,
		kStylisticAltOneOnSelector = 2,
		kStylisticAltOneOffSelector = 3,
		kStylisticAltTwoOnSelector = 4,
		kStylisticAltTwoOffSelector = 5,
		kStylisticAltThreeOnSelector = 6,
		kStylisticAltThreeOffSelector = 7,
		kStylisticAltFourOnSelector = 8,
		kStylisticAltFourOffSelector = 9,
		kStylisticAltFiveOnSelector = 10,
		kStylisticAltFiveOffSelector = 11,
		kStylisticAltSixOnSelector = 12,
		kStylisticAltSixOffSelector = 13,
		kStylisticAltSevenOnSelector = 14,
		kStylisticAltSevenOffSelector = 15,
		kStylisticAltEightOnSelector = 16,
		kStylisticAltEightOffSelector = 17,
		kStylisticAltNineOnSelector = 18,
		kStylisticAltNineOffSelector = 19,
		kStylisticAltTenOnSelector = 20,
		kStylisticAltTenOffSelector = 21,
		kStylisticAltElevenOnSelector = 22,
		kStylisticAltElevenOffSelector = 23,
		kStylisticAltTwelveOnSelector = 24,
		kStylisticAltTwelveOffSelector = 25,
		kStylisticAltThirteenOnSelector = 26,
		kStylisticAltThirteenOffSelector = 27,
		kStylisticAltFourteenOnSelector = 28,
		kStylisticAltFourteenOffSelector = 29,
		kStylisticAltFifteenOnSelector = 30,
		kStylisticAltFifteenOffSelector = 31,
		kStylisticAltSixteenOnSelector = 32,
		kStylisticAltSixteenOffSelector = 33,
		kStylisticAltSeventeenOnSelector = 34,
		kStylisticAltSeventeenOffSelector = 35,
		kStylisticAltEighteenOnSelector = 36,
		kStylisticAltEighteenOffSelector = 37,
		kStylisticAltNineteenOnSelector = 38,
		kStylisticAltNineteenOffSelector = 39,
		kStylisticAltTwentyOnSelector = 40,
		kStylisticAltTwentyOffSelector = 41
	};

	// Selectors for feature type kContextualAlternatesType
	enum
	{
		kContextualAlternatesOnSelector = 0,
		kContextualAlternatesOffSelector = 1,
		kSwashAlternatesOnSelector = 2,
		kSwashAlternatesOffSelector = 3,
		kContextualSwashAlternatesOnSelector = 4,
		kContextualSwashAlternatesOffSelector = 5
	};

	// Selectors for feature type kLowerCaseType
	enum
	{
		kDefaultLowerCaseSelector = 0,
		kLowerCaseSmallCapsSelector = 1,
		kLowerCasePetiteCapsSelector = 2
	};

	// Selectors for feature type kUpperCaseType
	enum
	{
		kDefaultUpperCaseSelector = 0,
		kUpperCaseSmallCapsSelector = 1,
		kUpperCasePetiteCapsSelector = 2
	};

	// Selectors for feature type kCJKRomanSpacingType
	enum
	{
		kHalfWidthCJKRomanSelector = 0,
		kProportionalCJKRomanSelector = 1,
		kDefaultCJKRomanSelector = 2,
		kFullWidthCJKRomanSelector = 3
	};

	inline constexpr aat::Feature f(uint16_t type, uint16_t selector)
	{
		return aat::Feature { .type = type, .selector = selector };
	}

	template <typename... Fs>
	inline std::vector<aat::Feature> ff(uint16_t type, Fs... selectors)
	{
		return std::vector { f(type, selectors)... };
	}

	using namespace aat;
	namespace F = feature;

	static util::hashmap<Tag, std::vector<aat::Feature>> g_feature_mapping = {
		{ F::liga, ff(kLigaturesType, kCommonLigaturesOnSelector) },
		{ F::rlig, ff(kLigaturesType, kRequiredLigaturesOnSelector) },
		{ F::clig, ff(kLigaturesType, kContextualLigaturesOnSelector) },
		{ F::dlig, ff(kLigaturesType, kRareLigaturesOnSelector) },
		{ F::hlig, ff(kLigaturesType, kHistoricalLigaturesOnSelector) },

		{ F::smcp, ff(kLowerCaseType, kLowerCaseSmallCapsSelector) },
		{ F::pcap, ff(kLowerCaseType, kLowerCasePetiteCapsSelector) },

		{ F::c2sc, ff(kUpperCaseType, kUpperCaseSmallCapsSelector) },
		{ F::c2pc, ff(kUpperCaseType, kUpperCasePetiteCapsSelector) },

		{ F::calt, ff(kContextualAlternatesType, kContextualAlternatesOnSelector) },
		{ F::swsh, ff(kContextualAlternatesType, kSwashAlternatesOnSelector) },
		{ F::cswh, ff(kContextualAlternatesType, kContextualSwashAlternatesOnSelector) },

		{ F::lnum, ff(kNumberCaseType, kUpperCaseNumbersSelector) },
		{ F::onum, ff(kNumberCaseType, kLowerCaseNumbersSelector) },
		{ F::tnum, ff(kNumberSpacingType, kMonospacedNumbersSelector) },
		{ F::pnum, ff(kNumberSpacingType, kProportionalNumbersSelector) },
		{ F::sups, ff(kVerticalPositionType, kSuperiorsSelector) },
		{ F::subs, ff(kVerticalPositionType, kInferiorsSelector) },
		{ F::ordn, ff(kVerticalPositionType, kOrdinalsSelector) },
		{ F::sinf, ff(kVerticalPositionType, kScientificInferiorsSelector) },

		{ F::frac, ff(kFractionsType, kDiagonalFractionsSelector) },
		{ F::afrc, ff(kFractionsType, kVerticalFractionsSelector) },
		{ F::mgrk, ff(kMathematicalExtrasType, kMathematicalGreekOnSelector) },

		{ F::vkna, ff(kAlternateKanaType, kAlternateVertKanaOnSelector) },
		{ F::hkna, ff(kAlternateKanaType, kAlternateHorizKanaOnSelector) },
		{ F::pkna, ff(kKanaSpacingType, kProportionalKanaSelector) },
		{ F::ruby, ff(kRubyKanaType, kRubyKanaOnSelector, kRubyKanaSelector) },

		{ F::fwid, { f(kTextSpacingType, kMonospacedTextSelector), f(kCJKRomanSpacingType, kFullWidthCJKRomanSelector) } },
		{ F::pwid, { f(kTextSpacingType, kProportionalTextSelector), f(kCJKRomanSpacingType, kProportionalCJKRomanSelector) } },
		{ F::hwid, { f(kTextSpacingType, kHalfWidthTextSelector), f(kCJKRomanSpacingType, kHalfWidthCJKRomanSelector) } },
		{ F::twid, ff(kTextSpacingType, kThirdWidthTextSelector) },
		{ F::qwid, ff(kTextSpacingType, kQuarterWidthTextSelector) },
		{ F::halt, ff(kTextSpacingType, kAltHalfWidthTextSelector) },
		{ F::palt, ff(kTextSpacingType, kAltProportionalTextSelector) },

		{ F::smpl, ff(kCharacterShapeType, kSimplifiedCharactersSelector) },
		{ F::trad, ff(kCharacterShapeType, kTraditionalCharactersSelector) },
		{ F::tnam, ff(kCharacterShapeType, kTraditionalNamesCharactersSelector) },
		{ F::expt, ff(kCharacterShapeType, kExpertCharactersSelector) },
		{ F::hojo, ff(kCharacterShapeType, kHojoCharactersSelector) },
		{ F::nlck, ff(kCharacterShapeType, kNLCCharactersSelector) },
		{ F::jp78, ff(kCharacterShapeType, kJIS1978CharactersSelector) },
		{ F::jp83, ff(kCharacterShapeType, kJIS1983CharactersSelector) },
		{ F::jp90, ff(kCharacterShapeType, kJIS1990CharactersSelector) },
		{ F::jp04, ff(kCharacterShapeType, kJIS2004CharactersSelector) },

		{ F::hngl, ff(kTransliterationType, kHanjaToHangulSelector) },

		{ F::ss01, ff(kStylisticAlternativesType, kStylisticAltOneOnSelector) },
		{ F::ss02, ff(kStylisticAlternativesType, kStylisticAltTwoOnSelector) },
		{ F::ss03, ff(kStylisticAlternativesType, kStylisticAltThreeOnSelector) },
		{ F::ss04, ff(kStylisticAlternativesType, kStylisticAltFourOnSelector) },
		{ F::ss05, ff(kStylisticAlternativesType, kStylisticAltFiveOnSelector) },
		{ F::ss06, ff(kStylisticAlternativesType, kStylisticAltSixOnSelector) },
		{ F::ss07, ff(kStylisticAlternativesType, kStylisticAltSevenOnSelector) },
		{ F::ss08, ff(kStylisticAlternativesType, kStylisticAltEightOnSelector) },
		{ F::ss09, ff(kStylisticAlternativesType, kStylisticAltNineOnSelector) },
		{ F::ss10, ff(kStylisticAlternativesType, kStylisticAltTenOnSelector) },
		{ F::ss11, ff(kStylisticAlternativesType, kStylisticAltElevenOnSelector) },
		{ F::ss12, ff(kStylisticAlternativesType, kStylisticAltTwelveOnSelector) },
		{ F::ss13, ff(kStylisticAlternativesType, kStylisticAltThirteenOnSelector) },
		{ F::ss14, ff(kStylisticAlternativesType, kStylisticAltFourteenOnSelector) },
		{ F::ss15, ff(kStylisticAlternativesType, kStylisticAltFifteenOnSelector) },
		{ F::ss16, ff(kStylisticAlternativesType, kStylisticAltSixteenOnSelector) },
		{ F::ss17, ff(kStylisticAlternativesType, kStylisticAltSeventeenOnSelector) },
		{ F::ss18, ff(kStylisticAlternativesType, kStylisticAltEighteenOnSelector) },
		{ F::ss19, ff(kStylisticAlternativesType, kStylisticAltNineteenOnSelector) },
		{ F::ss20, ff(kStylisticAlternativesType, kStylisticAltTwentyOnSelector) },

		{ F::case_, ff(kCaseSensitiveLayoutType, kCaseSensitiveLayoutOnSelector, kCaseSensitiveSpacingOnSelector) },
	};

	std::vector<aat::Feature> convertOFFFeatureToAAT(Tag feature)
	{
		auto it = g_feature_mapping.find(feature);
		if(it == g_feature_mapping.end())
			return {};

		return it->second;
	}
}
