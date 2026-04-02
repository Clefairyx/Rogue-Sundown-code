#include "RSAIController.h"
#include "RSCharacter.h"

#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <cfloat>

ARSAIController::ARSAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	bWantsPlayerState = true;
}

void ARSAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledBot = Cast<ARSCharacter>(InPawn);
	CurrentTarget = nullptr;
	CurrentState = ESimpleBotState::Roam;

	CurrentRoamDestination = FVector::ZeroVector;
	LastRoamDestination = FVector::ZeroVector;
	LastKnownTargetLocation = FVector::ZeroVector;

	NextTargetSearchTime = 0.0f;
	NextRoamRefreshTime = 0.0f;
	NextShootTime = 0.0f;
	NextStateRefreshTime = 0.0f;
	NextStuckCheckTime = 0.0f;

	LastStuckCheckLocation = FVector::ZeroVector;
	ConsecutiveStuckChecks = 0;

	if (!ControlledBot)
	{
		return;
	}

	ControlledBot->bUseControllerRotationYaw = false;

	if (UCharacterMovementComponent* MoveComp = ControlledBot->GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 620.0f, 0.0f);
		MoveComp->BrakingDecelerationWalking = 1600.0f;
		MoveComp->GroundFriction = 10.0f;
	}

	const float RandomYaw = FMath::FRandRange(0.0f, 360.0f);
	CurrentRoamDirection = FRotator(0.0f, RandomYaw, 0.0f).Vector().GetSafeNormal2D();

	PickNewRoamDestination();
	ResetStuckState();
}

void ARSAIController::OnUnPossess()
{
	Super::OnUnPossess();

	ControlledBot = nullptr;
	CurrentTarget = nullptr;
	CurrentRoamDestination = FVector::ZeroVector;
	LastRoamDestination = FVector::ZeroVector;
	LastKnownTargetLocation = FVector::ZeroVector;
	ResetStuckState();
}

void ARSAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateBotMovement(DeltaSeconds);
	UpdateStuckDetection();
}

void ARSAIController::UpdateBotMovement(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	if (!World || !ControlledBot || ControlledBot->IsDead())
	{
		return;
	}

	if (World->TimeSeconds >= NextTargetSearchTime)
	{
		UpdateTarget();
		NextTargetSearchTime = World->TimeSeconds + TargetSearchInterval;
	}

	if (CurrentTarget && IsValidCombatTarget(CurrentTarget))
	{
		CurrentState = ESimpleBotState::Seek;
		HandleSeekMovement(DeltaSeconds);
	}
	else
	{
		CurrentState = ESimpleBotState::Roam;
		HandleRoamMovement(DeltaSeconds);
	}
}

void ARSAIController::UpdateTarget()
{
	CurrentTarget = FindNearestEnemy();

	if (CurrentTarget)
	{
		LastKnownTargetLocation = CurrentTarget->GetActorLocation();
	}
}

ARSCharacter* ARSAIController::FindNearestEnemy() const
{
	UWorld* World = GetWorld();
	if (!World || !ControlledBot)
	{
		return nullptr;
	}

	ARSCharacter* BestTarget = nullptr;
	float BestScore = -FLT_MAX;

	for (TActorIterator<ARSCharacter> It(World); It; ++It)
	{
		ARSCharacter* Other = *It;
		if (!Other || Other == ControlledBot || Other->IsDead())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(
			ControlledBot->GetActorLocation(),
			Other->GetActorLocation());

		if (DistSq > FMath::Square(TargetSearchRange))
		{
			continue;
		}

		float Score = -FMath::Sqrt(DistSq);

		if (HasLineOfSightToTarget(Other))
		{
			Score += 450.0f;
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Other;
		}
	}

	return BestTarget;
}

bool ARSAIController::IsValidCombatTarget(const ARSCharacter* Target) const
{
	if (!ControlledBot || !Target || Target == ControlledBot || Target->IsDead())
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(
		ControlledBot->GetActorLocation(),
		Target->GetActorLocation());

	return DistSq <= FMath::Square(LoseTargetRange);
}

