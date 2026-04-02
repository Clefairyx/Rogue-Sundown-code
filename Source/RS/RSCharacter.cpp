// Copyright Epic Games, Inc. All Rights Reserved.

#include "RSCharacter.h"

#include "RS.h"
#include "RSPlayerController.h"
#include "RSGameMode.h"
#include "RSPlayerHUD.h"
#include "RSPlayerState.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "RSTracerActor.h"
#include "TimerManager.h"

ARSCharacter::ARSCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = false;

	GetCharacterMovement()->JumpZVelocity = 300.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	BodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetMesh());
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetLeaderPoseComponent(GetMesh());

	HeadMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(GetMesh());
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadMesh->SetLeaderPoseComponent(GetMesh());

	HoodMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HoodMesh"));
	HoodMesh->SetupAttachment(GetMesh());
	HoodMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HoodMesh->SetLeaderPoseComponent(GetMesh());

	Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Gun"));
	Gun->SetupAttachment(GetMesh());
	Gun->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

ARSPlayerState* ARSCharacter::GetRSPlayerState() const
{
	return GetPlayerState<ARSPlayerState>();
}

URSPlayerHUD* ARSCharacter::GetPlayerHUDWidget() const
{
	const ARSPlayerController* PC = Cast<ARSPlayerController>(GetController());
	if (!PC)
	{
		return nullptr;
	}

	return PC->GetPlayerHUDWidget();
}

void ARSCharacter::ApplyPlayerMaterials()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyPlayerMaterials: No PlayerController"));
		return;
	}

	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyPlayerMaterials: No LocalPlayer"));
		return;
	}

	const int32 PlayerIndex = LP->GetLocalPlayerIndex();

	UE_LOG(LogTemp, Warning, TEXT("ApplyPlayerMaterials: Character %s got PlayerIndex %d"),
		*GetName(), PlayerIndex);

	if (!PlayerMaterialSets.IsValidIndex(PlayerIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyPlayerMaterials: Invalid PlayerMaterialSets index %d. Num=%d"),
			PlayerIndex, PlayerMaterialSets.Num());
		return;
	}

	const FPlayerMaterialSet& Set = PlayerMaterialSets[PlayerIndex];

	if (HoodMesh && Set.HoodMaterial)
	{
		HoodMesh->SetMaterial(0, Set.HoodMaterial);
	}

	if (BodyMesh && Set.BodyMaterial)
	{
		BodyMesh->SetMaterial(1, Set.BodyMaterial);
	}
}

void ARSCharacter::SyncHUDState()
{
	UpdateHUDPlayerName();
	UpdateHUDHealth();
	UpdateHUDAmmo();
	UpdateHealingHUD();
}

void ARSCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (Gun)
	{
		Gun->AttachToComponent(
			GetMesh(),
			FAttachmentTransformRules::SnapToTargetIncludingScale,
			TEXT("GunSocket")
		);
	}

	if (BodyMesh && GetMesh())
	{
		BodyMesh->SetLeaderPoseComponent(GetMesh());
	}

	if (HoodMesh && GetMesh())
	{
		HoodMesh->SetLeaderPoseComponent(GetMesh());
	}
	if (HeadMesh && GetMesh())
	{
		HeadMesh->SetLeaderPoseComponent(GetMesh());
	}

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	Health = MaxHealth;
	Ammo = MaxAmmo;
	bIsDead = false;
	bIsHealing = false;
	ActiveShootMontage = nullptr;
	bIsShooting = false;
	bIsReloading = false;
	bIsHitReacting = false;
	bIsMeleeing = false;
	RawMoveInput = FVector2D::ZeroVector;
	MeleeActorsHit.Empty();

	GetWorldTimerManager().SetTimerForNextTick(this, &ARSCharacter::SyncHUDState);
}

FLinearColor ARSCharacter::GetHealthColor() const
{
	if (Health >= 50)
	{
		return FLinearColor::Green;
	}
	else if (Health >= 25)
	{
		return FLinearColor::Yellow;
	}

	return FLinearColor::Red;
}

void ARSCharacter::UpdateHUDHealth()
{
	if (URSPlayerHUD* PlayerHUD = GetPlayerHUDWidget())
	{
		const float HealthPercent = MaxHealth > 0 ? static_cast<float>(Health) / static_cast<float>(MaxHealth) : 0.0f;
		PlayerHUD->UpdateHealthUI(HealthPercent, GetHealthColor());
	}
}

void ARSCharacter::UpdateHUDAmmo()
{
	if (URSPlayerHUD* PlayerHUD = GetPlayerHUDWidget())
	{
		PlayerHUD->UpdateAmmoUI(Ammo, MaxAmmo);
	}
}

