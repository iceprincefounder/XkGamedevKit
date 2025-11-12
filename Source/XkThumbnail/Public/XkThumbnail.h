// Copyright Tencent Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class UXkThumbnailSettings;

class IXkThumbnailModule
	: public IModuleInterface
{
public:
	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */	static inline IXkThumbnailModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IXkThumbnailModule>("XkThumbnail");
	}
	
	/**
	* @return reference to Xk Thumbnail Settings
	*/	static inline UXkThumbnailSettings& GetEditorSettings()
	{
		IXkThumbnailModule& Module = IsInGameThread() ? Get() : FModuleManager::GetModuleChecked<IXkThumbnailModule>("XkThumbnail");
		UXkThumbnailSettings* Settings = Module.GetEditorSettingsInstance();
		check(Settings);
		return *Settings;
	}
protected:
	virtual UXkThumbnailSettings* GetEditorSettingsInstance() const = 0;
};


/**
 * Implements the FXkThumbnailModule module.
 */
class XKTHUMBNAIL_API FXkThumbnailModule
	: public IXkThumbnailModule
{
public:
	FXkThumbnailModule();

	static inline FXkThumbnailModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FXkThumbnailModule>("XkThumbnail");
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	/** Returns whether the Header View supports the given class */
	static bool DoesAssetSupportExportToThumbnail(const FAssetData& AssetData);
	static bool DoesAssetSupportConfigIconTexture(const FAssetData& AssetData);
	static TArray<UTexture2D*> ExportThumbnailAsTexture(const TArray<FAssetData> SelectedAssets, bool bTransient = false, bool bForceRenderThumbnail = false);
	static TArray<UTexture2D*> ConfigTextureAsIconUI(const TArray<FAssetData> SelectedAssets);
	static void RenderThumbnail(UObject* InObject, const uint32 InImageWidth, const uint32 InImageHeight, FTextureRenderTargetResource* InRenderTargetResource = NULL, FObjectThumbnail* OutThumbnail = NULL);

	static FString GetThumbnailFileName(UObject* InObject);
	static int32 GetFileNameUniqueID(const FString& FileName);

protected:
	virtual UXkThumbnailSettings* GetEditorSettingsInstance() const override;

private:
	void AddContentBrowserContextMenuExtender();
	void RemoveContentBrowserContextMenuExtender();

	static TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets);
	static void ExecuteSaveThumbnailAsTexture(FMenuBuilder& MenuBuilder, const TArray<FAssetData> SelectedAssets);
	static void ExecuteConfigTextureAsIconUI(FMenuBuilder& MenuBuilder, const TArray<FAssetData> SelectedAssets);

	FDelegateHandle ContentBrowserExtenderDelegateHandle;

	void CreateThumbnailSettings();

private:
	UXkThumbnailSettings* XkThumbnailSettings;
};