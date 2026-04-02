// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Logging/LogMacros.h"
#include "Animation/AnimMontage.h"
#include "Sound/SoundBase.h"
#include "Math/Vector2D.h"
#include "RSCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USkeletalMeshComponent;
class UMaterialInterface;
class ARSPlayerState;
class URSPlayerHUD;
class ARSTracerActor;
class USoundBase;

USTRUCT(BlueprintType)
struct FPlayerMaterialSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Customization")
	UMaterialInterface* HoodMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Customization")
	UMaterialInterface* BodyMaterial = nullptr;
};

UCLASS()
class RS_API ARSCharacter : public ACharacter
{
	GENERATED_BODY()

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

protected:

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Meshes", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Meshes", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* HoodMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Meshes", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* HeadMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Gun;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Customization", meta = (AllowPrivateAccess = "true"))
	TArray<FPlayerMaterialSet> PlayerMaterialSets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* MouseLookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* RollOrCrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* ShootAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* MeleeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* HealAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* RollMontage1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* IdleShootMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* MovingShootMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* MeleeMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HealMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ActiveShootMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll", meta = (AllowPrivateAccess = "true"))
	float RollCooldown = 0.8f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Roll", meta = (AllowPrivateAccess = "true"))
	bool bIsRolling = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Roll", meta = (AllowPrivateAccess = "true"))
	bool bCanRoll = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Roll", meta = (AllowPrivateAccess = "true"))
	FVector StoredRollDirection = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	FVector LastMoveInputDirection = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	bool bHasMovementInput = false;

	FTimerHandle RollCooldownTimerHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprint", meta = (AllowPrivateAccess = "true"))
	bool bSprintHeld = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprint", meta = (AllowPrivateAccess = "true"))
	bool bIsSprinting = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprint", meta = (AllowPrivateAccess = "true"))
	float WalkSpeed = 360.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprint", meta = (AllowPrivateAccess = "true"))
	float SprintSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Healing", meta = (AllowPrivateAccess = "true"))
	float HealingWalkSpeed = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Healing", meta = (AllowPrivateAccess = "true"))
	float HealDuration = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Healing", meta = (AllowPrivateAccess = "true"))
	int32 HealAmount = 12;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Healing", meta = (AllowPrivateAccess = "true"))
	bool bIsHealing = false;

	FTimerHandle HealTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	int32 MaxHealth = 100;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	int32 Health = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	USoundBase* ShootSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	USoundBase* HitReactSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	int32 Ammo = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	int32 MaxAmmo = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	int32 DamageAmount = 22;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	int32 MeleeDamageAmount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	float MeleeTraceRadius = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	float MeleeTraceDistance = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	bool bIsShooting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	bool bIsReloading = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	bool bIsHitReacting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	bool bIsDead = false;

	UPROPERTY()
	bool bDeathReported = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	bool bIsMeleeing = false;

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ARSTracerActor> TracerActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	FName GunMuzzleSocketName = TEXT("GunSocket");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	FVector2D RawMoveInput = FVector2D::ZeroVector;

	UPROPERTY()
	TArray<AActor*> MeleeActorsHit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim Assist", meta = (AllowPrivateAccess = "true"))
	float BaseLookSensitivityX = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim Assist", meta = (AllowPrivateAccess = "true"))
	float BaseLookSensitivityY = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim Assist", meta = (AllowPrivateAccess = "true"))
	float AimSlowdownMultiplier = 0.40f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim Assist", meta = (AllowPrivateAccess = "true"))
	float AimAssistRange = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim Assist", meta = (AllowPrivateAccess = "true"))
	float AimAssistRadius = 180.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aim Assist", meta = (AllowPrivateAccess = "true"))
	bool bAimSlowdownActive = false;

	void UpdateAimAssist();
	void ApplyPlayerMaterials();

protected:

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;

	void Move(const FInputActionValue& Value);
	void DoControllerLook(const FInputActionValue& Value);
	void DoMouseLook(const FInputActionValue& Value);

	void HandleRollOrCrouch(const FInputActionValue& Value);

	void StartRoll();
	void EndRoll();
	void ResetRoll();

	UFUNCTION()
	void OnRollMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void StartSprint();
	void StopSprint();
	void UpdateSprintState();
	bool CanSprint() const;

	void ShootInputStarted();
	void ReloadInputStarted();
	void MeleeInputStarted();
	void HealInputStarted();

	void StartShoot();
	void FinishShoot(bool bInterrupted);

	void StartReload();
	void FinishReload(bool bInterrupted);
	void CancelReload();

	void PlayControllerRumble(float Intensity, float Duration, bool bLeftLarge = true, bool bLeftSmall = false, bool bRightLarge = true, bool bRightSmall = true);

	void StartMelee();
	void FinishMelee(bool bInterrupted);

	void StartHeal();
	void FinishHeal();
	void CancelHeal();
	bool CanHeal() const;
	void UpdateHealingHUD();

	bool CanShoot() const;
	bool CanReload() const;
	bool CanMelee() const;

	UAnimMontage* GetShootMontageToPlay() const;

	void PerformShootTrace();
	void ApplyHitToActor(AActor* HitActor);

	void PerformMeleeTrace();
	void ApplyMeleeHitToActor(AActor* HitActor);

	void ReceiveHitReact();
	void FinishHitReact(bool bInterrupted);

	void Die();
	void FinishDeath(bool bInterrupted);

	void GetCombatViewPoint(FVector& OutLocation, FRotator& OutRotation) const;

	UFUNCTION()
	void OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnShootMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnHitReactMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnMeleeMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnHealMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void MeleeTraceNotify();

	ARSPlayerState* GetRSPlayerState() const;
	URSPlayerHUD* GetPlayerHUDWidget() const;

	void UpdateHUDHealth();
	void UpdateHUDAmmo();
	void UpdateHUDPlayerName();
	FLinearColor GetHealthColor() const;

public:

	ARSCharacter();

	void SyncHUDState();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(float Yaw, float Pitch);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void TakeCombatHit(int32 InDamage, ARSCharacter* Attacker);

	UFUNCTION(BlueprintPure, Category = "Sprint")
	bool IsSprinting() const { return bIsSprinting; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetAmmo() const { return Ammo; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsShooting() const { return bIsShooting; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsReloading() const { return bIsReloading; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsHitReacting() const { return bIsHitReacting; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsMeleeing() const { return bIsMeleeing; }

	UFUNCTION(BlueprintPure, Category = "Healing")
	bool IsHealing() const { return bIsHealing; }

	UFUNCTION(BlueprintCallable, Category = "AI")
	void AIStartShoot();

	UFUNCTION(BlueprintCallable, Category = "AI")
	void AIStartReload();

	UFUNCTION(BlueprintCallable, Category = "AI")
	void AIStartMelee();

	UFUNCTION(BlueprintCallable, Category = "AI")
	void AIStartHeal();

	UFUNCTION(BlueprintCallable, Category = "AI")
	void AIStartRoll();

	UFUNCTION(BlueprintCallable, Category = "AI")
	void AIStartSprint();

	UFUNCTION(BlueprintCallable, Category = "AI")
	void AIStopSprint();

	UFUNCTION(BlueprintPure, Category = "AI")
	bool AICanShoot() const;

	UFUNCTION(BlueprintPure, Category = "AI")
	bool AICanReload() const;

	UFUNCTION(BlueprintPure, Category = "AI")
	bool AICanMelee() const;

	UFUNCTION(BlueprintPure, Category = "AI")
	bool AICanHeal() const;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void AISetFacingRotation(const FRotator& NewRotation);

public:

	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE USkeletalMeshComponent* GetGun() const { return Gun; }
};