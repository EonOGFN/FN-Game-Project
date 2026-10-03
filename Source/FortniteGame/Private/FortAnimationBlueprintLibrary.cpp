#include "FortAnimationBlueprintLibrary.h"
#include "CustomCharacterPartAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
    float SampleWindChannel(float GameTime, float TimeOffset, float PerlinScale, float Frequency, float Amplitude, float Offset)
    {
        const float SamplePosition = ((GameTime + TimeOffset) * Frequency + Offset) * PerlinScale;
        return FMath::PerlinNoise1D(SamplePosition) * Amplitude;
    }
}

void UFortAnimationBlueprintLibrary::UpdateAnimSpinner(FAnimSpinner& SpinInformation, float DeltaSeconds, float NewRotationRate) {
}

float UFortAnimationBlueprintLibrary::GetRelevantAnimTimeRemainingFromCachedData(FCachedAnimRelevancyData& InCachedAnimRelevancyData, UAnimInstance* InAnimInstance) {
    return 0.0f;
}

float UFortAnimationBlueprintLibrary::GetRelevantAnimTimeRemainingFractionFromCachedData(FCachedAnimRelevancyData& InCachedAnimRelevancyData, UAnimInstance* InAnimInstance) {
    return 0.0f;
}

float UFortAnimationBlueprintLibrary::GetRelevantAnimTimeFromCachedData(FCachedAnimRelevancyData& InCachedAnimRelevancyData, UAnimInstance* InAnimInstance) {
    return 0.0f;
}

void UFortAnimationBlueprintLibrary::GetAlphaFromReferenceTransformDelta(float& AlphaOutput, float& RawOutput, const USkinnedMeshComponent* SkeletalMeshComponent, const FName BoneName, const FName BaseName, TEnumAsByte<EAlphaFromDeltaTypes::Type> DeltaComponentType, FVector2D InputRange, FVector2D OutputRange, FVector TwistAxis) {
}

FRotator UFortAnimationBlueprintLibrary::GetRandomWindRotator(float GameTime, float TimeOffset, float PerlinScale, const FRotator& Frequency, const FRotator& Amplitude, const FRotator& Offset) {
    return FRotator(
        SampleWindChannel(GameTime, TimeOffset, PerlinScale, Frequency.Pitch, Amplitude.Pitch, Offset.Pitch),
        SampleWindChannel(GameTime, TimeOffset, PerlinScale, Frequency.Yaw, Amplitude.Yaw, Offset.Yaw),
        SampleWindChannel(GameTime, TimeOffset, PerlinScale, Frequency.Roll, Amplitude.Roll, Offset.Roll));
}

FVector UFortAnimationBlueprintLibrary::GetRandomWindVector(float GameTime, float TimeOffset, float PerlinScale, const FVector& Frequency, const FVector& Amplitude, const FVector& Offset) {
    return FVector(
        SampleWindChannel(GameTime, TimeOffset, PerlinScale, Frequency.X, Amplitude.X, Offset.X),
        SampleWindChannel(GameTime, TimeOffset, PerlinScale, Frequency.Y, Amplitude.Y, Offset.Y),
        SampleWindChannel(GameTime, TimeOffset, PerlinScale, Frequency.Z, Amplitude.Z, Offset.Z));
}

FVector UFortAnimationBlueprintLibrary::ReorientWindVector(const FVector& WindVector, const UCustomCharacterPartAnimInstance* PartAnimInstance, FName JointName) {
    if (!IsValid(PartAnimInstance)) {
        return FVector::ZeroVector;
    }

    const USkeletalMeshComponent* MeshComponent = PartAnimInstance->GetOwnerSkeletalMeshComponent();
    if (!IsValid(MeshComponent)) {
        return FVector::ZeroVector;
    }

    return MeshComponent->GetSocketRotation(JointName).RotateVector(WindVector);
}

UFortAnimationBlueprintLibrary::UFortAnimationBlueprintLibrary() {
}

