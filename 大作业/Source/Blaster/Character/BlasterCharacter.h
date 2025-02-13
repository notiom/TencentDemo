// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Blaster/Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "Blaster/BlasterTypes/combatState.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 网络复制的函数
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 初始化的函数
	virtual void PostInitializeComponents() override;

	void PlayFireMontage(bool bAiming);

	// 播放淘汰动画
	void PlayElimMontage();

	void PlayReloadMontage();

	virtual void OnRep_ReplicatedMovement() override;

	void Elim();
	
		// 当玩家被淘汰时
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();
	virtual void Destroyed() override;

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void UpdateHUDHealth();

	void MoveForward(float _value);
	void MoveRight(float _value);
	void Turn(float _value);
	void LookUp(float _value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void ReloadButtonPressed();
	void DropButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void AimOffset(float DeltaTime);
	void CalculateAOPitch();
	void SimProxiesTurn();
	virtual void Jump() override;
	void FireButtonPressed();
	void FireWeapon();
	void FireButtonReleased();
	void PlayHitReactMontage();
	// 设置委托函数时必须将其标记为UFUNCTION
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor,
						float Damage,
						const UDamageType* DamageType,
						class AController* InstigatorController,
						AActor* DamageCauser);
	// 初始化hud
	void PollInit();
	void RotateInPlace(float DeltaTime);
private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	// 添加我们的弹簧臂相机组件
	class USpringArmComponent* cameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* followCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* overheadWidget;

	// 当该变量被复制时,调用onrep函数
	// 而onrep会自动传入一个属性发生变化之前的值
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* overlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* combat;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();


	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator startingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ElimMontage;

	// 换弹蒙太奇
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;

	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;
	float CalculateSpeed();

	/*
	* 玩家的健康状况
	*/

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnyWhere, Category = "Player Stats")
	// 发生变化时马上调用复制函数
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health();

	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;

	bool bElimmed = false;

	// 淘汰计时与重生
	FTimerHandle ElimTimer;

	// 需要在编辑器中进行配置，但不希望在游戏运行时修改的变量的宏
	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;
	void ElimTimerFinished();

	// 溶解效果


	// 时间线组件,动态变化一个值
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	// 时间线委托 它会在时间线的每一帧更新时调用
	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	// 动态实例 (运行时可以更改)
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// 需要设置在蓝图上的材料实例,静态的
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

	/*
	* Elim 
	*/

	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotSound;

	UPROPERTY()
	class ABlasterPlayerState* BlasterPlayerState;

public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool isAiming();
	FORCEINLINE float GetAo_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAo_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	AWeapon* GetEquippedWeapon();
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return followCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	ECombatState GetCombatState() const;
	FORCEINLINE UCombatComponent* GetCombat() const { return combat; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
};