void ARSCharacter::UpdateHUDPlayerName()
{
	if (URSPlayerHUD* PlayerHUD = GetPlayerHUDWidget())
	{
		if (APlayerState* PS = GetPlayerState())
		{
			PlayerHUD->UpdatePlayerNameUI(PS->GetPlayerName());
		}
	}
}

void ARSCharacter::UpdateHealingHUD()
{
	if (URSPlayerHUD* PlayerHUD = GetPlayerHUDWidget())
	{
		PlayerHUD->SetCrosshairVisible(!bIsHealing);
		PlayerHUD->SetHealingTextVisible(bIsHealing);
	}
}

void ARSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UE_LOG(LogRS, Warning, TEXT("SetupPlayerInputComponent called on %s"), *GetNameSafe(this));

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		UE_LOG(LogRS, Error, TEXT("SetupPlayerInputComponent: No PlayerController found on %s"), *GetNameSafe(this));
	}
	else
	{
		ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
		if (!LocalPlayer)
		{
			UE_LOG(LogRS, Error, TEXT("SetupPlayerInputComponent: No LocalPlayer found on %s"), *GetNameSafe(this));
		}
		else
		{
			UEnhancedInputLocalPlayerSubsystem* Subsystem =
				LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

			if (!Subsystem)
			{
				UE_LOG(LogRS, Error, TEXT("SetupPlayerInputComponent: No EnhancedInput subsystem found on %s"), *GetNameSafe(this));
			}
			else if (!DefaultMappingContext)
			{
				UE_LOG(LogRS, Error, TEXT("SetupPlayerInputComponent: DefaultMappingContext is NOT assigned on %s"), *GetNameSafe(this));
			}
			else
			{
				Subsystem->ClearAllMappings();
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
				UE_LOG(LogRS, Warning, TEXT("SetupPlayerInputComponent: Added mapping context %s"), *GetNameSafe(DefaultMappingContext));
			}
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		UE_LOG(LogRS, Warning, TEXT("EnhancedInputComponent found on %s"), *GetNameSafe(this));

		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ARSCharacter::DoJumpStart);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ARSCharacter::DoJumpEnd);
		}
		else
		{
			UE_LOG(LogRS, Error, TEXT("JumpAction is NOT assigned"));
		}

		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARSCharacter::Move);
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ARSCharacter::Move);
		}
		else
		{
			UE_LOG(LogRS, Error, TEXT("MoveAction is NOT assigned"));
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ARSCharacter::DoControllerLook);
		}
		else
		{
			UE_LOG(LogRS, Error, TEXT("LookAction is NOT assigned"));
		}

		if (MouseLookAction)
		{
			EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ARSCharacter::DoMouseLook);
		}
		else
		{
			UE_LOG(LogRS, Warning, TEXT("MouseLookAction is NOT assigned"));
		}

		if (RollOrCrouchAction)
		{
			EnhancedInputComponent->BindAction(RollOrCrouchAction, ETriggerEvent::Started, this, &ARSCharacter::HandleRollOrCrouch);
		}
		else
		{
			UE_LOG(LogRS, Error, TEXT("RollOrCrouchAction is NOT assigned"));
		}

		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ARSCharacter::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ARSCharacter::StopSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ARSCharacter::StopSprint);
		}
		else
		{
			UE_LOG(LogRS, Error, TEXT("SprintAction is NOT assigned"));
		}

		if (ShootAction)
		{
			EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Started, this, &ARSCharacter::ShootInputStarted);
		}
		else
		{
			UE_LOG(LogRS, Warning, TEXT("ShootAction is NOT assigned"));
		}

		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ARSCharacter::ReloadInputStarted);
		}
		else
		{
			UE_LOG(LogRS, Warning, TEXT("ReloadAction is NOT assigned"));
		}

		if (MeleeAction)
		{
			EnhancedInputComponent->BindAction(MeleeAction, ETriggerEvent::Started, this, &ARSCharacter::MeleeInputStarted);
		}
		else
		{
			UE_LOG(LogRS, Warning, TEXT("MeleeAction is NOT assigned"));
		}

		if (HealAction)
		{
			EnhancedInputComponent->BindAction(HealAction, ETriggerEvent::Started, this, &ARSCharacter::HealInputStarted);
		}
		else
		{
			UE_LOG(LogRS, Warning, TEXT("HealAction is NOT assigned"));
		}
	}
	else
	{
		UE_LOG(LogRS, Error, TEXT("'%s' Failed to find an Enhanced Input component!"), *GetNameSafe(this));
	}
}

void ARSCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (BodyMesh && GetMesh())
	{
		BodyMesh->SetLeaderPoseComponent(GetMesh());
	}

	if (HoodMesh && GetMesh())
	{
		HoodMesh->SetLeaderPoseComponent(GetMesh());
	}

	if (HeadMesh && GetMesh())
	{
		HeadMesh->SetLeaderPoseComponent(GetMesh());
	}

	if (Cast<APlayerController>(NewController))
	{
		ApplyPlayerMaterials();
		SyncHUDState();
	}

	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		PC->bForceFeedbackEnabled = true;
	}
}

void ARSCharacter::Move(const FInputActionValue& Value)
{
	UE_LOG(LogRS, Verbose, TEXT("Move fired"));

	if (bIsDead || bIsRolling)
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void ARSCharacter::DoMove(float Right, float Forward)
{
	RawMoveInput = FVector2D(Right, Forward);

	if (GetController() != nullptr)
	{
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);

		FVector DesiredMoveDirection = (ForwardDirection * Forward) + (RightDirection * Right);
		DesiredMoveDirection.Z = 0.0f;

		bHasMovementInput = !DesiredMoveDirection.IsNearlyZero();

		if (bHasMovementInput)
		{
			DesiredMoveDirection.Normalize();
			LastMoveInputDirection = DesiredMoveDirection;
		}
	}
	else
	{
		bHasMovementInput = false;
		RawMoveInput = FVector2D::ZeroVector;
	}

	UpdateSprintState();
}

void ARSCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() == nullptr)
	{
		return;
	}

	if (FMath::IsNearlyZero(Yaw) && FMath::IsNearlyZero(Pitch))
	{
		bAimSlowdownActive = false;
		return;
	}

	UpdateAimAssist();

	const float Slowdown = bAimSlowdownActive ? AimSlowdownMultiplier : 1.0f;
	const float DT = GetWorld()->GetDeltaSeconds();

	AddControllerYawInput(Yaw * BaseLookSensitivityX * Slowdown * DT * 100.0f);
	AddControllerPitchInput(Pitch * BaseLookSensitivityY * Slowdown * DT * 100.0f);
}

void ARSCharacter::DoControllerLook(const FInputActionValue& Value)
{
	if (GetController() == nullptr)
	{
		return;
	}

	const FVector2D LookAxis = Value.Get<FVector2D>();
	DoLook(LookAxis.X, LookAxis.Y);
}

void ARSCharacter::DoMouseLook(const FInputActionValue& Value)
{
	if (GetController() == nullptr)
	{
		return;
	}

	const FVector2D LookAxis = Value.Get<FVector2D>();

	if (FMath::IsNearlyZero(LookAxis.X) && FMath::IsNearlyZero(LookAxis.Y))
	{
		return;
	}

	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(LookAxis.Y);
}

void ARSCharacter::UpdateAimAssist()
{
	bAimSlowdownActive = false;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !GetWorld())
	{
		return;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

	const FVector Forward = CameraRotation.Vector();
	const FVector SearchCenter = CameraLocation + (Forward * 135.0f);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AimAssistOverlap), false);
	QueryParams.AddIgnoredActor(this);

	TArray<FOverlapResult> Overlaps;
	const bool bFoundAny = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		SearchCenter,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(AimAssistRange),
		QueryParams
	);

	DrawDebugSphere(GetWorld(), SearchCenter, AimAssistRange, 24, FColor::Cyan, false, 0.05f);

	if (!bFoundAny)
	{
		return;
	}

	float BestScore = -1.0f;
	ARSCharacter* BestTarget = nullptr;

	for (const FOverlapResult& Result : Overlaps)
	{
		ARSCharacter* HitCharacter = Cast<ARSCharacter>(Result.GetActor());
		if (!HitCharacter || HitCharacter == this || HitCharacter->IsDead())
		{
			continue;
		}

		FVector TargetLocation = HitCharacter->GetActorLocation();

		if (UCapsuleComponent* TargetCapsule = HitCharacter->GetCapsuleComponent())
		{
			TargetLocation.Z += TargetCapsule->GetScaledCapsuleHalfHeight() * 0.6f;
		}

		const FVector ToTarget = TargetLocation - CameraLocation;
		const float Distance = ToTarget.Size();
		if (Distance <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FVector ToTargetDir = ToTarget / Distance;
		const float Dot = FVector::DotProduct(Forward, ToTargetDir);

		if (Dot < 0.94f)
		{
			continue;
		}

		FHitResult LOSHit;
		FCollisionQueryParams LOSParams(SCENE_QUERY_STAT(AimAssistLOS), false);
		LOSParams.AddIgnoredActor(this);
		LOSParams.AddIgnoredActor(HitCharacter);

		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
			LOSHit,
			CameraLocation,
			TargetLocation,
			ECC_Visibility,
			LOSParams
		);

		if (bBlocked)
		{
			continue;
		}

		const float DistanceScore = 1.0f - FMath::Clamp(Distance / AimAssistRange, 0.0f, 1.0f);
		const float Score = (Dot * 0.8f) + (DistanceScore * 0.2f);

		DrawDebugSphere(GetWorld(), TargetLocation, 20.0f, 12, FColor::Green, false, 0.05f);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = HitCharacter;
		}
	}

	if (BestTarget)
	{
		bAimSlowdownActive = true;
	}
}

