// Copyright (C) 2024 owoDra

using UnrealBuildTool;

public class GCOnline : ModuleRules
{
	public GCOnline(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[]
            {
                ModuleDirectory,
                ModuleDirectory + "/GCOnline",
            }
        );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "CoreUObject", "Engine",

                "CoreOnline", "GameplayTags",

                "OnlineServicesInterface",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "DeveloperSettings", "ApplicationCore",

                "OnlineSubsystemUtils", "CommonUI",
            }
        );
    }
}
