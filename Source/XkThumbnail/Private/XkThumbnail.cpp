// Copyright Tencent Games, Inc. All Rights Reserved.

#include "XkThumbnail.h"
#include "XkThumbnailSettings.h"

#include "CanvasTypes.h"
#include "ContentBrowserModule.h"
#include "FileHelpers.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "ObjectTools.h"
#include "ContentStreaming.h"
#include "UnrealEdGlobals.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor/UnrealEdEngine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "AssetCompilingManager.h"
#include "ThumbnailRendering/BlueprintThumbnailRenderer.h"
#include "ThumbnailRendering/SkeletalMeshThumbnailRenderer.h"
#include "ThumbnailRendering/StaticMeshThumbnailRenderer.h"
#include "ThumbnailRendering/TextureThumbnailRenderer.h"

#define LOCTEXT_NAMESPACE "FXkThumbnailModule"


FXkThumbnailModule::FXkThumbnailModule()
{
	XkThumbnailSettings = nullptr;
}


void FXkThumbnailModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	CreateThumbnailSettings();
	AddContentBrowserContextMenuExtender();
}

void FXkThumbnailModule::ShutdownModule()
{
	RemoveContentBrowserContextMenuExtender();
	// Unregister settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule != nullptr)
	{
		if (XkThumbnailSettings != nullptr)
		{
			XkThumbnailSettings->RemoveFromRoot();
		}
		SettingsModule->UnregisterSettings("Project", "Plugins", "XkThumbnail");
	}

	if (!GExitPurge) // If GExitPurge Object is already gone
	{
		XkThumbnailSettings->RemoveFromRoot();
	}
	XkThumbnailSettings = nullptr;
}

bool FXkThumbnailModule::DoesAssetSupportExportToThumbnail(const FAssetData& AssetData)
{
	// #TODO (NanceDevDiaries) add more support as it comes. Example, SkeletalMesh once it's figured
	// The logic of its saved render data
	const FName AssetName = AssetData.AssetClassPath.GetAssetName();
	return AssetName == TEXT("StaticMesh") || AssetName == TEXT("Blueprint") || AssetName == TEXT("SkeletalMesh");
}

bool FXkThumbnailModule::DoesAssetSupportConfigIconTexture(const FAssetData& AssetData)
{
	const FName AssetName = AssetData.AssetClassPath.GetAssetName();
	return AssetName == TEXT("Texture2D");
}


