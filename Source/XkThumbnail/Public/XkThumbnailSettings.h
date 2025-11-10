// Copyright Tencent Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "XkThumbnailSettings.generated.h"

UCLASS(config = Editor, defaultconfig)
class XKTHUMBNAIL_API UXkThumbnailSettings : public UObject
{
	GENERATED_BODY()

	UXkThumbnailSettings(const FObjectInitializer& ObjectInitializer);
public:
	/** The root of the directory in which to save the exported Texture2Ds. */
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Oasis Building Thumbnail Settings", meta = (ContentDir), DisplayName="Texture 2D Save Directory")
	FDirectoryPath Texture2DSaveDir;

	/**
	 * The Prefix to append to the name of the exported Texture2Ds
	 */
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Oasis Building Thumbnail Settings")
	FString ThumbnailPrefix;
};
