// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjetM1S2 : ModuleRules
{
	public ProjetM1S2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ProjetM1S2",
			"ProjetM1S2/Variant_Horror",
			"ProjetM1S2/Variant_Horror/UI",
			"ProjetM1S2/Variant_Shooter",
			"ProjetM1S2/Variant_Shooter/AI",
			"ProjetM1S2/Variant_Shooter/UI",
			"ProjetM1S2/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