TArray<UTexture2D*> FXkThumbnailModule::ExportThumbnailAsTexture(const TArray<FAssetData> SelectedAssets, bool bTransient, bool bForceRenderThumbnail)
{
	TArray<UTexture2D*> NewTextureResults;
	for (const FAssetData& AssetData : SelectedAssets)
	{
		if (!DoesAssetSupportExportToThumbnail(AssetData))
		{
			// Skip unsupported class
			continue;
		}

		FString AssetName = GetThumbnailFileName(AssetData.GetAsset());

		if (int32 PathSeparatorIdx; AssetName.FindChar('/', PathSeparatorIdx))
		{
			// TextureName should not have any path separators in it
			return NewTextureResults;
		}

		// Create the new texture 2D and save it on disk
		FString PackageFilename;

		FString PackageName = GetEditorSettings().Texture2DSaveDir.Path;
		if (!PackageName.EndsWith("/"))
		{
			PackageName += "/";
		}
		PackageName += AssetName;

		UPackage* Package = CreatePackage(*PackageName);
		if (bTransient)
		{
			Package = GetTransientPackage();
		}
		Package->FullyLoad();

		UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *AssetName, RF_Public | RF_Standalone);
		if (bTransient)
		{
			NewTexture->SetFlags(RF_Transient);
		}
		NewTexture->MarkPackageDirty();

		// Load the image from the asset's loaded Thumbnail
		if (FPackageName::DoesPackageExist(AssetData.PackageName.ToString(), &PackageFilename))
		{
			if (bForceRenderThumbnail)
			{
				FThumbnailMap ThumbnailMap;
				const FName ObjectFullName = FName(*AssetData.GetFullName());
				TSet<FName> ObjectFullNames;
				ObjectFullNames.Add(ObjectFullName);
				ThumbnailTools::LoadThumbnailsFromPackage(PackageFilename, ObjectFullNames, ThumbnailMap);

				FObjectThumbnail* ObjectThumbnail = ThumbnailMap.Find(ObjectFullName);

				if (!ObjectThumbnail || bForceRenderThumbnail)
				{
					const int32 ImageWidth = ThumbnailTools::DefaultThumbnailSize;
					const int32 ImageHeight = ThumbnailTools::DefaultThumbnailSize;
					RenderThumbnail(AssetData.GetAsset(), ImageWidth, ImageHeight, NULL, ObjectThumbnail);
					ThumbnailTools::CacheThumbnail(AssetData.GetAsset()->GetFullName(), ObjectThumbnail, AssetData.GetAsset()->GetOutermost());
				}
			}

			FString PackageFullName;
			FString ObjectFullName = AssetData.GetAsset()->GetFullName();
			FName ObjectName = FName(ObjectFullName);
			ThumbnailTools::QueryPackageFileNameForObject(ObjectFullName, PackageFullName);
			FThumbnailMap ThumbnailMap;
			ThumbnailTools::ConditionallyLoadThumbnailsFromPackage(PackageFullName, { FName(ObjectFullName) }, ThumbnailMap);

			// there should always be a dummy thumbnail in here
			if (ThumbnailMap.Contains(ObjectName))
			{
				FObjectThumbnail Thumbnail = ThumbnailMap[ObjectName];
				if (Thumbnail.GetImageWidth() != 0 && Thumbnail.GetImageHeight() != 0)
				{
					// Cache the thumbnail  
					ThumbnailTools::CacheThumbnail(AssetData.GetAsset()->GetFullName(), &Thumbnail, AssetData.GetAsset()->GetOutermost());
				}
			}
			FObjectThumbnail* ObjectThumbnail = ThumbnailMap.Find(ObjectName);
			int32 SizeX = ObjectThumbnail->GetImageWidth();
			int32 SizeY = ObjectThumbnail->GetImageHeight();

			FTexturePlatformData* PlatformData = new FTexturePlatformData();
			PlatformData->SizeX = SizeX;
			PlatformData->SizeY = SizeY;
			PlatformData->SetNumSlices(1);
			PlatformData->PixelFormat = PF_B8G8R8A8;
			NewTexture->SetPlatformData(PlatformData);
			NewTexture->MipGenSettings = TMGS_NoMipmaps;

			int32 NumBlocksX = ObjectThumbnail->GetImageWidth() / GPixelFormats[PF_B8G8R8A8].BlockSizeX;
			int32 NumBlocksY = ObjectThumbnail->GetImageHeight() / GPixelFormats[PF_B8G8R8A8].
				BlockSizeY;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 2
			FTexture2DMipMap* Mip = new FTexture2DMipMap();
			Mip->SizeX = SizeX;
			Mip->SizeY = SizeY;
			Mip->SizeZ = 1;
#else
			FTexture2DMipMap* Mip = new FTexture2DMipMap(SizeX, SizeY, 1);
#endif
			NewTexture->GetPlatformData()->Mips.Add(Mip);
			Mip->BulkData.Lock(LOCK_READ_WRITE);
			Mip->BulkData.Realloc(
				static_cast<int64>(NumBlocksX) * NumBlocksY * GPixelFormats[PF_B8G8R8A8].BlockBytes);
			Mip->BulkData.Unlock();

			NewTexture->UpdateResource();

			NewTexture->Source.Init(SizeX, SizeY, 1,
				1,
				TSF_BGRA8, ObjectThumbnail->GetUncompressedImageData().GetData());
			NewTexture->LODGroup = TEXTUREGROUP_UI; // Prepare the asset for UI use
			NewTexture->CompressionSettings = TC_Default;
			// No need for "UserInterface2D", no need for alpha, it was also having issues making the asset have a thumbnail itself
			NewTexture->NeverStream = true;
			NewTexture->CompressionNoAlpha = true;
			NewTexture->UpdateResource();

			if (!bTransient)
			{
				Package->FullyLoad();
				Package->SetDirtyFlag(true);
				FEditorFileUtils::PromptForCheckoutAndSave({ Package }, false, false);
				FAssetRegistryModule::AssetCreated(NewTexture);
			}

			NewTextureResults.Add(NewTexture);
		}
	}
	return NewTextureResults;
}


