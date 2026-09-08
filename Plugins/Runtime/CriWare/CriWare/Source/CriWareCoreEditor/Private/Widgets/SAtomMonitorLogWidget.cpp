

#include "SAtomMonitorLogWidget.h"

#include "Fonts/FontMeasure.h"
#include "Styling/CoreStyle.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Docking/SDockTab.h"
#include "HAL/PlatformApplicationMisc.h"

#include "SAtomMonitorLogMultiSelectTableRow.h"

#define LOCTEXT_NAMESPACE "AtomEditor"

void FAtomMonitorLogWidgetCommands::RegisterCommands()
{
	UI_COMMAND(ClearLogText, "Clear Log", "Clear text within the current log window tab", EUserInterfaceActionType::Button,
		FInputChord());

	UI_COMMAND(CopyLogLines, "Copy", "Copies the selected log lines to the clipboard", EUserInterfaceActionType::Button,
		FInputChord(EModifierKey::Control, EKeys::C));

	UI_COMMAND(FindLogText, "Find", "Find text within the current log window tab", EUserInterfaceActionType::Button,
		FInputChord(EModifierKey::Control, EKeys::F));
}

// Hack for accessing private members in various Engine classes

// This template, is used to link an arbitrary class member, to the GetPrivate function
template<typename Accessor, typename Accessor::Member Member> struct AccessPrivate
{
	friend typename Accessor::Member GetPrivate(Accessor InAccessor)
	{
		return Member;
	}
};


// Need to define one of these, for every member you want to access (preferably in the .cpp) - for example:
#if 0
// This is used to aid in linking one member in FStackTracker, to the above template
struct FStackTrackerbIsEnabledAccessor
{
	using Member = bool FStackTracker::*;

	friend Member GetPrivate(FStackTrackerbIsEnabledAccessor);
};

// These combine the structs above, with the template for accessing private members, pointing to the specified member
template struct AccessPrivate<FStackTrackerbIsEnabledAccessor, &FStackTracker::bIsEnabled>;
#endif

// Example for calling private functions
#if 0
// Used, in combination with another template, for accessing private/protected members of classes
struct AShooterCharacterServerEquipWeaponAccessor
{
	using Member = void (AShooterCharacter::*)(AShooterWeapon* Weapon);

	friend Member GetPrivate(AShooterCharacterServerEquipWeaponAccessor);
};

// Combines the above struct, with the template used for accessing private/protected members
template struct AccessPrivate<AShooterCharacterServerEquipWeaponAccessor, &AShooterCharacter::ServerEquipWeapon>;

// To call private function:
//	(GET_PRIVATE(AShooterCharacter, CurChar, ServerEquipWeapon))(AShooterWeapon::StaticClass());
#endif

/**
 * Defines a class and template specialization, for a variable, needed for use with the GET_PRIVATE hook below
 *
 * @param InClass		The class being accessed (not a string, just the class, i.e. FStackTracker)
 * @param VarName		Name of the variable being accessed (again, not a string)
 * @param VarType		The type of the variable being accessed
 */
#define IMPLEMENT_GET_PRIVATE_VAR(InClass, VarName, VarType) \
	struct InClass##VarName##Accessor \
	{ \
		using Member = VarType InClass::*; \
		\
		friend Member GetPrivate(InClass##VarName##Accessor); \
	}; \
	\
	template struct AccessPrivate<InClass##VarName##Accessor, &InClass::VarName>;

 // @todo #JohnB: Unfortunately, this broke in VS2015 for functions, as '&InClass::FuncName' gives an error;
 //					strictly speaking, this falls within the C++ standard and should work, so perhaps make a bug report
 //					See SAtomMonitorLogWidget.cpp, for an alternative way of hack-accessing private members, using template specialization
#if 0
/**
 * Defines a class and template specialization, for a function, needed for use with the GET_PRIVATE hook below
 *
 * @param InClass		The class being accessed (not a string, just the class, i.e. FStackTracker)
 * @param FuncName		Name of the function being accessed (again, not a string)
 * @param FuncRet		The return type of the function
 * @param FuncParms		The function parameters
 * @param FuncModifier	(Optional) Modifier placed after the function - usually 'const'
 */
#define IMPLEMENT_GET_PRIVATE_FUNC_CONST(InClass, FuncName, FuncRet, FuncParms, FuncModifier) \
	struct InClass##FuncName##Accessor \
	{ \
		using Member = FuncRet (InClass::*)(FuncParms) FuncModifier; \
		\
		friend Member GetPrivate(InClass##FuncName##Accessor); \
	}; \
	\
	template struct AccessPrivate<InClass##FuncName##Accessor, &InClass::FuncName>;

#define IMPLEMENT_GET_PRIVATE_FUNC(InClass, FuncName, FuncRet, FuncParms) \
	IMPLEMENT_GET_PRIVATE_FUNC_CONST(InClass, FuncName, FuncRet, FuncParms, ;)
#endif


 /**
  * A macro for tidying up accessing of private members, through the above code
  *
  * @param InClass		The class being accessed (not a string, just the class, i.e. FStackTracker)
  * @param InObj			Pointer to an instance of the specified class
  * @param MemberName	Name of the member being accessed (again, not a string)
  * @return				The value of the member
  */
#define GET_PRIVATE(InClass, InObj, MemberName) (*InObj).*GetPrivate(InClass##MemberName##Accessor())

  /**
   * Redundant version of the above, for emphasizing the ability to assign a writable reference which can modify the original value
   */
#define GET_PRIVATE_REF(InClass, InObj, MemberName) GET_PRIVATE(InClass, InObj, MemberName)


   // @todo #JohnB: Restore if fixed in VS2015
#if 0
// Version of above, for calling private functions
#define CALL_PRIVATE(InClass, InObj, MemberName) (GET_PRIVATE(InClass, InObj, MemberName))
#endif


/**
 * Defines a class used for hack-accessing protected functions, through the CALL_PROTECTED macro below
 *
 * @param InClass		The class being accessed (not a string, just the class, i.e. FStackTracker)
 * @param FuncName		Name of the function being accessed (again, not a string)
 * @param FuncRet		The return type of the function
 * @param FuncParms		The function parameters
 * @param FuncParmNames	The names of the function parameters, as passed from one function to another
 * @param FuncModifier	(Optional) Modifier placed after the function - usually 'const'
 */
#define IMPLEMENT_GET_PROTECTED_FUNC_CONST(InClass, FuncName, FuncRet, FuncParms, FuncParmNames, FuncModifier) \
	struct InClass##FuncName##Accessor : public InClass \
	{ \
		FORCEINLINE FuncRet FuncName##Accessor(FuncParms) FuncModifier \
		{\
			return FuncName(FuncParmNames); \
		} \
	};

#define IMPLEMENT_GET_PROTECTED_FUNC(InClass, FuncName, FuncRet, FuncParms, FuncParmNames) \
	IMPLEMENT_GET_PROTECTED_FUNC_CONST(InClass, FuncName, FuncRet, FuncParms, FuncParmNames,)

 // Version of GET_PRIVATE, for calling protected functions
#define CALL_PROTECTED(InClass, InObj, MemberName) ((InClass##MemberName##Accessor*)&(*InObj))->MemberName##Accessor

// @todo #JohnBFeatureUI: It would be good to have both a 'Search' filter tab, and a 'Ctrl+F Find' button for each existing tab;
//				two different search abilities (should be easy to add too)

// @todo #JohnBFeatureUI: Could change the 'close' button, to a '+' button, which clones the current tab and enables search filtering


// @todo #JohnBReview: Perhaps have conditional tabs, which only appear if log entries of this type come up?
//				(such as: Warning, Error, Debug) UPDATE: Is this not already present?

// @todo #JohnBFeatureUI: Consider adding a "TabName (LineCount)" to the tab name (maybe just for important tabs, like Error/Warning)


// @todo #JohnBUI: Change the opacity, of background highlight for selected log entries - really ugly at the moment
//					(unfortunately, this requires creating a new FTableRowStyle, which is a bit non-trivial)

// @todo #JohnBUI: The 'failed to find text' message does not fade out properly, and hijacking ComputeVolatility does not help.
//					Find a way to fix this, eventually.


// Enable access to the private SEditableTextBox.EditableText variable, using the GET_PRIVATE macro
IMPLEMENT_GET_PRIVATE_VAR(SEditableTextBox, EditableText, TSharedPtr<SEditableText>);

// Enable access to SButton.Style
IMPLEMENT_GET_PRIVATE_VAR(SButton, Style, const FButtonStyle*);

// Enable reading SBorder::GetBorderBackgroundColor
IMPLEMENT_GET_PROTECTED_FUNC_CONST(SBorder, GetBorderBackgroundColor, FSlateColor, void,, const);

// Enable access to SDockTab::GetCurrentStyle, using the CALL_PROTECTED macro
IMPLEMENT_GET_PROTECTED_FUNC_CONST(SDockTab, GetCurrentStyle, const FDockTabStyle&, void,, const);


// @todo #JohnBRefactorUI: Perhaps move widget searching to a NUTSlate.h file, or such?

/**
 * Delegate used for recursively iterating a widgets child widgets, and testing if they match a search condition
 *
 * @param InWidget	The widget to be tested
 * @return			Whether or not the widget matches a search condition
 */
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnTestWidget, const TSharedRef<SWidget>& /* InWidget */);


/**
 * Iteratively searches through a widget and all its child widgets, for a-widget/widgets, that pass the test delegate conditions
 *
 * @param OutMatches	The array of widgets that results are passed to
 * @param InWidget		The widget to search
 * @param InTester		The delegate (usually a lambda) to do condition testing with
 * @param bMultiMatch	(Internal) Whether or not multiple matches are searched for (or just one single widget, the first match)
 * @return				Whether or not any widgets were found
 */
static bool SearchForWidgets(TArray<TSharedRef<SWidget>>& OutMatches, TSharedRef<SWidget> InWidget, FOnTestWidget InTester,
						bool bMultiMatch=true)
{
	bool bFoundWidget = false;

	if (InTester.Execute(InWidget))
	{
		OutMatches.Add(InWidget);
		bFoundWidget = true;
	}

	if (bMultiMatch || !bFoundWidget)
	{
		FChildren* ChildWidgets = InWidget->GetChildren();
		int32 ChildCount = ChildWidgets->Num();

		for (int32 i=0; i<ChildCount; i++)
		{
			TSharedRef<SWidget> CurChild = ChildWidgets->GetChildAt(i);

			bFoundWidget = SearchForWidgets(OutMatches, CurChild, InTester, bMultiMatch) || bFoundWidget;

			if (!bMultiMatch && bFoundWidget)
			{
				break;
			}
		}
	}

	return bFoundWidget;
}

/**
 * Iteratively searches through a widget and all its child widgets, for a widget, that passes the test delegate conditions
 *
 * @param InWidget		The widget to search
 * @param InTester		The delegate (usually a lambda) to do condition testing with
 * @return				The returned widget (or SNullWidget, if none)
 */
static TSharedRef<SWidget> SearchForWidget(TSharedRef<SWidget> InWidget, FOnTestWidget InTester)
{
	TSharedRef<SWidget> ReturnVal = SNullWidget::NullWidget;
	TArray<TSharedRef<SWidget>> Match;

	if (SearchForWidgets(Match, InWidget, InTester, false))
	{
		ReturnVal = Match[0];
	}

	return ReturnVal;
}

/**
 * Special tab type, that cannot be dragged/undocked from the tab bar
 */
class SLockedTab : public SDockTab
{
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return FReply::Handled();
	}
};

