// Copyright 2024 CLO Virtual Fashion. All rights reserved.

#include "Schemas/CloUsdGroomTranslator.h"

#if USE_USD_SDK
#include "GroomAsset.h"
#include "GroomComponent.h"

#include "USDIncludesStart.h"
#include "Objects/USDPrimLinkCache.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/tokens.h"
#include "USDIncludesEnd.h"
#include "UsdWrappers/UsdRelationship.h"

#include "Utils/CloLiveSyncCoreUtils.h"


static bool GUseInterchangeCLOGroomTranslator = true;
static FAutoConsoleVariableRef CvarUseInterchangeCLOGroomTranslator(
	TEXT("USD.UseInterchangeCLOGroomTranslator"),
	GUseInterchangeCLOGroomTranslator,
	TEXT(""));

FName FCloUsdGroomTranslator::CLOUsdRenderContext = TEXT("clo");

void FCloUsdGroomTranslator::CreateAssets()
{
	Super::CreateAssets();
}

USceneComponent* FCloUsdGroomTranslator::CreateComponents()
{
	return Super::CreateComponents();
}

void FCloUsdGroomTranslator::UpdateComponents(USceneComponent* SceneComponent)
{
	Super::UpdateComponents(SceneComponent);

	if (UGroomComponent* GroomComponent = Cast<UGroomComponent>(SceneComponent))
	{
		UGroomAsset* Groom = GroomComponent->GroomAsset.Get();
		TArray<UMaterialInterface*> MaterialAssets;
		
		if (Groom != nullptr)
		{
			for (auto Child : GetPrim().GetChildren())
			{
				for (auto Relationship : Child.GetRelationships())
				{
					TArray<UE::FSdfPath> Targets;
					Relationship.GetTargets(Targets);
					for (auto Target : Targets)
					{
						TArray<TWeakObjectPtr<UObject>> Assets = Context->PrimLinkCache->GetAllAssetsForPrim(Target.GetPrimPath());
						for (auto Asset : Assets)
						{
							if (Asset.IsValid() == false) continue;
	
							auto Material = Cast<UMaterialInterface>(Asset.Get());
							if (Material)
							{
								MaterialAssets.Add(Material);
							}
						}
					}
				}
			}
			
			TArray<FHairGroupsMaterial> HairMaterials;
			for (int i = 0; i < MaterialAssets.Num(); ++i)
			{
				FHairGroupsMaterial HairGroupMaterial;
				HairGroupMaterial.Material = MaterialAssets[i];
				FString NameString = FString::Printf(TEXT("CLO_Hair_%d"), i);
				HairGroupMaterial.SlotName = FName(*NameString);
				HairMaterials.Add(HairGroupMaterial);
			}
			
			Groom->SetHairGroupsMaterials(HairMaterials);
			GroomComponent->OverrideMaterials = MaterialAssets;
		}

	}
}

#undef LOCTEXT_NAMESPACE

#endif // #if USE_USD_SDK
