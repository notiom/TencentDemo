// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDSquareScore(int32 Score);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountDown(float CountdownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float GetServerTime();
	// ��ҿ��������ɹ����յ�����ʼ��ʱ����
	virtual void ReceivedPlayer() override; //�����������ʱ��ͬ��
	void OnMatchStateSet(FName State);
	void HandleMatchHasStarted();
	void HandleCooldown();

	/* myself*/

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ASquareActor> SquareClass;

	UFUNCTION(Server, Reliable)
	void ServerCreateSquare();

	UPROPERTY()
	class ABlasterHUD* BlasterHUD;
	/* myself*/
protected:
	virtual void BeginPlay() override;
	void SetHUDTime();
	void PollInit();
	// �ڿͻ��˺ͷ�����������ʱ��

	// ����ǰ��������ʱ��
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.f; // �ͻ��˺ͷ�����ʱ��֮��Ĳ���

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f; //ÿ5sͬ��һ��ʱ��

	float TimeSyncRunningTime = 0.f;
	void CheckTimeSync(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime);

protected:

	/* myself*/
	void AddSquareLocation();

	TArray<FVector> SquareLocations;

	void SpawnSquare(int32 RandomNum);

	/* myself*/
private:

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;
	float LevelStartingTime = 0.f;
	float MatchTime = 0.f; //��Ϸʱ��Ϊ2min
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverLay* CharacterOverLay;
	bool bInitializeCharacterOverLay = false;

	float HUDHealth;
	float HUDMaxHelath;

	bool bIsHandleStart = false;

public:
	float HUDScore;
	int32 HUDDefeats;
	int32 HUDSquarePoint;
};