/**
 * SAtomMonitorLogWidget
 */
void SAtomMonitorLogWidget::Construct(const FArguments& Args)
{
	LogWidgetCommands = MakeShareable(new FUICommandList);

	// Setup the context-menu/shortcut-key commands
	const FAtomMonitorLogWidgetCommands& Commands = FAtomMonitorLogWidgetCommands::Get();

	LogWidgetCommands->MapAction(Commands.ClearLogText, FExecuteAction::CreateRaw(this, &SAtomMonitorLogWidget::OnClear),
									FCanExecuteAction::CreateRaw(this, &SAtomMonitorLogWidget::CanClear));

	LogWidgetCommands->MapAction(Commands.CopyLogLines, FExecuteAction::CreateRaw(this, &SAtomMonitorLogWidget::OnCopy),
									FCanExecuteAction::CreateRaw(this, &SAtomMonitorLogWidget::CanCopy));

	LogWidgetCommands->MapAction(Commands.FindLogText, FExecuteAction::CreateRaw(this, &SAtomMonitorLogWidget::OnFind),
									FCanExecuteAction::CreateRaw(this, &SAtomMonitorLogWidget::CanFind));

	// Setup the tabbed log filters layout
	TSharedRef<FTabManager::FLayout> LogTabLayout = InitializeTabLayout(Args);

	// Lists widgets that share the same tooltip text
	TArray<TSharedPtr<SWidget>> AutoScrollWidgets;
	TArray<TSharedPtr<SWidget>> DeveloperWidgets;
	TArray<TSharedPtr<SWidget>> TimestampsWidgets;

	// Setup the child widgets
	ChildSlot
	[
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
		.VAlign(VAlign_Bottom)
		.Padding(2.0f, 2.0f)
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			/**
			 * Start/Stop button
			 */
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			.AutoWidth()
			[
				SNew(SButton)
				.ToolTipText(FText::FromString(TEXT("Start/Stop the monitoring.")))
				.OnClicked_Lambda([&]()
				{
					OnStartStopClicked.ExecuteIfBound();
					return FReply::Handled();
				})
				.Content()
				[
					SAssignNew(StartStopButtonText, STextBlock)
					.Text(FText::FromString(FString(TEXT("START"))))
				]
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			.AutoWidth()
			[
				SNew(SSpacer)
			]
			
			/**
			 * Search button
			 */
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			.AutoWidth()
			[
				SNew(SButton)
				.ToolTipText(FText::FromString(TEXT("Open the find bar for the current tab.")))
				.OnClicked_Lambda(
					[&]()
				{
					OnFind();
					return FReply::Handled();
				})
				[
					SNew(SImage)
					.Image(&FCoreStyle::Get().GetWidgetStyle<FSearchBoxStyle>("SearchBox").GlassImage)
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			.AutoWidth()
			[
				SNew(SSpacer)
			]

			/**
			 * AutoScroll checkbox
			 */
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			.AutoWidth()
			[
				SAssignNew(ArrayAddNew(AutoScrollWidgets), STextBlock)
				.Text(FText::FromString(FString(TEXT("AutoScroll:"))))
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			.AutoWidth()
			[
				SAssignNew(ArrayAddNew(AutoScrollWidgets), SCheckBox)
				// Disable the checkbox, if the current tab doesn't support autoscrolling
				.IsEnabled_Lambda([&]()
				{
					return CanAutoScroll(GetActiveTabInfo());
				})
				.IsChecked(ECheckBoxState::Checked)
				.OnCheckStateChanged_Lambda([&](ECheckBoxState NewAutoScrollState)
				{
					bAutoScroll = (NewAutoScrollState == ECheckBoxState::Checked);
					// Scroll to the end, if autoscroll was just re-enabled
					if (bAutoScroll)
					{
						for (auto CurTabInfo : LogTabs)
						{
							if (CanAutoScroll(CurTabInfo))
							{
								ScrollToEnd(CurTabInfo);
							}
						}
					}
				})
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			.AutoWidth()
			[
				SNew(SSpacer)
			]

			/**
			 * Developer checkbox
			 */
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			.AutoWidth()
			[
				SAssignNew(ArrayAddNew(DeveloperWidgets), STextBlock)
				.Text(FText::FromString(FString(TEXT("Developer:"))))
			]	
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			.AutoWidth()
			[
				SAssignNew(ArrayAddNew(DeveloperWidgets), SCheckBox)
				.IsChecked(ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([&](ECheckBoxState NewDeveloperState)
				{
					OnDeveloperClicked.ExecuteIfBound(NewDeveloperState == ECheckBoxState::Checked);
				})
			]
			/**
			 * Show Timestamps checkbox
			 */
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			.AutoWidth()
			[
				SAssignNew(ArrayAddNew(TimestampsWidgets), STextBlock)
				.Text(FText::FromString(FString(TEXT("Show Timestamps:"))))
			]	
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			.AutoWidth()
			[
				SAssignNew(ArrayAddNew(TimestampsWidgets), SCheckBox)
				.IsChecked(ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([&](ECheckBoxState NewTimestampsState)
				{
					bShowTimestamps = (NewTimestampsState == ECheckBoxState::Checked);
				})
			]
		]

		/**
		 * Log list filter tabs
		 */
		+SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding(2.0f, 2.0f)
		.FillHeight(1.0f)
		[
			LogTabManager->RestoreFrom(LogTabLayout, TSharedPtr<SWindow>()).ToSharedRef()	
		]

		/**
		 * status and Console command bar
		 */
		+SVerticalBox::Slot()
		.VAlign(VAlign_Bottom)
		.Padding(2.0f, 2.0f)
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			/**
			 * Console command context selector
			 */
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			.AutoWidth()
			[
				SAssignNew(ConsoleComboBox, SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&ConsoleContextList)
				.ToolTipText(FText::FromString(FString(TEXT("Select the context for executing console commands."))))
				// @todo #JohnBRefactorUI: Too big to inline? Separate into its own function?
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					FString ItemStr = *Item;
					FString ToolTipStr = TEXT("");

					if (ItemStr == TEXT("Global"))
					{
						ToolTipStr = TEXT("Execute the command outside the context of any unit test world.");
					}
					else if (ItemStr == TEXT("Local"))
					{
						ToolTipStr = TEXT("Execute the command on the local-client/unit-test.");
					}
					else if (ItemStr == TEXT("Server"))
					{
						ToolTipStr = TEXT("Execute the command on the game server associated with this unit test.");
					}
					else if (ItemStr == TEXT("Client"))
					{
						// @todo #JohnBFeatureUI: Update when implemented
						ToolTipStr = TEXT("(Not yet implemented) ") 
										TEXT("Execute the command on the client associated with this unit test.");
					}

					// @todo #JohnB: Custom context hints?

					return	SNew(STextBlock)
							.Text(FText::FromString(ItemStr))
							.ToolTipText(FText::FromString(ToolTipStr));
				})
				[
					SNew(STextBlock)
					.Text_Lambda([&]()
					{
						TSharedPtr<FString> Selection = ConsoleComboBox->GetSelectedItem();

						return FText::FromString(Selection.IsValid() ? *Selection : DefaultConsoleContext);
					})
				]
			]

			/**
			 * Console command edit box
			 */
			// @todo #JohnBFeatureUI: Borrow the auto-complete from SOutputLog's version of log windows
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			.FillWidth(1.0f)
			[
				SAssignNew(ConsoleTextBox, SEditableTextBox)
				.HintText(FText::FromString(TEXT("Console")))
				.ToolTipText(FText::FromString(TEXT("Executes a console command within the specified context.")))
				.ClearKeyboardFocusOnCommit(false)
				// @todo #JohnBRefactorUI: Too big to inline? Separate into its own function?
				.OnTextCommitted_Lambda([&](const FText& InText, ETextCommit::Type InCommitType)
				{
					if (OnConsoleCommand.IsBound())
					{
						FString Command = InText.ToString();

						if (InCommitType == ETextCommit::OnEnter && Command.Len() > 0)
						{
							TSharedPtr<FString> ComboSelection =
								(ConsoleComboBox.IsValid() ? ConsoleComboBox->GetSelectedItem() : nullptr);

							FString CommandContext = (ComboSelection.IsValid() ? *ComboSelection : DefaultConsoleContext);

							bool bSuccess = OnConsoleCommand.Execute(CommandContext, Command);

							// If the command executed successfully, wipe the text
							if (bSuccess)
							{
								ConsoleTextBox->SetText(FText());
							}
							// Otherwise, do 'select-all' on the text (user can still amend mistakes, or wipe by typing)
							else
							{
								// ConsoleTextBox->SelectAllText();
								(GET_PRIVATE(SEditableTextBox, ConsoleTextBox, EditableText))->SelectAllText();
							}
						}
					}
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SSpacer)
			]

			/**
			 * Monitor Server status
			 */
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			.AutoWidth()
			[
				SNew(SOverlay)
				+SOverlay::Slot()
				[
					SAssignNew(ServerStatusBackgroundIcon, SImage)
					.ColorAndOpacity(FSlateColor::UseForeground())
					.Image(FAppStyle::Get().GetBrush("DerivedData.RemoteCache.Unavailable"))
				]
				+ SOverlay::Slot()
				[
					SAssignNew(ServerStatusBadgeIcon, SImage)
					.ColorAndOpacity(FSlateColor::UseForeground())
					.Image(nullptr)
				]
				+ SOverlay::Slot()
				[
					SAssignNew(ServerStatusUploadIcon, SImage)
					.ColorAndOpacity(FLinearColor::Transparent)
					.Image(FAppStyle::Get().GetBrush("DerivedData.RemoteCache.Uploading"))
				]
				+ SOverlay::Slot()
				[
					SAssignNew(ServerStatusDownloadIcon, SImage)
					.ColorAndOpacity(FLinearColor::Transparent)
					.Image(FAppStyle::Get().GetBrush("DerivedData.RemoteCache.Downloading"))
				]
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			.AutoWidth()
			[
				SAssignNew(ServerStatusText, STextBlock)
			]
			
			/**
			 * In-Game Preview Connection status
			 */
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			.AutoWidth()
			[
				SAssignNew(InGamePreviewStatusText, STextBlock)
			]
		]
	];

	// Set widget tooltip text, for UI self-documentation
	for (auto CurWidget : AutoScrollWidgets)
	{
		CurWidget->SetToolTipText(FText::FromString(TEXT("Whether or not to automatically scroll to the bottom, as new log entries arrive.")));
	}

	for (auto CurWidget : DeveloperWidgets)
	{
		CurWidget->SetToolTipText(FText::FromString(TEXT("Whether or not to use developer mode (keeps the unit test and any server/client from closing.)")));
	}
}

TSharedRef<FTabManager::FLayout> SAtomMonitorLogWidget::InitializeTabLayout(const FArguments& Args)
{
	// Initialize the LogTabs array (which includes labels/tooltips, for each log type)

	// single

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("All"), TEXT("No filters - all log output is shown."),
		EAtomMonitorLogMode::All, 10, true)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("Playback"), TEXT("Playback log output is shown."),
		EAtomMonitorLogMode::Playback, 20, true)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("Error"), TEXT("Error log output is shown."),
		EAtomMonitorLogMode::Error, 30, true)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("LowLevelPlayback"), TEXT("LowLevelPlayback log output is shown."),
		EAtomMonitorLogMode::LowLevelPlayback, 40, true)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("SystemInfo"), TEXT("SystemInfo log output is shown."),
		EAtomMonitorLogMode::SystemInfo, 50, true)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("HandleInfo"), TEXT("HandleInfo log output is shown."),
		EAtomMonitorLogMode::HandleInfo, 60, true)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("CueLimit"), TEXT("CueLimit log output is shown."),
		EAtomMonitorLogMode::CueLimit, 70, true)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("Probability"), TEXT("Probability log output is shown."),
		EAtomMonitorLogMode::Probability, 80, true)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("Category"), TEXT("Category log output is shown."),
		EAtomMonitorLogMode::Category, 90, true)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("ExecutingInfo"), TEXT("ExecutingInfo log output is shown."),
		EAtomMonitorLogMode::ExecutingInfo, 100, true)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("Position3DInfo"), TEXT("Position3DInfo log output is shown."),
		EAtomMonitorLogMode::Position3DInfo, 110, true)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("User"), TEXT("Filter for local console command results."),
		EAtomMonitorLogMode::User, 120, true)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("VoiceVolume"), TEXT("Filter for local console command results."),
		EAtomMonitorLogMode::VoiceVolume, 130, true)));

	// composed

	/*LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("Summary"), TEXT("Filter for the most notable log entries."), 
		EAtomMonitorLogMode::Playback | EAtomMonitorLogMode::Error, 10)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("Advanced Summary"), TEXT("Filter for the most notable log entries, with extra/advanced information."),
		EAtomMonitorLogMode::CueLimit | EAtomMonitorLogMode::Position3DInfo | EAtomMonitorLogMode::ExecutingInfo, 20)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("System"), TEXT("Filter for locally-sourced log entries (i.e. no sub-process logs)."),
		EAtomMonitorLogMode::SystemInfo)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("Handle"), TEXT("Filter for the server process log entries."),
		EAtomMonitorLogMode::HandleInfo)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("Probability"), TEXT("Filter for the client process log entries."),
		EAtomMonitorLogMode::Probability)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("Debug"), TEXT("Filter for debug log entries."),
		EAtomMonitorLogMode::LowLevelPlayback, 5)));

	LogTabs.Add(MakeShareable(new FAtomMonitorLogTabInfo(
		TEXT("User"), TEXT("Filter for local console command results."),
		EAtomMonitorLogMode::User, 5)));*/

	// Initialize the tab manager, stack and layout
	TSharedRef<SDockTab> DudTab = SNew(SLockedTab);
	LogTabManager = FGlobalTabmanager::Get()->NewTabManager(DudTab);

	// Add tabs for each LogTabs entry, to the tab stack
	const TSharedRef<FTabManager::FStack> LogTabStack = FTabManager::NewStack();

	for (auto CurTabInfo : LogTabs)
	{
		LogTabManager->RegisterTabSpawner(CurTabInfo->TabIdName, FOnSpawnTab::CreateRaw(this, &SAtomMonitorLogWidget::SpawnLogTab));
		LogTabStack->AddTab(CurTabInfo->TabIdName, (CurTabInfo->bTabOpen ? ETabState::OpenedTab : ETabState::ClosedTab));
	}

	// Make the first tab in the stack, the foremost tab
	LogTabStack->SetForegroundTab(LogTabs[0]->TabIdName);

	// Setup the layout and add the stack; the layout internally handles how the tab widgets are laid out, upon creation (happens later)
	TSharedRef<FTabManager::FLayout> LogTabLayout = FTabManager::NewLayout("AtomMonitorLogTabLayout")
		->AddArea(FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split(LogTabStack)
		);

	return LogTabLayout;
}