void ARSCharacter::DoJumpStart()
{
	if (bIsDead || bIsRolling || bIsHitReacting || bIsMeleeing)
	{
		return;
	}

	if (bIsReloading)
	{
		CancelReload();
	}

	Jump();
}

void ARSCharacter::DoJumpEnd()
{
	StopJumping();
}

void ARSCharacter::HandleRollOrCrouch(const FInputActionValue& Value)
{
	if (bIsDead || bIsRolling || bIsSprinting || bIsShooting || bIsHitReacting || bIsMeleeing)
	{
		return;
	}

	if (bHasMovementInput)
	{
		StartRoll();
	}
}

void ARSCharacter::StartRoll()
{
	if (!bCanRoll || bIsDead || bIsRolling || bIsSprinting || bIsShooting || bIsHitReacting || bIsMeleeing)
	{
		return;
	}

	if (bIsReloading)
	{
		CancelReload();
	}

	if (bIsHealing)
	{
		CancelHeal();
	}

	if (!RollMontage1)
	{
		UE_LOG(LogRS, Warning, TEXT("StartRoll: RollMontage1 is not assigned."));
		return;
	}

	StopSprint();

	FVector MoveDirection = LastMoveInputDirection;

	if (MoveDirection.IsNearlyZero())
	{
		MoveDirection = GetActorForwardVector();
	}

	MoveDirection.Z = 0.0f;
	MoveDirection.Normalize();

	StoredRollDirection = MoveDirection;

	bIsRolling = true;
	bCanRoll = false;

	bUseControllerRotationYaw = false;
	SetActorRotation(StoredRollDirection.Rotation());
	GetCharacterMovement()->StopMovementImmediately();

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &ARSCharacter::OnRollMontageEnded);

		AnimInstance->Montage_Play(RollMontage1);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, RollMontage1);
	}
	else
	{
		UE_LOG(LogRS, Warning, TEXT("StartRoll: No AnimInstance found on character mesh."));
		EndRoll();
		return;
	}

	GetWorldTimerManager().SetTimer(
		RollCooldownTimerHandle,
		this,
		&ARSCharacter::ResetRoll,
		RollCooldown,
		false
	);
}

void ARSCharacter::OnRollMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == RollMontage1)
	{
		EndRoll();
	}
}

void ARSCharacter::EndRoll()
{
	bIsRolling = false;
	bUseControllerRotationYaw = true;
	UpdateSprintState();
}

void ARSCharacter::ResetRoll()
{
	bCanRoll = true;
}

bool ARSCharacter::CanSprint() const
{
	if (bIsDead || bIsRolling || bIsShooting || bIsReloading || bIsHitReacting || bIsMeleeing)
	{
		return false;
	}

	if (!bHasMovementInput)
	{
		return false;
	}

	if (RawMoveInput.Y <= 0.65f)
	{
		return false;
	}

	if (FMath::Abs(RawMoveInput.X) > 0.35f)
	{
		return false;
	}

	return true;
}

void ARSCharacter::StartSprint()
{
	if (bIsDead)
	{
		return;
	}

	if (bIsReloading)
	{
		CancelReload();
	}

	if (bIsHealing)
	{
		CancelHeal();
	}

	bSprintHeld = true;
	UpdateSprintState();
}

void ARSCharacter::StopSprint()
{
	bSprintHeld = false;
	UpdateSprintState();
}

void ARSCharacter::UpdateSprintState()
{
	const bool bShouldSprint = bSprintHeld && CanSprint();

	bIsSprinting = bShouldSprint;

	if (bIsHealing)
	{
		GetCharacterMovement()->MaxWalkSpeed = HealingWalkSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
	}
}

bool ARSCharacter::CanShoot() const
{
	return !bIsDead
		&& !bIsShooting
		&& !bIsReloading
		&& !bIsRolling
		&& !bIsHitReacting
		&& !bIsMeleeing
		&& Ammo > 0;
}

