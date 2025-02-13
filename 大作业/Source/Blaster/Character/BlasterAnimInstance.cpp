// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Kismet/KismetMathLibrary.h>
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterTypes/CombatState.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	blasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (blasterCharacter == nullptr)
	{
		blasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}

	if (blasterCharacter == nullptr) return;

	FVector velocity = blasterCharacter->GetVelocity();
	velocity.Z = 0.f;
	// 取动画播放速度为速度大小的模值
	speed = velocity.Size();

	bIsInAir = blasterCharacter->GetCharacterMovement()->IsFalling();

	// 如果加速度不为零,则播放动画
	bIsAccelerating = blasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;

	bWeaponEquipped = blasterCharacter->IsWeaponEquipped();
	EquippedWeapon = blasterCharacter->GetEquippedWeapon();
	bIsCrouched = blasterCharacter->bIsCrouched;
	bAiming = blasterCharacter->isAiming();
	TurningInPlace = blasterCharacter->GetTurningInPlace();
	bRotateRootBone = blasterCharacter->ShouldRotateRootBone();
	bElimmed = blasterCharacter->IsElimmed();
	// 角色的瞄准方向,即角色面向,即用鼠标控制的方向
	FRotator AimRotation = blasterCharacter->GetBaseAimRotation();

	// 角色的移动速度旋转方向,即按下wasd确定的方向
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(blasterCharacter->GetVelocity());

	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);

	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 15.f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	// 角色面向,本来是等同于GetBaseAimRotation()，但是关闭了根据旋转调整面向,所以需要获得actor的旋转
	CharacterRotation = blasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);

	const float Target = Delta.Yaw / DeltaTime;
	// 进行插值
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);

	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = blasterCharacter->GetAo_Yaw();
	Ao_Pitch = blasterCharacter->GetAo_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && blasterCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"),ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;

		blasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);

		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (blasterCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);

			// 该计算式终点坐标公式为从右手起点开始 + 方向向量
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - blasterCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
		}

	}
	bUseFABRIK = blasterCharacter->GetCombatState() != ECombatState::ECS_Reloading;
	bUseAimOffsets = blasterCharacter->GetCombatState() != ECombatState::ECS_Reloading && !blasterCharacter->GetDisableGameplay();
	bTransformRightHand = blasterCharacter->GetCombatState() != ECombatState::ECS_Reloading && !blasterCharacter->GetDisableGameplay();
}
