// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RS : ModuleRules
{
    public RS(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "AIModule",
            "NavigationSystem",
            "StateTreeModule",
            "GameplayStateTreeModule",
            "UMG",
            "MoviePlayer",
            "Slate",
            "SlateCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });

        PublicIncludePaths.AddRange(new string[]
        {
            "RS",
            "RS/Variant_Platforming",
            "RS/Variant_Platforming/Animation",
            "RS/Variant_Combat",
            "RS/Variant_Combat/AI",
            "RS/Variant_Combat/Animation",
            "RS/Variant_Combat/Gameplay",
            "RS/Variant_Combat/Interfaces",
            "RS/Variant_Combat/UI",
            "RS/Variant_SideScrolling",
            "RS/Variant_SideScrolling/AI",
            "RS/Variant_SideScrolling/Gameplay",
            "RS/Variant_SideScrolling/Interfaces",
            "RS/Variant_SideScrolling/UI"
        });
    }
}