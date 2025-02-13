#pragma once

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECS_Unoccupied UMETA(DisplayName = "Unoccupied"), // ø’œ–
	ECS_Reloading UMETA(DisplayName = "Reloading"), // ªªµØ÷–

	ECS_MAX UMETA(DisplayName = "Default Max")
};