// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WP_4th : ModuleRules
{
	public WP_4th(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate",
			"MotionWarping",
			"Niagara",
			"NiagaraCore",
            "CableComponent"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "AnimGraphRuntime", "AnimGraphRuntime", "CableComponent", "CableComponent" });

		PublicIncludePaths.AddRange(new string[] {
			"WP_4th",
			"WP_4th/Variant_Shooter",
			"WP_4th/Variant_Shooter/AI",
			"WP_4th/Variant_Shooter/UI",
			"WP_4th/Variant_Shooter/Weapons",
			"WP_4th/Weapon",
			"WP_4th/Test",
            "WP_4th/Zipline",
            "WP_4th/Character/Components/ZiplineComp"

		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
