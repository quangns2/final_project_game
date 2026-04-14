// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "Styling/SlateStyle.h"

/** Manages the style which provides resources for niagara editor widgets. */
class FCloLiveSyncEditorStyle : public FSlateStyleSet
{
public:
	static CLOLIVESYNCEDITOR_API void Register();
	static CLOLIVESYNCEDITOR_API void Unregister();
	static CLOLIVESYNCEDITOR_API void Shutdown();

	static CLOLIVESYNCEDITOR_API void ReloadTextures();

	static CLOLIVESYNCEDITOR_API const FCloLiveSyncEditorStyle& Get();

	static CLOLIVESYNCEDITOR_API void ReinitializeStyle();
    static CLOLIVESYNCEDITOR_API FName GetStyleName();

private:	
    FCloLiveSyncEditorStyle();

	static TSharedPtr<FCloLiveSyncEditorStyle> LiveSyncEditorStyle;
};
