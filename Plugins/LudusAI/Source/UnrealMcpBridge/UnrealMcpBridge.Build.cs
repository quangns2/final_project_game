using UnrealBuildTool;

public class UnrealMcpBridge : ModuleRules
{
    public UnrealMcpBridge(ReadOnlyTargetRules Target) : base(Target)
    {
		bUsePrecompiled = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
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
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "Json",
                "Projects",
                "HotReload",
                "RHI",
                "RenderCore",
                "ImageWrapper",
                "UMG",
                "InputCore"
            }
        );

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",
                    "PythonScriptPlugin"
                }
            );
        }

        // Add ixwebsocket
        PrivateIncludePaths.Add(PluginDirectory + "/ThirdParty/ixwebsocket/include");
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicAdditionalLibraries.Add(PluginDirectory + "/ThirdParty/ixwebsocket/lib/ixwebsocket.lib");
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicAdditionalLibraries.Add(PluginDirectory + "/ThirdParty/ixwebsocket/lib/Linux/libixwebsocket.a");
        }
    }
}