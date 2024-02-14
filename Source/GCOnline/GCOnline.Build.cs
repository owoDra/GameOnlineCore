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
                ModuleDirectory + "/GCOnline/Service",
                ModuleDirectory + "/GCOnline/Auth",
                ModuleDirectory + "/GCOnline/Lobby",
                ModuleDirectory + "/GCOnline/LocalUser",
                ModuleDirectory + "/GCOnline/Privilege",
            }
        );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "CoreUObject", "Engine",

                "CoreOnline", "OnlineServicesInterface",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "DeveloperSettings", "ApplicationCore",

                "OnlineSubsystemUtils",
            }
        );
    }
}