bool ARSAIController::HasLineOfSightToTarget(const ARSCharacter* Target) const
{
	UWorld* World = GetWorld();
	if (!World || !ControlledBot || !Target)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(SimpleBotLOS), false);
	Params.AddIgnoredActor(ControlledBot);

	const FVector Start = ControlledBot->GetActorLocation() + FVector(0.f, 0.f, EyeHeightOffset);

	const FVector AimPoints[] =
	{
		Target->GetActorLocation() + FVector(0.f, 0.f, 70.f),
		Target->GetActorLocation() + FVector(0.f, 0.f, 100.f),
		Target->GetActorLocation() + FVector(0.f, 0.f, 120.f)
	};

	for (const FVector& End : AimPoints)
	{
		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(
			Hit,
			Start,
			End,
			ECC_Visibility,
			Params);

		if (!bHit || Hit.GetActor() == Target)
		{
			return true;
		}
	}

	return false;
}

void ARSAIController::FaceTarget(const ARSCharacter* Target, float DeltaSeconds)
{
	if (!ControlledBot || !Target)
	{
		return;
	}

	const FVector Start = ControlledBot->GetActorLocation() + FVector(0.0f, 0.0f, EyeHeightOffset);

	FVector TargetOrigin;
	FVector TargetExtent;
	Target->GetActorBounds(true, TargetOrigin, TargetExtent);

	FVector End = TargetOrigin + FVector(0.0f, 0.0f, TargetExtent.Z * 0.65f);

	const float HeightDelta = End.Z - Start.Z;
	if (HeightDelta > 0.0f)
	{
		End.Z += FMath::Clamp(HeightDelta * 0.10f, 0.0f, 25.0f);
	}

	const FRotator DesiredAim = (End - Start).Rotation();
	SetControlRotation(DesiredAim);

	const FRotator DesiredYaw(0.0f, DesiredAim.Yaw, 0.0f);
	const FRotator SmoothedYaw = FMath::RInterpTo(
		ControlledBot->GetActorRotation(),
		DesiredYaw,
		DeltaSeconds,
		CombatTurnInterpSpeed);

	ControlledBot->AISetFacingRotation(SmoothedYaw);

	UE_LOG(LogTemp, Warning, TEXT("Bot Aim Pitch: %f"), DesiredAim.Pitch);
}

void ARSAIController::FaceMovementDirection(const FVector& MoveDir, float DeltaSeconds)
{
	if (!ControlledBot || MoveDir.IsNearlyZero())
	{
		return;
	}

	const FRotator DesiredYaw(0.0f, MoveDir.GetSafeNormal2D().Rotation().Yaw, 0.0f);

	const FRotator SmoothedYaw = FMath::RInterpTo(
		ControlledBot->GetActorRotation(),
		DesiredYaw,
		DeltaSeconds,
		CombatTurnInterpSpeed);

	ControlledBot->AISetFacingRotation(SmoothedYaw);
}

void ARSAIController::HandleRoamMovement(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	if (!World || !ControlledBot)
	{
		return;
	}

	if (ControlledBot->IsReloading())
	{
		ControlledBot->AIStopSprint();
		return;
	}

	const bool bNeedNewDestination =
		CurrentRoamDestination.IsNearlyZero() ||
		FVector::Dist2D(ControlledBot->GetActorLocation(), CurrentRoamDestination) <= ReachedDestinationDistance ||
		World->TimeSeconds >= NextRoamRefreshTime;

	if (bNeedNewDestination)
	{
		PickNewRoamDestination();
		NextRoamRefreshTime = World->TimeSeconds + RoamRefreshInterval;
	}

	FVector DesiredMoveDir = CurrentRoamDestination - ControlledBot->GetActorLocation();
	DesiredMoveDir.Z = 0.0f;
	DesiredMoveDir = DesiredMoveDir.GetSafeNormal2D();

	if (DesiredMoveDir.IsNearlyZero())
	{
		return;
	}

	FVector FinalMoveDir;
	if (FindAvoidanceDirection(DesiredMoveDir, FinalMoveDir))
	{
		FaceMovementDirection(FinalMoveDir, DeltaSeconds);
		ControlledBot->AIStartSprint();
		ApplyMoveInput(FinalMoveDir, 1.0f);
	}
	else
	{
		FVector EmergencyDir;
		if (FindEmergencyBypassDirection(DesiredMoveDir, EmergencyDir))
		{
			FaceMovementDirection(EmergencyDir, DeltaSeconds);
			ControlledBot->AIStartSprint();
			ApplyMoveInput(EmergencyDir, 0.85f);
		}
		else
		{
			PickNewRoamDestination();
		}
	}
}