TSharedRef<SDockTab> SAtomMonitorLogWidget::SpawnLogTab(const FSpawnTabArgs& InSpawnTabArgs)
{
	FName CurTabName = InSpawnTabArgs.GetTabId().TabType;

	TSharedRef<FAtomMonitorLogTabInfo> CurTabInfo = *LogTabs.FindByPredicate([&CurTabName](const TSharedRef<FAtomMonitorLogTabInfo>& CurElement)
	{
		return CurElement->TabIdName == CurTabName;
	});

	// Have the 'find bar', use the same close button style as tabs
	const FDockTabStyle* const TabStyle = &FCoreStyle::Get().GetWidgetStyle<FDockTabStyle>("Docking.Tab");
	const FButtonStyle* const CloseButtonStyle = &TabStyle->CloseButtonStyle;

	TSharedRef<SDockTab> ReturnVal = SNew(SLockedTab)
		.TabRole(ETabRole::MajorTab)
		.Label(FText::FromString(CurTabInfo->Label))
		// For tabs, must specify a ToolTip parameter - can't use ToolTipText, as the internal tab code ignores it (a bug)
		.ToolTip(SNew(SToolTip).Text(FText::FromString(CurTabInfo->ToolTip)))
		// Can't close these tabs
		//.OnCanCloseTab_Lambda([]() { return false; }) // Can't prevent this as it stops the editor from being able to close while the widget is open[
		//.TabWellContentLeft()
		//[
		//	SNew(SHorizontalBox)
		//	+SHorizontalBox::Slot()
		//	.VAlign(VAlign_Center)
		//	[
		//		SNew(STextBlock)
		//		.Text(FText::FromString(FString(TEXT("Filter:"))))
		//		.ToolTipText(FText::FromString(FString(TEXT("The type of filtering to be applied to log output."))))
		//	]
		//]
		.Content()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.VAlign(VAlign_Top)
			.Padding(0.0f, 2.0f)
			.AutoHeight()
			[
				SNew(SOverlay)
				+SOverlay::Slot()
				[
					SAssignNew(ArrayAddNew(CurTabInfo->FindWidgets), SBorder)
					.Visibility(EVisibility::Collapsed)
					.Padding(TabStyle->TabPadding)
					.BorderImage(&TabStyle->ForegroundBrush)
				]
				+SOverlay::Slot()
				[
					SNew(SHorizontalBox)

					// IMPORTANT: Do NOT use SSearchBox, as getting the style to use left-side buttons, can't be done inline
					//				(runtime tweaks or duplicating styles in general, appears to be completely impractical)
					/**
					 * 'Close Find Bar' 'x' button
					 */
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(2.0f, 0.0f)
					.AutoWidth()
					[
						SAssignNew(ArrayAddNew(CurTabInfo->FindWidgets), SButton)
						.Visibility(EVisibility::Collapsed)
						.ToolTipText(FText::FromString(TEXT("Close the find bar.")))
						.ButtonStyle(CloseButtonStyle)
						.ContentPadding(0)
						.OnClicked_Lambda([&]()
						{
							TSharedPtr<FAtomMonitorLogTabInfo> ActiveTab = GetActiveTabInfo();

							for (TSharedPtr<SWidget>& CurWidget : ActiveTab->FindWidgets)
							{
								CurWidget->SetVisibility(EVisibility::Collapsed);
							}

							if (bAutoScroll)
							{
								ScrollToEnd(ActiveTab.ToSharedRef());
							}

							if (ActiveTab->bHighlightFind)
							{
								UpdateFindHighlight(ActiveTab, false);
							}
							else
							{
								ActiveTab->LogListView->RequestListRefresh();
							}

							return FReply::Handled();
						})
						[
							SNew(SSpacer)
							.Size(CloseButtonStyle->Normal.ImageSize)
						]
					]
					/**
					 * 'Find' label
					 */
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(2.0f, 0.0f)
					.AutoWidth()
					[
						SAssignNew(ArrayAddNew(CurTabInfo->FindWidgets), STextBlock)
						.Visibility(EVisibility::Collapsed)
						.Text(FText::FromString(TEXT("Find:")))
					]
					/**
					 * Find previous button
					 */
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(2.0f, 0.0f)
					.AutoWidth()
					[
						SAssignNew(ArrayAddNew(CurTabInfo->FindWidgets), SButton)
						.Visibility(EVisibility::Collapsed)
						.ToolTipText(FText::FromString(TEXT("Find the previous occurrence of the specified text.")))
						.Text(FText::FromString(TEXT("Prev")))
						.OnClicked_Lambda([&]()
						{
							auto ActiveTabInfo = GetActiveTabInfo();
							ScrollToText(ActiveTabInfo.ToSharedRef(), ActiveTabInfo->FindBox->GetText().ToString(), true);
							return FReply::Handled();
						})
					]
					/**
					 * Find Next button
					 */
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(2.0f, 0.0f)
					.AutoWidth()
					[
						SAssignNew(ArrayAddNew(CurTabInfo->FindWidgets), SButton)
						.Visibility(EVisibility::Collapsed)
						.ToolTipText(FText::FromString(TEXT("Find the next occurrence of the specified text.")))
						.Text(FText::FromString(TEXT("Next")))
						.OnClicked_Lambda([&]()
						{
							auto ActiveTabInfo = GetActiveTabInfo();
							ScrollToText(ActiveTabInfo.ToSharedRef(), ActiveTabInfo->FindBox->GetText().ToString());
							return FReply::Handled();
						})
					]
					/**
					 * Highlight checkbox
					 */
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(2.f, 2.f)
					.AutoWidth()
					[
						SAssignNew(ArrayAddNew(CurTabInfo->FindWidgets), STextBlock)
						.Visibility(EVisibility::Collapsed)
						.Text(FText::FromString(FString(TEXT("Highlight:"))))
					]
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(2.f, 2.f)
					.AutoWidth()
					[
						SAssignNew(ArrayAddNew(CurTabInfo->FindWidgets), SCheckBox)
						.Visibility(EVisibility::Collapsed)
						.IsChecked((CurTabInfo->bHighlightFind ? ECheckBoxState::Checked : ECheckBoxState::Unchecked))
						.OnCheckStateChanged_Lambda([&](ECheckBoxState NewHighlightState)
						{
							TSharedPtr<FAtomMonitorLogTabInfo> ActiveTab = GetActiveTabInfo();
							bool bHighlight = (NewHighlightState == ECheckBoxState::Checked);

							ActiveTab->bHighlightFind = bHighlight;

							UpdateFindHighlight(ActiveTab, bHighlight, ActiveTab->LastFind);
						})
					]
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(2.0f, 0.0f)
					.AutoWidth()
					[
						SAssignNew(ArrayAddNew(CurTabInfo->FindWidgets), SSpacer)
						.Visibility(EVisibility::Collapsed)
					]
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.Padding(2.0f, 0.0f)
					[
						SNew(SOverlay)
						/**
						 * Find edit box
						 */
						+SOverlay::Slot()
						[
							SAssignNew(CurTabInfo->FindBox, SEditableTextBox)
							.Visibility(EVisibility::Collapsed)
							.HintText(FText::FromString(TEXT("Find")))
							.ToolTipText(FText::FromString(TEXT("Finds the specified text, within the current log tab.")))
							.ClearKeyboardFocusOnCommit(false)
							.OnTextCommitted_Lambda([&](const FText& InText, ETextCommit::Type InCommitType)
							{
								if (InCommitType == ETextCommit::OnEnter)
								{
									auto ActiveTabInfo = GetActiveTabInfo();
									ScrollToText(ActiveTabInfo.ToSharedRef(), InText.ToString(),
													ActiveTabInfo->bLastFindWasUp);
								}
							})
						]
						/**
						 * Find error label
						 */
						+SOverlay::Slot()
						.HAlign(HAlign_Right)
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.HAlign(HAlign_Right)
							.VAlign(VAlign_Center)
							.Padding(2.f, 2.f)
							.AutoWidth()
							[
								SAssignNew(CurTabInfo->FindErrorLabel, SBorder)
								.Visibility(EVisibility::Collapsed)
								.Padding(TabStyle->TabPadding)
								.BorderImage(&TabStyle->ForegroundBrush)
								.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 1.f))
								[
									SAssignNew(CurTabInfo->FindErrorText, STextBlock)
									.ColorAndOpacity(FLinearColor(1.f, 0.5f, 0.f, 1.f))
								]
							]
						]
					]
				]
			]
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				/**
				 * Log list view
				 */
				SAssignNew(CurTabInfo->LogListView, SListView<TSharedRef<FAtomMonitorLogLine>>)
				.ListItemsSource(&CurTabInfo->TabLogLines)
				.OnGenerateRow(this, &SAtomMonitorLogWidget::OnGenerateLogRow)
				.OnContextMenuOpening_Lambda([&]()
				{
					FMenuBuilder MenuBuilder(true, LogWidgetCommands);
					MenuBuilder.AddMenuEntry(FAtomMonitorLogWidgetCommands::Get().ClearLogText);
					MenuBuilder.AddMenuSeparator();
					MenuBuilder.AddMenuEntry(FAtomMonitorLogWidgetCommands::Get().CopyLogLines);
					MenuBuilder.AddMenuEntry(FAtomMonitorLogWidgetCommands::Get().FindLogText);
					return MenuBuilder.MakeWidget();
				})
			]
		];

	CurTabInfo->FindWidgets.Add(CurTabInfo->FindBox);
	CurTabInfo->FindWidgets.Add(CurTabInfo->FindErrorLabel);
	CurTabInfo->FindWidgets.Add(CurTabInfo->FindErrorText);
	CurTabInfo->TabWidget = ReturnVal;

	// Disable the close button on the tab - a bit hacky, as need to 'manually' find the button (here it's identified by its style)
	TSharedRef<SWidget> CloseButton = SearchForWidget(ReturnVal, FOnTestWidget::CreateLambda([&ReturnVal](const TSharedRef<SWidget>& InWidget)
	{
		bool bFound = false;
		if (InWidget->GetType() == FName(TEXT("SButton")))
		{
			TSharedRef<SButton> CurButton = StaticCastSharedRef<SButton>(InWidget);
			const FButtonStyle* ButtonStyle = GET_PRIVATE(SButton, CurButton, Style);
			const FDockTabStyle& CurTabStyle = CALL_PROTECTED(SDockTab, ReturnVal, GetCurrentStyle)();
			bFound = ButtonStyle == &CurTabStyle.CloseButtonStyle;
		}
		return bFound;
	}));

	// If the close button was found, hide it
	if (CloseButton != SNullWidget::NullWidget)
	{
		StaticCastSharedRef<SButton>(CloseButton)->SetVisibility(EVisibility::Hidden);
	}

	return ReturnVal;
}