TArray<UTexture2D*> FXkThumbnailModule::ConfigTextureAsIconUI(const TArray<FAssetData> SelectedAssets)
{
	TArray<UTexture2D*> NewTextureResults;
	TextureMipGenSettings MipGenSettings = GetEditorSettings().MipGenSettings;
	TextureGroup LODGroup = GetEditorSettings().LODGroup;
	int32 MaxTextureSize = GetEditorSettings().MaxTextureSize;
	for (const FAssetData& AssetData : SelectedAssets)
	{
		if (!DoesAssetSupportConfigIconTexture(AssetData))
		{
			// Skip unsupported class
			continue;
		}
		UTexture2D* Texture2D = Cast<UTexture2D>(AssetData.GetAsset());
		Texture2D->Modify();
		Texture2D->MipGenSettings = MipGenSettings;
		Texture2D->LODGroup = LODGroup;
		Texture2D->MaxTextureSize = MaxTextureSize;
		Texture2D->UpdateResource();
		NewTextureResults.Add(Texture2D);
	}
	return NewTextureResults;
}


// Copy and modified from engine code: ThumbnailTools::RenderThumbnail
void FXkThumbnailModule::RenderThumbnail(UObject* InObject, const uint32 InImageWidth, const uint32 InImageHeight, FTextureRenderTargetResource* InTextureRenderTargetResource, FObjectThumbnail* OutThumbnail)
{
	if (!FApp::CanEverRender())
	{
		return;
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(FXkThumbnailModule::RenderThumbnail);

	// Renderer must be initialized before generating thumbnails
	check(GIsRHIInitialized);

	// Store dimensions
	if (OutThumbnail)
	{
		OutThumbnail->SetImageSize(InImageWidth, InImageHeight);
	}

	// Grab the actual render target resource from the texture.  Note that we're absolutely NOT ALLOWED to
	// dereference this pointer.  We're just passing it along to other functions that will use it on the render
	// thread.  The only thing we're allowed to do is check to see if it's NULL or not.
	FTextureRenderTargetResource* RenderTargetResource = InTextureRenderTargetResource;
	if (RenderTargetResource == NULL)
	{
		// No render target was supplied, just use a scratch texture render target
		const uint32 MinRenderTargetSize = FMath::Max(InImageWidth, InImageHeight);
		UTextureRenderTarget2D* RenderTargetTexture = GEditor->GetScratchRenderTarget(MinRenderTargetSize);
		check(RenderTargetTexture != NULL);

		// Make sure the input dimensions are OK.  The requested dimensions must be less than or equal to
		// our scratch render target size.
		check(InImageWidth <= RenderTargetTexture->GetSurfaceWidth());
		check(InImageHeight <= RenderTargetTexture->GetSurfaceHeight());

		RenderTargetResource = RenderTargetTexture->GameThread_GetRenderTargetResource();
	}
	check(RenderTargetResource != NULL);

	// Create a canvas for the render target and clear it to black
	FCanvas Canvas(RenderTargetResource, NULL, FGameTime::GetTimeSinceAppStart(), GMaxRHIFeatureLevel);
	Canvas.Clear(FLinearColor::Black);

	// Get the rendering info for this object
	FThumbnailRenderingInfo* RenderInfo = GUnrealEd ? GUnrealEd->GetThumbnailManager()->GetRenderingInfo(InObject) : nullptr;

	// Wait for pending load requests.
	FlushAsyncLoading();

	// Wait for shader and other asset compilation to finish.
	FAssetCompilingManager::Get().FinishAllCompilation();

	// Force all mips to load.
	UTexture::ForceUpdateTextureStreaming();

	// Force all streamed resources to finish.
	IStreamingManager::Get().StreamAllResources(0.0f);

	// If this object's thumbnail will be rendered to a texture on the GPU.
	bool bUseGPUGeneratedThumbnail = true;

	if (RenderInfo != NULL && RenderInfo->Renderer != NULL)
	{
		// Make sure we suppress any message dialogs that might result from constructing
		// or initializing any of the renderable objects.
		TGuardValue<bool> Unattended(GIsRunningUnattendedScript, true);

		const float ZoomFactor = 1.0f;

		uint32 DrawWidth = InImageWidth;
		uint32 DrawHeight = InImageHeight;
		if (OutThumbnail)
		{
			// Find how big the thumbnail WANTS to be
			uint32 DesiredWidth = 0;
			uint32 DesiredHeight = 0;
			{
				// Currently we only allow textures/icons (and derived classes) to override our desired size
				// @todo CB: Some thumbnail renderers (like particles and lens flares) hard code their own
				//	   arbitrary thumbnail size even though they derive from TextureThumbnailRenderer
				if (RenderInfo->Renderer->IsA(UTextureThumbnailRenderer::StaticClass()))
				{
					RenderInfo->Renderer->GetThumbnailSize(
						InObject,
						ZoomFactor,
						DesiredWidth,		// Out
						DesiredHeight);	// Out
				}
			}

			// Does this thumbnail have a size associated with it?  Materials and textures often do!
			if (DesiredWidth > 0 && DesiredHeight > 0)
			{
				// Scale the desired size down if it's too big, preserving aspect ratio
				if (DesiredWidth > InImageWidth)
				{
					DesiredHeight = (DesiredHeight * InImageWidth) / DesiredWidth;
					DesiredWidth = InImageWidth;
				}
				if (DesiredHeight > InImageHeight)
				{
					DesiredWidth = (DesiredWidth * InImageHeight) / DesiredHeight;
					DesiredHeight = InImageHeight;
				}

				// Update dimensions
				DrawWidth = FMath::Max<uint32>(1, DesiredWidth);
				DrawHeight = FMath::Max<uint32>(1, DesiredHeight);
				OutThumbnail->SetImageSize(DrawWidth, DrawHeight);
			}
		}

		// Draw the thumbnail
		const int32 XPos = 0;
		const int32 YPos = 0;
		const bool bAdditionalViewFamily = false;
		RenderInfo->Renderer->Draw(
			InObject,
			XPos,
			YPos,
			DrawWidth,
			DrawHeight,
			RenderTargetResource,
			&Canvas,
			bAdditionalViewFamily
		);
	}

	// GPU based thumbnail rendering only
	if (bUseGPUGeneratedThumbnail)
	{
		// Tell the rendering thread to draw any remaining batched elements
		Canvas.Flush_GameThread();


		{
			ENQUEUE_RENDER_COMMAND(UpdateThumbnailRTCommand)(
				[RenderTargetResource](FRHICommandListImmediate& RHICmdList)
				{
					TransitionAndCopyTexture(RHICmdList, RenderTargetResource->GetRenderTargetTexture(), RenderTargetResource->TextureRHI, {});
				});

			if (OutThumbnail)
			{
				const FIntRect InSrcRect(0, 0, OutThumbnail->GetImageWidth(), OutThumbnail->GetImageHeight());

				TArray<uint8>& OutData = OutThumbnail->AccessImageData();

				OutData.Empty();
				OutData.AddUninitialized(OutThumbnail->GetImageWidth() * OutThumbnail->GetImageHeight() * sizeof(FColor));

				// Copy the contents of the remote texture to system memory
				// prefer GetRenderTargetImage()
				RenderTargetResource->ReadPixelsPtr((FColor*)OutData.GetData(), FReadSurfaceDataFlags(), InSrcRect);
			}
		}
	}
}


FString FXkThumbnailModule::GetThumbnailFileName(UObject* InObject)
{
	FString Result;
	FString UniqueID = FString::FromInt(InObject->GetUniqueID());
	FString GamePath = InObject->GetPathName();
	if (int32 PathEnd; GamePath.FindLastChar('/', PathEnd))
	{
		++PathEnd;
		Result += GamePath;
		Result.RightChopInline(PathEnd);
		int32 extensionIdx;
		if (Result.FindChar('.', extensionIdx))
		{
			Result.LeftInline(extensionIdx);
		}
		GamePath.LeftInline(PathEnd);
		FString Prefix = GetEditorSettings().ThumbnailPrefix;
		FString NameWithPrefix = Prefix + Result + TEXT("_") + UniqueID;
		Result = NameWithPrefix;
	}
	else
	{
		Result = "T_Thumbnail";
	}
	return Result;
}


int32 FXkThumbnailModule::GetFileNameUniqueID(const FString& FileName)
{
	int32 Result = -1;

	if (int32 LastUnderscoreIndex; FileName.FindLastChar('_', LastUnderscoreIndex))
	{
		if (int32 DotIndex; FileName.FindLastChar('.', DotIndex) && DotIndex > LastUnderscoreIndex)
		{
			FString Substring = FileName.Mid(LastUnderscoreIndex + 1, DotIndex - LastUnderscoreIndex - 1);

			if (Substring.IsNumeric())
			{
				Result = FCString::Atoi(*Substring);
			}
		}
	}
	return Result;
}


UXkThumbnailSettings* FXkThumbnailModule::GetEditorSettingsInstance() const
{
	return XkThumbnailSettings;
}


void FXkThumbnailModule::AddContentBrowserContextMenuExtender()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(
		"ContentBrowser");
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = ContentBrowserModule.
		GetAllAssetViewContextMenuExtenders();

	CBMenuExtenderDelegates.Add(
		FContentBrowserMenuExtender_SelectedAssets::CreateStatic(
			&FXkThumbnailModule::OnExtendContentBrowserAssetSelectionMenu));
	ContentBrowserExtenderDelegateHandle = CBMenuExtenderDelegates.Last().GetHandle();
}