void ARSAIController::HandleSeekMovement(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	if (!World || !ControlledBot || !CurrentTarget)
	{
		return;
	}

	const FVector BotLocation = ControlledBot->GetActorLocation();
	const FVector TargetLocation = CurrentTarget->GetActorLocation();
	const FVector ToTarget3D = TargetLocation - BotLocation;

	FVector ToTarget2D = ToTarget3D;
	ToTarget2D.Z = 0.0f;

	const float DistanceToTarget = ToTarget3D.Size();
	const bool bHasLOS = HasLineOfSightToTarget(CurrentTarget);
	const FVector SeparationDir = GetSeparationDirection();
	const float HeightDelta = TargetLocation.Z - BotLocation.Z;
	const bool bTargetIsHighAbove = HeightDelta > HighTargetHeightThreshold;

	FaceTarget(CurrentTarget, DeltaSeconds);
	TryReload();

	if (ControlledBot->IsReloading())
	{
		ControlledBot->AIStopSprint();

		if (DistanceToTarget < PreferredMinDistance * 1.08f && !ToTarget2D.IsNearlyZero())
		{
			FVector BackAwayDir = -ToTarget2D.GetSafeNormal2D();
			FVector FinalMoveDir;

			if (FindAvoidanceDirection(BackAwayDir, FinalMoveDir))
			{
				FaceMovementDirection(FinalMoveDir, DeltaSeconds);
				ApplyMoveInput(FinalMoveDir, 0.35f);
			}
			else
			{
				FVector EmergencyDir;
				if (FindEmergencyBypassDirection(BackAwayDir, EmergencyDir))
				{
					FaceMovementDirection(EmergencyDir, DeltaSeconds);
					ApplyMoveInput(EmergencyDir, 0.30f);
				}
			}
		}

		return;
	}

	TryShoot(DistanceToTarget);

	if (bHasLOS && bTargetIsHighAbove && DistanceToTarget < PreferredMaxDistance * 0.95f)
	{
		ControlledBot->AIStopSprint();

		FVector BackAwayDir = -ToTarget2D.GetSafeNormal2D();
		if (!SeparationDir.IsNearlyZero())
		{
			BackAwayDir = (BackAwayDir + SeparationDir * HighTargetSeparationWeight).GetSafeNormal2D();
		}

		if (!BackAwayDir.IsNearlyZero())
		{
			FVector FinalMoveDir;
			if (FindAvoidanceDirection(BackAwayDir, FinalMoveDir))
			{
				FaceTarget(CurrentTarget, DeltaSeconds);
				ApplyMoveInput(FinalMoveDir, HighTargetBackAwayStrength);
			}
			else
			{
				FVector EmergencyDir;
				if (FindEmergencyBypassDirection(BackAwayDir, EmergencyDir))
				{
					FaceTarget(CurrentTarget, DeltaSeconds);
					ApplyMoveInput(EmergencyDir, 0.70f);
				}
			}
		}

		TryShoot(DistanceToTarget);
		return;
	}

	if (bHasLOS && DistanceToTarget >= PreferredMinDistance && DistanceToTarget <= PreferredMaxDistance)
	{
		ControlledBot->AIStopSprint();
	}

	if (DistanceToTarget < PreferredMinDistance * 1.08f)
	{
		ControlledBot->AIStopSprint();

		FVector BackAwayDir = (-ToTarget2D.GetSafeNormal2D() + SeparationDir * 0.35f).GetSafeNormal2D();
		if (BackAwayDir.IsNearlyZero())
		{
			BackAwayDir = -ToTarget2D.GetSafeNormal2D();
		}

		FVector FinalMoveDir;
		if (FindAvoidanceDirection(BackAwayDir, FinalMoveDir))
		{
			FaceMovementDirection(FinalMoveDir, DeltaSeconds);
			ApplyMoveInput(FinalMoveDir, BackAwayMoveStrength);
		}
		else
		{
			FVector EmergencyDir;
			if (FindEmergencyBypassDirection(BackAwayDir, EmergencyDir))
			{
				FaceMovementDirection(EmergencyDir, DeltaSeconds);
				ApplyMoveInput(EmergencyDir, 0.70f);
			}
			else
			{
				FaceMovementDirection(BackAwayDir, DeltaSeconds);
				ApplyMoveInput(BackAwayDir, 0.45f);
			}
		}
		return;
	}

	if (DistanceToTarget > PreferredMaxDistance || !bHasLOS || DistanceToTarget > ShootRange * 0.85f)
	{
		FVector SeekDir = ToTarget2D.GetSafeNormal2D();

		if (!bHasLOS && !LastKnownTargetLocation.IsNearlyZero())
		{
			SeekDir = (LastKnownTargetLocation - BotLocation).GetSafeNormal2D();
		}

		if (SeekDir.IsNearlyZero())
		{
			return;
		}

		FVector FinalMoveDir;
		if (FindAvoidanceDirection(SeekDir, FinalMoveDir))
		{
			FaceMovementDirection(FinalMoveDir, DeltaSeconds);
			ControlledBot->AIStartSprint();
			ApplyMoveInput(FinalMoveDir, SeekMoveStrength);
		}
		else
		{
			FVector EmergencyDir;
			if (FindEmergencyBypassDirection(SeekDir, EmergencyDir))
			{
				FaceMovementDirection(EmergencyDir, DeltaSeconds);
				ControlledBot->AIStartSprint();
				ApplyMoveInput(EmergencyDir, 0.85f);
			}
			else
			{
				ControlledBot->AIStopSprint();
			}
		}

		return;
	}

	ControlledBot->AIStopSprint();

	if (!SeparationDir.IsNearlyZero() && FMath::FRand() < HoldPositionNudgeChance)
	{
		FaceMovementDirection(SeparationDir, DeltaSeconds);
		ApplyMoveInput(SeparationDir, HoldPositionNudgeStrength);
	}
}

