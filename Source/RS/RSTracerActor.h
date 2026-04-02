#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RSTracerActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class RS_API ARSTracerActor : public AActor
{
	GENERATED_BODY()

public:
	ARSTracerActor();

	void InitTracer(const FVector& Start, const FVector& End, float LifeSeconds = 0.05f);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* TracerMesh;
};