TSharedRef<ITableRow> SAtomMonitorLogWidget::OnGenerateLogRow(TSharedRef<FAtomMonitorLogLine> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	EAtomMonitorLogMode CurLogType = Item->LogType;

	// Various types of special font formatting
	const int32 FontSize = 9;
	FSlateFontInfo RenderFont = FCoreStyle::GetDefaultFontStyle("Regular", FontSize);
	/*if (!!(CurLogType & EAtomMonitorLogMode::StyleMonospace))
	{
		RenderFont = FCoreStyle::GetDefaultFontStyle("Mono", FontSize);
	}
	else if (!!(CurLogType & EAtomMonitorLogMode::StyleBold) && !!(CurLogType & EAtomMonitorLogMode::StyleItalic))
	{
		RenderFont = FCoreStyle::GetDefaultFontStyle("BoldCondensedItalic", FontSize);
	}
	else if (!!(CurLogType & EAtomMonitorLogMode::StyleBold))
	{
		RenderFont = FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
	}
	else if (!!(CurLogType & EAtomMonitorLogMode::StyleItalic))
	{
		RenderFont = FCoreStyle::GetDefaultFontStyle("Italic", FontSize);
	}*/

	FString RenderText;
	if (bShowTimestamps)
	{
		FString TimestampText = Item->LogTimestamp.ToString(TEXT("[%h:%m:%s.%u] "));
		TimestampText.RemoveFromStart(TEXT("+"));

		RenderText = TimestampText + *Item->LogLine;
	}
	else
	{
		RenderText = *Item->LogLine;
	}

	// Pseudo-underline; just adds newline, and then underlines with lots of ----
	/*if (!!(CurLogType & ELogType::StyleUnderline) && RenderText.Len() > 0)
	{
		const TCHAR* TextToUnderline = *RenderText;
		const TCHAR* UnderlineEnd = TextToUnderline + RenderText.Len()-1;
		uint32 TotalLen = RenderText.Len();

		// Only underline up to the last alphanumeric character
		while (UnderlineEnd > TextToUnderline && !FChar::IsAlnum(*UnderlineEnd))
		{
			UnderlineEnd--;
			TotalLen--;
		}


		const TSharedRef<FSlateFontMeasure> FontMeasure =
											FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

		FVector2D UnderlineDim = FontMeasure->Measure(TextToUnderline, 0, TotalLen, RenderFont);
		FVector2D BaseDim = FontMeasure->Measure(TEXT("-"), RenderFont);

		uint32 UnderlineCharCount = FMath::FloorToInt(UnderlineDim.X / BaseDim.X);

		if (UnderlineCharCount > 0)
		{
			RenderText += FString(TEXT("\r\n")) + FString::ChrN(UnderlineCharCount, '-');
		}
	}*/

	return SNew(SAtomMonitorLogMultiSelectTableRow<TSharedRef<FString>>, OwnerTable)
		.Content()
		[
			SNew(STextBlock)
			.Text(FText::FromString(RenderText))
			.HighlightText(Item->LogHighlight.Len() > 0 ? FText::FromString(Item->LogHighlight) : TAttribute<FText>())
			.HighlightColor(FLinearColor(0.f, 1.f, 0.f, 0.25f))
			.Font(RenderFont)
			.ColorAndOpacity(Item->LogColor)
			.ToolTip(SNew(SToolTip)
			[
				// Apart from Text/Font, this is a copy-paste from SToolTip
				SNew(STextBlock)
				.Text(FText::FromString(RenderText))
				.Font(RenderFont)
				.ColorAndOpacity(FSlateColor::UseForeground())
				.WrapTextAt_Static(&SToolTip::GetToolTipWrapWidth)
			])
		];
}

