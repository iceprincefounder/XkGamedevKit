// Copyright ©XUKAI. All Rights Reserved.

using UnrealBuildTool;

public class XkGamedevCore : ModuleRules
{
	public XkGamedevCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				System.IO.Path.Combine(GetModuleDirectory("Renderer"), "Private"),
			}
			);

		PrivateIncludePaths.AddRange(
			new string[] {
				System.IO.Path.Combine(GetModuleDirectory("Renderer"), "Private"),
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
				"Water",
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Engine",
				"Projects",
				"Slate",
				"SlateCore",
				"Landscape",
				"Json",
				"RHI",
				"DeveloperSettings",
				// ... add private dependencies that you statically link with here ...	
			}
			);
	}
}