#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UObject/NoExportTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Animation/CachedAnimData.h"
#include "AnimSpinner.h"
#include "EAlphaFromDeltaTypes.h"
#include "FortAnimationBlueprintLibrary.generated.h"

class UAnimInstance;
class UCustomCharacterPartAnimInstance;
class USkinnedMeshComponent;

UCLASS(Blueprintable)
class FORTNITEGAME_API UFortAnimationBlueprintLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    UFortAnimationBlueprintLibrary();
    UFUNCTION(BlueprintCallable)
    static void UpdateAnimSpinner(UPARAM(Ref) FAnimSpinner& SpinInformation, float DeltaSeconds, float NewRotationRate);
    
    UFUNCTION(BlueprintCallable, BlueprintPure)
    static float GetRelevantAnimTimeRemainingFromCachedData(UPARAM(Ref) FCachedAnimRelevancyData& InCachedAnimRelevancyData, UAnimInstance* InAnimInstance);
    
    UFUNCTION(BlueprintCallable, BlueprintPure)
    static float GetRelevantAnimTimeRemainingFractionFromCachedData(UPARAM(Ref) FCachedAnimRelevancyData& InCachedAnimRelevancyData, UAnimInstance* InAnimInstance);
    
    UFUNCTION(BlueprintCallable, BlueprintPure)
    static float GetRelevantAnimTimeFromCachedData(UPARAM(Ref) FCachedAnimRelevancyData& InCachedAnimRelevancyData, UAnimInstance* InAnimInstance);
    
    UFUNCTION(BlueprintCallable, BlueprintPure)
    static void GetAlphaFromReferenceTransformDelta(float& AlphaOutput, float& RawOutput, const USkinnedMeshComponent* SkeletalMeshComponent, const FName BoneName, const FName BaseName, TEnumAsByte<EAlphaFromDeltaTypes::Type> DeltaComponentType, FVector2D InputRange, FVector2D OutputRange, FVector TwistAxis);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Fortnite|Animation|Wind")
    static FRotator GetRandomWindRotator(float GameTime, float TimeOffset, float PerlinScale, const FRotator& Frequency, const FRotator& Amplitude, const FRotator& Offset);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Fortnite|Animation|Wind")
    static FVector GetRandomWindVector(float GameTime, float TimeOffset, float PerlinScale, const FVector& Frequency, const FVector& Amplitude, const FVector& Offset);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Fortnite|Animation|Wind")
    static FVector ReorientWindVector(const FVector& WindVector, const UCustomCharacterPartAnimInstance* PartAnimInstance, FName JointName);
    
};

