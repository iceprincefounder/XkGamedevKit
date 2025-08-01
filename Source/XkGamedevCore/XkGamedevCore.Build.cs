// Copyright ©ICEPRINCE. All Rights Reserved.

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
                "RHI",
                "Engine",
				"RenderCore",
                "Renderer",
                "InputCore",
                "EnhancedInput",
                "SlateCore",
                "Slate",
                "UMG",
                "Niagara",
                "Landscape",
                "HeadMountedDisplay",
				"NavigationSystem",
				"AIModule",
                "GeometryCore",
                "GeometryFramework",
				"ProceduralMeshComponent",
				"Water",
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"Json",
				"DeveloperSettings",
				// ... add private dependencies that you statically link with here ...	
			}
			);
	}
}