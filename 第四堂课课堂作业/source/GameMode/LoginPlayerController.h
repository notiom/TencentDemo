// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LoginPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ALoginPlayerController : public APlayerController
{
    GENERATED_BODY()

public:

    // 玩家登录成功后调用
    UFUNCTION(Server, Reliable)
    void ServerPlayerLoginComplete();

    class ALoginGameState* LoginGameState;

    void UpdateLoadingScreenHUD(float Percent);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

};
