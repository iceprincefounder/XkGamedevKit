// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class XkGamedevCore : ModuleRules
{
	public XkGamedevCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				"$(EngineDir)/Source/Runtime/Renderer/Private",
				// ... add public include paths required here ...
			}
			);

		PrivateIncludePaths.AddRange(
			new string[] {
				"$(EngineDir)/Source/Runtime/Renderer/Private",
				// ... add other private include paths required here ...
			}
			);

		PublicDependencyModuleNames.AddRange(
			new string[] 
			{
				"Core",
				"CoreUObject",
				"Engine",
				"RenderCore",
				"InputCore",
				"SlateCore",
				"HeadMountedDisplay",
				"NavigationSystem",
				"AIModule",
				"Niagara",
				"EnhancedInput",
				"Landscape",
				"ProceduralMeshComponent",
				"Slate",
				"UMG",
				"RHI",
				"Renderer",
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"Slate",
				"SlateCore",
				"Landscape",
				"Json",
				"RHI",
				// ... add private dependencies that you statically link with here ...	
			}
			);
	}
}