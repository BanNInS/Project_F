
#include "Customizations/CriWareNameSelector.h"

#include "DetailLayoutBuilder.h"

FText FCriWareNameSelectorGenerator::GetComboBoxContent() const
{
	FName DesiredName = CachedCallbacks.GetCurrentlySelectedName();
	return FText::FromName(DesiredName);
}

FText FCriWareNameSelectorGenerator::GetComboBoxToolTip() const
{
	return FText::FromString(CachedCallbacks.GetTooltipText());
}

TSharedRef<SWidget> FCriWareNameSelectorGenerator::MakeNameSelectorWidget(TArray<FName>& InNameArray, FCriWareNameSelectorCallbacks&& InCallbacks)
{
	CachedNameArray.Reset();

	for (FName& InName : InNameArray)
	{
		CachedNameArray.Add(MakeShared<FName>(InName));
	}

	CachedCallbacks = MoveTemp(InCallbacks);

	FName CurrentlySelectedName = CachedCallbacks.GetCurrentlySelectedName();
	TSharedPtr<FName> InitialSelectedName;

	for (TSharedPtr<FName>& NameSelection : CachedNameArray)
	{
		if (*NameSelection == CurrentlySelectedName)
		{
			InitialSelectedName = NameSelection;
			break;
		}
	}

	return SNew(SComboBox<TSharedPtr<FName>>)
		.OnGenerateWidget(this, &FCriWareNameSelectorGenerator::HandleResponseComboBoxGenerateWidget)
		.OptionsSource(&CachedNameArray)
		.OnSelectionChanged(this, &FCriWareNameSelectorGenerator::OnSelectionChanged)
		.InitiallySelectedItem(InitialSelectedName)
		.IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute())
		.ContentPadding(FMargin(2.0f, 2.0f))
		.Content()
		[
			SNew(STextBlock)
			.Text(this, &FCriWareNameSelectorGenerator::GetComboBoxContent)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ToolTipText(this, &FCriWareNameSelectorGenerator::GetComboBoxToolTip)
		];
}

void FCriWareNameSelectorGenerator::OnSelectionChanged(TSharedPtr<FName> NameItem, ESelectInfo::Type SelectInfo)
{
	check(NameItem.IsValid());

	CachedCallbacks.OnNewNameSelected(*NameItem);
}

TSharedRef<SWidget> FCriWareNameSelectorGenerator::HandleResponseComboBoxGenerateWidget(TSharedPtr<FName> StringItem)
{
	return SNew(STextBlock)
		.Text(FText::FromName(*StringItem))
		.Font(IDetailLayoutBuilder::GetDetailFont());
}
