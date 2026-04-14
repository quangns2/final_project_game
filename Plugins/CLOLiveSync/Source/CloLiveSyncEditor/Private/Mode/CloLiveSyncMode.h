// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "EdMode.h"

#include "Communication/CloLiveSyncServer.h"
#include "Communication/LiveSyncProtocol.h"
#include "Objects/USDSchemaTranslator.h"

class AUsdStageActor;
class CloLiveSyncServer;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnFilePathReceived, const FString& /* FilePath */);

class FLiveSyncPaletteTool
{
public:
    FLiveSyncPaletteTool(const TMap<FName, TArray<FName>>& InValidPaletteTools)
        : ValidPaletteTools(InValidPaletteTools)
    {
    }

    bool SetPaletteName(const FName& InPalette)
    {
        if (!ValidPaletteTools.Contains(InPalette))
        {
            return false;
        }

        PaletteName = InPalette;

        if (!ValidPaletteTools[PaletteName].Contains(GetToolName()))
        {
            if (!ValidPaletteTools[PaletteName].IsEmpty())
            {
                ToolName = ValidPaletteTools[PaletteName][0];
            }
        }

        return true;
    }

    const FName& GetPaletteName() const
    {
        return PaletteName;
    }

    bool SetToolName(const FName& InToolName)
    {
        if (!IsVaildToolName(InToolName))
        {
        	return false;
        }

        ToolName = InToolName;
        return true;
    }

    const FName& GetToolName() const
    {
        return ToolName;
    }

    bool IsVaildToolName(const FName& InToolName)
    {
        if (!ValidPaletteTools[GetPaletteName()].Contains(InToolName))
        {
            return false;
        }

        return true;
    }

private:
    FName PaletteName;
    FName ToolName;
    TMap<FName, TArray<FName>> ValidPaletteTools;
};

class CLOLIVESYNCEDITOR_API FCloLiveSyncMode : public FEdMode
{
public:
    static const FEditorModeID EM_CloLiveSync;

public:
    FCloLiveSyncMode();
    virtual ~FCloLiveSyncMode();

    // FEdMode interface
    virtual bool UsesToolkits() const override;
    virtual void Initialize() override;
    virtual void Enter() override;
    virtual void Exit() override;
    // End of FEdMode interface

    TWeakPtr<FLiveSyncPaletteTool> GetCurrentPaletteTool() const;

    void Update(const CloLiveSync::USDOptions& InUpdateOption);
    void SendFilePath(const FString& InFilePath);
    void SaveUsdStage(FString& OutFolderPath);
    void SaveAsClothAsset(FString& OutFolderPath);

    bool IsConnected() const;

private:
    void OnFilePathReceived(const std::string& Utf8FilePath);
    void OnClientConnectionCountChanged(const int clientCount);
    void FocusOn(AActor* Actor);

    void RegisterLiveSyncUsdTranslator();
    void UnregisterLiveSyncUsdTranslator();

    FString CreateFormattedUpdateMessage(const TSharedPtr<IPlugin>& Plugin, const FString& CurrentVersion) const;
    void OpenUpdateInfoDialog();
    void OpenSubstanceInfoDialog();

    void SetUsdOutputFolderPath(const FString& OutputPath);
    void ResetUsdOutputFolderPath();

    void RenameAllAssetsInFolder(const FString& OutputPath, const FString& USDStageName);
    bool DoesPathContainAssets(const FString& OutputPath, const FString& USDStageName);

    void PrepareRestPoseAnimationsForSave(AUsdStageActor* StageActor, const FString& TargetFolderPath, TMap<FString, FString>& OutRestPoseToSkelMeshMap);
    void FinalizeRestPoseAnimationLinks(const TMap<FString, FString>& InRestPoseToSkelMeshMap);
    void ConvertLevelSequencesToSpawnable(const FString& OutputPath, const FString& USDStageName);
    void AdjustLevelSequenceGCFrameRanges(const FString& OutputPath, const FString& USDStageName, const UE::FUsdStage& UsdStage);
    void DeleteUSDStageRootActor(const FString& USDStageName);

    void ClearUnreferencedUSDStageAssets();

    AUsdStageActor* GetStageActor();

public:
    FOnFilePathReceived OnFilePathReceivedDelegate;

private:
    TSharedPtr<FLiveSyncPaletteTool> CurrentPaletteTool;
    FString LatestFilePath;
    FDelegateHandle FilePathReceivedHandle;
    TUniquePtr<CloLiveSyncServer> LiveSyncServer;
    TMap<FString, FRegisteredSchemaTranslatorHandle> MapCLOTranslatorHandle;
    TAtomic<bool> bIsConnected;
    TWeakObjectPtr<AUsdStageActor> WeakStageActorPtr;
    FDelegateHandle PostUsdImportDelegateHandle;
};
