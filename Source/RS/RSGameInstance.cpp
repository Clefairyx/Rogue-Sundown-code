#include "RSGameInstance.h"

#include "MoviePlayer.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/Texture2D.h"
#include "Engine/LocalPlayer.h"

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"

#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"

class SRSLoadingScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRSLoadingScreen) {}
		SLATE_ARGUMENT(TWeakObjectPtr<URSGameInstance>, GameInstance)
		SLATE_ARGUMENT(UTexture2D*, BackgroundTexture)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		GameInstance = InArgs._GameInstance;
		DisplayedProgress = 0.0f;

		if (InArgs._BackgroundTexture)
		{
			BackgroundBrush = MakeShared<FSlateBrush>();
			BackgroundBrush->SetResourceObject(InArgs._BackgroundTexture);
			BackgroundBrush->DrawAs = ESlateBrushDrawType::Image;
			BackgroundBrush->ImageSize = FVector2D(1920.0f, 1080.0f);
		}

		ChildSlot
			[
				SNew(SOverlay)

					// Background image
					+ SOverlay::Slot()
					[
						SNew(SImage)
							.Image(BackgroundBrush.IsValid() ? BackgroundBrush.Get() : FCoreStyle::Get().GetBrush("WhiteBrush"))
					]

					// Slight dark overlay so text/progress are readable
					+ SOverlay::Slot()
					[
						SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
							.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.25f))
					]

					// Center content
					+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Bottom)
					.Padding(0.f, 0.f, 0.f, 140.f)
					[
						SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(0, 0, 0, 16)
							[
								SNew(STextBlock)
									.Text(FText::FromString(TEXT("Loading...")))
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							[
								SNew(SBox)
									.WidthOverride(400.0f)
									[
										SNew(SProgressBar)
											.Percent(this, &SRSLoadingScreen::GetProgressBarPercent)
									]
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(0, 12, 0, 0)
							[
								SNew(STextBlock)
									.Text(this, &SRSLoadingScreen::GetProgressText)
							]
					]
			];
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

		if (GameInstance.IsValid())
		{
			DisplayedProgress = GameInstance->GetCurrentLoadingProgress();
		}
	}

private:
	TOptional<float> GetProgressBarPercent() const
	{
		return DisplayedProgress;
	}

	FText GetProgressText() const
	{
		const int32 PercentInt = FMath::RoundToInt(DisplayedProgress * 100.0f);
		return FText::FromString(FString::Printf(TEXT("%d%%"), PercentInt));
	}

private:
	TWeakObjectPtr<URSGameInstance> GameInstance;
	TSharedPtr<FSlateBrush> BackgroundBrush;
	float DisplayedProgress = 0.0f;
};

void URSGameInstance::Init()
{
	Super::Init();

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &URSGameInstance::BeginLoadingScreen);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &URSGameInstance::EndLoadingScreen);
}

void URSGameInstance::Shutdown()
{
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	Super::Shutdown();
}

void URSGameInstance::SetupLocalPlayersForGame()
{
	const int32 DesiredPlayers = GetDesiredLocalPlayerCount();

	UE_LOG(LogTemp, Warning, TEXT("RSGameInstance: SetupLocalPlayersForGame called. DesiredPlayers = %d | ActualExistingPlayers = %d"),
		DesiredPlayers,
		Super::GetNumLocalPlayers());

	// Remove extras first
	while (Super::GetNumLocalPlayers() > DesiredPlayers)
	{
		const int32 LastIndex = Super::GetNumLocalPlayers() - 1;
		ULocalPlayer* LastPlayer = GetLocalPlayerByIndex(LastIndex);

		if (!LastPlayer)
		{
			UE_LOG(LogTemp, Warning, TEXT("RSGameInstance: Failed to get local player at index %d for removal."), LastIndex);
			break;
		}

		UE_LOG(LogTemp, Warning, TEXT("RSGameInstance: Removing local player at index %d | %s"),
			LastIndex,
			*GetNameSafe(LastPlayer));

		RemoveLocalPlayer(LastPlayer);
	}

	// Add missing players
	for (int32 i = Super::GetNumLocalPlayers(); i < DesiredPlayers; ++i)
	{
		FString Error;
		ULocalPlayer* NewPlayer = CreateLocalPlayer(i, Error, true);

		if (!Error.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("RSGameInstance: CreateLocalPlayer failed for index %d: %s"), i, *Error);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("RSGameInstance: Created local player %d | Result: %s"),
				i,
				*GetNameSafe(NewPlayer));
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("RSGameInstance: SetupLocalPlayersForGame complete. ActualExistingPlayers = %d"),
		Super::GetNumLocalPlayers());
}

void URSGameInstance::ResetToSingleLocalPlayer()
{
	UE_LOG(LogTemp, Warning, TEXT("RSGameInstance: ResetToSingleLocalPlayer called."));

	NumLocalPlayers = 1;

	while (Super::GetNumLocalPlayers() > 1)
	{
		const int32 LastIndex = Super::GetNumLocalPlayers() - 1;
		ULocalPlayer* LastPlayer = GetLocalPlayerByIndex(LastIndex);

		if (!LastPlayer)
		{
			UE_LOG(LogTemp, Warning, TEXT("RSGameInstance: Failed to get local player at index %d for removal."), LastIndex);
			break;
		}

		UE_LOG(LogTemp, Warning, TEXT("RSGameInstance: Removing extra local player at index %d | %s"),
			LastIndex,
			*GetNameSafe(LastPlayer));

		RemoveLocalPlayer(LastPlayer);
	}
}

void URSGameInstance::TravelToLevel(const FName LevelName)
{
	PendingLevelName = LevelName;
	CurrentLoadingProgress = 0.0f;

	UE_LOG(LogTemp, Log, TEXT("RSGameInstance: TravelToLevel called for %s"), *LevelName.ToString());

	UGameplayStatics::OpenLevel(this, LevelName);
}

void URSGameInstance::BeginLoadingScreen(const FString& MapName)
{
	UE_LOG(LogTemp, Log, TEXT("RSGameInstance: BeginLoadingScreen for %s"), *MapName);

	CurrentLoadingProgress = 0.2f;

	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = false;
	LoadingScreen.bWaitForManualStop = true;
	LoadingScreen.bAllowEngineTick = false;
	LoadingScreen.MinimumLoadingScreenDisplayTime = 0.5f;
	LoadingScreen.WidgetLoadingScreen =
		SNew(SRSLoadingScreen)
		.GameInstance(TWeakObjectPtr<URSGameInstance>(this))
		.BackgroundTexture(LoadingBackgroundTexture);

	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
}

void URSGameInstance::EndLoadingScreen(UWorld* LoadedWorld)
{
	UE_LOG(LogTemp, Log, TEXT("RSGameInstance: EndLoadingScreen"));

	PendingLevelName = NAME_None;
}

void URSGameInstance::StopLoadingScreen()
{
	if (GetMoviePlayer()->IsMovieCurrentlyPlaying())
	{
		UE_LOG(LogTemp, Log, TEXT("RSGameInstance: Stopping loading screen manually"));
		GetMoviePlayer()->StopMovie();
	}
}