void FXkThumbnailModule::RemoveContentBrowserContextMenuExtender()
{
	if (ContentBrowserExtenderDelegateHandle.IsValid() && FModuleManager::Get().IsModuleLoaded("ContentBrowser"))
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>(
			"ContentBrowser");
		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = ContentBrowserModule.
			GetAllAssetViewContextMenuExtenders();
		CBMenuExtenderDelegates.RemoveAll([this](const FContentBrowserMenuExtender_SelectedAssets& Delegate)
		{
			return Delegate.GetHandle() == ContentBrowserExtenderDelegateHandle;
		});
	}
}

TSharedRef<FExtender> FXkThumbnailModule::OnExtendContentBrowserAssetSelectionMenu(
	const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();

	// #todo filter out to only have assets that have a thumbnail already (Actors, not dataTables/dataAssets etc)

	Extender = MakeShared<FExtender>();

	bool bAllAssetSupportExportToThumbnail = true;
	for (const FAssetData& AssetData : SelectedAssets)
	{
		if (!DoesAssetSupportExportToThumbnail(AssetData))
		{
			bAllAssetSupportExportToThumbnail = false;
			break;
		}
	}
	bool bAllAssetSupportConfigIconTexture = true;
	for (const FAssetData& AssetData : SelectedAssets)
	{
		if (!DoesAssetSupportConfigIconTexture(AssetData))
		{
			bAllAssetSupportConfigIconTexture = false;
			break;
		}
	}
	if (bAllAssetSupportExportToThumbnail)
	{
		Extender->AddMenuExtension(
			"CommonAssetActions",
			EExtensionHook::After,
			nullptr,
			FMenuExtensionDelegate::CreateStatic(&ExecuteSaveThumbnailAsTexture, SelectedAssets)
		);
	}
	if (bAllAssetSupportConfigIconTexture)
	{
		Extender->AddMenuExtension(
			"CommonAssetActions",
			EExtensionHook::After,
			nullptr,
			FMenuExtensionDelegate::CreateStatic(&ExecuteConfigTextureAsIconUI, SelectedAssets)
		);
	}
	return Extender;
}

