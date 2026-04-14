// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

using UnrealBuildTool;

public class CloLiveSyncEditor : ModuleRules
{
	public CloLiveSyncEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "AnimGraph",
                "ApplicationCore",
                "Boost",
                "Chaos",
                "ChaosCloth",
                "ChaosClothAsset",
                "ChaosClothAssetDataflowNodes",
                "ChaosClothAssetEngine",
                "CloLiveSyncCore",
                "Core",
                "CoreUObject",
                "DataflowCore",
                "DataflowEditor",
                "DataflowEngine",
                "DesktopWidgets",
                "EditorFramework",
                "EditorScriptingUtilities",
                "Engine",
                "GeometryCache",
                "InputCore",
                "LevelEditor",
                "LevelSequence",
                "LevelSequenceEditor",
                "MaterialBaking",
                "MaterialEditor",
                "MovieScene",
                "Networking",
                "Sequencer",
                "ProceduralMeshComponent",
                "Projects",
                "RHI",
                "SkeletalMerging",
                "Slate",
                "SlateCore",
                "TextureCompressor",
                "UnrealEd",
                "UnrealUSDWrapper",
                "USDClasses",
                "USDExporter",
                "USDSchemas",
                "USDStage",
                "USDStageEditor",
                "USDStageEditorViewModels",
                "USDStageImporter",
                "USDUtilities",
                "WorkspaceMenuStructure"
            }
			);
	}
}
