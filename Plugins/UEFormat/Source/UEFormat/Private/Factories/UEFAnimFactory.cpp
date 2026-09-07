#include "Factories/UEFAnimFactory.h"
#include "ComponentReregisterContext.h"
#include "Animation/AnimSequence.h"
#include "Widgets/Anim/UEFAnimImportOptions.h"
#include "Widgets/Anim/UEFAnimWidget.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IMainFrameModule.h"
#include "Misc/FeedbackContext.h"
#include "Misc/ScopedSlowTask.h"
#include "Readers/UEFAnimReader.h"
#include "Widgets/SWindow.h"

UEFAnimFactory::UEFAnimFactory(const FObjectInitializer & ObjectInitializer): Super(ObjectInitializer)
{
	Formats.Add(TEXT("ueanim; UEANIM Animation File"));
	SupportedClass = UAnimSequence::StaticClass();
	bCreateNew = false;
	bEditorImport = true;
	SettingsImporter = CreateDefaultSubobject<UEFAnimImportOptions>(TEXT("Anim Options"));
}

UObject* UEFAnimFactory::FactoryCreateFile(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, const FString& Filename, const TCHAR* Params, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	FScopedSlowTask SlowTask(5, NSLOCTEXT("UEFAnimFactory", "BeginReadUEAnimFile", "Reading UEAnim file"), true);
	if (Warn->GetScopeStack().Num() == 0)
		SlowTask.MakeDialog(true);

	SlowTask.EnterProgressFrame(0);

	UEFAnimReader Data = UEFAnimReader(Filename);
	if (!Data.Read())
		return nullptr;

	if (SettingsImporter->bInitialized == false)
	{
		TSharedPtr<UEFAnimWidget> ImportOptionsWindow;
		TSharedPtr<SWindow> ParentWindow;
		if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
		{
			IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
			ParentWindow = MainFrame.GetParentWindow();
		}

		TSharedRef<SWindow> Window = SNew(SWindow).Title(FText::FromString(TEXT("Animation Import Options"))).SizingRule(ESizingRule::Autosized);
		Window->SetContent(SAssignNew(ImportOptionsWindow, UEFAnimWidget).WidgetWindow(Window));
		SettingsImporter = ImportOptionsWindow.Get()->Stun;
		FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);
		bImport = ImportOptionsWindow.Get()->ShouldImport();
		bImportAll = ImportOptionsWindow.Get()->ShouldImportAll();
		SettingsImporter->bInitialized = true;
	}

	UAnimSequence* AnimSequence = NewObject<UAnimSequence>(Parent, Name, Flags);
	USkeleton* Skeleton = SettingsImporter->Skeleton;

	AnimSequence->SetSkeleton(Skeleton);
	AnimSequence->SequenceLength = Data.NumFrames > 1 ? (Data.NumFrames - 1) / Data.FramesPerSecond : 0.f;
	AnimSequence->SetRawNumberOfFrame(Data.NumFrames);

	FScopedSlowTask ImportTask(Data.Tracks.Num(), FText::FromString("Importing UEAnim Animation"));
	ImportTask.MakeDialog(false);

	for (const auto& Track : Data.Tracks)
	{
		ImportTask.EnterProgressFrame();

		FName BoneName = Track.TrackName.c_str();
		auto PosKeys = Track.TrackPosKeys;
		auto RotKeys = Track.TrackRotKeys;
		auto ScaleKeys = Track.TrackScaleKeys;

		TArray<FVector> FinalPosKeys;
		TArray<FQuat> FinalRotKeys;
		TArray<FVector> FinalScaleKeys;
		FinalPosKeys.SetNum(Data.NumFrames);
		FinalRotKeys.SetNum(Data.NumFrames);
		FinalScaleKeys.SetNum(Data.NumFrames);

		FVector PrevPos = FVector::ZeroVector;
		FQuat PrevRot = FQuat::Identity;
		FVector PrevScale = FVector::OneVector;

		int PosIndex = 0, RotIndex = 0, ScaleIndex = 0;
		for (auto j = 0; j < Data.NumFrames; j++)
		{
			if (PosIndex < PosKeys.Num() && PosKeys[PosIndex].Frame == j)
			{
				FinalPosKeys[j] = PosKeys[PosIndex].VectorValue;
				PrevPos = PosKeys[PosIndex].VectorValue;
				PosIndex++;
			}
			else
				FinalPosKeys[j] = PrevPos;
			
			if (RotIndex < RotKeys.Num() && RotKeys[RotIndex].Frame == j)
			{
				FinalRotKeys[j] = RotKeys[RotIndex].QuatValue;
				PrevRot = RotKeys[RotIndex].QuatValue;
				RotIndex++;
			}
			else
				FinalRotKeys[j] = PrevRot;

			if (ScaleIndex < ScaleKeys.Num() && ScaleKeys[ScaleIndex].Frame == j)
			{
				FinalScaleKeys[j] = ScaleKeys[ScaleIndex].VectorValue;
				PrevScale = ScaleKeys[ScaleIndex].VectorValue;
				ScaleIndex++;
			}
			else
				FinalScaleKeys[j] = PrevScale;
		}

		FRawAnimSequenceTrack RawTrack;
		RawTrack.PosKeys = MoveTemp(FinalPosKeys);
		RawTrack.RotKeys = MoveTemp(FinalRotKeys);
		RawTrack.ScaleKeys = MoveTemp(FinalScaleKeys);
		AnimSequence->AddNewRawTrack(BoneName, &RawTrack);
	}

	for (auto i = 0; i < Data.Curves.Num(); i++)
	{
		ImportTask.EnterProgressFrame();

		TArray<FRichCurveKey> RichCurves;
		for (const auto& Key : Data.Curves[i].CurveKeys)
		{
			FRichCurveKey RichKey;
			RichKey.Time = Key.Frame / Data.FramesPerSecond;
			RichKey.Value = Key.FloatValue;
			RichCurves.Add(RichKey);
		}

		FSmartName CurveName;
		Skeleton->AddSmartNameAndModify(USkeleton::AnimCurveMappingName, Data.Curves[i].CurveName.c_str(), CurveName);
		AnimSequence->RawCurveData.AddCurveData(CurveName, 0, ERawCurveTrackTypes::RCT_Float);
		if (FFloatCurve* Curve = static_cast<FFloatCurve*>(AnimSequence->RawCurveData.GetCurveData(CurveName.UID, ERawCurveTrackTypes::RCT_Float)))
			Curve->FloatCurve.SetKeys(RichCurves);
	}
	
	if (!bImportAll)
		SettingsImporter->bInitialized = false;

	AnimSequence->MarkRawDataAsModified();
	AnimSequence->PostEditChange();

	FAssetRegistryModule::AssetCreated(AnimSequence);
	FGlobalComponentReregisterContext RecreateComponents;

	return AnimSequence;
}