void FXkThumbnailModule::ExecuteSaveThumbnailAsTexture(FMenuBuilder& MenuBuilder, const TArray<FAssetData> SelectedAssets)
{
	// developed from engine code and help from a mix of https://forums.unrealengine.com/t/copy-asset-thumbnail-to-new-texture2d/138054/4
	// and https://isaratech.com/save-a-procedurally-generated-texture-as-a-new-asset/
	// and https://arrowinmyknee.com/2020/08/28/asset-right-click-menu-in-ue4/
	// and https://dev.epicgames.com/community/snippets/lw1/procedural-texture-with-c
	// and https://forums.unrealengine.com/t/programatically-created-asset-fails-to-save-or-crashes-the-editor/724517
	MenuBuilder.BeginSection("CreateTextureOffThumbnail", LOCTEXT("CreateTextureOffThumbnailMenuHeading", "XkThumbnail"));
	{
		// Add Menu Entry Here
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Thumbnail_NewTexture", "Export Thumbnail"),
			LOCTEXT("Thumbnail_NewTextureTooltip",
					"Will export asset's thumbnail and put it in a folder defined in the project settings"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([SelectedAssets]()
			{
				ExportThumbnailAsTexture(SelectedAssets);
			})),
			NAME_None,
			EUserInterfaceActionType::Button);
	}
	MenuBuilder.EndSection();
}