void ARSAIController::TryShoot(float DistanceToTarget)
{
	UWorld* World = GetWorld();
	if (!World || !ControlledBot || !CurrentTarget)
	{
		return;
	}

	if (ControlledBot->IsReloading())
	{
		return;
	}

	if (World->TimeSeconds < NextShootTime)
	{
		return;
	}

	if (!ControlledBot->AICanShoot())
	{
		return;
	}

	if (DistanceToTarget > ShootRange)
	{
		return;
	}

	if (!HasLineOfSightToTarget(CurrentTarget))
	{
		return;
	}

	ControlledBot->AIStartShoot();
	NextShootTime = World->TimeSeconds + FMath::FRandRange(MinShootInterval, MaxShootInterval);
}

void ARSAIController::TryReload()
{
	if (!ControlledBot)
	{
		return;
	}

	if (ControlledBot->IsReloading())
	{
		return;
	}

	if (ControlledBot->GetAmmo() <= 0 && ControlledBot->AICanReload())
	{
		ControlledBot->AIStartReload();
	}
}

void ARSAIController::PickNewRoamDestination()
{
	UWorld* World = GetWorld();
	if (!World || !ControlledBot)
	{
		return;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		CurrentRoamDestination = ControlledBot->GetActorLocation();
		return;
	}

	const FVector BotLocation = ControlledBot->GetActorLocation();

	for (int32 Attempt = 0; Attempt < MaxRoamPointAttempts; ++Attempt)
	{
		const float YawOffset = FMath::FRandRange(-RoamDirectionJitterAngle, RoamDirectionJitterAngle);
		const FVector CandidateDirection =
			FRotator(0.0f, YawOffset, 0.0f).RotateVector(CurrentRoamDirection).GetSafeNormal2D();

		const float CandidateDistance = FMath::FRandRange(MinRoamDistance, MaxRoamDistance);
		const FVector DesiredPoint = BotLocation + (CandidateDirection * CandidateDistance);

		FNavLocation NavLocation;
		const bool bFoundNavPoint = NavSys->ProjectPointToNavigation(
			DesiredPoint,
			NavLocation,
			FVector(300.0f, 300.0f, 500.0f));

		if (!bFoundNavPoint)
		{
			continue;
		}

		if (!LastRoamDestination.IsNearlyZero())
		{
			const float DistFromLast = FVector::Dist2D(LastRoamDestination, NavLocation.Location);
			if (DistFromLast < MinDistanceFromLastRoamPoint)
			{
				continue;
			}
		}

		LastRoamDestination = CurrentRoamDestination;
		CurrentRoamDestination = NavLocation.Location;
		CurrentRoamDirection = (CurrentRoamDestination - BotLocation).GetSafeNormal2D();
		ResetStuckState();
		return;
	}

	CurrentRoamDestination = BotLocation;
}

