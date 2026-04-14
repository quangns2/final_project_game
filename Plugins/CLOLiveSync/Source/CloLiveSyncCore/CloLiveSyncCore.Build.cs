// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

using UnrealBuildTool;

public class CloLiveSyncCore : ModuleRules
{
    public CloLiveSyncCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        bUseRTTI = true;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
			}
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Boost",
                "CoreUObject",
                "Engine",
                "ImageWrapper",
                "Networking",
                "Projects",
                "RHI",
                "Sockets",
                "USDClasses",
                "USDSchemas",
                "USDStage",
			    "USDUtilities",
                "HairStrandsCore",
                "XmlParser"
            }
            );

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "ContentBrowserData",
                    "EditorScriptingUtilities",
                    "UnrealUSDWrapper",
                    "USDExporter",
                    "USDStageImporter",
                    "AssetTools",
                    "UnrealEd",
                }
                );
        }
    }
}
