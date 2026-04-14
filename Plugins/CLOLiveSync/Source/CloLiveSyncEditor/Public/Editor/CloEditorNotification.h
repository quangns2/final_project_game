// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Notifications/NotificationManager.h"
#include "TickableEditorObject.h"
#include "Widgets/SWindow.h"


class CLOLIVESYNCEDITOR_API FCloEditorProgressNotification
{
public:
    FCloEditorProgressNotification() {}

public:
    void StartNotification(int32 InStartWorkNum, FText ProgressText);
    void UpdateNotification();
    void StopNotification();

    static void CreateNotification(const FText& ProgressText, float ExpireDuration = 3.0f);
    
    static void CreateProgressNotification(int32 InStartWorkNum, const FText& ProgressText, bool ShowMessage = true);
    static void UpdateProgressInfo(FText ProgressText = FText(), int32 InTotalWorkDone = INDEX_NONE, int32 InTotalWorkNum = INDEX_NONE);
    static void AssignWindow(TWeakPtr<SWindow> InWindow);
    static void DestroyProgressNotification();

    static int32 GetCurrentProgress();
private:
    FProgressNotificationHandle NotificationHandle;
    FTimerHandle TimerHandle;

    FText ProgressMessage;
    int32 TotalWorkNeeded = 0;
    int32 CurrentWorkCompleted = 0;

    static TWeakPtr<SWindow> TargetWindow;
    static FCloEditorProgressNotification* ProgressNotification;
};
