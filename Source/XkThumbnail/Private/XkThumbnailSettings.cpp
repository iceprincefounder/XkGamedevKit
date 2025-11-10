// Copyright Tencent Games, Inc. All Rights Reserved.


#include "XkThumbnailSettings.h"
#include "UObject/ConstructorHelpers.h"

UXkThumbnailSettings::UXkThumbnailSettings(const FObjectInitializer& ObjectInitializer)
{
	Texture2DSaveDir.Path = TEXT("/Game/__Thumbnail__/");
	ThumbnailPrefix = TEXT("");
}