TSharedPtr<FAtomMonitorLogTabInfo> SAtomMonitorLogWidget::GetActiveTabInfo() const
{
	TSharedPtr<FAtomMonitorLogTabInfo> ReturnVal = nullptr;

	for (auto CurTabInfo : LogTabs)
	{
		TSharedPtr<SDockTab> CurTab = CurTabInfo->TabWidget.Pin();

		if (CurTab.IsValid() && CurTab->IsForeground())
		{
			ReturnVal = CurTabInfo;
			break;
		}
	}

	return ReturnVal;
}

void SAtomMonitorLogWidget::AddLine(EAtomMonitorLogMode InLogType, TSharedRef<FString> LogLine, FTimespan LogTimestamp/*=FTimespan::Zero()*/, FSlateColor LogColor/*=FSlateColor::UseForeground()*/, bool bTakeTabFocus/*=false*/)
{
	TSharedRef<FAtomMonitorLogLine> CurLogEntry = MakeShareable(new FAtomMonitorLogLine(InLogType, LogLine, LogTimestamp, LogColor));

	// Add the line to the master list
	LogLines.Add(CurLogEntry);

	TSharedPtr<FAtomMonitorLogTabInfo> ActiveTab = GetActiveTabInfo();

	auto MatchesTabFilter = [&](const TSharedPtr<FAtomMonitorLogTabInfo>& InTab)
	{
		return InTab->Filter == EAtomMonitorLogMode::All || !!(InTab->Filter & InLogType);
	};

	bool bLineInTabFocus = ActiveTab.IsValid() && MatchesTabFilter(ActiveTab);
	TSharedPtr<FAtomMonitorLogTabInfo> FocusTab = nullptr;

	// Then add it to each log tab, if it passes that tabs filter
	for (TSharedRef<FAtomMonitorLogTabInfo>& CurTabInfo : LogTabs)
	{
		if (MatchesTabFilter(CurTabInfo))
		{
			// If the tab is not presently open, open it now
			if (!CurTabInfo->bTabOpen && LogTabManager.IsValid())
			{
				LogTabManager->TryInvokeTab(CurTabInfo->TabIdName);

				// The new tab has stolen focus, now restore the old tabs focus
				LogTabManager->TryInvokeTab(ActiveTab->TabIdName);

				CurTabInfo->bTabOpen = true;
			}

			// If the line is requesting focus, but is not currently in focus, select a tab for focusing
			if (bTakeTabFocus && !bLineInTabFocus)
			{
				if (CurTabInfo != ActiveTab && (!FocusTab.IsValid() || CurTabInfo->Priority < FocusTab->Priority))
				{
					FocusTab = CurTabInfo;
				}
			}

			CurTabInfo->TabLogLines.Add(CurLogEntry);

			auto CurLogListView = CurTabInfo->LogListView;

			if (bAutoScroll && CanAutoScroll(CurTabInfo))
			{
				CurLogListView->RequestScrollIntoView(CurLogEntry);
			}

			FString HighlightFindStr = (CurTabInfo->bHighlightFind && CurTabInfo->FindBox->GetVisibility() != EVisibility::Collapsed) ?
										CurTabInfo->LastFind : TEXT("");

			if (HighlightFindStr.Len() > 0 && LogLine->Contains(HighlightFindStr))
			{
				UpdateFindHighlight(CurTabInfo, true, CurTabInfo->LastFind);
			}
			else
			{
				CurLogListView->RequestListRefresh();
			}
		}
	}

	// If a focus change is required, perform it
	if (FocusTab.IsValid() && LogTabManager.IsValid())
	{
		LogTabManager->TryInvokeTab(FocusTab->TabIdName);
	}
}