bool ARSAIController::IsPathBlocked(const FVector& MoveDir, float Distance, float ProbeHeight) const
{
	UWorld* World = GetWorld();
	if (!World || !ControlledBot || MoveDir.IsNearlyZero())
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SimpleBotPathProbe), false);
	Params.AddIgnoredActor(ControlledBot);

	const FVector Start = ControlledBot->GetActorLocation() + FVector(0.0f, 0.0f, ProbeHeight);
	const FVector End = Start + (MoveDir.GetSafeNormal2D() * Distance);

	return World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
}

bool ARSAIController::FindAvoidanceDirection(const FVector& DesiredMoveDir, FVector& OutMoveDir) const
{
	if (!ControlledBot || DesiredMoveDir.IsNearlyZero())
	{
		return false;
	}

	const FVector ForwardDir = DesiredMoveDir.GetSafeNormal2D();

	if (!IsPathBlocked(ForwardDir, ForwardObstacleProbeDistance, LowProbeHeight))
	{
		OutMoveDir = ForwardDir;
		return true;
	}

	const FVector LeftDir =
		FRotator(0.0f, -DiagonalProbeAngle, 0.0f).RotateVector(ForwardDir).GetSafeNormal2D();
	const FVector RightDir =
		FRotator(0.0f, DiagonalProbeAngle, 0.0f).RotateVector(ForwardDir).GetSafeNormal2D();

	const bool bLeftFree = !IsPathBlocked(LeftDir, ForwardObstacleProbeDistance, LowProbeHeight);
	const bool bRightFree = !IsPathBlocked(RightDir, ForwardObstacleProbeDistance, LowProbeHeight);

	if (bLeftFree && !bRightFree)
	{
		OutMoveDir = LeftDir;
		return true;
	}

	if (bRightFree && !bLeftFree)
	{
		OutMoveDir = RightDir;
		return true;
	}

	if (bLeftFree && bRightFree)
	{
		OutMoveDir = FMath::RandBool() ? LeftDir : RightDir;
		return true;
	}

	return FindEmergencyBypassDirection(DesiredMoveDir, OutMoveDir);
}