bool ARSCharacter::CanReload() const
{
	return !bIsDead
		&& !bIsReloading
		&& !bIsShooting
		&& !bIsRolling
		&& !bIsHitReacting
		&& !bIsMeleeing
		&& Ammo < MaxAmmo;
}

bool ARSCharacter::CanMelee() const
{
	return !bIsDead
		&& !bIsMeleeing
		&& !bIsRolling
		&& !bIsShooting
		&& !bIsHitReacting;
}

bool ARSCharacter::CanHeal() const
{
	return !bIsDead
		&& !bIsHealing
		&& !bIsShooting
		&& !bIsRolling
		&& !bIsHitReacting
		&& !bIsMeleeing
		&& Health < MaxHealth;
}

UAnimMontage* ARSCharacter::GetShootMontageToPlay() const
{
	const float Speed2D = GetVelocity().Size2D();

	if (Speed2D > 5.0f)
	{
		return MovingShootMontage;
	}

	return IdleShootMontage;
}

void ARSCharacter::ShootInputStarted()
{
	if (bIsHealing)
	{
		CancelHeal();
	}

	if (bIsSprinting)
	{
		StopSprint();
	}

	if (!CanShoot())
	{
		return;
	}

	StartShoot();
}

void ARSCharacter::ReloadInputStarted()
{
	if (bIsHealing)
	{
		CancelHeal();
	}

	if (bIsDead || bIsShooting || bIsRolling || bIsHitReacting || bIsReloading || bIsMeleeing)
	{
		return;
	}

	if (Ammo >= MaxAmmo)
	{
		return;
	}

	if (bIsSprinting)
	{
		StopSprint();
	}

	StartReload();
}

void ARSCharacter::MeleeInputStarted()
{
	if (bIsReloading)
	{
		CancelReload();
	}

	if (bIsHealing)
	{
		CancelHeal();
	}

	if (bIsSprinting)
	{
		StopSprint();
	}

	if (!CanMelee())
	{
		return;
	}

	StartMelee();
}

void ARSCharacter::HealInputStarted()
{
	if (bIsReloading)
	{
		CancelReload();
	}
	
	if (!CanHeal())
	{
		return;
	}

	if (bIsSprinting)
	{
		StopSprint();
	}

	StartHeal();
}

void ARSCharacter::StartShoot()
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogRS, Warning, TEXT("StartShoot: No AnimInstance found."));
		return;
	}

	UAnimMontage* MontageToPlay = GetShootMontageToPlay();
	if (!MontageToPlay)
	{
		UE_LOG(LogRS, Warning, TEXT("StartShoot: No shoot montage assigned."));
		return;
	}

	PerformShootTrace();

	if (ShootSound)
	{
		UGameplayStatics::PlaySound2D(this, ShootSound);
	}

	Ammo = FMath::Clamp(Ammo - 1, 0, MaxAmmo);
	UpdateHUDAmmo();

	bIsShooting = true;
	ActiveShootMontage = MontageToPlay;

	PlayControllerRumble(1.0f, 0.40f, true, true, true, true);

	AnimInstance->Montage_Play(MontageToPlay);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &ARSCharacter::OnShootMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, MontageToPlay);
}

void ARSCharacter::OnShootMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == ActiveShootMontage)
	{
		FinishShoot(bInterrupted);
	}
}

void ARSCharacter::FinishShoot(bool bInterrupted)
{
	bIsShooting = false;
	ActiveShootMontage = nullptr;

	if (!bInterrupted && Ammo == 0)
	{
		StartReload();
	}
}

void ARSCharacter::StartReload()
{
	if (!CanReload())
	{
		return;
	}

	if (bIsHealing)
	{
		CancelHeal();
	}

	if (bIsSprinting)
	{
		StopSprint();
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogRS, Warning, TEXT("StartReload: No AnimInstance found."));
		return;
	}

	if (!ReloadMontage)
	{
		UE_LOG(LogRS, Warning, TEXT("StartReload: ReloadMontage is not assigned."));
		return;
	}

	bIsReloading = true;

	AnimInstance->Montage_Play(ReloadMontage);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &ARSCharacter::OnReloadMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, ReloadMontage);
}

void ARSCharacter::OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == ReloadMontage)
	{
		FinishReload(bInterrupted);
	}
}

void ARSCharacter::FinishReload(bool bInterrupted)
{
	bIsReloading = false;

	if (!bInterrupted)
	{
		Ammo = MaxAmmo;
		UpdateHUDAmmo();
	}
}

