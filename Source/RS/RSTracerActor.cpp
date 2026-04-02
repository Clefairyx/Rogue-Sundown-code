#include "RSTracerActor.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

ARSTracerActor::ARSTracerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	TracerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TracerMesh"));
	RootComponent = TracerMesh;

	TracerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TracerMesh->SetGenerateOverlapEvents(false);
	TracerMesh->SetCastShadow(false);
}

void ARSTracerActor::InitTracer(const FVector& Start, const FVector& End, float LifeSeconds)
{
	const FVector Direction = End - Start;
	const float Length = Direction.Size();

	if (Length <= KINDA_SMALL_NUMBER)
	{
		Destroy();
		return;
	}

	const FVector Midpoint = Start + (Direction * 0.5f);
	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(Start, End);

	SetActorLocation(Midpoint);

	// Default cylinder points along local Z, so rotate it
	SetActorRotation(LookAtRotation + FRotator(90.f, 0.f, 0.f));

	// Built-in cylinder is usually 200 units tall, not 100
	const float MeshLength = 200.0f;
	const float LengthScale = Length / MeshLength;

	// Thin beam
	SetActorScale3D(FVector(0.03f, 0.03f, LengthScale));

	SetLifeSpan(LifeSeconds);
}