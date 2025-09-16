// Copyright ©ICEPRINCE. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "XkHexagonGeom.generated.h"

USTRUCT(BlueprintType)
struct XKGAMEDEVCORE_API FXkHexagonEdge
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Edge")
	int32 A;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Edge")
	int32 B;
};