void ARSCharacter::CancelReload()
{
	if (!bIsReloading)
	{
		return;
	}

	bIsReloading = false;

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (ReloadMontage)
		{
			AnimInstance->Montage_Stop(0.2f, ReloadMontage);
		}
	}
}

void ARSCharacter::StartMelee()
{
	if (!CanMelee())
	{
		return;
	}

	if (bIsHealing)
	{
		CancelHeal();
	}

	if (bIsSprinting)
	{
		StopSprint();
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogRS, Warning, TEXT("StartMelee: No AnimInstance found."));
		return;
	}

	if (!MeleeMontage)
	{
		UE_LOG(LogRS, Warning, TEXT("StartMelee: MeleeMontage is not assigned."));
		return;
	}

	bIsMeleeing = true;
	MeleeActorsHit.Empty();

	PlayControllerRumble(1.0f, 0.40f, true, true, true, true);

	AnimInstance->Montage_Play(MeleeMontage);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &ARSCharacter::OnMeleeMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, MeleeMontage);
}

void ARSCharacter::OnMeleeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == MeleeMontage)
	{
		FinishMelee(bInterrupted);
	}
}

void ARSCharacter::FinishMelee(bool bInterrupted)
{
	bIsMeleeing = false;
	MeleeActorsHit.Empty();
}

void ARSCharacter::StartHeal()
{
	if (!CanHeal())
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogRS, Warning, TEXT("StartHeal: No AnimInstance found."));
		return;
	}

	if (!HealMontage)
	{
		UE_LOG(LogRS, Warning, TEXT("StartHeal: HealMontage is not assigned."));
		return;
	}

	bIsHealing = true;
	bSprintHeld = false;
	bIsSprinting = false;

	if (ARSPlayerController* RSPC = Cast<ARSPlayerController>(GetController()))
	{
		RSPC->SetCrosshairVisible(false);
	}

	GetCharacterMovement()->MaxWalkSpeed = HealingWalkSpeed;
	UpdateHealingHUD();

	AnimInstance->Montage_Play(HealMontage);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &ARSCharacter::OnHealMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, HealMontage);

	GetWorldTimerManager().SetTimer(
		HealTimerHandle,
		this,
		&ARSCharacter::FinishHeal,
		HealDuration,
		false
	);
}

void ARSCharacter::FinishHeal()
{
	if (!bIsHealing)
	{
		return;
	}

	bIsHealing = false;
	GetWorldTimerManager().ClearTimer(HealTimerHandle);

	Health = FMath::Clamp(Health + HealAmount, 0, MaxHealth);
	UpdateHUDHealth();

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (HealMontage)
		{
			AnimInstance->Montage_Stop(0.2f, HealMontage);
		}
	}

	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	UpdateHealingHUD();
	UpdateSprintState();

	if (ARSPlayerController* RSPC = Cast<ARSPlayerController>(GetController()))
	{
		RSPC->SetCrosshairVisible(true);
	}
}

void ARSCharacter::CancelHeal()
{
	if (!bIsHealing)
	{
		return;
	}

	bIsHealing = false;
	GetWorldTimerManager().ClearTimer(HealTimerHandle);

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (HealMontage)
		{
			AnimInstance->Montage_Stop(0.2f, HealMontage);
		}
	}

	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	UpdateHealingHUD();
	UpdateSprintState();
}

void ARSCharacter::OnHealMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == HealMontage && bInterrupted && bIsHealing)
	{
		CancelHeal();
	}
}

void ARSCharacter::MeleeTraceNotify()
{
	if (!bIsMeleeing || bIsDead)
	{
		return;
	}

	PerformMeleeTrace();
}

void ARSCharacter::PerformMeleeTrace()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	const FName SocketName(TEXT("GunSocket"));

	if (!MeshComp->DoesSocketExist(SocketName))
	{
		UE_LOG(LogRS, Warning, TEXT("PerformMeleeTrace: Socket %s does not exist on mesh."), *SocketName.ToString());
		return;
	}

	const FVector TraceStart = MeshComp->GetSocketLocation(SocketName);
	const FRotator SocketRotation = MeshComp->GetSocketRotation(SocketName);
	const FVector TraceEnd = TraceStart + (SocketRotation.Vector() * MeleeTraceDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	TArray<FHitResult> HitResults;
	const bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(MeleeTraceRadius),
		QueryParams
	);

	DrawDebugSphere(GetWorld(), TraceStart, MeleeTraceRadius, 16, FColor::Blue, false, 1.5f);
	DrawDebugSphere(GetWorld(), TraceEnd, MeleeTraceRadius, 16, FColor::Green, false, 1.5f);
	DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Yellow, false, 1.5f, 0, 2.0f);

	if (!bHit)
	{
		return;
	}

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == this)
		{
			continue;
		}

		if (MeleeActorsHit.Contains(HitActor))
		{
			continue;
		}

		ApplyMeleeHitToActor(HitActor);
	}
}