void FXkThumbnailModule::ExecuteConfigTextureAsIconUI(FMenuBuilder& MenuBuilder, const TArray<FAssetData> SelectedAssets)
{
	MenuBuilder.BeginSection("CreateTextureOffThumbnail", LOCTEXT("CreateTextureOffThumbnailMenuHeading", "XkThumbnail"));
	{
		// Add Menu Entry Here
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Thumbnail_ConfigIcon", "Config as Icon UI"),
			LOCTEXT("Thumbnail_ConfigIconTooltip",
				"Will config asset's as icon texture."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([SelectedAssets]()
				{
					ConfigTextureAsIconUI(SelectedAssets);
				})),
			NAME_None,
			EUserInterfaceActionType::Button);
	}
	MenuBuilder.EndSection();
}

void FXkThumbnailModule::CreateThumbnailSettings()
{
	XkThumbnailSettings = NewObject<UXkThumbnailSettings>(
		GetTransientPackage(), UXkThumbnailSettings::StaticClass());
	check(XkThumbnailSettings);
	XkThumbnailSettings->AddToRoot();

	// Register settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{		ISettingsSectionPtr EditorSettingsSection = SettingsModule->RegisterSettings(
			"Editor", "Plugins", "XkThumbnail",
			NSLOCTEXT("XkThumbnail", "XkThumbnailSettingsName",
					  "Xk Thumbnail"),
			NSLOCTEXT("XkThumbnail", "XkThumbnailSettingsDescription",
					  "Configure Xk Thumbnail Editor Defaults."),
			XkThumbnailSettings);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FXkThumbnailModule, XkThumbnail)