bool ARSAIController::FindEmergencyBypassDirection(const FVector& DesiredMoveDir, FVector& OutMoveDir) const
{
	if (!ControlledBot || DesiredMoveDir.IsNearlyZero())
	{
		return false;
	}

	const FVector ForwardDir = DesiredMoveDir.GetSafeNormal2D();

	const FVector Left45 = FRotator(0.0f, -45.0f, 0.0f).RotateVector(ForwardDir).GetSafeNormal2D();
	const FVector Right45 = FRotator(0.0f, 45.0f, 0.0f).RotateVector(ForwardDir).GetSafeNormal2D();
	const FVector HardLeft = FVector::CrossProduct(FVector::UpVector, ForwardDir).GetSafeNormal2D();
	const FVector HardRight = FVector::CrossProduct(ForwardDir, FVector::UpVector).GetSafeNormal2D();
	const FVector BackDir = -ForwardDir;

	const FVector Candidates[5] = { Left45, Right45, HardLeft, HardRight, BackDir };

	for (const FVector& Candidate : Candidates)
	{
		if (!IsPathBlocked(Candidate, ForwardObstacleProbeDistance, LowProbeHeight))
		{
			OutMoveDir = Candidate;
			return true;
		}
	}

	return false;
}

FVector ARSAIController::GetSeparationDirection() const
{
	UWorld* World = GetWorld();
	if (!World || !ControlledBot)
	{
		return FVector::ZeroVector;
	}

	FVector Separation = FVector::ZeroVector;

	for (TActorIterator<ARSCharacter> It(World); It; ++It)
	{
		ARSCharacter* Other = *It;
		if (!Other || Other == ControlledBot || Other->IsDead())
		{
			continue;
		}

		const FVector ToMe = ControlledBot->GetActorLocation() - Other->GetActorLocation();
		const float Dist = ToMe.Size2D();

		if (Dist > KINDA_SMALL_NUMBER && Dist < PersonalSpaceDistance)
		{
			Separation += ToMe.GetSafeNormal2D() * (1.0f - (Dist / PersonalSpaceDistance));
		}
	}

	return Separation.GetSafeNormal2D();
}

void ARSAIController::UpdateStuckDetection()
{
	UWorld* World = GetWorld();
	if (!World || !ControlledBot)
	{
		return;
	}

	if (World->TimeSeconds < NextStuckCheckTime)
	{
		return;
	}

	const FVector CurrentLocation = ControlledBot->GetActorLocation();

	if (!LastStuckCheckLocation.IsNearlyZero())
	{
		const float DistanceMoved = FVector::Dist2D(CurrentLocation, LastStuckCheckLocation);

		if (DistanceMoved < StuckDistanceThreshold)
		{
			++ConsecutiveStuckChecks;
		}
		else
		{
			ConsecutiveStuckChecks = 0;
		}

		if (ConsecutiveStuckChecks >= 2)
		{
			ConsecutiveStuckChecks = 0;

			if (CurrentState == ESimpleBotState::Roam)
			{
				PickNewRoamDestination();
			}
			else
			{
				if (CurrentTarget)
				{
					FVector ToTarget = CurrentTarget->GetActorLocation() - ControlledBot->GetActorLocation();
					ToTarget.Z = 0.0f;

					FVector EmergencyDir;
					if (FindEmergencyBypassDirection(ToTarget.GetSafeNormal2D(), EmergencyDir))
					{
						FaceMovementDirection(EmergencyDir, World->GetDeltaSeconds());
						ApplyMoveInput(EmergencyDir, 0.85f);
					}
				}
			}
		}
	}

	LastStuckCheckLocation = CurrentLocation;
	NextStuckCheckTime = World->TimeSeconds + StuckCheckInterval;
}

void ARSAIController::ResetStuckState()
{
	LastStuckCheckLocation = FVector::ZeroVector;
	NextStuckCheckTime = 0.0f;
	ConsecutiveStuckChecks = 0;
}

void ARSAIController::ApplyMoveInput(const FVector& WorldDirection, float Scale)
{
	if (!ControlledBot || WorldDirection.IsNearlyZero())
	{
		return;
	}

	const FVector Dir = WorldDirection.GetSafeNormal2D();

	const float Right = FVector::DotProduct(Dir, ControlledBot->GetActorRightVector());
	const float Forward = FVector::DotProduct(Dir, ControlledBot->GetActorForwardVector());

	ControlledBot->DoMove(Right * Scale, Forward * Scale);
}