void SAtomMonitorLogWidget::OnMonitorStateChanged(const FString& HostIp, uint16 HostPort, EAtomMonitorLogState InMonitorState)
{
	if (StartStopButtonText.IsValid())
	{
		if (InMonitorState == EAtomMonitorLogState::Started)
		{
			StartStopButtonText->SetText(LOCTEXT("AtomMonitorLogWidgetStopButton", "STOP"));
		}
		else if (InMonitorState == EAtomMonitorLogState::Stopped)
		{
			StartStopButtonText->SetText(LOCTEXT("AtomMonitorLogWidgetStartButton", "START"));
		}
	}

	if (ServerStatusText.IsValid())
	{
		if (InMonitorState == EAtomMonitorLogState::Started || InMonitorState == EAtomMonitorLogState::InGamePreviewing)
		{
			ServerStatusText->SetText(FText::FromString(FString::Printf(TEXT("%s:%d"), *HostIp, HostPort)));
		}
		else if (InMonitorState == EAtomMonitorLogState::Stopped)
		{
			ServerStatusText->SetText(FText::FromString(TEXT("Stopped")));
		}
	}

	SetServerStatusIcon(InMonitorState);
}

void SAtomMonitorLogWidget::OnMonitorRemoteStateChanged(const FString& ClientIp, bool bIsConnected)
{
	if (InGamePreviewStatusText.IsValid())
	{
		if (bIsConnected)
		{
			InGamePreviewStatusText->SetText(FText::FromString(FString::Printf(TEXT(" (%s)"), *ClientIp)));
			SetServerStatusIcon(EAtomMonitorLogState::InGamePreviewing);
		}
		else
		{
			InGamePreviewStatusText->SetText(FText::FromString(FString::Printf(TEXT(""))));
			SetServerStatusIcon(EAtomMonitorLogState::Started);
		}
	}
}