void ARSCharacter::ApplyMeleeHitToActor(AActor* HitActor)
{
	ARSCharacter* HitCharacter = Cast<ARSCharacter>(HitActor);
	if (!HitCharacter || HitCharacter == this)
	{
		return;
	}

	MeleeActorsHit.Add(HitCharacter);
	HitCharacter->TakeCombatHit(MeleeDamageAmount, this);
}

void ARSCharacter::PerformShootTrace()
{
	FVector ViewLocation;
	FRotator ViewRotation;
	GetCombatViewPoint(ViewLocation, ViewRotation);

	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + (ViewRotation.Vector() * 10000.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredComponent(Gun);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	const FVector FinalTraceEnd = bHit ? HitResult.ImpactPoint : TraceEnd;

	FVector TracerStart = TraceStart;

	if (Gun && Gun->DoesSocketExist(GunMuzzleSocketName))
	{
		TracerStart = Gun->GetSocketLocation(GunMuzzleSocketName);
	}

	if (TracerActorClass)
	{
		ARSTracerActor* Tracer = GetWorld()->SpawnActor<ARSTracerActor>(
			TracerActorClass,
			TracerStart,
			FRotator::ZeroRotator
		);

		if (Tracer)
		{
			Tracer->InitTracer(TracerStart, FinalTraceEnd, 0.08f);
		}
	}

	if (bHit && HitResult.GetActor())
	{
		ApplyHitToActor(HitResult.GetActor());
	}
}

void ARSCharacter::ApplyHitToActor(AActor* HitActor)
{
	ARSCharacter* HitCharacter = Cast<ARSCharacter>(HitActor);
	if (HitCharacter && HitCharacter != this)
	{
		HitCharacter->TakeCombatHit(DamageAmount, this);
	}
}

void ARSCharacter::TakeCombatHit(int32 InDamage, ARSCharacter* Attacker)
{
	if (InDamage <= 0 || Health <= 0 || bIsDead)
	{
		return;
	}

	if (bIsRolling || bIsHitReacting)
	{
		return;
	}

	Health = FMath::Clamp(Health - InDamage, 0, MaxHealth);
	UpdateHUDHealth();

	if (Health <= 0)
	{
		if (ARSPlayerState* VictimPS = GetRSPlayerState())
		{
			VictimPS->AddDeath();
		}

		if (Attacker && Attacker != this)
		{
			if (ARSPlayerState* AttackerPS = Attacker->GetRSPlayerState())
			{
				AttackerPS->AddKill();
			}
		}

		Die();
		return;
	}

	ReceiveHitReact();
}

void ARSCharacter::ReceiveHitReact()
{
	if (bIsDead || bIsHitReacting || bIsRolling || !HitReactMontage)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	if (bIsHealing)
	{
		CancelHeal();
	}

	if (bIsSprinting)
	{
		StopSprint();
	}

	if (bIsReloading)
	{
		CancelReload();
	}

	if (bIsShooting && ActiveShootMontage)
	{
		AnimInstance->Montage_Stop(0.2f, ActiveShootMontage);
		bIsShooting = false;
		ActiveShootMontage = nullptr;
	}

	if (bIsMeleeing && MeleeMontage)
	{
		AnimInstance->Montage_Stop(0.2f, MeleeMontage);
		bIsMeleeing = false;
		MeleeActorsHit.Empty();
	}

	bIsHitReacting = true;

	PlayControllerRumble(1.0f, 0.60f, true, true, true, true);

	if (HitReactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitReactSound, GetActorLocation());
	}

	AnimInstance->Montage_Play(HitReactMontage);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &ARSCharacter::OnHitReactMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, HitReactMontage);
}

void ARSCharacter::OnHitReactMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == HitReactMontage)
	{
		FinishHitReact(bInterrupted);
	}
}

void ARSCharacter::FinishHitReact(bool bInterrupted)
{
	bIsHitReacting = false;
}

