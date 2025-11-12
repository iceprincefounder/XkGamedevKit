// Copyright Tencent Games, Inc. All Rights Reserved.


#include "XkThumbnailSettings.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/TextureDefines.h"

UXkThumbnailSettings::UXkThumbnailSettings(const FObjectInitializer& ObjectInitializer)
{
	Texture2DSaveDir.Path = TEXT("/Game/__Thumbnail__/");
	ThumbnailPrefix = TEXT("");

	MipGenSettings = TMGS_Sharpen3;
	LODGroup = TEXTUREGROUP_UI;
	MaxTextureSize = 128;
}