void SAtomMonitorLogWidget::SetServerStatusIcon(EAtomMonitorLogState InMonitorState) const
{
	if (ServerStatusBadgeIcon.IsValid())
	{
		if (InMonitorState == EAtomMonitorLogState::Started)
		{
			ServerStatusBackgroundIcon->SetImage(FAppStyle::Get().GetBrush("DerivedData.RemoteCache.IdleBG"));
			ServerStatusBadgeIcon->SetImage(FAppStyle::Get().GetBrush("DerivedData.RemoteCache.Idle"));
			ServerStatusBadgeIcon->SetColorAndOpacity(FSlateColor::UseForeground());
			ServerStatusUploadIcon->SetColorAndOpacity(FLinearColor::Transparent);
			ServerStatusDownloadIcon->SetColorAndOpacity(FLinearColor::Transparent);
		}
		else if (InMonitorState == EAtomMonitorLogState::InGamePreviewing)
		{
			ServerStatusBackgroundIcon->SetImage(FAppStyle::Get().GetBrush("DerivedData.RemoteCache.BusyBG"));
			ServerStatusBadgeIcon->SetImage(FAppStyle::Get().GetBrush("DerivedData.RemoteCache.Busy"));
			ServerStatusUploadIcon->SetColorAndOpacity(FLinearColor::White);
			ServerStatusDownloadIcon->SetColorAndOpacity(FLinearColor::White);
		}
		else if (InMonitorState == EAtomMonitorLogState::Stopped)
		{
			ServerStatusBackgroundIcon->SetImage(FAppStyle::Get().GetBrush("DerivedData.RemoteCache.Unavailable"));
			ServerStatusBadgeIcon->SetImage(nullptr);
			ServerStatusUploadIcon->SetColorAndOpacity(FLinearColor::Transparent);
			ServerStatusDownloadIcon->SetColorAndOpacity(FLinearColor::Transparent);
		}
	}
}


FReply SAtomMonitorLogWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	FReply ReturnVal = FReply::Unhandled();

	// Pass the input event on to the widget command handler
	if (LogWidgetCommands.IsValid() && LogWidgetCommands->ProcessCommandBindings(InKeyEvent))
	{
		ReturnVal = FReply::Handled();
	}

	return ReturnVal;
}

void SAtomMonitorLogWidget::OnClear()
{
	TSharedPtr<FAtomMonitorLogTabInfo> ActiveTabInfo = GetActiveTabInfo();

	if (ActiveTabInfo.IsValid() && ActiveTabInfo->LogListView.IsValid())
	{
		ActiveTabInfo->TabLogLines.Empty();
		ActiveTabInfo->LogListView->RequestListRefresh();
	}
}

bool SAtomMonitorLogWidget::CanClear() const
{
	TSharedPtr<FAtomMonitorLogTabInfo> ActiveTabInfo = GetActiveTabInfo();

	if (ActiveTabInfo.IsValid() && ActiveTabInfo->TabLogLines.Num() > 0)
	{
		return true;
	}

	return false;
}

void SAtomMonitorLogWidget::OnCopy()
{
	TSharedPtr<FAtomMonitorLogTabInfo> ActiveTabInfo = GetActiveTabInfo();

	if (ActiveTabInfo.IsValid() && ActiveTabInfo->LogListView.IsValid())
	{
		TArray<TSharedRef<FAtomMonitorLogLine>> SelectedLines = ActiveTabInfo->LogListView->GetSelectedItems();

		if (SelectedLines.Num())
		{
			// Make sure the selected items are sorted in descending order
			SelectedLines.Sort([&ActiveTabInfo](const TSharedRef<FAtomMonitorLogLine>& A, const TSharedRef<FAtomMonitorLogLine>& B)
			{
				return ActiveTabInfo->TabLogLines.IndexOfByKey(A) < ActiveTabInfo->TabLogLines.IndexOfByKey(B);
			});

			FString CopiedLines;

			for (int Line = 0; Line < SelectedLines.Num(); Line++)
			{
				CopiedLines += SelectedLines[Line]->LogLine.Get();

				if (Line < SelectedLines.Num() - 1)
				{
					CopiedLines += LINE_TERMINATOR;
				}
			}

			FPlatformApplicationMisc::ClipboardCopy(*CopiedLines);
		}
	}
}

bool SAtomMonitorLogWidget::CanCopy() const
{
	TSharedPtr<FAtomMonitorLogTabInfo> ActiveTabInfo = GetActiveTabInfo();

	if (ActiveTabInfo.IsValid() && ActiveTabInfo->LogListView.IsValid() && ActiveTabInfo->LogListView->GetNumItemsSelected() > 0)
	{
		return true;
	}

	return false;
}