void ARSCharacter::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	if (ARSPlayerController* RSPC = Cast<ARSPlayerController>(GetController()))
	{
		RSPC->SetCrosshairVisible(false);
		RSPC->SetIgnoreMoveInput(true);
		RSPC->SetIgnoreLookInput(false);
	}

	if (bIsHealing)
	{
		CancelHeal();
	}

	if (bIsSprinting)
	{
		StopSprint();
	}

	if (bIsReloading)
	{
		CancelReload();
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;

	if (AnimInstance)
	{
		if (bIsShooting && ActiveShootMontage)
		{
			AnimInstance->Montage_Stop(0.2f, ActiveShootMontage);
			bIsShooting = false;
			ActiveShootMontage = nullptr;
		}

		if (bIsMeleeing && MeleeMontage)
		{
			AnimInstance->Montage_Stop(0.2f, MeleeMontage);
			bIsMeleeing = false;
			MeleeActorsHit.Empty();
		}

		if (bIsHitReacting && HitReactMontage)
		{
			AnimInstance->Montage_Stop(0.2f, HitReactMontage);
			bIsHitReacting = false;
		}
	}

	bIsRolling = false;
	bCanRoll = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
		MoveComp->Velocity = FVector::ZeroVector;
		MoveComp->bOrientRotationToMovement = false;
		bUseControllerRotationYaw = false;
		bUseControllerRotationPitch = false;
		bUseControllerRotationRoll = false;
	}

	SetActorRotation(GetActorRotation());

	if (CameraBoom)
	{
		CameraBoom->bUsePawnControlRotation = true;
	}

	if (FollowCamera)
	{
		FollowCamera->bUsePawnControlRotation = false;
	}

	if (!AnimInstance)
	{
		return;
	}

	if (DeathMontage)
	{
		AnimInstance->Montage_Play(DeathMontage);

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &ARSCharacter::OnDeathMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, DeathMontage);
	}
}

void ARSCharacter::OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == DeathMontage)
	{
		FinishDeath(bInterrupted);
	}
}

void ARSCharacter::FinishDeath(bool bInterrupted)
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (bDeathReported)
	{
		return;
	}

	bDeathReported = true;

	// Notify GameMode
	if (ARSGameMode* GM = Cast<ARSGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->HandlePlayerDeath(this);
	}
}

void ARSCharacter::PlayControllerRumble(float Intensity, float Duration, bool bLeftLarge, bool bLeftSmall, bool bRightLarge, bool bRightSmall)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		UE_LOG(LogRS, Warning, TEXT("Rumble failed: no PlayerController"));
		return;
	}

	if (!PC->IsLocalController())
	{
		UE_LOG(LogRS, Warning, TEXT("Rumble failed: controller is not local"));
		return;
	}

	UE_LOG(LogRS, Warning, TEXT("Rumble fired: %s intensity=%.2f duration=%.2f"), *GetName(), Intensity, Duration);

	FLatentActionInfo LatentInfo;
	PC->PlayDynamicForceFeedback(
		Intensity,
		Duration,
		bLeftLarge,
		bLeftSmall,
		bRightLarge,
		bRightSmall,
		EDynamicForceFeedbackAction::Start,
		LatentInfo
	);
}

void ARSCharacter::AIStartShoot()
{
	StartShoot();
}

void ARSCharacter::AIStartReload()
{
	StartReload();
}

void ARSCharacter::AIStartMelee()
{
	StartMelee();
}

void ARSCharacter::AIStartHeal()
{
	StartHeal();
}

void ARSCharacter::AIStartRoll()
{
	StartRoll();
}

void ARSCharacter::AIStartSprint()
{
	StartSprint();
}

void ARSCharacter::AIStopSprint()
{
	StopSprint();
}

bool ARSCharacter::AICanShoot() const
{
	return CanShoot();
}

bool ARSCharacter::AICanReload() const
{
	return CanReload();
}

bool ARSCharacter::AICanMelee() const
{
	return CanMelee();
}

bool ARSCharacter::AICanHeal() const
{
	return CanHeal();
}

void ARSCharacter::AISetFacingRotation(const FRotator& NewRotation)
{
	if (!Controller || IsDead() || bIsRolling)
	{
		return;
	}

	const FRotator CurrentControlRotation = Controller->GetControlRotation();
	const FRotator NewControlRotation(
		CurrentControlRotation.Pitch,
		NewRotation.Yaw,
		CurrentControlRotation.Roll
	);

	Controller->SetControlRotation(NewControlRotation);
	SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
}

void ARSCharacter::GetCombatViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	OutLocation = FVector::ZeroVector;
	OutRotation = FRotator::ZeroRotator;

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->GetPlayerViewPoint(OutLocation, OutRotation);
		return;
	}

	if (Controller)
	{
		OutLocation = GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
		OutRotation = Controller->GetControlRotation();
		return;
	}

	if (FollowCamera)
	{
		OutLocation = FollowCamera->GetComponentLocation();
		OutRotation = FollowCamera->GetComponentRotation();
		return;
	}

	OutLocation = GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
	OutRotation = GetActorRotation();
}