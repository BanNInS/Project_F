
#pragma once

#include "Styling/AppStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateStyleRegistry.h"

#define LOCTEXT_NAMESPACE "AtomWidgets"

#define CRI_API CRIWAREATOMWIDGETS_API

namespace AtomWidgets
{
	class FSlateStyle final : public FSlateStyleSet
	{
	public:
		static CRI_API FSlateStyle& Get();

		static CRI_API FName GetStyleName()
		{
			const FLazyName StyleName = "AtomWidgets";
			return StyleName.Resolve();
		}

		const FSlateBrush& GetBrushEnsured(FName InName)
		{
			const ISlateStyle* AtomWidgetsStyle = FSlateStyleRegistry::FindSlateStyle(GetStyleName());
			if (ensureMsgf(AtomWidgetsStyle, TEXT("Missing slate style '%s'"), *GetStyleName().ToString()))
			{
				const FSlateBrush* Brush = AtomWidgetsStyle->GetBrush(InName);
				if (ensureMsgf(Brush, TEXT("Missing brush '%s'"), *InName.ToString()))
				{
					return *Brush;
				}
			}

			if (const FSlateBrush* NoBrush = FAppStyle::GetBrush("NoBrush"))
			{
				return *NoBrush;
			}

			return *DefaultBrush;
		}

		FSlateStyle();
	};
} // namespace

#undef LOCTEXT_NAMESPACE

#undef CRI_API