void SAtomMonitorLogWidget::OnFind()
{
	TSharedPtr<FAtomMonitorLogTabInfo> ActiveTabInfo = GetActiveTabInfo();

	if (ActiveTabInfo.IsValid())
	{
		// Make the find bar visible
		for (auto CurFindWidget : ActiveTabInfo->FindWidgets)
		{
			if (CurFindWidget.IsValid())
			{
				CurFindWidget->SetVisibility(EVisibility::Visible);
			}
		}

		// Hide the search wrap-around label, by default
		if (ActiveTabInfo->FindErrorLabel->GetVisibility() != EVisibility::Collapsed)
		{
			ActiveTabInfo->FindErrorLabel->SetVisibility(EVisibility::Collapsed);
		}

		FString HighlightFindStr = ActiveTabInfo->bHighlightFind ? ActiveTabInfo->LastFind : TEXT("");

		if (HighlightFindStr.Len() > 0)
		{
			UpdateFindHighlight(ActiveTabInfo, true, ActiveTabInfo->LastFind);
		}

		FSlateApplication::Get().SetKeyboardFocus(ActiveTabInfo->FindBox);
	}
}

bool SAtomMonitorLogWidget::CanFind() const
{
	return GetActiveTabInfo().IsValid();
}

void SAtomMonitorLogWidget::ScrollToEnd(TSharedRef<FAtomMonitorLogTabInfo> InTab)
{
	auto CurLogListView = InTab->LogListView;
	auto& CurTabLogLines = InTab->TabLogLines;

	if (CurLogListView.IsValid() && CurTabLogLines.Num() > 0)
	{
		CurLogListView->RequestScrollIntoView(CurTabLogLines[CurTabLogLines.Num()-1]);
		CurLogListView->RequestListRefresh();
	}
}

void SAtomMonitorLogWidget::ScrollToText(TSharedRef<FAtomMonitorLogTabInfo> InTab, FString FindText, bool bSearchUp/*=false*/)
{
	auto CurLogListView = InTab->LogListView;
	auto& CurTabLogLines = InTab->TabLogLines;

	InTab->LastFind = FindText;

	if (CurLogListView.IsValid() && CurTabLogLines.Num() > 0)
	{
		int32 FindStartIdx = INDEX_NONE;
		TArray<TSharedRef<FAtomMonitorLogLine>> SelectedLines = CurLogListView->GetSelectedItems();

		if (SelectedLines.Num() > 0)
		{
			FindStartIdx = CurTabLogLines.IndexOfByKey(SelectedLines[0]);
		}
		else if (bSearchUp)
		{
			FindStartIdx = CurTabLogLines.Num()-1;
		}
		else
		{
			FindStartIdx = 0;
		}

		int32 SearchDir = (bSearchUp ? -1 : 1);
		int32 FoundIdx = INDEX_NONE;
		bool bSearchWrapped = false;

		for (int32 Index = FindStartIdx + SearchDir; true; Index += SearchDir)
		{
			// When the start/end of the array is reached, wrap-around to the other end
			if (Index < 0 || Index >= CurTabLogLines.Num())
			{
				Index = (bSearchUp ? CurTabLogLines.Num()-1 : 0);
				bSearchWrapped = true;
			}

			// Moved out of 'for' condition, and into the loop, to avoid infinite recursion in rare circumstances
			if (Index == FindStartIdx)
			{
				break;
			}

			if (CurTabLogLines[Index]->LogLine->Contains(FindText))
			{
				FoundIdx = Index;
				break;
			}
		}

		if (FoundIdx != INDEX_NONE)
		{
			CurLogListView->SetSelection(CurTabLogLines[FoundIdx], ESelectInfo::OnKeyPress);
			CurLogListView->RequestScrollIntoView(CurTabLogLines[FoundIdx]);

			if (!InTab->bHighlightFind)
			{
				CurLogListView->RequestListRefresh();
			}
		}

		if (InTab->bHighlightFind)
		{
			UpdateFindHighlight(InTab, true, FindText);
		}

		// If the search wrapped-around or failed, displaying the error label briefly
		TSharedPtr<SBorder> ErrorLabel = InTab->FindErrorLabel;

		if (FoundIdx == INDEX_NONE || bSearchWrapped)
		{
			TSharedPtr<STextBlock> ErrorText = InTab->FindErrorText;

			if (FoundIdx == INDEX_NONE)
			{
				ErrorText->SetText(FText::FromString(TEXT("Failed to find text.")));
			}
			else if (bSearchUp)
			{
				ErrorText->SetText(FText::FromString(TEXT("Searched past top of list. Starting again from bottom.")));
			}
			else
			{
				ErrorText->SetText(FText::FromString(TEXT("Searched past bottom of list. Starting again from top.")));
			}

			ErrorLabel->SetBorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 1.f));
			ErrorLabel->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 1.f));
			ErrorLabel->SetVisibility(EVisibility::HitTestInvisible);
		}
		// Otherwise, hide the label
		else
		{
			ErrorLabel->SetVisibility(EVisibility::Collapsed);
		}

		InTab->bLastFindWasUp = bSearchUp;
	}
}

bool SAtomMonitorLogWidget::CanAutoScroll(TSharedPtr<FAtomMonitorLogTabInfo> InTab)
{
	bool bReturnVal = true;

	if (InTab.IsValid())
	{
		// If the current tab is currently displaying the find bar, auto scrolling is not possible
		if (InTab->FindBox.IsValid() && InTab->FindBox->GetVisibility() != EVisibility::Collapsed)
		{
			bReturnVal = false;
		}
	}

	return bReturnVal;
}

void SAtomMonitorLogWidget::UpdateFindHighlight(TSharedPtr<FAtomMonitorLogTabInfo> InTab, bool bHighlight, FString HighlightText/*=TEXT("")*/)
{
	if (!bHighlight || HighlightText.Len() == 0)
	{
		for (TSharedRef<FAtomMonitorLogLine>& CurLine : InTab->TabLogLines)
		{
			CurLine->LogHighlight.Empty();
		}
	}
	else
	{
		for (TSharedRef<FAtomMonitorLogLine>& CurLine : InTab->TabLogLines)
		{
			if (CurLine->LogLine->Contains(HighlightText))
			{
				CurLine->LogHighlight = HighlightText;
			}
			else
			{
				CurLine->LogHighlight.Empty();
			}
		}
	}

	InTab->LogListView->RebuildList();
	InTab->LogListView->Invalidate(EInvalidateWidget::Layout);
}

void SAtomMonitorLogWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	TSharedPtr<FAtomMonitorLogTabInfo> CurActiveTab = GetActiveTabInfo();

	if (CurActiveTab.IsValid())
	{
		TSharedPtr<SBorder> ActiveFindLabel = CurActiveTab->FindErrorLabel;

		if (ActiveFindLabel.IsValid() && ActiveFindLabel->GetVisibility() != EVisibility::Collapsed)
		{
			const float FadeDuration = 2.f;
			FLinearColor CurColor = CALL_PROTECTED(SBorder, ActiveFindLabel, GetBorderBackgroundColor)().GetSpecifiedColor();

			CurColor.A -= (InDeltaTime / FadeDuration);

			if (CurColor.A > 0.1f)
			{
				ActiveFindLabel->SetBorderBackgroundColor(CurColor);
				ActiveFindLabel->SetColorAndOpacity(CurColor);
			}
			else
			{
				ActiveFindLabel->SetVisibility(EVisibility::Collapsed);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
