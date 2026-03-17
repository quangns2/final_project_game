using UnrealBuildTool;

public class LudusCefConfig : ModuleRules
{
    public LudusCefConfig(ReadOnlyTargetRules Target) : base(Target)
    {
		bUsePrecompiled = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Latest;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );
    }
}
