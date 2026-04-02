#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "RSAIController.generated.h"

class ARSCharacter;

UENUM()
enum class ESimpleBotState : uint8
{
	Roam,
	Seek
};

UCLASS()
class RS_API ARSAIController : public AAIController
{
	GENERATED_BODY()

public:
	ARSAIController();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	UPROPERTY()
	ARSCharacter* ControlledBot = nullptr;

	UPROPERTY()
	ARSCharacter* CurrentTarget = nullptr;

	ESimpleBotState CurrentState = ESimpleBotState::Roam;

	FVector CurrentRoamDestination = FVector::ZeroVector;
	FVector LastRoamDestination = FVector::ZeroVector;
	FVector CurrentRoamDirection = FVector::ForwardVector;
	FVector LastKnownTargetLocation = FVector::ZeroVector;

	float NextTargetSearchTime = 0.0f;
	float NextRoamRefreshTime = 0.0f;
	float NextShootTime = 0.0f;
	float NextStateRefreshTime = 0.0f;
	float NextStuckCheckTime = 0.0f;

	FVector LastStuckCheckLocation = FVector::ZeroVector;
	int32 ConsecutiveStuckChecks = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Awareness")
	float TargetSearchRange = 5500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Awareness")
	float LoseTargetRange = 7000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Awareness")
	float TargetSearchInterval = 0.10f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Awareness")
	float EyeHeightOffset = 70.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Awareness")
	float TargetAimHeightOffset = 110.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Combat")
	float ShootRange = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Combat")
	float PreferredMinDistance = 320.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Combat")
	float PreferredMaxDistance = 720.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Combat")
	float MinShootInterval = 0.14f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Combat")
	float MaxShootInterval = 0.24f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Combat")
	float CombatTurnInterpSpeed = 6.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Combat")
	float BackAwayMoveStrength = 0.92f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Combat")
	float SeekMoveStrength = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Combat")
	float HoldPositionNudgeStrength = 0.06f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Combat")
	float HoldPositionNudgeChance = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Combat")
	float StateRefreshInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Combat|Vertical")
	float HighTargetHeightThreshold = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Combat|Vertical")
	float HighTargetBackAwayStrength = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Combat|Vertical")
	float HighTargetSeparationWeight = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Roam")
	float RoamRefreshInterval = 1.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Roam")
	float MinRoamDistance = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Roam")
	float MaxRoamDistance = 1200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Roam")
	float RoamDirectionJitterAngle = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Roam")
	int32 MaxRoamPointAttempts = 16;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Roam")
	float MinDistanceFromLastRoamPoint = 250.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Movement")
	float ReachedDestinationDistance = 140.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Movement")
	float ForwardObstacleProbeDistance = 145.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Movement")
	float SideObstacleProbeDistance = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Movement")
	float LowProbeHeight = 45.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Movement")
	float DiagonalProbeAngle = 32.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Movement")
	float StuckDistanceThreshold = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Movement")
	float StuckCheckInterval = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Movement")
	float PersonalSpaceDistance = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Combat|Vertical")
	float HighTargetAimScale = 0.45f;

	UPROPERTY(EditDefaultsOnly, Category = "Bot|Combat|Vertical")
	float MaxExtraHighTargetAimOffset = 70.0f;

private:
	void UpdateBotMovement(float DeltaSeconds);
	void UpdateTarget();
	ARSCharacter* FindNearestEnemy() const;

	void HandleRoamMovement(float DeltaSeconds);
	void HandleSeekMovement(float DeltaSeconds);

	bool IsValidCombatTarget(const ARSCharacter* Target) const;
	bool HasLineOfSightToTarget(const ARSCharacter* Target) const;

	void FaceTarget(const ARSCharacter* Target, float DeltaSeconds);
	void FaceMovementDirection(const FVector& MoveDir, float DeltaSeconds);

	void TryShoot(float DistanceToTarget);
	void TryReload();

	void PickNewRoamDestination();

	bool IsPathBlocked(const FVector& MoveDir, float Distance, float ProbeHeight) const;
	bool FindAvoidanceDirection(const FVector& DesiredMoveDir, FVector& OutMoveDir) const;
	bool FindEmergencyBypassDirection(const FVector& DesiredMoveDir, FVector& OutMoveDir) const;

	FVector GetSeparationDirection() const;

	void UpdateStuckDetection();
	void ResetStuckState();

	void ApplyMoveInput(const FVector& WorldDirection, float Scale);
};