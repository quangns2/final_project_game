// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#include "Editor/CloEditorNotification.h"

#include "Editor.h"
#include "Engine/EngineTypes.h"
#include "Widgets/Notifications/SNotificationList.h"

TWeakPtr<SWindow> FCloEditorProgressNotification::TargetWindow = nullptr;
FCloEditorProgressNotification* FCloEditorProgressNotification::ProgressNotification = nullptr;

void FCloEditorProgressNotification::StartNotification(int32 InTotalWorkNeeded, FText ProgressText)
{
    CurrentWorkCompleted = 0;
    TotalWorkNeeded = InTotalWorkNeeded;
    NotificationHandle = FSlateNotificationManager::Get().StartProgressNotification(ProgressText, TotalWorkNeeded);

    AsyncTask(ENamedThreads::GameThread, [&]()
        {
		    GEditor->GetTimerManager()->SetTimer(TimerHandle,
		        FTimerDelegate::CreateLambda([&]()
		            {
		                if (NotificationHandle.IsValid())
		                {
		                    if (TotalWorkNeeded == CurrentWorkCompleted)
		                    {
		                        StopNotification();
		                    }
		                    else
		                    {
		                        UpdateNotification();
		                    }
		                }
		            }), 0.5f, true);
        });
}

void FCloEditorProgressNotification::UpdateNotification()
{
    FSlateNotificationManager::Get().UpdateProgressNotification(NotificationHandle, CurrentWorkCompleted, TotalWorkNeeded, ProgressMessage);
}

void FCloEditorProgressNotification::StopNotification()
{
    if (TimerHandle.IsValid())
    {
        AsyncTask(ENamedThreads::GameThread, [&]()
            {
                GEditor->GetTimerManager()->ClearTimer(TimerHandle);
            });
    }

    FSlateNotificationManager::Get().CancelProgressNotification(NotificationHandle);
    NotificationHandle.Reset();
    TotalWorkNeeded = 0;
    CurrentWorkCompleted = 0;
}

void FCloEditorProgressNotification::CreateNotification(const FText& ProgressText, float ExpireDuration)
{
    FNotificationInfo notificationPopupInfo(ProgressText);
    notificationPopupInfo.ExpireDuration = ExpireDuration;
    if (TargetWindow.IsValid())
    {
        notificationPopupInfo.ForWindow = TargetWindow.Pin();
    }
    FSlateNotificationManager::Get().AddNotification(notificationPopupInfo);
}

void FCloEditorProgressNotification::CreateProgressNotification(int32 InStartWorkNum, const FText& ProgressText, bool ShowMessage)
{
    if (ProgressNotification != nullptr)
    {
        ProgressNotification->StopNotification();
        ProgressNotification = nullptr;
    }

    ProgressNotification = new FCloEditorProgressNotification();
    ProgressNotification->StartNotification(InStartWorkNum, ProgressText);
    
    if (ShowMessage)
    {
        CreateNotification(ProgressText);
    }
}

void FCloEditorProgressNotification::UpdateProgressInfo(FText ProgressText, int32 InTotalWorkDone, int32 InTotalWorkNum)
{
    if (ProgressNotification == nullptr)
    {
        return;
    }

    ProgressNotification->ProgressMessage = ProgressText.IsEmpty() ? ProgressNotification->ProgressMessage : ProgressText;
    // Should I have check if currentWorkCompleted is greater than totalWorkNeeded?
    ProgressNotification->TotalWorkNeeded = InTotalWorkNum == -1 ? ProgressNotification->TotalWorkNeeded : InTotalWorkNum;
    ProgressNotification->CurrentWorkCompleted = InTotalWorkDone == -1 ? ProgressNotification->CurrentWorkCompleted : InTotalWorkDone;

    // forcibly refresh the notification
    ProgressNotification->UpdateNotification();
}

void FCloEditorProgressNotification::AssignWindow(TWeakPtr<SWindow> InWindow)
{
    TargetWindow = InWindow;
}

void FCloEditorProgressNotification::DestroyProgressNotification()
{
    if (ProgressNotification != nullptr)
    {
        ProgressNotification->StopNotification();
        ProgressNotification = nullptr;
    }
}

int32 FCloEditorProgressNotification::GetCurrentProgress()
{
    if (ProgressNotification != nullptr)
    {
        return ProgressNotification->CurrentWorkCompleted;
    }

    return -1;
}
